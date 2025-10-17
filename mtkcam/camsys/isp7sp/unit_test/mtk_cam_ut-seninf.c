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

#include "mtk_cam_ut.h"
#include "mtk_cam_ut-engines.h"
#include "mtk_cam_regs.h"

#define SAT_MUX_FACTOR 8
#define SV_NORMAL_MUX_FACTOR 1
#define RAW_MUX_FACTOR 4
#define PDP_MUX_FACTOR 1

static unsigned int testmdl_hblank = 0x80;
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

/* seninf */
static int get_test_hmargin(int w, int h, int clk_cnt, int clk_mhz, int fps)
{
	int target_h = clk_mhz * (1000000/fps) / w * max(16/(clk_cnt+1), 1);

	return max(target_h - h, 0x80);
}

#define SENINF_OFFSET 0x1000

static int get_seninf_by_mux_id(int mux_id)
{
	int seninf_id;

	switch (mux_id) {
	case SENINF_MUX1:
	case SENINF_MUX7:
	case SENINF_MUX11:
		seninf_id = SENINF_1; //using seninf_1
		break;
	case SENINF_MUX2:
	case SENINF_MUX8:
	case SENINF_MUX12:
		seninf_id = SENINF_2; //using seninf_2
		break;
	case SENINF_MUX3:
	case SENINF_MUX9:
	case SENINF_MUX13:
		seninf_id = SENINF_3; //using seninf_3
		break;
	case SENINF_MUX4:
	case SENINF_MUX10:
	case SENINF_MUX14:
		seninf_id = SENINF_4; //using seninf_4
		break;
	case SENINF_MUX5:
		seninf_id = SENINF_5; //using seninf_5
		break;
	case SENINF_MUX6:
		seninf_id = SENINF_6; //using seninf_6
		break;
	case SENINF_MUX15:
		seninf_id = SENINF_7; //using seninf_7
		break;
	default:
		return -EINVAL;
	}
	return seninf_id;
}

static int check_is_seninf_idle(
	struct mtk_ut_seninf_device *seninf, int seninf_idx)
{
	int i;

	for (i = 0; i < SENINF_NUM; i++) {
		if (i != seninf_idx)
			continue;

		return (seninf->seninf_status[i] == USING) ? USING : IDLE;
	}

	return -EINVAL;
}

static int get_mux_by_tg_idx(struct device *dev, int tg_idx)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int cam_type = -1;
	int range_begin, range_end;
	int mux_id, seninf_idx;

	if (tg_idx >= seninf->cammux_camsv_sat_range[0] &&
		tg_idx <= seninf->cammux_camsv_sat_range[1]) {
		cam_type = TYPE_CAMSV_SAT;

	} else if (tg_idx >= seninf->cammux_camsv_range[0] &&
			   tg_idx <= seninf->cammux_camsv_range[1]) {
		cam_type = TYPE_CAMSV_NORMAL;

	} else if (tg_idx >= seninf->cammux_raw_range[0] &&
			   tg_idx <= seninf->cammux_raw_range[1]) {
		cam_type = TYPE_RAW;

	} else if (tg_idx >= seninf->cammux_pdp_range[0] &&
			   tg_idx <= seninf->cammux_pdp_range[1]) {
		cam_type = TYPE_PDP;
	}

	dev_info(dev, "[%s] cam_type %d tg_idx %d\n",
		__func__, cam_type, tg_idx);

	switch (cam_type) {
	case TYPE_CAMSV_SAT:
		range_begin = seninf->mux_camsv_sat_range[0];
		range_end = seninf->mux_camsv_sat_range[1];
		break;

	case TYPE_CAMSV_NORMAL:
		range_begin = seninf->mux_camsv_range[0];
		range_end = seninf->mux_camsv_range[1];
		break;

	case TYPE_RAW:
		range_begin = seninf->mux_raw_range[0];
		range_end = seninf->mux_raw_range[1];
		break;

	case TYPE_PDP:
		range_begin = seninf->mux_pdp_range[0];
		range_end = seninf->mux_pdp_range[1];
		break;

	default:
		dev_info(dev, "[%s] error: invailed cammux_range_id %d\n",
		 __func__, cam_type);
		return -ENODEV;
	}

	for (mux_id = range_begin; mux_id <= range_end; mux_id++) {
		if (seninf->seninf_mux_status[mux_id] != IDLE)
			continue;

		dev_info(dev, "[%s] target idle mux %d with cam_type %d\n",
			__func__, mux_id, cam_type);

		seninf_idx = get_seninf_by_mux_id(mux_id);
		if (seninf_idx < 0) {
			dev_info(dev,
				"%s error get_seninf_by_mux_id return failed\n", __func__);
			return -ENODEV;
		}

		if (check_is_seninf_idle(seninf, seninf_idx) != IDLE)
			continue;

		dev_info(dev, "[%s] target idle seninf %d with mux %d cam_type %d\n",
			__func__, seninf_idx, mux_id, cam_type);

		seninf->seninf_mux_status[mux_id] = USING;
		return mux_id;
	}

	dev_info(dev, "[%s] error: cannot get idle seninf_mux in cam_tpye %d\n",
			 __func__, cam_type);

	return -EINVAL;
}

