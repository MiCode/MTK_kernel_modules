// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Christopher Chen <christopher.chen@mediatek.com>
 *
 */

#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include "mtk_imgsys-engine-isp8.h"
#include "mtk_imgsys-debug.h"
/* TODO */
#include "smi.h"

// GCE header
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <linux/delay.h>

#define DL_CHECK_ENG_NUM IMGSYS_ENG_NUM
#define WPE_HW_SET    3
#define ADL_HW_SET    2
#define SW_RST   (0x000C)
#define DBG_SW_CLR   (0x0260)

#define HAVE_IMGSYS_ENG_ADL_A   0
#define HAVE_IMGSYS_ENG_ADL_B   0
#define HAVE_IMGSYS_ENG_XTR     0

#define LOG_LEGNTH (64)
#define FINAL_LOG_LENGTH (LOG_LEGNTH * 4)
#define DDREN_ACK_TIMEOUT_CNT 1000000

const unsigned int g_imgsys_main_reg_base      = (0x34000000);
const unsigned int g_imgsys_dip_top_reg_base   = (0x34110000);
const unsigned int g_imgsys_dip_nr_reg_base    = (0x34130000);
const unsigned int g_imgsys_wpe1_dip1_reg_base = (0x34220000);
const unsigned int g_imgsys_wpe2_dip1_reg_base = (0x34520000);
const unsigned int g_imgsys_wpe3_dip1_reg_base = (0x34620000);
const unsigned int g_imgsys_traw_dip1_reg_base = (0x34710000);

bool imgsys_dip_7sp_dbg_enable(void)
{
	return imgsys_dip_dbg_en;
}

bool imgsys_traw_7sp_dbg_enable(void)
{
	return imgsys_traw_dbg_en;
}

bool imgsys_wpe_7sp_dbg_enable(void)
{
	return imgsys_wpe_dbg_en;
}

bool imgsys_omc_8_dbg_enable(void)
{
	return imgsys_omc_dbg_en;
}

bool imgsys_pqdip_7sp_dbg_enable(void)
{
	return imgsys_pqdip_dbg_en;
}

bool imgsys_me_7sp_dbg_enable(void)
{
	return imgsys_me_dbg_en;
}

/* Should follow the order of IMGSYS_ENG_xxx in enum_imgsys_engine */
struct imgsys_dbg_engine_t dbg_engine_name_list[DL_CHECK_ENG_NUM] = {
	{IMGSYS_ENG_WPE_EIS,  "WPE_EIS"},
	{IMGSYS_ENG_WPE_TNR,  "WPE_TNR"},
	{IMGSYS_ENG_WPE_LITE, "WPE_LITE"},
	{IMGSYS_ENG_OMC_TNR,  "OMC_TNR"},
	{IMGSYS_ENG_OMC_LITE, "OMC_LITE"},
	{IMGSYS_ENG_ADL_A,    "ADL_A"},
	{IMGSYS_ENG_ADL_B,    "ADL_A"},
	{IMGSYS_ENG_TRAW,     "TRAW"},
	{IMGSYS_ENG_LTR,      "LTRAW"},
	{IMGSYS_ENG_XTR,      "XTRAW"},
	{IMGSYS_ENG_DIP,      "DIP"},
	{IMGSYS_ENG_PQDIP_A,  "PQDIP_A"},
	{IMGSYS_ENG_PQDIP_B,  "PQDIP_B"},
	{IMGSYS_ENG_ME,       "ME"},
};

void __iomem *imgsysmainRegBA;
void __iomem *wpedip1RegBA;
void __iomem *wpedip2RegBA;
void __iomem *wpedip3RegBA;
void __iomem *dipRegBA;
void __iomem *dip1RegBA;
void __iomem *dip2RegBA;
void __iomem *trawRegBA;
void __iomem *adlARegBA;
void __iomem *adlBRegBA;
void __iomem *imgsysddrenRegBA;
int imgsys_ddr_en;

