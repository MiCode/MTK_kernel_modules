// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>

#include "mtk_cam_ut.h"
#include "mtk_cam_ut-engines.h"
#include "mtk_cam_regs.h"

#define MT6899_IOMOM_VERSIONS "mt6899"
#define MT6991_IOMOM_VERSIONS "mt6991"
const char *iomem_ver;
static unsigned int testmdl_hblank = 0x400;
module_param(testmdl_hblank, int, 0644);
MODULE_PARM_DESC(testmdl_hblank, "h-blanking for testmdl");

#define WRITE_BITS(addr, offset, mask, val) do { \
	u32 __v = readl(addr); \
	u32 field_mask = mask << offset;\
	__v &= ~field_mask; \
	__v |= (((val) << offset) & field_mask); \
	writel(__v, addr); \
} while (0)

#define WRITE_SENINF_TOP_MUX_BITS(addr, mux_idx, val) do { \
	u32 idx = mux_idx % 4; \
	u32 offset = idx * 8;\
	WRITE_BITS(addr, offset, 0x1f, val);\
} while (0)

#define SET_TAG(ptr, page, sel, vc, dt, first) do { \
	u32 offset = 24;\
	WRITE_BITS(ISP_SENINF_CAM_MUX_PCSR_OPT(ptr), offset, 0x7, page);\
	offset = sel * 8;\
	WRITE_BITS(ISP_SENINF_CAM_MUX_PCSR_VC_SEL(ptr), offset, 0x1f, vc);\
	offset = ((sel + 1) * 8) - 1;\
	WRITE_BITS(ISP_SENINF_CAM_MUX_PCSR_VC_SEL(ptr), offset, 0x1, first);\
	offset = sel * 8;\
	WRITE_BITS(ISP_SENINF_CAM_MUX_PCSR_DT_SEL(ptr), offset, 0x1f, dt);\
	offset = ((sel + 1) * 8) - 1;\
	WRITE_BITS(ISP_SENINF_CAM_MUX_PCSR_DT_SEL(ptr), offset, 0x1, first);\
} while (0)

enum CAMMUX_TYPE_ENUM {
	TYPE_CAMSV_SAT,
	TYPE_CAMSV_NORMAL,
	TYPE_RAW,
	TYPE_PDP,
	TYPE_MAX_NUM,
};

static enum tg_enum_remap tg_remap(enum tg_enum tg)
{
	switch (tg) {
	case camsv_tg_0:
		return camsv_tg_0_remap;
	case camsv_tg_1:
		return camsv_tg_1_remap;
	case camsv_tg_2:
		return camsv_tg_2_remap;
	case camsv_tg_3:
		return camsv_tg_3_remap;
	case camsv_tg_4:
		return camsv_tg_4_remap;
	case raw_tg_0:
		return raw_tg_0_remap;
	case raw_tg_1:
		return raw_tg_1_remap;
	case raw_tg_2:
		return raw_tg_2_remap;
	case pdp_tg_0:
		return pdp_tg_0_remap;
	case pdp_tg_1:
		return pdp_tg_1_remap;
	case pdp_tg_2:
		return pdp_tg_2_remap;
	default:
		return camsv_tg_0_remap;
	}
}

/* seninf */
static int get_test_hmargin(int w, int h, int clk_cnt, int clk_mhz, int fps)
{
	int target_h = clk_mhz * (1000000/fps) / w * max(16/(clk_cnt+1), 1);

	return max(target_h - h, 0x80);
}

#define SENINF_TM_OFFSET 0x200