static int ut_seninf_set_top_mux(struct device *dev, void __iomem *seninf_base,
			int mux_idx, int seninf_src)
{

	switch (mux_idx) {
	case SENINF_MUX1:
	case SENINF_MUX2:
	case SENINF_MUX3:
	case SENINF_MUX4:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_0(seninf_base),
				mux_idx,
				seninf_src);
		break;
	case SENINF_MUX5:
	case SENINF_MUX6:
	case SENINF_MUX7:
	case SENINF_MUX8:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_1(seninf_base),
				mux_idx,
				seninf_src);
		break;

	case SENINF_MUX9:
	case SENINF_MUX10:
	case SENINF_MUX11:
	case SENINF_MUX12:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_2(seninf_base),
				mux_idx,
				seninf_src);
		break;
	case SENINF_MUX13:
	case SENINF_MUX14:
	case SENINF_MUX15:
	case SENINF_MUX16:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_3(seninf_base),
				mux_idx,
				seninf_src);
		break;
	case SENINF_MUX17:
	case SENINF_MUX18:
	case SENINF_MUX19:
	case SENINF_MUX20:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_4(seninf_base),
				mux_idx,
				seninf_src);
		break;
	case SENINF_MUX21:
	case SENINF_MUX22:
		WRITE_SENINF_TOP_MUX_BITS(
				ISP_SENINF_TOP_MUX_CTRL_5(seninf_base),
				mux_idx,
				seninf_src);
		break;
	default:
		dev_info(dev, "invalid mux_idx %d\n", mux_idx);
		return -EINVAL;
	}


	dev_info(dev,
	"top_mux_ctrl_0: 0x%x   top_mux_ctrl_1: 0x%x   top_mux_ctrl_2: 0x%x   top_mux_ctrl_3: 0x%x   top_mux_ctrl_4: 0x%x   top_mux_ctrl_5: 0x%x\n",
		readl(ISP_SENINF_TOP_MUX_CTRL_0(seninf_base)),
		readl(ISP_SENINF_TOP_MUX_CTRL_1(seninf_base)),
		readl(ISP_SENINF_TOP_MUX_CTRL_2(seninf_base)),
		readl(ISP_SENINF_TOP_MUX_CTRL_3(seninf_base)),
		readl(ISP_SENINF_TOP_MUX_CTRL_4(seninf_base)),
		readl(ISP_SENINF_TOP_MUX_CTRL_5(seninf_base)));
	return 0;
}