void imgsys_main_init(struct mtk_imgsys_dev *imgsys_dev)
{
	struct resource adl;
	int ddr_en = 0;
	imgsys_ddr_en = 0;

	pr_info("%s: +.\n", __func__);

	imgsysmainRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_TOP);
	if (!imgsysmainRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap imgsys_top registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	wpedip1RegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_WPE1_DIP1);
	if (!wpedip1RegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip1 registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	wpedip2RegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_WPE2_DIP1);
	if (!wpedip2RegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip2 registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	wpedip3RegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_WPE3_DIP1);
	if (!wpedip3RegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip3 registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	dipRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_DIP_TOP);
	if (!dipRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap dip_top registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	dip1RegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_DIP_TOP_NR);
	if (!dip1RegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap dip_top_nr registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	dip2RegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_DIP_TOP_NR2);
	if (!dip2RegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap dip_top_nr2 registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}


	trawRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_TRAW_TOP);
	if (!trawRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap traw_dip1 registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	adl.start = 0;
	if (of_address_to_resource(imgsys_dev->dev->of_node, REG_MAP_E_ADL_A, &adl)) {
		dev_info(imgsys_dev->dev, "%s: of_address_to_resource fail\n", __func__);
		return;
	}
	if (adl.start) {
		adlARegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_ADL_A);
		if (!adlARegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap adl a registers\n",
									__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
					__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		adlBRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_ADL_B);
		if (!adlBRegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap adl b registers\n",
									__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
					__func__, imgsys_dev->dev->of_node->name);
			return;
		}
	} else {
		adlARegBA = NULL;
		adlBRegBA = NULL;
		dev_info(imgsys_dev->dev, "%s Do not have ADL hardware.\n", __func__);
	}

	if (of_property_read_u32(imgsys_dev->dev->of_node,
		"mediatek,imgsys-ddr-en", &ddr_en) != 0) {
		dev_info(imgsys_dev->dev, "ddr_en is not exist\n");
	} else {
		imgsys_ddr_en = ddr_en;
		dev_info(imgsys_dev->dev, "ddr_en(%d/%d)\n", ddr_en, imgsys_ddr_en);
	}

		imgsysddrenRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_IMG_VCORE);
		if (!imgsysddrenRegBA) {
			dev_info(imgsys_dev->dev, "%s: Unable to ioremap img_vcore registers\n",
					__func__);
			dev_info(imgsys_dev->dev, "%s: of_iomap fail, devnode(%s).\n",
					__func__, imgsys_dev->dev->of_node->name);
			return;
		}

	pr_info("%s: -.\n", __func__);
}

void imgsys_main_set_init(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *WpeRegBA = 0L;
	void __iomem *ADLRegBA = 0L;
	void __iomem *pWpeCtrl = 0L;
	void __iomem *DdrRegBA = 0L;
	unsigned int HwIdx = 0;
	uint32_t count;
	uint32_t value;
	int i, num;

	pr_debug("%s: +.\n", __func__);

	DdrRegBA = imgsysddrenRegBA;
	/* HW DDREN*/
	value = 0x1fd;
	iowrite32(value, (DdrRegBA + 0x10));

	/* Wait platform resources ack */
	count = 0;
	value = ioread32((void *)(DdrRegBA + 0x14));
	while ((value & 0x1fc) != 0x1fc) {
		count++;
		if (count > DDREN_ACK_TIMEOUT_CNT) {
			pr_err("[%s][%d] wait platorm resources done timeout", __func__, __LINE__);
				break;
		}
		udelay(5);
			value = ioread32((void *)(DdrRegBA + 0x14));
	}

	if (imgsys_dev == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}

	num = imgsys_dev->larbs_num - 1;
	for (i = 0; i < num; i++)
		mtk_smi_larb_clamp_and_lock(imgsys_dev->larbs[i], 1);

	for (HwIdx = 0; HwIdx < WPE_HW_SET; HwIdx++) {
		if (HwIdx == 0)
			WpeRegBA = wpedip1RegBA;
		else if (HwIdx == 1)
			WpeRegBA = wpedip2RegBA;
		else
			WpeRegBA = wpedip3RegBA;

		/* Wpe Macro HW Reset */
		pWpeCtrl = (void *)(WpeRegBA + SW_RST);
		iowrite32(0xF, pWpeCtrl);
		/* Clear HW Reset */
		iowrite32(0x0, pWpeCtrl);
	}

	HwIdx = 0;
	if (adlARegBA || adlBRegBA) {
		/* Reset ADL A */
		for (HwIdx = 0; HwIdx < ADL_HW_SET; HwIdx++) {
			if (HwIdx == 0)
				ADLRegBA = adlARegBA;
			else if (HwIdx == 1)
				ADLRegBA = adlBRegBA;
			if (!ADLRegBA)
				continue;
			value = ioread32((void *)(ADLRegBA + 0x300));
			value |= ((0x1 << 8) | (0x1 << 9));
			iowrite32(value, (ADLRegBA + 0x300));
			count = 0;
			while (count < 1000000) {
				value = ioread32((void *)(ADLRegBA + 0x300));
				if ((value & 0x3) == 0x3)
					break;
				count++;
			}
			value = ioread32((void *)(ADLRegBA + 0x300));
			value &= ~((0x1 << 8) | (0x1 << 9));
			iowrite32(value, (ADLRegBA + 0x300));
		}
	}
	iowrite32(0x1FF, (void *)(imgsysmainRegBA + DBG_SW_CLR));
	iowrite32(0x0, (void *)(imgsysmainRegBA + DBG_SW_CLR));

	iowrite32(0xF0, (void *)(imgsysmainRegBA + SW_RST));
	iowrite32(0x0, (void *)(imgsysmainRegBA + SW_RST));

	iowrite32(0x3C, (void *)(trawRegBA + SW_RST));
	iowrite32(0x0, (void *)(trawRegBA + SW_RST));

	iowrite32(0x3FC03, (void *)(dipRegBA + SW_RST));
	iowrite32(0x0, (void *)(dipRegBA + SW_RST));

	for (i = 0; i < num; i++)
		mtk_smi_larb_clamp_and_lock(imgsys_dev->larbs[i], 0);

	pr_debug("%s: -. qof ver = %d\n", __func__, imgsys_dev->qof_ver);
}