static int ut_seninf_set_testmdl(struct device *dev,
				 int width, int height,
				 int pattern,/*unused*/
				 u8 exp_num,
				 struct mtk_cam_ut_tm_para *tm_para,
				 int para_cnt)
{
	int i;
	struct mtk_cam_ut_tm_para *para;
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	unsigned int seninf_idx, outmux_idx;
	int tag, exp_no;
	void __iomem *seninf_top;
	void __iomem *seninf_async;
	void __iomem *seninf_tm;
	void __iomem *outmux_base;
	const u16 dummy_pxl = testmdl_hblank, h_margin = 0x1000;
	//const u8 clk_div_cnt = (16 >> (pixmode_lg2 ? 1 : 0)) - 1;
	const u8 clk_div_cnt = 0xF;
	const u16 dum_vsync = get_test_hmargin(width + dummy_pxl,
					      height + h_margin,
					      clk_div_cnt, 416, 30);
	int width_tm = (width >> 1);
	u8 set_outmux_list[camsys_tg_max];
	u8 pix_m = 0;
	u8 last_vc = 0;

	memset(set_outmux_list, 0, sizeof(set_outmux_list));

	if (width_tm % 8) // width_tm must be 8x, ceil to 8x
		width_tm = ((width_tm >> 3) + 1) << 3;

	// hard code seninf idx use first seninf_idx
	seninf_idx = 0;

	dev_info(dev, "%s width %d x height %d dum_vsync %d clk_div_cnt %d, to width_tm %d\n",
		 __func__, width, height, dum_vsync,
		 clk_div_cnt, width_tm);

	if (!exp_num)// exp_num is 0, set to 1
		exp_num = 1;

	/* test mdl */
	seninf->seninf_status[seninf_idx] = USING;
	seninf_top = seninf->base_top;
	seninf_async = seninf->base_async;
	seninf_tm = seninf->base_tm + seninf_idx * SENINF_TM_OFFSET;

	writel(height << 16 | width_tm, ISP_SENINF_TM_SIZE(seninf_tm));
	writel((clk_div_cnt << 16 | 0x801) | (((exp_num - 1) & 0x7) << 12), ISP_SENINF_TM_CORE0_CTL(seninf_tm));
	writel(dum_vsync << 16 | dummy_pxl, ISP_SENINF_TM_DUM(seninf_tm));
	writel(0x2b, ISP_SENINF_TM_CON0(seninf_tm));// dt
	if (exp_num > 1) {
		writel(0x1012b, ISP_SENINF_TM_CON1(seninf_tm));// vcdt
		writel((0x64 << 4) | (0x1 << 30), ISP_SENINF_TM_EXP1_CTRL(seninf_tm));// sof offset 100 and dedicated fs
		last_vc = 1;
	}
	if (exp_num > 2) {
		writel(0x2022b, ISP_SENINF_TM_CON2(seninf_tm));// vcdt
		writel((0xc8 << 4) | (0x1 << 30), ISP_SENINF_TM_EXP2_CTRL(seninf_tm));// sof offset 200 and dedicated fs
		last_vc = 2;
	}

	/* seninf async */
	writel(0x1, ISP_SENINF_ASYNC_CFG(seninf_async));

	/* seninf top */
	writel(0x1000000, ISP_SENINF_TOP_CTL(seninf_top));
	writel(0x0000003F, ISP_SENINF_TOP_ASYNC_CG(seninf_top));
	writel(0x7FFFFF, ISP_SENINF_TOP_OUTMUX_CG(seninf_top));

	for (i = 0; i < para_cnt; i++) {
		para = tm_para + i;
		if (!strcasecmp(iomem_ver, MT6899_IOMOM_VERSIONS))
			outmux_idx = (unsigned int)tg_remap(para->tg_idx);
		else
			outmux_idx = (unsigned int)para->tg_idx;
		exp_no = para->exp_no;
		tag = para->tag;
		pix_m = (para->pixmode == tm_pix_mode_16) ? 1 : 0;
		dev_info(dev, "%s seninf_idx %u outmux_idx %u tag %d pixmode %d\n",
			 __func__, seninf_idx, outmux_idx, tag, para->pixmode);

		if (outmux_idx >= (unsigned int)camsys_tg_max)
			continue;

		set_outmux_list[outmux_idx] = 1;

		/* outmux */
		outmux_base = seninf->base_outmux[outmux_idx];
		writel(pix_m, ISP_SENINF_OUTMUX_PIX_MODE(outmux_base));
		writel(0x0 | (last_vc << 16), ISP_SENINF_OUTMUX_SOURCE_CFG0(outmux_base));
		writel(0x0, ISP_SENINF_OUTMUX_SRC_SEL(outmux_base));
		writel(0x2b0001 | (exp_no << 8), ISP_SENINF_OUTMUX_TAG_VCDT(outmux_base, tag));

		if (tag >= tag_0 && tag <= tag_3)
			writel((exp_no << (tag * 8)), ISP_SENINF_OUTMUX_SOURCE_CFG1(outmux_base));
		else if (tag >= tag_4 && tag <= tag_7)
			writel((exp_no << ((tag - tag_4) * 8)), ISP_SENINF_OUTMUX_SOURCE_CFG2(outmux_base));
	}

	for (i = 0; i < camsys_tg_max; i++) {
		if (set_outmux_list[i]) {
			outmux_base = seninf->base_outmux[i];
			writel(0x1, ISP_SENINF_OUTMUX_CFG_RDY(outmux_base));
			writel(0x1, ISP_SENINF_OUTMUX_CFG_DONE(outmux_base));
		}

	}


	return 0;
}

static int ut_seninf_reset(struct device *dev)
{
	return 0;
}

static void ut_seninf_set_ops(struct device *dev)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);

	seninf->ops.set_size = ut_seninf_set_testmdl;
	seninf->ops.reset = ut_seninf_reset;
}

static int mtk_ut_seninf_component_bind(struct device *dev,
					struct device *master,
					void *data)
{
	struct mtk_cam_ut *ut = data;

	dev_dbg(dev, "%s\n", __func__);
	ut->seninf = dev;

	return 0;
}