static int ut_seninf_set_cammux_tag(struct device *dev,
		void __iomem *seninf_base, int tg_idx, int tag)
{
		int page, tag_sel;
		void __iomem *cam_mux_addr = (void *)(seninf_base + tg_idx * 0x40);

	if (tag >= 0 && tag <= 31) {
		page = tag / 4;
		tag_sel = tag % 4;
		SET_TAG(cam_mux_addr, page, tag_sel, 0, 0, 1);
/*
		dev_info(dev,
			"[before] cam_mux_base: 0x%p   CAM_MUX_TAG_VC_SEL_addr: 0x%p   CAM_MUX_TAG_VC_SEL: 0x%x   CAM_MUX_TAG_DT_SEL: 0x%x\n",
		(unsigned int *)cam_mux_addr,
		ISP_SENINF_CAM_MUX_PCSR_VC_SEL((unsigned int *)cam_mux_addr),
		readl(ISP_SENINF_CAM_MUX_PCSR_VC_SEL((unsigned int *)cam_mux_addr)),
		readl(ISP_SENINF_CAM_MUX_PCSR_DT_SEL((unsigned int *)cam_mux_addr)));
*/
		dev_info(dev, "page = %d, tag_sel=%d\n", page, tag_sel);
/*
		dev_info(dev,
			"[after] cam_mux_base: 0x%p   CAM_MUX_TAG_VC_SEL_addr: 0x%p   CAM_MUX_TAG_VC_SEL: 0x%x   CAM_MUX_TAG_DT_SEL: 0x%x\n",
		(unsigned int *)cam_mux_addr,
		ISP_SENINF_CAM_MUX_PCSR_VC_SEL((unsigned int *)cam_mux_addr),
		readl(ISP_SENINF_CAM_MUX_PCSR_VC_SEL((unsigned int *)cam_mux_addr)),
		readl(ISP_SENINF_CAM_MUX_PCSR_DT_SEL((unsigned int *)cam_mux_addr)));
*/
	}

	return 0;
}

static int ut_seninf_get_muxvr_by_mux(struct device *dev, unsigned int mux)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);

	int mux_vr = mux;
	int sat_mux_first = seninf->mux_camsv_sat_range[0];
	int sat_mux_second = seninf->mux_camsv_sat_range[1];
	int sat_muxvr_first = seninf->muxvr_camsv_sat_range[0];

	int sv_normal_mux_first = seninf->mux_camsv_range[0];
	int sv_normal_mux_second = seninf->mux_camsv_range[1];
	int sv_normal_muxvr_first = seninf->muxvr_camsv_range[0];

	int raw_mux_first = seninf->mux_raw_range[0];
	int raw_mux_second = seninf->mux_raw_range[1];
	int raw_muxvr_first = seninf->muxvr_raw_range[0];

	int pdp_mux_first = seninf->mux_pdp_range[0];
	int pdp_mux_secnond = seninf->mux_pdp_range[1];
	int pdp_muxvr_first = seninf->muxvr_pdp_range[0];

	if (mux < sat_mux_first) {
		dev_info(dev,
				"[%s][ERROR] Input(mux_id %d) is invalid\n", __func__, mux);

	} else if ((mux >= sat_mux_first) && (mux <= sat_mux_second)) {  // sat camsv

		mux_vr = ((mux - sat_mux_first) * SAT_MUX_FACTOR) + sat_mux_first;

	} else if ((mux >= sv_normal_mux_first) && (mux <= sv_normal_mux_second)) {  // normal camsv

		mux_vr = (mux - sv_normal_mux_first) + sv_normal_muxvr_first;

	} else if ((mux >= raw_mux_first) && (mux <= raw_mux_second)) {  // raw

		mux_vr = ((mux - raw_mux_first) * RAW_MUX_FACTOR) + raw_muxvr_first;

	} else if ((mux >= pdp_mux_first) && (mux <= pdp_mux_secnond)) {  // PDP

		mux_vr = (mux - pdp_mux_first) + pdp_muxvr_first;

	} else {
		dev_info(dev,
				"[%s][ERROR] Input(mux_id %d) is invalid\n", __func__, mux);
	}

	dev_info(dev,
				"[%s] sat_based %d sv_based %d raw_based %d, pdp_based %d\n",
				__func__,
				sat_mux_first,
				sat_muxvr_first,
				sv_normal_muxvr_first,
				raw_muxvr_first);

	dev_info(dev,
				"[%s] Input(mux_id %d, Output(mux_vr %d)\n",
				__func__, mux, mux_vr);

	return mux_vr;
}