void imgsys_main_cmdq_set_init(struct mtk_imgsys_dev *imgsys_dev, void *pkt, int hw_idx)
{
	struct cmdq_pkt *package = NULL;

	if (imgsys_dev == NULL || pkt == NULL) {
		dump_stack();
		pr_err("[%s][%d] param fatal error!", __func__, __LINE__);
		return;
	}
	package = (struct cmdq_pkt *)pkt;

	cmdq_pkt_write(package, NULL, (g_imgsys_main_reg_base + SW_RST) /*address*/,
			       0xF0, 0xffffffff);
	cmdq_pkt_write(package, NULL, (g_imgsys_main_reg_base + SW_RST) /*address*/,
			       0x0, 0xffffffff);

	cmdq_pkt_write(package, NULL, (g_imgsys_main_reg_base + DBG_SW_CLR) /*address*/,
			       0x1FF, 0xffffffff);
	cmdq_pkt_write(package, NULL, (g_imgsys_main_reg_base + DBG_SW_CLR) /*address*/,
			       0x0, 0xffffffff);
}

void imgsys_main_uninit(struct mtk_imgsys_dev *imgsys_dev)
{
	pr_debug("%s: +.\n", __func__);

	if (imgsysmainRegBA) {
		iounmap(imgsysmainRegBA);
		imgsysmainRegBA = 0L;
	}

	if (wpedip1RegBA) {
		iounmap(wpedip1RegBA);
		wpedip1RegBA = 0L;
	}

	if (wpedip2RegBA) {
		iounmap(wpedip2RegBA);
		wpedip2RegBA = 0L;
	}

	if (wpedip3RegBA) {
		iounmap(wpedip3RegBA);
		wpedip3RegBA = 0L;
	}

	if (dipRegBA) {
		iounmap(dipRegBA);
		dipRegBA = 0L;
	}

	if (dip1RegBA) {
		iounmap(dip1RegBA);
		dip1RegBA = 0L;
	}

	if (dip2RegBA) {
		iounmap(dip2RegBA);
		dip2RegBA = 0L;
	}

	if (trawRegBA) {
		iounmap(trawRegBA);
		trawRegBA = 0L;
	}

	if (adlARegBA) {
		iounmap(adlARegBA);
		adlARegBA = 0L;
	}

	if (adlBRegBA) {
		iounmap(adlBRegBA);
		adlBRegBA = 0L;
	}

		if (imgsysddrenRegBA) {
			iounmap(imgsysddrenRegBA);
			imgsysddrenRegBA = 0L;
		}

	imgsys_ddr_en = 0;
	pr_debug("%s: -.\n", __func__);
}

void imgsys_debug_dump_routine(struct mtk_imgsys_dev *imgsys_dev,
	const struct module_ops *imgsys_modules,
	int imgsys_module_num, unsigned int hw_comb)
{
	bool module_on[IMGSYS_MOD_MAX] = {
		false, false, false, false, false, false, false, false, false};
	int i = 0;

	dev_info(imgsys_dev->dev,
			"%s: hw comb set: 0x%x\n",
			__func__, hw_comb);

	imgsys_dl_debug_dump(imgsys_dev, hw_comb);

	if ((hw_comb & IMGSYS_ENG_WPE_EIS) || (hw_comb & IMGSYS_ENG_WPE_TNR)
		 || (hw_comb & IMGSYS_ENG_WPE_LITE))
		module_on[IMGSYS_MOD_WPE] = true;
	if ((hw_comb & IMGSYS_ENG_OMC_TNR) || (hw_comb & IMGSYS_ENG_OMC_LITE))
		module_on[IMGSYS_MOD_OMC] = true;
	if ((hw_comb & IMGSYS_ENG_ADL_A) || (hw_comb & IMGSYS_ENG_ADL_B))
		module_on[IMGSYS_MOD_ADL] = true;
	if ((hw_comb & IMGSYS_ENG_TRAW) || (hw_comb & IMGSYS_ENG_LTR)
		 || (hw_comb & IMGSYS_ENG_XTR))
		module_on[IMGSYS_MOD_TRAW] = true;
	if ((hw_comb & IMGSYS_ENG_DIP))
		module_on[IMGSYS_MOD_DIP] = true;
	if ((hw_comb & IMGSYS_ENG_PQDIP_A) || (hw_comb & IMGSYS_ENG_PQDIP_B))
		module_on[IMGSYS_MOD_PQDIP] = true;
	if ((hw_comb & IMGSYS_ENG_ME))
		module_on[IMGSYS_MOD_ME] = true;

	/* in case module driver did not set imgsys_modules in module order */
	dev_info(imgsys_dev->dev,
			"%s: imgsys_module_num: %d\n",
			__func__, imgsys_module_num);
	for (i = 0 ; i < imgsys_module_num ; i++) {
		if (module_on[imgsys_modules[i].module_id])
			imgsys_modules[i].dump(imgsys_dev, hw_comb);
	}
}