static void mtk_ut_seninf_component_unbind(struct device *dev,
					   struct device *master,
					   void *data)
{
	struct mtk_cam_ut *ut = data;

	dev_dbg(dev, "%s\n", __func__);
	ut->seninf = NULL;
}

static const struct component_ops mtk_ut_seninf_component_ops = {
	.bind = mtk_ut_seninf_component_bind,
	.unbind = mtk_ut_seninf_component_unbind,
};

static int mtk_ut_seninf_of_probe(struct platform_device *pdev,
			    struct mtk_ut_seninf_device *seninf)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i, clks;
	struct device_node *tmp_node = NULL;
	int index;

	if (of_property_read_string(dev->of_node, "mtk-iomem-ver", &iomem_ver))
		iomem_ver = "mt6991";
	dev_info(dev, "mtk_iomem_ver = %s\n", iomem_ver);

	/* top base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "seninf-top");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	seninf->base_top = devm_ioremap_resource(dev, res);
	if (IS_ERR(seninf->base_top)) {
		dev_info(dev, "failed to map register base top\n");
		return PTR_ERR(seninf->base_top);
	}
	dev_info(dev, "seninf, map_addr=0x%lx\n", (unsigned long)seninf->base_top);

	/* async base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "seninf-async-top");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	seninf->base_async = devm_ioremap_resource(dev, res);
	if (IS_ERR(seninf->base_async)) {
		dev_info(dev, "failed to map register base async\n");
		return PTR_ERR(seninf->base_async);
	}
	dev_info(dev, "seninf, map_addr=0x%lx\n", (unsigned long)seninf->base_async);

	/* tm base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "seninf-tm");
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	seninf->base_tm = devm_ioremap_resource(dev, res);
	if (IS_ERR(seninf->base_tm)) {
		dev_info(dev, "failed to map register base tm\n");
		return PTR_ERR(seninf->base_tm);
	}
	dev_info(dev, "seninf, map_addr=0x%lx\n", (unsigned long)seninf->base_tm);

	/* outmux base register */
	i = 0;
	while ((tmp_node = of_find_compatible_node(tmp_node, NULL, "mediatek,seninf-outmux"))) {
		index = of_property_match_string(tmp_node, "reg-names", "base");
		if (index < 0) {
			/* Fail */
			dev_info(dev, "get seninf outmux reg base failed\n");
		} else {
			/* Success */
			dev_info(dev, "get seninf outmux reg base succeeded\n");

			seninf->base_outmux[i] = devm_of_iomap(dev, tmp_node, index, NULL);
			if (IS_ERR(seninf->base_outmux[i]))
				dev_info(dev, "seninf outmux[%d] ioremap failed\n", i);
			else
				i++;
		}
	}
	seninf->num_outmux = i;


	clks = of_count_phandle_with_args(pdev->dev.of_node,
				"clocks", "#clock-cells");

	seninf->num_clks = (clks == -ENOENT) ? 0:clks;
	dev_info(dev, "clk_num:%d\n", seninf->num_clks);

	if (seninf->num_clks) {
		seninf->clks = devm_kcalloc(dev, seninf->num_clks,
					    sizeof(*seninf->clks), GFP_KERNEL);
		if (!seninf->clks)
			return -ENODEV;
	}

	for (i = 0; i < seninf->num_clks; i++) {
		seninf->clks[i] = of_clk_get(pdev->dev.of_node, i);
		dev_info(dev, "get clk[%d](0x%p)\n", i, seninf->clks[i]);
		if (IS_ERR(seninf->clks[i])) {
			dev_info(dev, "failed to get clk[%d]\n", i);
			return -ENODEV;
		}
	}

	// init mux_status as all mux are free to used
	for (i = 0; i < SENINF_MUX_NUM; i++)
		seninf->seninf_mux_status[i] = IDLE;

	// init seninf_status as all mux are free to used
	for (i = 0; i < SENINF_NUM; i++)
		seninf->seninf_status[i] = IDLE;

	return 0;
}

static int mtk_ut_seninf_pm_runtime_enable(struct mtk_ut_seninf_device *seninf)
{
	int i;

	seninf->pm_domain_cnt = of_count_phandle_with_args(seninf->dev->of_node,
					"power-domains",
					"#power-domain-cells");
	dev_info(seninf->dev, "pm_domain_cnt = %d\n", seninf->pm_domain_cnt);
	pm_runtime_enable(seninf->dev);
	if (seninf->pm_domain_cnt > 1) {
		seninf->pm_domain_devs = devm_kcalloc(seninf->dev, seninf->pm_domain_cnt,
					sizeof(*seninf->pm_domain_devs), GFP_KERNEL);
		if (!seninf->pm_domain_devs)
			return -ENOMEM;

		for (i = 0; i < seninf->pm_domain_cnt; i++) {
			seninf->pm_domain_devs[i] = dev_pm_domain_attach_by_id(seninf->dev, i);

			if (IS_ERR_OR_NULL(seninf->pm_domain_devs[i])) {
				dev_info(seninf->dev, "%s: fail to probe pm id %d\n", __func__, i);
				seninf->pm_domain_devs[i] = NULL;
			}
		}
	}

	return 0;
}