static int ut_seninf_set_testmdl(struct device *dev,
				 int width, int height,
				 int pixmode_lg2,
				 int pattern,
				 int tg_idx,
				 int tag)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	unsigned int cam_mux_ctrl;
	int mux_idx;
	int seninf_idx, mux_vr;
	void __iomem *cam_mux_ctrl_addr;
	void __iomem *seninf_base;
	void __iomem *mux_base;
	const u16 dummy_pxl = testmdl_hblank, h_margin = 0x1000;
	const u8 clk_div_cnt = (16 >> (pixmode_lg2 ? 1 : 0)) - 1;
	const u16 dum_vsync = get_test_hmargin(width + dummy_pxl,
					      height + h_margin,
					      clk_div_cnt, 416, 30);

	mux_idx = get_mux_by_tg_idx(dev, tg_idx);
	if (mux_idx < 0) {
		dev_info(dev, "%s error get_mux_by_tg_idx return failed\n", __func__);
		return -ENODEV;
	}

	seninf_idx = get_seninf_by_mux_id(mux_idx);
	if (seninf_idx < 0) {
		dev_info(dev, "%s error get_seninf_by_mux_id return failed\n", __func__);
		return -ENODEV;
	}

	dev_info(dev, "%s width %d x height %d dum_vsync %d pixmode_lg2 %d clk_div_cnt %d\n",
		 __func__, width, height, dum_vsync,
		 pixmode_lg2, clk_div_cnt);
	dev_info(dev, "%s seninf_idx %d tg_idx %d\n", __func__, seninf_idx, tg_idx);

	/* test mdl */
	seninf->seninf_status[seninf_idx] = USING;
	seninf_base = seninf->base + seninf_idx * SENINF_OFFSET;
	writel((height + h_margin) << 16 | width, ISP_SENINF_TM_SIZE(seninf_base));
	writel(clk_div_cnt, ISP_SENINF_TM_CLK(seninf_base));
	writel(dum_vsync << 16 | dummy_pxl, ISP_SENINF_TM_DUM(seninf_base));
	writel(0xa0a << 12 | pattern << 4 | 0x1, ISP_SENINF_TM_CTL(seninf_base));
	writel(0x1, ISP_SENINF_TSETMDL_CTRL(seninf_base));
	writel(0x1, ISP_SENINF_CTRL(seninf_base));

	/* set seninf_top_mux */
	if (ut_seninf_set_top_mux(dev, seninf->base, mux_idx, seninf_idx)) {
		dev_info(dev, "ut_seninf_set_top_mux return failed\n");
		return -EINVAL;
	}

	/* set seninf_mux */
	mux_base = seninf->base + mux_idx * SENINF_OFFSET;
	if ((tg_idx >= raw_tg_0) && (tg_idx <= raw_tg_2))
		writel(0x1f << 16 | 3 << 8 | 0x1, ISP_SENINF_MUX_CTRL_1(mux_base));
	else
		writel(0x1f << 16 | pixmode_lg2 << 8 | 0x1, ISP_SENINF_MUX_CTRL_1(mux_base));
	writel(0x1, ISP_SENINF_MUX_CTRL_0(mux_base));
	writel(0x1 << 3, ISP_SENINF_MUX_OPT(mux_base));  // EN VC split

	/* cam mux ctrl */
	cam_mux_ctrl_addr =
		(void *)(ISP_SENINF_CAM_MUX_PCSR_CTRL(seninf->base) + tg_idx * 0x40);

	if (ut_seninf_set_cammux_tag(dev, seninf->base, tg_idx, tag)) {
		dev_info(dev, "ut_seninf_set_cammux_tag return failed\n");
		return -EINVAL;
	}

	mux_vr = ut_seninf_get_muxvr_by_mux(dev, mux_idx);
	if (mux_vr < 0) {
		dev_info(dev, "ut_seninf_get_muxvr_by_mux return failed\n");
		return -EINVAL;
	}

	cam_mux_ctrl = readl(cam_mux_ctrl_addr);
	cam_mux_ctrl &= 0xFFFF7800;
	cam_mux_ctrl |= mux_vr;
	if ((tg_idx >= raw_tg_0) && (tg_idx <= raw_tg_2))
		cam_mux_ctrl |= (2 << 8); /* chk pix mode */
	else
		cam_mux_ctrl |= (pixmode_lg2 << 8); /* chk pix mode */
	cam_mux_ctrl |= 0x80; /* cam mux en */
	cam_mux_ctrl |= 0x8000; /* cam mux check en */
	writel(cam_mux_ctrl, cam_mux_ctrl_addr);
	dev_info(dev, "CAM_MUX_PCSR_%d: 0x%x\n", tg_idx, cam_mux_ctrl);

	return 0;
}