void imgsys_cg_debug_dump(struct mtk_imgsys_dev *imgsys_dev, unsigned int hw_comb)
{
	unsigned int i = 0;

	if (!imgsysmainRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap imgsys_top registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	for (i = 0; i <= 0x500; i += 0x10) {
		dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
		__func__, (unsigned int)(g_imgsys_main_reg_base + i),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + i)),
		(unsigned int)(g_imgsys_main_reg_base + i + 0x4),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + i + 0x4)),
		(unsigned int)(g_imgsys_main_reg_base + i + 0x8),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + i + 0x8)),
		(unsigned int)(g_imgsys_main_reg_base + i + 0xc),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + i + 0xc)));
	}

	if (hw_comb & IMGSYS_ENG_DIP) {
		if (!dipRegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap dip registers\n",
									__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
					__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		for (i = 0; i <= 0x100; i += 0x10) {
			dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
			__func__, (unsigned int)(g_imgsys_dip_top_reg_base + i),
			(unsigned int)ioread32((void *)(dipRegBA + i)),
			(unsigned int)(g_imgsys_dip_top_reg_base + i + 0x4),
			(unsigned int)ioread32((void *)(dipRegBA + i + 0x4)),
			(unsigned int)(g_imgsys_dip_top_reg_base + i + 0x8),
			(unsigned int)ioread32((void *)(dipRegBA + i + 0x8)),
			(unsigned int)(g_imgsys_dip_top_reg_base + i + 0xc),
			(unsigned int)ioread32((void *)(dipRegBA + i + 0xc)));
		}

		if (!dip1RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap dip1 registers\n",
									__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
					__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		for (i = 0; i <= 0x100; i += 0x10) {
			dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
			__func__, (unsigned int)(g_imgsys_dip_nr_reg_base + i),
			(unsigned int)ioread32((void *)(dip1RegBA + i)),
			(unsigned int)(g_imgsys_dip_nr_reg_base + i + 0x4),
			(unsigned int)ioread32((void *)(dip1RegBA + i + 0x4)),
			(unsigned int)(g_imgsys_dip_nr_reg_base + i + 0x8),
			(unsigned int)ioread32((void *)(dip1RegBA + i + 0x8)),
			(unsigned int)(g_imgsys_dip_nr_reg_base + i + 0xc),
			(unsigned int)ioread32((void *)(dip1RegBA + i + 0xc)));
		}
	}

	if (hw_comb & IMGSYS_ENG_WPE_EIS) {
		if (!wpedip1RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip1 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		for (i = 0; i <= 0x100; i += 0x10) {
			dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
				__func__, (unsigned int)(g_imgsys_wpe1_dip1_reg_base + i),
			(unsigned int)ioread32((void *)(wpedip1RegBA + i)),
			(unsigned int)(g_imgsys_wpe1_dip1_reg_base + i + 0x4),
			(unsigned int)ioread32((void *)(wpedip1RegBA + i + 0x4)),
			(unsigned int)(g_imgsys_wpe1_dip1_reg_base + i + 0x8),
			(unsigned int)ioread32((void *)(wpedip1RegBA + i + 0x8)),
			(unsigned int)(g_imgsys_wpe1_dip1_reg_base + i + 0xc),
			(unsigned int)ioread32((void *)(wpedip1RegBA + i + 0xc)));
		}
	}

	if ((hw_comb & IMGSYS_ENG_WPE_TNR) || (hw_comb & IMGSYS_ENG_OMC_TNR)) {
		if (!wpedip2RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip2 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		for (i = 0; i <= 0x100; i += 0x10) {
			dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
				__func__, (unsigned int)(g_imgsys_wpe2_dip1_reg_base + i),
			(unsigned int)ioread32((void *)(wpedip2RegBA + i)),
			(unsigned int)(g_imgsys_wpe2_dip1_reg_base + i + 0x4),
			(unsigned int)ioread32((void *)(wpedip2RegBA + i + 0x4)),
			(unsigned int)(g_imgsys_wpe2_dip1_reg_base + i + 0x8),
			(unsigned int)ioread32((void *)(wpedip2RegBA + i + 0x8)),
			(unsigned int)(g_imgsys_wpe2_dip1_reg_base + i + 0xc),
			(unsigned int)ioread32((void *)(wpedip2RegBA + i + 0xc)));
		}
	}

	if ((hw_comb & IMGSYS_ENG_WPE_LITE) || (hw_comb & IMGSYS_ENG_OMC_LITE)) {
		if (!wpedip3RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip3 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}

		for (i = 0; i <= 0x100; i += 0x10) {
			dev_info(imgsys_dev->dev, "%s: [0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X][0x%08X 0x%08X]",
				__func__, (unsigned int)(g_imgsys_wpe3_dip1_reg_base + i),
			(unsigned int)ioread32((void *)(wpedip3RegBA + i)),
			(unsigned int)(g_imgsys_wpe3_dip1_reg_base + i + 0x4),
			(unsigned int)ioread32((void *)(wpedip3RegBA + i + 0x4)),
			(unsigned int)(g_imgsys_wpe3_dip1_reg_base + i + 0x8),
			(unsigned int)ioread32((void *)(wpedip3RegBA + i + 0x8)),
			(unsigned int)(g_imgsys_wpe3_dip1_reg_base + i + 0xc),
			(unsigned int)ioread32((void *)(wpedip3RegBA + i + 0xc)));
		}
	}
}