static int mtk_ut_seninf_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_seninf_device *seninf;
	int ret;

	dev_info(dev, "%s\n", __func__);

	seninf = devm_kzalloc(dev, sizeof(*seninf), GFP_KERNEL);
	if (!seninf)
		return -ENOMEM;

	seninf->dev = dev;
	dev_set_drvdata(dev, seninf);

	ret = mtk_ut_seninf_of_probe(pdev, seninf);
	if (ret)
		return ret;

	ut_seninf_set_ops(dev);

	mtk_ut_seninf_pm_runtime_enable(seninf);

	ret = component_add(dev, &mtk_ut_seninf_component_ops);
	if (ret)
		return ret;

	dev_info(dev, "%s: success\n", __func__);
	return 0;
}

static int mtk_ut_seninf_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int i;

	dev_info(dev, "%s\n", __func__);

	for (i = 0; i < seninf->num_clks; i++) {
		if (seninf->clks[i])
			clk_put(seninf->clks[i]);
	}

	pm_runtime_disable(dev);
	if (seninf->pm_domain_cnt > 1) {
		if (!seninf->pm_domain_devs)
			return -EINVAL;

		for (i = 0; i < seninf->pm_domain_cnt; i++) {
			if (seninf->pm_domain_devs[i])
				dev_pm_domain_detach(seninf->pm_domain_devs[i], 1);
		}
	}

	component_del(dev, &mtk_ut_seninf_component_ops);
	return 0;
}

static int mtk_ut_seninf_pm_suspend(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}

static int mtk_ut_seninf_pm_resume(struct device *dev)
{
	dev_dbg(dev, "- %s\n", __func__);
	return 0;
}

static int mtk_ut_seninf_runtime_suspend(struct device *dev)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int i;
	int ret = 0;

	dev_info(dev, "pm_domain_cnt = %d\n", seninf->pm_domain_cnt);
	if (seninf->pm_domain_cnt > 1) {
		if (!seninf->pm_domain_devs)
			return -ENOMEM;
		for (i = seninf->pm_domain_cnt - 1; i >= 0; i--) {
			if (seninf->pm_domain_devs[i] != NULL) {
				ret = pm_runtime_put_sync(seninf->pm_domain_devs[i]);
				dev_info(dev, "pm_runtime_put_sync[%d], ret(%d)\n", i, ret);
			}
		}
	}

	for (i = 0; i < seninf->num_clks; i++)
		clk_disable_unprepare(seninf->clks[i]);

	return 0;
}

static int mtk_ut_seninf_runtime_resume(struct device *dev)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int i;
	int ret = 0;

	dev_info(dev, "pm_domain_cnt = %d\n", seninf->pm_domain_cnt);
	if (seninf->pm_domain_cnt > 1) {
		if (!seninf->pm_domain_devs)
			return -EINVAL;
		for (i = 0; i < seninf->pm_domain_cnt; i++) {
			if (seninf->pm_domain_devs[i] != NULL) {
				ret = pm_runtime_get_sync(seninf->pm_domain_devs[i]);
				dev_info(dev, "pm_runtime_get_sync[%d], ret(%d)\n", i, ret);
			}
		}
	}

	for (i = 0; i < seninf->num_clks; i++) {
		ret = clk_prepare_enable(seninf->clks[i]);
		dev_info(seninf->dev, "clk[%d](0x%p), ret(%d)\n", i, seninf->clks[i], ret);
	}

	return 0;
}

static const struct dev_pm_ops mtk_ut_seninf_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_ut_seninf_pm_suspend, mtk_ut_seninf_pm_resume)
	SET_RUNTIME_PM_OPS(mtk_ut_seninf_runtime_suspend, mtk_ut_seninf_runtime_resume,
			   NULL)
};

static const struct of_device_id mtk_ut_seninf_of_ids[] = {
	{.compatible = "mediatek,seninf-core",},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_ut_seninf_of_ids);

struct platform_driver mtk_ut_seninf_driver = {
	.probe   = mtk_ut_seninf_probe,
	.remove  = mtk_ut_seninf_remove,
	.driver  = {
		.name  = "mtk-cam seninf-ut",
		.of_match_table = of_match_ptr(mtk_ut_seninf_of_ids),
		.pm     = &mtk_ut_seninf_pm_ops,
	}
};