static int ut_seninf_reset(struct device *dev)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int min_seninfmux = 0;
	int max_seninfmux = 0;
	int i;

	max_seninfmux = seninf->mux_pdp_range[1];

	min_seninfmux = seninf->mux_camsv_sat_range[0];

	writel(0x3, ISP_SENINF_CAM_MUX_GCSR_CTRL(seninf->base));
	udelay(1);
	writel(0x0, ISP_SENINF_CAM_MUX_GCSR_CTRL(seninf->base));
	dev_info(dev, "cam_mux reset done\n");

	for (i = min_seninfmux; i < max_seninfmux; i++) {
		writel(0x04, (ISP_SENINF_MUX_CTRL_0(seninf->base) + i * 0x1000));
		udelay(1);
		writel(0x00, (ISP_SENINF_MUX_CTRL_0(seninf->base) + i * 0x1000));
		seninf->seninf_mux_status[i] = IDLE;
		dev_info(dev, "seninf_mux_%d reset done\n", i);
	}

	writel(0x01, seninf->base);
	udelay(1);
	writel(0x00, seninf->base);
	dev_info(dev, "seninf_top reset done\n");

	for (i = 0; i < SENINF_NUM; i++)
		seninf->seninf_status[i] = IDLE;
	dev_info(dev, "seninf reset done\n");

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

static int seninf_of_probe_range(struct device *dev, const char *range_prop,
				 int range[2])
{
	int i, ret;

	for (i = 0; i < 2; i++) {

		ret = of_property_read_u32_index(dev->of_node, range_prop,
						 i, range + i);
		if (ret) {
			dev_info(dev,
				 "%s: ERROR: read property %s, index %d failed, ret:%d\n",
				 __func__, range_prop, i, ret);
			return -1;
		}
	}

	dev_info(dev, "%s: %s: range [%d, %d]\n",
		 __func__, range_prop, range[0], range[1]);

	return 0;
}

static int mtk_ut_seninf_of_probe(struct platform_device *pdev,
			    struct mtk_ut_seninf_device *seninf)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i, clks;

	/* base register */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_info(dev, "failed to get mem\n");
		return -ENODEV;
	}

	seninf->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(seninf->base)) {
		dev_info(dev, "failed to map register base\n");
		return PTR_ERR(seninf->base);
	}
	dev_info(dev, "seninf, map_addr=0x%lx\n", (unsigned long)seninf->base);

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
		if (IS_ERR(seninf->clks[i])) {
			dev_info(dev, "failed to get clk %d\n", i);
			return -ENODEV;
		}
	}

	/* mux */
	if (seninf_of_probe_range(dev, "mux-camsv-sat-range",
				  seninf->mux_camsv_sat_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "mux-camsv-normal-range",
				  seninf->mux_camsv_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "mux-raw-range",
				  seninf->mux_raw_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "mux-pdp-range",
				  seninf->mux_pdp_range))
		return -ENODEV;

	/* mux */
	if (seninf_of_probe_range(dev, "muxvr-camsv-sat-range",
				  seninf->muxvr_camsv_sat_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "muxvr-camsv-normal-range",
				  seninf->muxvr_camsv_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "muxvr-raw-range",
				  seninf->muxvr_raw_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "muxvr-pdp-range",
				  seninf->muxvr_pdp_range))
		return -ENODEV;

	/* cammux */
	if (seninf_of_probe_range(dev, "cammux-camsv-sat-range",
				  seninf->cammux_camsv_sat_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "cammux-camsv-normal-range",
			  seninf->cammux_camsv_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "cammux-raw-range",
				  seninf->cammux_raw_range))
		return -ENODEV;

	if (seninf_of_probe_range(dev, "cammux-pdp-range",
				  seninf->cammux_pdp_range))
		return -ENODEV;


	// init mux_status as all mux are free to used
	for (i = 0; i < SENINF_MUX_NUM; i++)
		seninf->seninf_mux_status[i] = IDLE;

	// init seninf_status as all mux are free to used
	for (i = 0; i < SENINF_NUM; i++)
		seninf->seninf_status[i] = IDLE;

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

	pm_runtime_enable(dev);

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

	for (i = 0; i < seninf->num_clks; i++)
		clk_disable_unprepare(seninf->clks[i]);

	return 0;
}

static int mtk_ut_seninf_runtime_resume(struct device *dev)
{
	struct mtk_ut_seninf_device *seninf = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < seninf->num_clks; i++)
		clk_prepare_enable(seninf->clks[i]);

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