void imgsys_dl_checksum_dump(struct mtk_imgsys_dev *imgsys_dev,
	unsigned int hw_comb, char *logBuf_path,
	char *logBuf_inport, char *logBuf_outport, int dl_path)
{
	/*void __iomem *imgsysmainRegBA = 0L;*/
	/*void __iomem *wpedip1RegBA = 0L;*/
	/*void __iomem *wpedip2RegBA = 0L;*/
	/*void __iomem *wpedip3RegBA = 0L;*/
	unsigned int checksum_dbg_sel = 0x0;
	unsigned int original_dbg_sel_value = 0x0;
	char logBuf_final[FINAL_LOG_LENGTH];
	int debug0_req[2] = {0, 0};
	int debug0_rdy[2] = {0, 0};
	int debug0_checksum[2] = {0, 0};
	int debug1_line_cnt[2] = {0, 0};
	int debug1_pix_cnt[2] = {0, 0};
	int debug2_line_cnt[2] = {0, 0};
	int debug2_pix_cnt[2] = {0, 0};
	unsigned int dbg_sel_value[2] = {0x0, 0x0};
	unsigned int debug0_value[2] = {0x0, 0x0};
	unsigned int debug1_value[2] = {0x0, 0x0};
	unsigned int debug2_value[2] = {0x0, 0x0};
	unsigned int wpe_pqdip_mux_v = 0x0;
	unsigned int wpe_pqdip_mux2_v = 0x0;
	unsigned int wpe_pqdip_mux3_v = 0x0;
	char logBuf_temp[LOG_LEGNTH];
	int ret;

	memset((char *)logBuf_final, 0x0, sizeof(logBuf_final));
	logBuf_final[strlen(logBuf_final)] = '\0';
	memset((char *)logBuf_temp, 0x0, sizeof(logBuf_temp));
	logBuf_temp[strlen(logBuf_temp)] = '\0';

	dev_info(imgsys_dev->dev,
		"%s: + hw_comb/path(0x%x/%s) dl_path:%d, start dump\n",
		__func__, hw_comb, logBuf_path, dl_path);
	/* iomap registers */
	/*imgsysmainRegBA = of_iomap(imgsys_dev->dev->of_node, REG_MAP_E_TOP);*/
	if (!imgsysmainRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap imgsys_top registers\n",
								__func__);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
		return;
	}

	if (hw_comb & IMGSYS_ENG_WPE_EIS) {
		/* macro_comm status */
		if (!wpedip1RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip1 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}
		wpe_pqdip_mux_v = (unsigned int)ioread32((void *)(wpedip1RegBA + 0xA8));
	}

	if ((hw_comb & IMGSYS_ENG_WPE_TNR) || (hw_comb & IMGSYS_ENG_OMC_TNR)) {
		if (!wpedip2RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip2 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}
		wpe_pqdip_mux2_v = (unsigned int)ioread32((void *)(wpedip2RegBA + 0xA8));
	}

	if ((hw_comb & IMGSYS_ENG_WPE_LITE) || (hw_comb & IMGSYS_ENG_OMC_LITE)) {
		if (!wpedip3RegBA) {
			dev_info(imgsys_dev->dev, "%s Unable to ioremap wpe_dip3 registers\n",
				__func__);
			dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
				__func__, imgsys_dev->dev->of_node->name);
			return;
		}
		wpe_pqdip_mux3_v = (unsigned int)ioread32((void *)(wpedip3RegBA + 0xA8));
	}

	/* dump information */
	if (dl_path == IMGSYS_DL_NO_CHECK_SUM_DUMP) {
	} else {
		/*dump former engine in DL (imgsys main in port) status */
		checksum_dbg_sel = (unsigned int)((dl_path << 1) | (0 << 0));
		original_dbg_sel_value = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x4C));
		original_dbg_sel_value = original_dbg_sel_value & 0xff00ffff; /*clear last time data*/
		dbg_sel_value[0] = (original_dbg_sel_value | 0x1 |
			((checksum_dbg_sel << 16) & 0x00ff0000));
		writel(dbg_sel_value[0], (imgsysmainRegBA + 0x4C));
		dbg_sel_value[0] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x4C));
		debug0_value[0] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x200));
		debug0_checksum[0] = (debug0_value[0] & 0x0000ffff);
		debug0_rdy[0] = (debug0_value[0] & 0x00800000) >> 23;
		debug0_req[0] = (debug0_value[0] & 0x01000000) >> 24;
		debug1_value[0] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x204));
		debug1_line_cnt[0] = ((debug1_value[0] & 0xffff0000) >> 16) & 0x0000ffff;
		debug1_pix_cnt[0] = (debug1_value[0] & 0x0000ffff);
		debug2_value[0] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x208));
		debug2_line_cnt[0] = ((debug2_value[0] & 0xffff0000) >> 16) & 0x0000ffff;
		debug2_pix_cnt[0] = (debug2_value[0] & 0x0000ffff);

		/*dump later engine in DL (imgsys main out port) status */
		checksum_dbg_sel = (unsigned int)((dl_path << 1) | (1 << 0));
		dbg_sel_value[1] = (original_dbg_sel_value | 0x1 |
			((checksum_dbg_sel << 16) & 0x00ff0000));
		writel(dbg_sel_value[1], (imgsysmainRegBA + 0x4C));
		dbg_sel_value[1] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x4C));
		debug0_value[1] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x200));
		debug0_checksum[1] = (debug0_value[1] & 0x0000ffff);
		debug0_rdy[1] = (debug0_value[1] & 0x00800000) >> 23;
		debug0_req[1] = (debug0_value[1] & 0x01000000) >> 24;
		debug1_value[1] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x204));
		debug1_line_cnt[1] = ((debug1_value[1] & 0xffff0000) >> 16) & 0x0000ffff;
		debug1_pix_cnt[1] = (debug1_value[1] & 0x0000ffff);
		debug2_value[1] = (unsigned int)ioread32((void *)(imgsysmainRegBA + 0x208));
		debug2_line_cnt[1] = ((debug2_value[1] & 0xffff0000) >> 16) & 0x0000ffff;
		debug2_pix_cnt[1] = (debug2_value[1] & 0x0000ffff);

		if (debug0_req[0] == 1) {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s req to send data to %s/",
				logBuf_inport, logBuf_outport);
			if (ret > sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		} else {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s not send data to %s/",
				logBuf_inport, logBuf_outport);
			if (ret > sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		}
		strncat(logBuf_final, logBuf_temp, strlen(logBuf_temp));
		memset((char *)logBuf_temp, 0x0, sizeof(logBuf_temp));
		logBuf_temp[strlen(logBuf_temp)] = '\0';
		if (debug0_rdy[0] == 1) {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s rdy to receive data from %s",
				logBuf_outport, logBuf_inport);
			if (ret > sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		} else {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s not rdy to receive data from %s",
				logBuf_outport, logBuf_inport);
			if (ret >= sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		}
		strncat(logBuf_final, logBuf_temp, strlen(logBuf_temp));
		dev_info(imgsys_dev->dev,
			"%s: %s", __func__, logBuf_final);

		memset((char *)logBuf_final, 0x0, sizeof(logBuf_final));
		logBuf_final[strlen(logBuf_final)] = '\0';
		memset((char *)logBuf_temp, 0x0, sizeof(logBuf_temp));
		logBuf_temp[strlen(logBuf_temp)] = '\0';
		if (debug0_req[1] == 1) {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s req to send data to %sPIPE/",
				logBuf_outport, logBuf_outport);
			if (ret >= sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		} else {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%s not send data to %sPIPE/",
				logBuf_outport, logBuf_outport);
			if (ret >= sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		}
		strncat(logBuf_final, logBuf_temp, strlen(logBuf_temp));
		memset((char *)logBuf_temp, 0x0, sizeof(logBuf_temp));
		logBuf_temp[strlen(logBuf_temp)] = '\0';
		if (debug0_rdy[1] == 1) {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%sPIPE rdy to receive data from %s",
				logBuf_outport, logBuf_outport);
			if (ret >= sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		} else {
			ret = snprintf(logBuf_temp, sizeof(logBuf_temp),
				"%sPIPE not rdy to receive data from %s",
				logBuf_outport, logBuf_outport);
			if (ret >= sizeof(logBuf_temp))
				dev_dbg(imgsys_dev->dev,
					"%s: string truncated\n", __func__);
		}
		strncat(logBuf_final, logBuf_temp, strlen(logBuf_temp));
		dev_info(imgsys_dev->dev,
			"%s: %s", __func__, logBuf_final);
		dev_info(imgsys_dev->dev,
			"%s: in_req/in_rdy/out_req/out_rdy = %d/%d/%d/%d,(checksum: in/out) = (%d/%d)",
			__func__,
			debug0_req[0], debug0_rdy[0],
			debug0_req[1], debug0_rdy[1],
			debug0_checksum[0], debug0_checksum[1]);
		dev_info(imgsys_dev->dev,
			"%s: info01 in_line/in_pix/out_line/out_pix = %d/%d/%d/%d",
			__func__,
			debug1_line_cnt[0], debug1_pix_cnt[0], debug1_line_cnt[1],
			debug1_pix_cnt[1]);
		dev_info(imgsys_dev->dev,
			"%s: info02 in_line/in_pix/out_line/out_pix = %d/%d/%d/%d",
			__func__,
			debug2_line_cnt[0], debug2_pix_cnt[0], debug2_line_cnt[1],
			debug2_pix_cnt[1]);
	}
	dev_info(imgsys_dev->dev, "%s: ===(%s): %s DBG INFO===",
		__func__, logBuf_path, logBuf_inport);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x4C), dbg_sel_value[0]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x200), debug0_value[0]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x204), debug1_value[0]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x208), debug2_value[0]);

	dev_info(imgsys_dev->dev, "%s: ===(%s): %s DBG INFO===",
		__func__, logBuf_path, logBuf_outport);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x4C), dbg_sel_value[1]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x200), debug0_value[1]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x204), debug1_value[1]);
	dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x208), debug2_value[1]);

	dev_info(imgsys_dev->dev, "%s: ===(%s): IMGMAIN CG INFO===",
		__func__, logBuf_path);
	dev_info(imgsys_dev->dev, "%s: CG_CON  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x0),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + 0x0)));
	dev_info(imgsys_dev->dev, "%s: CG_SET  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x4),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + 0x4)));
	dev_info(imgsys_dev->dev, "%s: CG_CLR  0x%08X %08X", __func__,
		(unsigned int)(g_imgsys_main_reg_base + 0x8),
		(unsigned int)ioread32((void *)(imgsysmainRegBA + 0x8)));

	if (hw_comb & IMGSYS_ENG_WPE_EIS) {
		dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
			(unsigned int)(g_imgsys_wpe1_dip1_reg_base + 0xA8), wpe_pqdip_mux_v);
	} else if ((hw_comb & IMGSYS_ENG_WPE_TNR) || (hw_comb & IMGSYS_ENG_OMC_TNR)) {
		dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
			(unsigned int)(g_imgsys_wpe2_dip1_reg_base + 0xA8), wpe_pqdip_mux2_v);
	} else if (hw_comb & IMGSYS_ENG_WPE_LITE || (hw_comb & IMGSYS_ENG_OMC_LITE)) {
		dev_info(imgsys_dev->dev, "%s:  0x%08X %08X", __func__,
			(unsigned int)(g_imgsys_wpe3_dip1_reg_base + 0xA8), wpe_pqdip_mux3_v);
	}

	/*iounmap(imgsysmainRegBA);*/
}

void imgsys_dl_debug_dump(struct mtk_imgsys_dev *imgsys_dev, unsigned int hw_comb)
{
	int dl_path = 0;
	char logBuf_path[LOG_LEGNTH];
	char logBuf_inport[LOG_LEGNTH];
	char logBuf_outport[LOG_LEGNTH];
	char logBuf_eng[LOG_LEGNTH];
	int i = 0, get = false;
	int ret = 0;

	memset((char *)logBuf_path, 0x0, sizeof(logBuf_path));
	logBuf_path[strlen(logBuf_path)] = '\0';
	memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
	logBuf_inport[strlen(logBuf_inport)] = '\0';
	memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
	logBuf_outport[strlen(logBuf_outport)] = '\0';

	for (i = 0 ; i < DL_CHECK_ENG_NUM ; i++) {
		memset((char *)logBuf_eng, 0x0, sizeof(logBuf_eng));
		logBuf_eng[strlen(logBuf_eng)] = '\0';
		if (hw_comb & dbg_engine_name_list[i].eng_e) {
			if (get) {
				ret = snprintf(logBuf_eng, sizeof(logBuf_eng), "-%s",
					dbg_engine_name_list[i].eng_name);
				if (ret >= sizeof(logBuf_eng))
					dev_dbg(imgsys_dev->dev,
						"%s: string truncated\n", __func__);
			} else {
				ret = snprintf(logBuf_eng, sizeof(logBuf_eng), "%s",
					dbg_engine_name_list[i].eng_name);
				if (ret >= sizeof(logBuf_eng))
					dev_dbg(imgsys_dev->dev,
						"%s: string truncated\n", __func__);
			}
			get = true;
		}
		strncat(logBuf_path, logBuf_eng, strlen(logBuf_eng));
	}
	memset((char *)logBuf_eng, 0x0, sizeof(logBuf_eng));
	logBuf_eng[strlen(logBuf_eng)] = '\0';
	ret = snprintf(logBuf_eng, sizeof(logBuf_eng), "%s", " FAIL");
	if (ret >= sizeof(logBuf_eng))
		dev_dbg(imgsys_dev->dev,
			"%s: string truncated\n", __func__);
	strncat(logBuf_path, logBuf_eng, strlen(logBuf_eng));

	dev_info(imgsys_dev->dev, "%s: %s\n",
			__func__, logBuf_path);
	switch (hw_comb) {
	/*DL checksum case*/
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_TRAW):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_LTR):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
#if HAVE_IMGSYS_ENG_XTR
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_XTR):
		dl_path = IMGSYS_DL_WPEE_XTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"XTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
#endif
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW):
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		dev_info(imgsys_dev->dev,
			"%s: we dont have checksum for WPELITE DL TRAW\n",
			__func__);
		break;
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR):
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		dev_info(imgsys_dev->dev,
			"%s: we dont have checksum for WPELITE DL LTRAW\n",
			__func__);
		break;
#if HAVE_IMGSYS_ENG_XTR
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_XTR):
		dl_path = IMGSYS_DL_WPET_TRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"XTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		dev_info(imgsys_dev->dev,
			"%s: we dont have checksum for WPELITE DL XTRAW\n",
			__func__);
		break;
#endif
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW):
		dl_path = IMGSYS_DL_OMC_TNR_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR):
		dl_path = IMGSYS_DL_OMC_TNR_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_LITE | IMGSYS_ENG_TRAW):
		dl_path = IMGSYS_DL_OMC_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_LITE | IMGSYS_ENG_LTR):
		dl_path = IMGSYS_DL_OMC_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			 "DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_OMC_TNR_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_NO_CHECK_SUM_DUMP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_LITE_TO_PQDIP_A_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_NO_CHECK_SUM_DUMP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_TRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_LTRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_OMC_TNR_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_TRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
				"TRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_OMC_TNR_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_LTRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_TRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_LTRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_DIP |
		IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_OMC_TNR_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"OMC_TNR");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_TRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_EIS");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"WPE_LITE");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_LTRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_TRAW |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dev_info(imgsys_dev->dev,
			"%s: TOBE CHECKED SELECTION BASED ON FMT..\n",
			__func__);
		break;
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_OMC_TNR | IMGSYS_ENG_LTR |
		IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dev_info(imgsys_dev->dev,
			"%s: TOBE CHECKED SELECTION BASED ON FMT..\n",
			__func__);
		break;
	case (IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_TRAW | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_TRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"TRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_LTR | IMGSYS_ENG_DIP):
	case (IMGSYS_ENG_LTR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_LTR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_LTR | IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A |
		IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_LTRAW_TO_DIP;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"LTRAW");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A):
	case (IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_B):
	case (IMGSYS_ENG_DIP | IMGSYS_ENG_PQDIP_A | IMGSYS_ENG_PQDIP_B):
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_A;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPA");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		/**/
		memset((char *)logBuf_inport, 0x0, sizeof(logBuf_inport));
		logBuf_inport[strlen(logBuf_inport)] = '\0';
		memset((char *)logBuf_outport, 0x0, sizeof(logBuf_outport));
		logBuf_outport[strlen(logBuf_outport)] = '\0';
		dl_path = IMGSYS_DL_DIP_TO_PQDIP_B;
		ret = snprintf(logBuf_inport, sizeof(logBuf_inport), "%s",
			"DIP");
		if (ret >= sizeof(logBuf_inport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		ret = snprintf(logBuf_outport, sizeof(logBuf_outport), "%s",
			"PQDIPB");
		if (ret >= sizeof(logBuf_outport))
			dev_dbg(imgsys_dev->dev,
				"%s: string truncated\n", __func__);
		imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
			logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		break;
	case (IMGSYS_ENG_ADL_A | IMGSYS_ENG_LTR):
	case (IMGSYS_ENG_ADL_A | IMGSYS_ENG_ADL_B | IMGSYS_ENG_LTR):
		/**
		 * dl_path = IMGSYS_DL_ADLA_LTRAW;
		 * snprintf(logBuf_inport, sizeof(logBuf_inport), "%s", "ADL");
		 * snprintf(logBuf_outport, sizeof(logBuf_outport), "%s", "LTRAW");
		 * imgsys_dl_checksum_dump(imgsys_dev, hw_comb,
		 *  logBuf_path, logBuf_inport, logBuf_outport, dl_path);
		 */
		dev_info(imgsys_dev->dev,
			"%s: we dont have checksum for ADL DL LTRAW\n",
			__func__);
		break;
	case (IMGSYS_ENG_ME):
		imgsys_cg_debug_dump(imgsys_dev, hw_comb);
		break;
	default:
		break;
	}

	dev_info(imgsys_dev->dev, "%s: -\n", __func__);
}

