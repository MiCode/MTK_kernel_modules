// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/minmax.h>

#include "mtk_cam-seninf.h"
#include "mtk_cam-seninf-hw.h"
#include "mtk_cam-seninf-regs.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-top-ctrl.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-seninf1-mux.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-seninf1.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-seninf1-csi2.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-cammux-gcsr.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-cammux-pcsr.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-mipi-csi-top-ctrl.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-mipi-rx-ana-cdphy-csi0a.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-csi0-cphy.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-csi0-dphy.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-csirx_mac_csi0.h"
#include "mtk_csi_phy_3_1/mtk_cam-seninf-csirx_mac_top.h"

#include "mtk_cam-seninf_control-7sp.h"
#include "mtk_cam-seninf-route.h"
#include "mtk_cam-seninf-tsrec.h"
#include "imgsensor-user.h"

#define SENINF_CK 312000000
#define CYCLE_MARGIN 1
#define RESYNC_DMY_CNT 4
#define DPHY_SETTLE_DEF 100  // ns
#define CPHY_SETTLE_DEF 70 //60~80ns
#define DPHY_TRAIL_SPEC 224
#define FT_30_FPS 33

#define PIX_MODE_16_REG_VAL 4
//#define SCAN_SETTLE

#define MT6989_IOMOM_VERSIONS "mt6989"
#define MT6878_IOMOM_VERSIONS "mt6878"

static struct mtk_cam_seninf_ops *_seninf_ops = &mtk_csi_phy_3_1;
static struct mtk_cam_seninf_irq_event_st vsync_detect_seninf_irq_event;
#define SENINF_IRQ_FIFO_LEN 36
#define VSYNC_DUMP_BUF_MAX_LEN 2048

#define SENINF_SNPRINTF(buf, len, fmt, ...) { \
	len += snprintf(buf + len, VSYNC_DUMP_BUF_MAX_LEN - len, fmt, ##__VA_ARGS__); \
}

#define SET_DI_CTRL(ptr, s, vc) do { \
	SENINF_BITS(ptr, SENINF_CSI2_S##s##_DI_CTRL, \
			RG_CSI2_S##s##_DT_SEL, vc->dt); \
	SENINF_BITS(ptr, SENINF_CSI2_S##s##_DI_CTRL, \
			RG_CSI2_S##s##_VC_SEL, vc->vc); \
	SENINF_BITS(ptr, SENINF_CSI2_S##s##_DI_CTRL, \
			RG_CSI2_S##s##_DT_INTERLEAVE_MODE, 1); \
	SENINF_BITS(ptr, SENINF_CSI2_S##s##_DI_CTRL, \
			RG_CSI2_S##s##_VC_INTERLEAVE_EN, 1); \
} while (0)

#define SET_CH_CTRL(ptr, ch, s) \
	SENINF_BITS(ptr, SENINF_CSI2_CH##ch##_CTRL, \
		RG_CSI2_CH##ch##_S##s##_GRP_EN, 1)

#define SET_DI_CH_CTRL(ptr, s, vc) do { \
	SET_DI_CTRL(ptr, s, vc); \
	SET_CH_CTRL(ptr, 0, s); \
	if (vc->group == VC_CH_GROUP_1) \
		SET_CH_CTRL(ptr, 1, s); \
	else if (vc->group == VC_CH_GROUP_2) \
		SET_CH_CTRL(ptr, 2, s); \
	else if (vc->group == VC_CH_GROUP_3) \
		SET_CH_CTRL(ptr, 3, s); \
} while (0)

#define SET_TAG(ptr, page, sel, vc, dt, first) do { \
	SENINF_BITS(ptr, SENINF_CAM_MUX_PCSR_OPT, \
		    RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL, page); \
	SENINF_BITS(ptr, SENINF_CAM_MUX_PCSR_TAG_VC_SEL, \
		    RG_SENINF_CAM_MUX_TAG_VC_SEL_4N_##sel, vc); \
	SENINF_BITS(ptr, SENINF_CAM_MUX_PCSR_TAG_VC_SEL, \
		    RG_SENINF_CAM_MUX_TAG_VC_SEL_4N_##sel##_EN, first); \
	SENINF_BITS(ptr, SENINF_CAM_MUX_PCSR_TAG_DT_SEL, \
		    RG_SENINF_CAM_MUX_TAG_DT_SEL_4N_##sel, dt); \
	SENINF_BITS(ptr, SENINF_CAM_MUX_PCSR_TAG_DT_SEL, \
		    RG_SENINF_CAM_MUX_TAG_DT_SEL_4N_##sel##_EN, first); \
} while (0)

#define SET_TM_VC_DT(ptr, s, vc, dt) do { \
	SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_DT##s, dt); \
	SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_VSYNC_VC##s, vc); \
	SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_HSYNC_VC##s, vc); \
} while (0)

#define SET_VC_SPLIT(ptr, sel, b, vc, ref) do { \
	SENINF_BITS(ptr, SENINF_MUX_VC_SEL##sel, \
		    RG_SENINF_MUX_B##b##_VC_SEL, vc); \
	SENINF_BITS(ptr, SENINF_MUX_VC_SEL##sel, \
		    RG_SENINF_MUX_B##b##_VC_REF_EN, ref); \
} while (0)

#define SHOW(buf, len, fmt, ...) { \
	len += snprintf(buf + len, PAGE_SIZE - len, fmt, ##__VA_ARGS__); \
}

#ifdef INIT_DESKEW_UT
uint adb_data_rate;
uint adb_seninf_clk;

module_param(adb_data_rate, uint, 0644);
MODULE_PARM_DESC(adb_data_rate, "adb_data_rate");

module_param(adb_seninf_clk, uint, 0644);
MODULE_PARM_DESC(adb_seninf_clk, "adb_seninf_clk");
#endif /* INIT_DESKEW_UT */

static u64 settle_formula(u64 settle_ns, u64 seninf_ck)
{
	u64 _val = (settle_ns * seninf_ck);

	if (_val % 1000000000)
		_val = 1 + (_val / 1000000000) - 6;
	else
		_val = (_val / 1000000000) - 6;

	return _val;
}


static int mtk_cam_seninf_init_iomem(struct seninf_ctx *ctx,
			      void __iomem *if_base, void __iomem *ana_base)
{
	int i, j, k;

	ctx->reg_ana_csi_rx[CSI_PORT_0] =
	ctx->reg_ana_csi_rx[CSI_PORT_0A] = ana_base + 0;
	ctx->reg_ana_csi_rx[CSI_PORT_0B] = ana_base + 0x1000;

	ctx->reg_ana_csi_rx[CSI_PORT_1] =
	ctx->reg_ana_csi_rx[CSI_PORT_1A] = ana_base + 0x8000;
	ctx->reg_ana_csi_rx[CSI_PORT_1B] = ana_base + 0x9000;

	ctx->reg_ana_csi_rx[CSI_PORT_2A] =
	ctx->reg_ana_csi_rx[CSI_PORT_2] = ana_base + 0x10000;
	ctx->reg_ana_csi_rx[CSI_PORT_2B] = ana_base + 0x11000;

	ctx->reg_ana_csi_rx[CSI_PORT_3A] =
	ctx->reg_ana_csi_rx[CSI_PORT_3] = ana_base + 0x18000;
	ctx->reg_ana_csi_rx[CSI_PORT_3B] = ana_base + 0x19000;

	ctx->reg_ana_csi_rx[CSI_PORT_4A] =
	ctx->reg_ana_csi_rx[CSI_PORT_4] = ana_base + 0x20000;
	ctx->reg_ana_csi_rx[CSI_PORT_4B] = ana_base + 0x21000;

	ctx->reg_ana_csi_rx[CSI_PORT_5A] =
	ctx->reg_ana_csi_rx[CSI_PORT_5] = ana_base + 0x28000;
	ctx->reg_ana_csi_rx[CSI_PORT_5B] = ana_base + 0x29000;

	ctx->reg_ana_dphy_top[CSI_PORT_0A] =
	ctx->reg_ana_dphy_top[CSI_PORT_0B] =
	ctx->reg_ana_dphy_top[CSI_PORT_0] = ana_base + 0x2000;

	ctx->reg_ana_dphy_top[CSI_PORT_1A] =
	ctx->reg_ana_dphy_top[CSI_PORT_1B] =
	ctx->reg_ana_dphy_top[CSI_PORT_1] = ana_base + 0xA000;

	ctx->reg_ana_dphy_top[CSI_PORT_2A] =
	ctx->reg_ana_dphy_top[CSI_PORT_2B] =
	ctx->reg_ana_dphy_top[CSI_PORT_2] = ana_base + 0x12000;

	ctx->reg_ana_dphy_top[CSI_PORT_3A] =
	ctx->reg_ana_dphy_top[CSI_PORT_3B] =
	ctx->reg_ana_dphy_top[CSI_PORT_3] = ana_base + 0x1A000;

	ctx->reg_ana_dphy_top[CSI_PORT_4A] =
	ctx->reg_ana_dphy_top[CSI_PORT_4B] =
	ctx->reg_ana_dphy_top[CSI_PORT_4] = ana_base + 0x22000;

	ctx->reg_ana_dphy_top[CSI_PORT_5A] =
	ctx->reg_ana_dphy_top[CSI_PORT_5B] =
	ctx->reg_ana_dphy_top[CSI_PORT_5] = ana_base + 0x2A000;

	ctx->reg_ana_cphy_top[CSI_PORT_0A] =
	ctx->reg_ana_cphy_top[CSI_PORT_0B] =
	ctx->reg_ana_cphy_top[CSI_PORT_0] = ana_base + 0x3000;

	ctx->reg_ana_cphy_top[CSI_PORT_1A] =
	ctx->reg_ana_cphy_top[CSI_PORT_1B] =
	ctx->reg_ana_cphy_top[CSI_PORT_1] = ana_base + 0xB000;

	ctx->reg_ana_cphy_top[CSI_PORT_2A] =
	ctx->reg_ana_cphy_top[CSI_PORT_2B] =
	ctx->reg_ana_cphy_top[CSI_PORT_2] = ana_base + 0x13000;

	ctx->reg_ana_cphy_top[CSI_PORT_3A] =
	ctx->reg_ana_cphy_top[CSI_PORT_3B] =
	ctx->reg_ana_cphy_top[CSI_PORT_3] = ana_base + 0x1B000;

	ctx->reg_ana_cphy_top[CSI_PORT_4A] =
	ctx->reg_ana_cphy_top[CSI_PORT_4B] =
	ctx->reg_ana_cphy_top[CSI_PORT_4] = ana_base + 0x23000;

	ctx->reg_ana_cphy_top[CSI_PORT_5A] =
	ctx->reg_ana_cphy_top[CSI_PORT_5B] =
	ctx->reg_ana_cphy_top[CSI_PORT_5] = ana_base + 0x2B000;

	ctx->reg_mipi_csi_top_ctrl[MIPI_CSI_TOP_CTRL_0] = ana_base + 0xf00;
	ctx->reg_mipi_csi_top_ctrl[MIPI_CSI_TOP_CTRL_1] = ana_base + 0x10f00;
	/* ana_base = 0x11c8_0000 */

	ctx->reg_csirx_mac_csi[CSI_PORT_0]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_0A] = ana_base + 0x5000;
	ctx->reg_csirx_mac_csi[CSI_PORT_0B] = ana_base + 0x6000;

	ctx->reg_csirx_mac_csi[CSI_PORT_1]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_1A] = ana_base + 0xD000;
	ctx->reg_csirx_mac_csi[CSI_PORT_1B] = ana_base + 0xE000;

	ctx->reg_csirx_mac_csi[CSI_PORT_2]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_2A] = ana_base + 0x15000;
	ctx->reg_csirx_mac_csi[CSI_PORT_2B] = ana_base + 0x16000;

	ctx->reg_csirx_mac_csi[CSI_PORT_3]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_3A] = ana_base + 0x1D000;
	ctx->reg_csirx_mac_csi[CSI_PORT_3B] = ana_base + 0x1E000;

	ctx->reg_csirx_mac_csi[CSI_PORT_4]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_4A] = ana_base + 0x25000;
	ctx->reg_csirx_mac_csi[CSI_PORT_4B] = ana_base + 0x26000;

	ctx->reg_csirx_mac_csi[CSI_PORT_5]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_5A] = ana_base + 0x2D000;
	ctx->reg_csirx_mac_csi[CSI_PORT_5B] = ana_base + 0x2E000;

	ctx->reg_csirx_mac_top[CSI_PORT_0A] =
	ctx->reg_csirx_mac_top[CSI_PORT_0B] =
	ctx->reg_csirx_mac_top[CSI_PORT_0]  = ana_base + 0x4000;

	ctx->reg_csirx_mac_top[CSI_PORT_1A] =
	ctx->reg_csirx_mac_top[CSI_PORT_1B] =
	ctx->reg_csirx_mac_top[CSI_PORT_1]  = ana_base + 0xC000;

	ctx->reg_csirx_mac_top[CSI_PORT_2A] =
	ctx->reg_csirx_mac_top[CSI_PORT_2B] =
	ctx->reg_csirx_mac_top[CSI_PORT_2]  = ana_base + 0x14000;

	ctx->reg_csirx_mac_top[CSI_PORT_3A] =
	ctx->reg_csirx_mac_top[CSI_PORT_3B] =
	ctx->reg_csirx_mac_top[CSI_PORT_3]  = ana_base + 0x1C000;

	ctx->reg_csirx_mac_top[CSI_PORT_4A] =
	ctx->reg_csirx_mac_top[CSI_PORT_4B] =
	ctx->reg_csirx_mac_top[CSI_PORT_4]  = ana_base + 0x24000;

	ctx->reg_csirx_mac_top[CSI_PORT_5A] =
	ctx->reg_csirx_mac_top[CSI_PORT_5B] =
	ctx->reg_csirx_mac_top[CSI_PORT_5]  = ana_base + 0x2C000;

	ctx->reg_if_top = if_base;

	for (i = SENINF_1; i < _seninf_ops->seninf_num; i++) {
		ctx->reg_if_ctrl[i] = if_base + 0x0200 + (0x1000 * i);
		ctx->reg_if_tg[i] = if_base + 0x0200 + (0x1000 * i);
		ctx->reg_if_csi2[i] = if_base + 0x0a00 + (0x1000 * i);
	}


	for (j = SENINF_MUX1; j < _seninf_ops->mux_num; j++)
		ctx->reg_if_mux[j] = if_base + 0x0d00 + (0x1000 * j);

	for (k = SENINF_CAM_MUX0; k < _seninf_ops->cam_mux_num; k++)
		ctx->reg_if_cam_mux_pcsr[k] = if_base + 0x17000 + (0x0040 * k);

	ctx->reg_if_cam_mux_gcsr = if_base + 0x17F00;


	return 0;
}

static int mtk_cam_seninf_init_port(struct seninf_ctx *ctx, int port)
{
	int portNum;

	if (port >= CSI_PORT_0A)
		portNum = (port - CSI_PORT_0A) >> 1;
	else
		portNum = port;

	ctx->port = port;
	ctx->portNum = portNum;
	ctx->portA = CSI_PORT_0A + (portNum << 1);
	ctx->portB = ctx->portA + 1;
	ctx->is_4d1c = (port == portNum);

	switch (port) {
	case CSI_PORT_0:
	case CSI_PORT_0A:
		ctx->seninfIdx = SENINF_1;
		break;
	case CSI_PORT_0B:
		ctx->seninfIdx = SENINF_2;
		break;
	case CSI_PORT_1:
	case CSI_PORT_1A:
		ctx->seninfIdx = SENINF_3;
		break;
	case CSI_PORT_1B:
		ctx->seninfIdx = SENINF_4;
		break;
	case CSI_PORT_2:
	case CSI_PORT_2A:
		ctx->seninfIdx = SENINF_5;
		break;
	case CSI_PORT_2B:
		ctx->seninfIdx = SENINF_6;
		break;
	case CSI_PORT_3:
	case CSI_PORT_3A:
		ctx->seninfIdx = SENINF_7;
		break;
	case CSI_PORT_3B:
		ctx->seninfIdx = SENINF_8;
		break;
	case CSI_PORT_4:
	case CSI_PORT_4A:
		ctx->seninfIdx = SENINF_9;
		break;
	case CSI_PORT_4B:
		ctx->seninfIdx = SENINF_10;
		break;
	case CSI_PORT_5:
	case CSI_PORT_5A:
		ctx->seninfIdx = SENINF_11;
		break;
	case CSI_PORT_5B:
		ctx->seninfIdx = SENINF_12;
		break;

	default:
		dev_info(ctx->dev, "invalid port %d\n", port);
		return -EINVAL;
	}

	return 0;
}

static int mtk_cam_seninf_is_cammux_used(struct seninf_ctx *ctx, int cam_mux)
{
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	return SENINF_READ_BITS(pSeninf_cam_mux_pcsr,
		SENINF_CAM_MUX_PCSR_CTRL, RG_SENINF_CAM_MUX_PCSR_EN);
}

static int mtk_cam_seninf_cammux(struct seninf_ctx *ctx, int cam_mux)
{
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	SENINF_BITS(pSeninf_cam_mux_pcsr,
			SENINF_CAM_MUX_PCSR_CTRL, RG_SENINF_CAM_MUX_PCSR_EN, 1);


	SENINF_WRITE_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_STATUS,
			 (1 << RO_SENINF_CAM_MUX_PCSR_HSIZE_ERR_IRQ_SHIFT) |
			 (1 << RO_SENINF_CAM_MUX_PCSR_VSIZE_ERR_IRQ_SHIFT) |
			 (1 << RO_SENINF_CAM_MUX_PCSR_VSYNC_IRQ_SHIFT));//clr irq

	seninf_logd(ctx,
		"cam_mux %d EN 0x%x IRQ_EN 0x%x IRQ_STATUS 0x%x\n",
		cam_mux,
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL),
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_EN),
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_STATUS));

	return 0;
}

static int mtk_cam_seninf_enable_cam_mux_vsync_irq(struct seninf_ctx *ctx, bool enable, int cam_mux)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	int tmp = 0;

	if ((cam_mux >= 0) && (cam_mux <= 31)) {
		tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN);
		if (enable)
			tmp |= (1 << cam_mux);
		else
			tmp &= ~(1 << cam_mux);
		SENINF_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, tmp);
	} else if ((cam_mux >= 32) && (cam_mux <= 63)) {
		cam_mux -= 32;
		tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN);
		if (enable)
			tmp |= (1 << cam_mux);
		else
			tmp &= ~(1 << cam_mux);
		SENINF_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, tmp);
	}
	return 0;
}

static int mtk_cam_seninf_disable_cammux(struct seninf_ctx *ctx, int cam_mux)
{
	void *pSeninf_cam_mux_pcsr = NULL;
	int i;
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];
	mtk_cam_seninf_enable_cam_mux_vsync_irq(ctx, 0, cam_mux);

	SENINF_BITS(pSeninf_cam_mux_pcsr,
			SENINF_CAM_MUX_PCSR_CTRL, CAM_MUX_PCSR_NEXT_SRC_SEL, 0x3f);

	SENINF_BITS(pSeninf_cam_mux_pcsr,
			SENINF_CAM_MUX_PCSR_CTRL, RG_SENINF_CAM_MUX_PCSR_SRC_SEL, 0x3f);

	SENINF_BITS(pSeninf_cam_mux_pcsr,
			SENINF_CAM_MUX_PCSR_CTRL, RG_SENINF_CAM_MUX_PCSR_EN, 0);

	/* clear tags */
	for (i = 0; i < 4; i++) {
		SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT,
			    RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL, i);
		SENINF_WRITE_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_VC_SEL, 0);
		SENINF_WRITE_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_DT_SEL, 0);
	}

	SENINF_WRITE_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_STATUS,
			 (1 << RO_SENINF_CAM_MUX_PCSR_HSIZE_ERR_IRQ_SHIFT) |
			 (1 << RO_SENINF_CAM_MUX_PCSR_VSIZE_ERR_IRQ_SHIFT) |
			 (1 << RO_SENINF_CAM_MUX_PCSR_VSYNC_IRQ_SHIFT));//clr irq

	seninf_logd(ctx,
		"inner:%d,cam_mux:%d,EN:0x%x,IRQ_EN:0x%x,IRQ_STATUS:0x%x\n",
		SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_DYN_CTRL, RG_SENINF_CAM_MUX_GCSR_DYN_PAGE_SEL),
		cam_mux,
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL),
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_EN),
		SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_IRQ_STATUS));

	return 0;
}

static int mtk_cam_seninf_disable_all_cammux(struct seninf_ctx *ctx)
{
	int i = 0;
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;

	for (i = SENINF_CAM_MUX0; i < _seninf_ops->cam_mux_num; i++)
		mtk_cam_seninf_disable_cammux(ctx, i);

	seninf_logd(ctx,
		"all SENINF_CAM_MUX_GCSR_MUX_EN 0x%x\n",
		SENINF_READ_REG(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_MUX_EN));

	return 0;
}

static int mtk_cam_seninf_set_top_mux_ctrl(
	struct seninf_ctx *ctx, int mux_idx, int seninf_src)
{
	void *pSeninf = ctx->reg_if_top;
	struct seninf_core *core = ctx->core;

	if (core == NULL) {
		dev_info(ctx->dev, "%s core is NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&core->seninf_top_mux_mutex);
	switch (mux_idx) {
	case SENINF_MUX1:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			    RG_SENINF_MUX1_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX2:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			    RG_SENINF_MUX2_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX3:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			    RG_SENINF_MUX3_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX4:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			    RG_SENINF_MUX4_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX5:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			    RG_SENINF_MUX5_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX6:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			    RG_SENINF_MUX6_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX7:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			    RG_SENINF_MUX7_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX8:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			    RG_SENINF_MUX8_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX9:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			    RG_SENINF_MUX9_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX10:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			    RG_SENINF_MUX10_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX11:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			    RG_SENINF_MUX11_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX12:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			    RG_SENINF_MUX12_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX13:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
				RG_SENINF_MUX13_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX14:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
				RG_SENINF_MUX14_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX15:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
				RG_SENINF_MUX15_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX16:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
				RG_SENINF_MUX16_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX17:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
				RG_SENINF_MUX17_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX18:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
				RG_SENINF_MUX18_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX19:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
				RG_SENINF_MUX19_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX20:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
				RG_SENINF_MUX20_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX21:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_5,
				RG_SENINF_MUX21_SRC_SEL, seninf_src);
		break;
	case SENINF_MUX22:
		SENINF_BITS(pSeninf, SENINF_TOP_MUX_CTRL_5,
				RG_SENINF_MUX22_SRC_SEL, seninf_src);
			break;

	default:
		mutex_unlock(&core->seninf_top_mux_mutex);
		dev_info(ctx->dev, "invalid mux_idx %d\n", mux_idx);
		return -EINVAL;
	}
	mutex_unlock(&core->seninf_top_mux_mutex);

	dev_info(ctx->dev,
		"mux:%d,TOP_MUX_CTRL_0(0x%x),TOP_MUX_CTRL_1(0x%x),TOP_MUX_CTRL_2(0x%x),TOP_MUX_CTRL_3(0x%x),TOP_MUX_CTRL_4(0x%x),TOP_MUX_CTRL_5(0x%x)\n",
		mux_idx,
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_0),
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_1),
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_2),
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_3),
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_4),
		SENINF_READ_REG(pSeninf, SENINF_TOP_MUX_CTRL_5));

	return 0;
}

static int mtk_cam_seninf_get_top_mux_ctrl(struct seninf_ctx *ctx, int mux_idx)
{
	void *pSeninf = ctx->reg_if_top;
	unsigned int seninf_src = 0;


	switch (mux_idx) {
	case SENINF_MUX1:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
				RG_SENINF_MUX1_SRC_SEL);
		break;
	case SENINF_MUX2:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
				RG_SENINF_MUX2_SRC_SEL);
		break;
	case SENINF_MUX3:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			RG_SENINF_MUX3_SRC_SEL);
			break;
	case SENINF_MUX4:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_0,
			RG_SENINF_MUX4_SRC_SEL);
		break;
	case SENINF_MUX5:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			RG_SENINF_MUX5_SRC_SEL);
		break;
	case SENINF_MUX6:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			RG_SENINF_MUX6_SRC_SEL);
		break;
	case SENINF_MUX7:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			RG_SENINF_MUX7_SRC_SEL);
		break;
	case SENINF_MUX8:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_1,
			RG_SENINF_MUX8_SRC_SEL);
		break;
	case SENINF_MUX9:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			RG_SENINF_MUX9_SRC_SEL);
		break;
	case SENINF_MUX10:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			RG_SENINF_MUX10_SRC_SEL);
		break;
	case SENINF_MUX11:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			RG_SENINF_MUX11_SRC_SEL);
		break;
	case SENINF_MUX12:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_2,
			RG_SENINF_MUX12_SRC_SEL);
		break;
	case SENINF_MUX13:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
			RG_SENINF_MUX13_SRC_SEL);
		break;
	case SENINF_MUX14:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
			RG_SENINF_MUX14_SRC_SEL);
		break;
	case SENINF_MUX15:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
			RG_SENINF_MUX15_SRC_SEL);
		break;
	case SENINF_MUX16:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_3,
			RG_SENINF_MUX16_SRC_SEL);
		break;
	case SENINF_MUX17:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
			RG_SENINF_MUX17_SRC_SEL);
		break;
	case SENINF_MUX18:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
			RG_SENINF_MUX18_SRC_SEL);
		break;
	case SENINF_MUX19:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
			RG_SENINF_MUX19_SRC_SEL);
		break;
	case SENINF_MUX20:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_4,
			RG_SENINF_MUX20_SRC_SEL);
		break;
	case SENINF_MUX21:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_5,
			RG_SENINF_MUX21_SRC_SEL);
		break;
	case SENINF_MUX22:
		seninf_src = SENINF_READ_BITS(pSeninf, SENINF_TOP_MUX_CTRL_5,
			RG_SENINF_MUX22_SRC_SEL);
		break;

	default:
		dev_info(ctx->dev, "invalid mux_idx %d", mux_idx);
		return -EINVAL;
	}

	return seninf_src;
}

static int mtk_cam_seninf_get_cammux_ctrl(struct seninf_ctx *ctx, int cam_mux)
{
	int ret = 0;
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	ret = SENINF_READ_BITS(pSeninf_cam_mux_pcsr,
		SENINF_CAM_MUX_PCSR_CTRL, RG_SENINF_CAM_MUX_PCSR_SRC_SEL);

	return ret;
}

static u32 mtk_cam_seninf_get_cammux_res(struct seninf_ctx *ctx, int cam_mux)
{
	u32 ret = 0;
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	ret = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CHK_RES);
	return ret;
}

static u32 mtk_cam_seninf_get_cammux_exp(struct seninf_ctx *ctx, int cam_mux)
{
	void *pSeninf_cam_mux_pcsr = NULL;
	u32 ret = 0;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];


	ret = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CHK_CTL);
	return ret;
}

static u32 mtk_cam_seninf_get_cammux_err(struct seninf_ctx *ctx, int cam_mux)
{
	void *pSeninf_cam_mux_pcsr = NULL;
	u32 ret = 0;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];


	ret = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CHK_ERR_RES);
	return ret;
}


static int mtk_cam_seninf_set_cammux_vc(struct seninf_ctx *ctx, int cam_mux,
				 int vc_sel, int dt_sel, int vc_en, int dt_en)
{
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	seninf_logd(ctx,
		"cam_mux:%d,vc:0x%x,dt:0x%x,vc_en:%d,dt_en:%d\n",
		cam_mux, vc_sel, dt_sel, vc_en, dt_en);

	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_VC_SEL, vc_sel);
	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_DT_SEL, dt_sel);
	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_VC_SEL_EN, vc_en);
	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_DT_SEL_EN, dt_en);

	return 0;
}

static int mtk_cam_seninf_set_cammux_tag(struct seninf_ctx *ctx, int cam_mux,
				int vc_sel, int dt_sel, int tag, int first)
{
	void *pSeninf_cam_mux_pcsr = NULL;
	int page, tag_sel;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	if (tag >= 0 && tag <= 31) {
		page = tag / 4;
		tag_sel = tag % 4;

		switch (tag_sel) {
		case 0:
			SET_TAG(pSeninf_cam_mux_pcsr, page, 0, vc_sel, dt_sel, first);
			break;
		case 1:
			SET_TAG(pSeninf_cam_mux_pcsr, page, 1, vc_sel, dt_sel, first);
			break;
		case 2:
			SET_TAG(pSeninf_cam_mux_pcsr, page, 2, vc_sel, dt_sel, first);
			break;
		case 3:
			SET_TAG(pSeninf_cam_mux_pcsr, page, 3, vc_sel, dt_sel, first);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int mtk_cam_seninf_switch_to_cammux_inner_page(struct seninf_ctx *ctx, bool inner)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;

	seninf_logd(ctx, "inner %d\n", inner);

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_DYN_CTRL, RG_SENINF_CAM_MUX_GCSR_DYN_PAGE_SEL, inner ? 0 : 1);

	return 0;
}

static int mtk_cam_seninf_set_cammux_next_ctrl(struct seninf_ctx *ctx, int src, int target)
{
	void *pSeninf_cam_mux_pcsr = NULL;

	if (target  < 0 || target  >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			target,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[target];

	seninf_logd(ctx, "cam_mux:%d,src:%d\n", target, src);

	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL,
					CAM_MUX_PCSR_NEXT_SRC_SEL, src);

	if (src != 0x3f) {
		u32 in_ctrl, in_opt, out_ctrl, out_opt;
		u32 in_tag_vc, in_tag_dt, out_tag_vc, out_tag_dt;

		mtk_cam_seninf_switch_to_cammux_inner_page(ctx, true);
		in_ctrl = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL);
		in_opt = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT);
		in_tag_vc = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
		in_tag_dt = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_DT_SEL);
		mtk_cam_seninf_switch_to_cammux_inner_page(ctx, false);
		out_ctrl = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL);
		out_opt = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_OPT);
		out_tag_vc = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
		out_tag_dt = SENINF_READ_REG(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_TAG_DT_SEL);
		mtk_cam_seninf_switch_to_cammux_inner_page(ctx, true);
		dev_info(ctx->dev,
			" %s cam_mux %d in|out PCSR CTRL 0x%x|0x%x OPT 0x%x|0x%x TAG_VC 0x%x|0x%x TAG_DT 0x%x|0x%x\n",
			__func__,
			target,
			in_ctrl,
			out_ctrl,
			in_opt,
			out_opt,
			in_tag_vc,
			out_tag_vc,
			in_tag_dt,
			out_tag_dt);
	}

	return 0;
}


static int mtk_cam_seninf_set_cammux_src(struct seninf_ctx *ctx, int src, int target,
				  int exp_hsize, int exp_vsize, int dt)
{
	int exp_dt_hsize = exp_hsize;
	void *pSeninf_cam_mux_pcsr = NULL;

	if (target  < 0 || target  >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			target,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[target];

	/* two times width size when yuv422 */
	if (dt == 0x3f)
		exp_dt_hsize = exp_hsize << 1;

	seninf_logd(ctx,
		"cam_mux:%d,src:%d,exp_hsize:%d/%d,exp_vsize:%d,dt:0x%x\n",
		target, src, exp_hsize, exp_dt_hsize, exp_vsize, dt);

	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL,
					RG_SENINF_CAM_MUX_PCSR_SRC_SEL, src);
	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CHK_CTL,
					RG_SENINF_CAM_MUX_PCSR_EXP_HSIZE, exp_dt_hsize);
	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CHK_CTL,
					RG_SENINF_CAM_MUX_PCSR_EXP_VSIZE, exp_vsize);
	return 0;
}

static void mtk_cam_seninf_reset_dt_remap(void *pCsi2_mac)
{
	if (pCsi2_mac) {
		SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, 0);
		SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, 0);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW16_DT, 0x2e);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW14_DT, 0x2d);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW12_DT, 0x2c);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW10_DT, 0x2b);
	}
}

static int mtk_cam_seninf_remap_dt(void *pCsi2_mac, struct seninf_vc *vc, int dt_remap_index)
{
	int remap_ret = 0;

	if (!vc || !pCsi2_mac)
		return -1;

	switch (dt_remap_index) {
	case 0:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT0, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT0_SEL,
						vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT0_EN, 0x1);
		break;
	case 1:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1_SEL,
						vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1_EN, 0x1);
		break;
	case 2:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2_SEL,
						vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2_EN, 0x1);
		break;
	case 3:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3_SEL,
						vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3_EN, 0x1);
		break;
	default:
		remap_ret = -1;
		break;
	}

	if (remap_ret >= 0) {
		switch (vc->dt) {
		case 0x2e:
			/* map raw16 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW16_DT, 0x2f);
			break;
		case 0x2d:
			/* map raw14 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW14_DT, 0x2f);
			break;
		case 0x2c:
			/* map raw12 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW12_DT, 0x2f);
			break;
		case 0x2b:
			/* map raw10 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW10_DT, 0x2f);
			break;
		default:
			break;
		}
	}

	return remap_ret;
}
static int mtk_cam_seninf_set_vc(struct seninf_ctx *ctx, int intf,
			  struct seninf_vcinfo *vcinfo, struct seninf_glp_dt *glpinfo)
{
	void *pSeninf_csi2 = ctx->reg_if_csi2[(unsigned int)intf];
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	int i, ret, dt_remap_index = 0, j, k = 0, seq_dt_flag = 0;
	struct seninf_vc *vc;
	int dt_remap_table[4] = {0};

	if (!vcinfo || !vcinfo->cnt)
		return 0;

	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S0_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S1_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S2_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S3_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S4_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S5_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S6_DI_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_S7_DI_CTRL, 0);

	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_CH0_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_CH1_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_CH2_CTRL, 0);
	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_CH3_CTRL, 0);

	mtk_cam_seninf_reset_dt_remap(pCsi2_mac);

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		if (vc->dt_remap_to_type > MTK_MBUS_FRAME_DESC_REMAP_NONE &&
			vc->dt_remap_to_type <= MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14) {
			if (dt_remap_index == 0) {
				dt_remap_table[dt_remap_index] = vc->dt;
				ret = mtk_cam_seninf_remap_dt(pCsi2_mac, vc, dt_remap_index);
				dev_info(ctx->dev,
					"ret(%d) idx(%d) vc[%d] dt 0x%x remap to %d\n",
					ret, dt_remap_index, i, vc->dt, vc->dt_remap_to_type);
				dt_remap_index++;
			} else {
				j = 0;
				while (j < dt_remap_index && dt_remap_index < DT_REMAP_MAX_CNT) {
					if (vc->dt != dt_remap_table[j])
						j++;
					else
						break;
				}

				if (j == dt_remap_index && dt_remap_index < DT_REMAP_MAX_CNT) {
					dt_remap_table[dt_remap_index] = vc->dt;
					ret = mtk_cam_seninf_remap_dt(pCsi2_mac, vc,
									dt_remap_index);
					dev_info(ctx->dev,
						"ret(%d) idx(%d) vc[%d] dt 0x%x remap to %d\n",
						ret, dt_remap_index, i, vc->dt,
						vc->dt_remap_to_type);
					dt_remap_index++;
				}
			}
		}

		/* Set SEQ DT  */
		if (vc->dt >= 0x10 && vc->dt <= 0x17) {
			if (glpinfo->cnt){
				for (k = 0; k < glpinfo->cnt; k++){
					switch (k) {
					case 0:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT0, glpinfo->dt[k]);
						break;
					case 1:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT1, glpinfo->dt[k]);
						break;
					case 2:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT2, glpinfo->dt[k]);
						break;
					case 3:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT3, glpinfo->dt[k]);
						break;
					}
				}
			} else {
				switch (k) {
					case 0:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT0, vc->dt);
						break;
					case 1:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT1, vc->dt);
						break;
					case 2:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT2, vc->dt);
						break;
					case 3:
						SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2,
							RG_SEQ_DT3, vc->dt);
						break;
					}
				k++;
			}
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2, RG_SEQ14_DT_EN, 1);
			seq_dt_flag = 1;
			dev_info(ctx->dev, "[%s] CSIRX_MAC_CSI2_FORCEDT2: 0x%08x\n",
				__func__, SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2));
		}

		switch (i) {
		case 0:
			SET_DI_CH_CTRL(pSeninf_csi2, 0, vc);
			break;
		case 1:
			SET_DI_CH_CTRL(pSeninf_csi2, 1, vc);
			break;
		case 2:
			SET_DI_CH_CTRL(pSeninf_csi2, 2, vc);
			break;
		case 3:
			SET_DI_CH_CTRL(pSeninf_csi2, 3, vc);
			break;
		case 4:
			SET_DI_CH_CTRL(pSeninf_csi2, 4, vc);
			break;
		case 5:
			SET_DI_CH_CTRL(pSeninf_csi2, 5, vc);
			break;
		case 6:
			SET_DI_CH_CTRL(pSeninf_csi2, 6, vc);
			break;
		case 7:
			SET_DI_CH_CTRL(pSeninf_csi2, 7, vc);
			break;
		}
	}

/* General Long Packet Data Types: 0x10-0x17 */
	if (ctx->core->force_glp_en && !seq_dt_flag) {
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_OPT,
				RG_CSI2_GENERIC_LONG_PACKET_EN, 1);
		dev_info(ctx->dev, "enable generic long packet\n");
	}

	seninf_logd(ctx,
		"DI_CTRL 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S0_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S1_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S2_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S3_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S4_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S5_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S6_DI_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_S7_DI_CTRL));

	seninf_logd(ctx,
		"CH_CTRL 0x%x 0x%x 0x%x 0x%x\n",
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_CH0_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_CH1_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_CH2_CTRL),
		SENINF_READ_REG(pSeninf_csi2, SENINF_CSI2_CH3_CTRL));

	return 0;
}

static int mtk_cam_seninf_set_mux_ctrl(struct seninf_ctx *ctx, int mux,
				int hsPol, int vsPol, int src_sel,
				int pixel_mode)
{
	unsigned int temp = 0;
	void *pSeninf_mux;

	pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	//1A00 4D04[3:0] select source group
	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_1,
		    RG_SENINF_MUX_SRC_SEL,
			src_sel);

	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_1,
		    RG_SENINF_MUX_PIX_MODE_SEL,
			pixel_mode);

//	set_camux_checker_pixel_mode(0, pixel_mode);

	// Enable vc split
	SENINF_BITS(pSeninf_mux, SENINF_MUX_OPT,
		    RG_SENINF_MUX_VS_SPLIT_EN, 1);

	SENINF_BITS(pSeninf_mux, SENINF_MUX_OPT,
		    RG_SENINF_MUX_HSYNC_POL, hsPol);
	SENINF_BITS(pSeninf_mux, SENINF_MUX_OPT,
		    RG_SENINF_MUX_VSYNC_POL, vsPol);

	temp = SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_0);
	SENINF_WRITE_REG(pSeninf_mux, SENINF_MUX_CTRL_0, temp | 0x6);//reset
	SENINF_WRITE_REG(pSeninf_mux, SENINF_MUX_CTRL_0, temp & 0xFFFFFFF9);

	dev_info(ctx->dev, "SENINF_MUX_CTRL_0(0x%x), SENINF_MUX_CTRL_1(0x%x), SENINF_MUX_OPT(0x%x)",
		 SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_0),
		 SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_1),
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_OPT));
	return 0;
}

static int mtk_cam_seninf_set_mux_vc_split_all(struct seninf_ctx *ctx,
				int mux, u8 cam_type)
{
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	void *pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];
	struct seninf_vc *vc;
	int i;

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];

		if (cam_type == TYPE_CAMSV_SAT) {
			switch (vc->muxvr_offset) {
			case 0:
				SET_VC_SPLIT(pSeninf_mux, 0, 0, vc->vc, (i == 0));
				break;
			case 1:
				SET_VC_SPLIT(pSeninf_mux, 0, 1, vc->vc, (i == 0));
				break;
			case 2:
				SET_VC_SPLIT(pSeninf_mux, 0, 2, vc->vc, (i == 0));
				break;
			case 3:
				SET_VC_SPLIT(pSeninf_mux, 0, 3, vc->vc, (i == 0));
				break;
			case 4:
				SET_VC_SPLIT(pSeninf_mux, 1, 4, vc->vc, (i == 0));
				break;
			case 5:
				SET_VC_SPLIT(pSeninf_mux, 1, 5, vc->vc, (i == 0));
				break;
			case 6:
				SET_VC_SPLIT(pSeninf_mux, 1, 6, vc->vc, (i == 0));
				break;
			case 7:
				SET_VC_SPLIT(pSeninf_mux, 1, 7, vc->vc, (i == 0));
				break;
			default:
				dev_info(ctx->dev,
				 "%s: skip vc split setting for more than tag 7\n", __func__);
				break;
			}
		} else if (cam_type == TYPE_RAW) {
			switch (vc->muxvr_offset) {
			case 0:
				SET_VC_SPLIT(pSeninf_mux, 0, 0, vc->vc, (i == 0));
				break;
			case 1:
				SET_VC_SPLIT(pSeninf_mux, 0, 1, vc->vc, (i == 0));
				break;
			case 2:
				SET_VC_SPLIT(pSeninf_mux, 0, 2, vc->vc, (i == 0));
				break;
			case 3:
				SET_VC_SPLIT(pSeninf_mux, 0, 3, vc->vc, (i == 0));
				break;
			default:
				dev_info(ctx->dev,
						"%s: skip vc split setting (%d) for more than tag 3\n",
						__func__, vc->muxvr_offset);
				break;
			}
		} else {
			dev_info(ctx->dev,
						"%s: skip vc split setting, due to cam_type is %d\n",
						__func__, cam_type);
		}
	}

	seninf_logi(ctx,
		"set mux:%d,vc split SEL0(0x%x),SEL1(0x%x)\n",
		mux,
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_VC_SEL0),
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_VC_SEL1));

	return 0;
}

static int mtk_cam_seninf_update_mux_pixel_mode(struct seninf_ctx *ctx, int mux,
				int pixel_mode)
{
	unsigned int temp = 0;
	void *pSeninf_mux;

	pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];


	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_1,
		    RG_SENINF_MUX_PIX_MODE_SEL,
			pixel_mode);

	temp = SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_0);
	SENINF_WRITE_REG(pSeninf_mux, SENINF_MUX_CTRL_0, temp | 0x6);//reset
	SENINF_WRITE_REG(pSeninf_mux, SENINF_MUX_CTRL_0, temp & 0xFFFFFFF9);

	dev_info(ctx->dev, "%s mux %d SENINF_MUX_CTRL_0(0x%x), SENINF_MUX_CTRL_1(0x%x), SENINF_MUX_OPT(0x%x)",
		__func__,
		mux,
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_0),
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_1),
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_OPT));

	return 0;
}


static int mtk_cam_seninf_set_mux_crop(struct seninf_ctx *ctx, int mux,
				int start_x, int end_x, int enable)
{
	void *pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	SENINF_BITS(pSeninf_mux, SENINF_MUX_CROP_PIX_CTRL,
		    RG_SENINF_MUX_CROP_START_NPIX_CNT, start_x / 8);
	SENINF_BITS(pSeninf_mux, SENINF_MUX_CROP_PIX_CTRL,
		    RG_SENINF_MUX_CROP_END_NPIX_CNT,
		start_x / 8 + (end_x - start_x + 1) / 8 - 1 + (((end_x -
		start_x + 1) % 8) > 0 ? 1 : 0));
	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_1,
		    RG_SENINF_MUX_CROP_EN, enable);

	dev_info(ctx->dev, "MUX_CROP_PIX_CTRL 0x%x MUX_CTRL_1 0x%x\n",
		 SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CROP_PIX_CTRL),
		SENINF_READ_REG(pSeninf_mux, SENINF_MUX_CTRL_1));

	dev_info(ctx->dev, "mux %d, start %d, end %d, enable %d\n",
		 mux, start_x, end_x, enable);

	return 0;
}

static int mtk_cam_seninf_is_mux_used(struct seninf_ctx *ctx, int mux)
{
	void *pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	return SENINF_READ_BITS(pSeninf_mux, SENINF_MUX_CTRL_0, SENINF_MUX_EN);
}

static int mtk_cam_seninf_mux(struct seninf_ctx *ctx, int mux)
{
	void *pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	dev_info(ctx->dev, "%s enable mux %d\n",
		 __func__, mux);

	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_0, SENINF_MUX_EN, 1);
	return 0;
}

static int mtk_cam_seninf_disable_mux(struct seninf_ctx *ctx, int mux)
{
	int i;
	void *pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	seninf_logi(ctx, "disable mux %d\n", mux);

	SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_0, SENINF_MUX_EN, 0);

	// also disable CAM_MUX with input from mux
	for (i = SENINF_CAM_MUX0; i < _seninf_ops->cam_mux_num; i++) {
		if (mux == mux_vr2mux(ctx, mtk_cam_seninf_get_cammux_ctrl(ctx, i)))
			mtk_cam_seninf_disable_cammux(ctx, i);
	}

	return 0;
}

static int mtk_cam_seninf_disable_all_mux(struct seninf_ctx *ctx)
{
	int i;
	void *pSeninf_mux;

	for (i = 0; i < _seninf_ops->mux_num; i++) {
		pSeninf_mux = ctx->reg_if_mux[i];
		SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_0, SENINF_MUX_EN, 0);
	}

	return 0;
}

static int mtk_cam_seninf_set_cammux_chk_pixel_mode(struct seninf_ctx *ctx,
					     int cam_mux, int pixel_mode)
{
	void *pSeninf_cam_mux_pcsr = NULL;

	if (cam_mux < 0 || cam_mux >= _seninf_ops->cam_mux_num) {
		dev_info(ctx->dev,
			"%s err cam_mux %d invalid (0~SENINF_CAM_MUX_NUM:%d)\n",
			__func__,
			cam_mux,
			_seninf_ops->cam_mux_num);
		return 0;
	}
	pSeninf_cam_mux_pcsr = ctx->reg_if_cam_mux_pcsr[cam_mux];

	SENINF_BITS(pSeninf_cam_mux_pcsr, SENINF_CAM_MUX_PCSR_CTRL,
					RG_SENINF_CAM_MUX_PCSR_CHK_PIX_MODE, pixel_mode);
	return 0;
}

static int get_seninf_test_model_src(struct seninf_ctx *ctx, int mux, int *intf)
{
	if (ctx == NULL) {
		seninf_aee_print("[AEE] error, [%s] ctx is NULL", __func__);
		return -1;
	}

	if (intf == NULL) {
		dev_info(ctx->dev, "[%s] intf is NULL\n", __func__);
		return -1;
	}

	switch (mux) {
	case SENINF_MUX1:
	case SENINF_MUX7:
	case SENINF_MUX11:
		*intf = SENINF_1; //using seninf_1
		break;
	case SENINF_MUX2:
	case SENINF_MUX8:
	case SENINF_MUX12:
		*intf = SENINF_2; //using seninf_2
		break;
	case SENINF_MUX3:
	case SENINF_MUX9:
	case SENINF_MUX13:
		*intf = SENINF_3; //using seninf_3
		break;
	case SENINF_MUX4:
	case SENINF_MUX10:
	case SENINF_MUX14:
		*intf = SENINF_4; //using seninf_4
		break;
	case SENINF_MUX5:
		*intf = SENINF_5; //using seninf_5
		break;
	case SENINF_MUX6:
		*intf = SENINF_6; //using seninf_6
		break;
	case SENINF_MUX15:
		*intf = SENINF_7; //using seninf_7
		break;
	default:
		dev_info(ctx->dev,
		"[%s] this seninf_mux_%d can't find supported seninf\n",
		__func__, mux);
		return -1;
	}
	return 0;
}

static int mtk_cam_seninf_set_test_model(struct seninf_ctx *ctx,
				  int mux, int cam_mux, int pixel_mode,
				  int filter, int con, int vc, int dt, int muxvr_ofs)
{
	int intf;
	void *pSeninf;
	void *pSeninf_tg;
	void *pSeninf_mux;
	int mux_vr;

	if (get_seninf_test_model_src(ctx, mux, &intf)) {
		dev_info(ctx->dev, "[%s] get_seninf_test_model_src failed\n", __func__);
		return -1;
	}

	mux_vr = mux2mux_vr(ctx, mux, cam_mux, muxvr_ofs);

	pSeninf = ctx->reg_if_ctrl[(unsigned int)intf];
	pSeninf_tg = ctx->reg_if_tg[(unsigned int)intf];
	pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	_seninf_ops->_reset(ctx, intf);
	mtk_cam_seninf_mux(ctx, mux);
	mtk_cam_seninf_set_mux_ctrl(ctx, mux,
				    0, 0, TEST_MODEL, pixel_mode);
	mtk_cam_seninf_set_top_mux_ctrl(ctx, mux, intf);

	mtk_cam_seninf_set_cammux_vc(ctx, cam_mux, vc, dt,
				filter, filter);
	mtk_cam_seninf_set_cammux_tag(ctx, cam_mux, vc, dt,
				cam_mux /*tag*/, (con == 0));
	mtk_cam_seninf_set_cammux_src(ctx, mux_vr, cam_mux, 0, 0, 0);
	mtk_cam_seninf_set_cammux_chk_pixel_mode(ctx, cam_mux, pixel_mode);
	mtk_cam_seninf_cammux(ctx, cam_mux);

	SENINF_BITS(pSeninf, SENINF_TESTMDL_CTRL, RG_SENINF_TESTMDL_EN, 1);
	SENINF_BITS(pSeninf, SENINF_CTRL, SENINF_EN, 1);

	SENINF_BITS(pSeninf_tg, TM_SIZE, TM_LINE, 4224);
	SENINF_BITS(pSeninf_tg, TM_SIZE, TM_PXL, 5632);
	SENINF_BITS(pSeninf_tg, TM_CLK, TM_CLK_CNT, 7);

	SENINF_BITS(pSeninf_tg, TM_DUM, TM_VSYNC, 100);
	SENINF_BITS(pSeninf_tg, TM_DUM, TM_DUMMYPXL, 100);

	SENINF_BITS(pSeninf_tg, TM_CTL, TM_PAT, 0xC);
	SENINF_BITS(pSeninf_tg, TM_CTL, TM_EN, 1);

	// stg
	if (filter) {
		SENINF_BITS(pSeninf_tg, TM_STAGGER_CTL, EXP_NUM, con);
		SENINF_BITS(pSeninf_tg, TM_STAGGER_CTL, VSYNC_DELAY, 0xa);
		SENINF_BITS(pSeninf_tg, TM_STAGGER_CTL, STAGGER_MODE_EN, filter);

		switch (con) {
		case 0:
			SET_TM_VC_DT(pSeninf_tg, 0, vc, dt);
			break;
		case 1:
			SET_TM_VC_DT(pSeninf_tg, 1, vc, dt);
			break;
		case 2:
			SET_TM_VC_DT(pSeninf_tg, 2, vc, dt);
			break;
		case 3:
			SET_TM_VC_DT(pSeninf_tg, 3, vc, dt);
			break;
		case 4:
			SET_TM_VC_DT(pSeninf_tg, 4, vc, dt);
			break;
		case 5:
			SET_TM_VC_DT(pSeninf_tg, 5, vc, dt);
			break;
		case 6:
			SET_TM_VC_DT(pSeninf_tg, 6, vc, dt);
			break;
		case 7:
			SET_TM_VC_DT(pSeninf_tg, 7, vc, dt);
			break;
		}
	}

	return 0;
}

static int csirx_phyA_power_on(struct seninf_ctx *ctx, int portIdx, int en)
{
	void *base = ctx->reg_ana_csi_rx[(unsigned int)portIdx];

	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L0_T0AB_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L1_T1AB_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L2_T1BC_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T0BC_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T0CA_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T1CA_EQ_OS_CAL_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_LPF_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_CORE_EN, 0);
	udelay(200);

	if (en) {
		SENINF_BITS(base, CDPHY_RX_ANA_0,
			    RG_CSI0_BG_CORE_EN, 1);
		udelay(30);
		SENINF_BITS(base, CDPHY_RX_ANA_0,
			    RG_CSI0_BG_LPF_EN, 1);
		udelay(1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_L0_T0AB_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_L1_T1AB_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_L2_T1BC_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_XX_T0BC_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_XX_T0CA_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8,
			    RG_CSI0_XX_T1CA_EQ_OS_CAL_EN, 1);
		udelay(1);
	}

	seninf_logd(ctx,
		"portIdx %d en %d CDPHY_RX_ANA_0 0x%x ANA_8 0x%x\n",
		portIdx, en,
		SENINF_READ_REG(base, CDPHY_RX_ANA_0),
		SENINF_READ_REG(base, CDPHY_RX_ANA_8));

	return 0;
}
#ifdef CSI_EFUSE_SET
static int apply_efuse_data(struct seninf_ctx *ctx)
{
	int ret = 0;
	int port;
	void *base;
	unsigned int m_csi_efuse = ctx->m_csi_efuse;

	if (m_csi_efuse == 0) {
		seninf_logd(ctx, "No efuse data. Returned.\n");
		return -1;
	}

	if (ctx->is_4d1c || (ctx->port == ctx->portA)) {
		port = ctx->port;
		base = ctx->reg_ana_csi_rx[(unsigned int)port];
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			RG_CSI0_L0P_T0A_HSRT_CODE, (m_csi_efuse >> 27) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			RG_CSI0_L0N_T0B_HSRT_CODE, (m_csi_efuse >> 27) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			RG_CSI0_L1P_T0C_HSRT_CODE, (m_csi_efuse >> 22) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			RG_CSI0_L1N_T1A_HSRT_CODE, (m_csi_efuse >> 22) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			RG_CSI0_L2P_T1B_HSRT_CODE, (m_csi_efuse >> 17) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			RG_CSI0_L2N_T1C_HSRT_CODE, (m_csi_efuse >> 17) & 0x1f);
		dev_info(ctx->dev,
			"CSI%dA,CDPHY_RX_ANA_2/3/4:(0x%x)/(0x%x)/(0x%x),Efuse Data:(0x%08x)",
			ctx->portNum,
			SENINF_READ_REG(base, CDPHY_RX_ANA_2),
			SENINF_READ_REG(base, CDPHY_RX_ANA_3),
			SENINF_READ_REG(base, CDPHY_RX_ANA_4),
			ctx->m_csi_efuse);
	}

	if (ctx->is_4d1c || (ctx->port == ctx->portB)) {
		port = ctx->portB;
		base = ctx->reg_ana_csi_rx[(unsigned int)port];
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			RG_CSI0_L0P_T0A_HSRT_CODE, (m_csi_efuse >> 12) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			RG_CSI0_L0N_T0B_HSRT_CODE, (m_csi_efuse >> 12) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			RG_CSI0_L1P_T0C_HSRT_CODE, (m_csi_efuse >> 7) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			RG_CSI0_L1N_T1A_HSRT_CODE, (m_csi_efuse >> 7) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			RG_CSI0_L2P_T1B_HSRT_CODE, (m_csi_efuse >> 2) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			RG_CSI0_L2N_T1C_HSRT_CODE, (m_csi_efuse >> 2) & 0x1f);
		dev_info(ctx->dev,
			"CSI%dB,CDPHY_RX_ANA_2/3/4:(0x%x)/(0x%x)/(0x%x),Efuse Data:(0x%08x)",
			ctx->portNum,
			SENINF_READ_REG(base, CDPHY_RX_ANA_2),
			SENINF_READ_REG(base, CDPHY_RX_ANA_3),
			SENINF_READ_REG(base, CDPHY_RX_ANA_4),
			ctx->m_csi_efuse);
	}

	return ret;
}
#endif
static int csirx_phyA_init(struct seninf_ctx *ctx)
{
	int i, port;
	void *base;

	for (i = 0; i <= ctx->is_4d1c; i++) {
		port = i ? ctx->portB : ctx->port;
		base = ctx->reg_ana_csi_rx[(unsigned int)port];
		SENINF_BITS(base, CDPHY_RX_ANA_1,
			    RG_CSI0_BG_LPRX_VTL_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_1,
			    RG_CSI0_BG_LPRX_VTH_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_BG_ALP_RX_VTL_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_BG_ALP_RX_VTH_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_1,
			    RG_CSI0_BG_VREF_SEL, 0x8);
		SENINF_BITS(base, CDPHY_RX_ANA_1,
			    RG_CSI0_CDPHY_EQ_DES_VREF_SEL, 0x2);
		SENINF_BITS(base, CDPHY_RX_ANA_5,
			    RG_CSI0_CDPHY_EQ_BW, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_5,
			    RG_CSI0_CDPHY_EQ_IS, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_5,
			    RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_5,
			    RG_CSI0_CDPHY_EQ_SR0, 0x0);

		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_L0P_T0A_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_L0N_T0B_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			    RG_CSI0_L1P_T0C_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_3,
			    RG_CSI0_L1N_T1A_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			    RG_CSI0_L2P_T1B_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_4,
			    RG_CSI0_L2N_T1C_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_0,
			    RG_CSI0_CPHY_T0_CDR_FIRST_EDGE_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_0,
			    RG_CSI0_CPHY_T1_CDR_FIRST_EDGE_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_CPHY_T0_CDR_SELF_CAL_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_2,
			    RG_CSI0_CPHY_T1_CDR_SELF_CAL_EN, 0x0);

		SENINF_BITS(base, CDPHY_RX_ANA_6,
			    RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x4);//TODO
		SENINF_BITS(base, CDPHY_RX_ANA_7,
			    RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x4);//TODO
		SENINF_BITS(base, CDPHY_RX_ANA_6,
			    RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_6,
			    RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_6,
			    RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7,
			    RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7,
			    RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7,
			    RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x9);

		/* non-ULPS mode
		 *	0 : ULPS mode
		 *	1 : non-ULPS mode
		 */
		SENINF_BITS(base, CDPHY_RX_ANA_5,
				RG_CSI0_ULPS_IGNORE_EN, 0x1);

		/* LDO_LP_mode ON
		 *	0 : LDO_LP_mode OFF
		 *	1 : LDO_LP_mode ON
		 */
		SENINF_BITS(base, CDPHY_RX_ANA_14,
				RG_CSI0_LDO_LP_EN, 0x1);

		/* EQ_LATCH_EN[8]
		 *	0 : EQ_LATCH_DISABLE
		 *	1 : EQ_LATCH_EN
		 */
		SENINF_BITS(base, CDPHY_RX_ANA_8,
				RG_CSI0_RESERVE, (0x1 << 8));
	}

#ifdef CSI_EFUSE_SET
	apply_efuse_data(ctx);
#endif

	return 0;
}

#ifdef SCAN_SETTLE
static int set_trail(struct seninf_ctx *ctx, u16 hs_trail)
{
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;


	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
		    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
		    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
		    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
		    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,

			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, 1);


	SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_CTRL,
			RG_SENINF_CAM_MUX_GCSR_SW_RST, 1);
	udelay(1);
	SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_CTRL,
			RG_SENINF_CAM_MUX_GCSR_SW_RST, 0);

	return 0;

}



static int set_settle(struct seninf_ctx *ctx, u16 settle, bool hs_trail_en)
{

	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;


	int settle_delay_dt, settle_delay_ck;

	settle_delay_dt = settle;


	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
		    RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
		    RG_CDPHY_RX_LD1_TRIO1_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
		    RG_CDPHY_RX_LD2_TRIO2_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
		    RG_CDPHY_RX_LD3_TRIO3_HS_SETTLE_PARAMETER,
		    settle_delay_dt);

	settle_delay_ck = settle;


	SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
		    RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER,
		    settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
		    RG_DPHY_RX_LC1_HS_SETTLE_PARAMETER,
		    settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, hs_trail_en);


	SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_CTRL,
			RG_SENINF_CAM_MUX_GCSR_SW_RST, 1);
	udelay(1);
	SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_CTRL,
			RG_SENINF_CAM_MUX_GCSR_SW_RST, 0);


	return 0;
}
#endif

static int csirx_dphy_init(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	u64 settle_delay_dt, settle_delay_ck, hs_trail, hs_trail_en;
	int bit_per_pixel = 10;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	u64 data_rate = 0;
	struct seninf_core *core = ctx->core;
	u64 csi_clk = SENINF_CK;

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	csi_clk =
		core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk ?
		(u64)core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk * CSI_CLK_FREQ_MULTIPLIER :
		SENINF_CK;

	if (ctx->is_cphy) {
		if (ctx->csi_param.not_fixed_trail_settle) {
			settle_delay_dt = ctx->csi_param.cphy_settle
					? ctx->csi_param.cphy_settle
					: ctx->cphy_settle_delay_dt;
		} else {
			settle_delay_dt = ctx->csi_param.cphy_settle;
			if (settle_delay_dt == 0) {
				settle_delay_dt = settle_formula(CPHY_SETTLE_DEF, csi_clk);
				dev_info(ctx->dev,
					 "cphy settle val: (%u, %llu) => %llu\n",
					 CPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
			} else {
				u64 temp = csi_clk * settle_delay_dt;

				if (temp % 1000000000)
					settle_delay_dt = 1 + (temp / 1000000000);
				else
					settle_delay_dt = (temp / 1000000000);
			}
		}
		settle_delay_ck = settle_delay_dt;
	} else {
		if (!ctx->csi_param.not_fixed_dphy_settle) {
			settle_delay_dt = settle_delay_ck =
				settle_formula(DPHY_SETTLE_DEF, csi_clk);
			settle_delay_ck = 0;
			dev_info(ctx->dev,
				 "dphy settle val: (%u, %llu) => %llu\n",
				 DPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
		} else {
			if (ctx->csi_param.not_fixed_trail_settle) {
				settle_delay_dt = ctx->csi_param.dphy_data_settle
					? ctx->csi_param.dphy_data_settle
					: ctx->dphy_settle_delay_dt;
				settle_delay_ck = ctx->csi_param.dphy_clk_settle
						? ctx->csi_param.dphy_clk_settle
						: ctx->settle_delay_ck;
			} else {
				settle_delay_dt = ctx->csi_param.dphy_data_settle;
				if (settle_delay_dt == 0) {
					settle_delay_dt =
						settle_formula(DPHY_SETTLE_DEF, csi_clk);
					dev_info(ctx->dev,
						 "dphy settle val: (%u, %llu) => %llu\n",
						 DPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
				} else {
					u64 temp = csi_clk * settle_delay_dt;

					if (temp % 1000000000)
						settle_delay_dt = 1 + (temp / 1000000000);
					else
						settle_delay_dt = (temp / 1000000000);
				}
				settle_delay_ck = ctx->csi_param.dphy_clk_settle;
				if (settle_delay_ck == 0) {
					settle_delay_ck =
						settle_formula(DPHY_SETTLE_DEF, csi_clk);
					dev_info(ctx->dev,
						 "dphy settle val: (%u, %llu) => %llu\n",
						 DPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
				} else {
					u64 temp = csi_clk * settle_delay_ck;

					if (temp % 1000000000)
						settle_delay_ck = 1 + (temp / 1000000000);
					else
						settle_delay_ck = (temp / 1000000000);
				}
			}
		}
	}

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
		    RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
		    RG_CDPHY_RX_LD1_TRIO1_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
		    RG_CDPHY_RX_LD2_TRIO2_HS_SETTLE_PARAMETER,
		    settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
		    RG_CDPHY_RX_LD3_TRIO3_HS_SETTLE_PARAMETER,
		    settle_delay_dt);


	SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
		    RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER,
		    settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
		    RG_DPHY_RX_LC1_HS_SETTLE_PARAMETER,
		    settle_delay_ck);

	/*Settle delay by lane*/
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
		    RG_CDPHY_RX_LD0_TRIO0_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
		    RG_CDPHY_RX_LD1_TRIO1_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
		    RG_CDPHY_RX_LD2_TRIO2_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
		    RG_CDPHY_RX_LD3_TRIO3_HS_PREPARE_PARAMETER, 0);

	if (ctx->csi_param.not_fixed_trail_settle) {
		hs_trail = ctx->csi_param.dphy_trail
			? ctx->csi_param.dphy_trail
			: ctx->hs_trail_parameter;
	} else {
		u64 temp = 0;
		u64 ui_224 = 0;

		hs_trail = 0;
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		do_div(data_rate, ctx->num_data_lanes);
		ui_224 = (DPHY_TRAIL_SPEC * 1000) / (data_rate / 1000000);

		if (ctx->csi_param.dphy_trail == 0 ||
			ctx->csi_param.dphy_trail > ui_224)
			hs_trail = 0;
		else {

			temp = ui_224 - ctx->csi_param.dphy_trail;
			temp *= csi_clk;

			if (temp % 1000000000)
				hs_trail = 1 + (temp / 1000000000);
			else
				hs_trail = (temp / 1000000000);
		}
	}

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
		    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
		    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
		    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
		    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_PARAMETER, hs_trail);

	if (!ctx->is_cphy) {

		if (ctx->csi_param.not_fixed_trail_settle) {
			data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
			do_div(data_rate, ctx->num_data_lanes);
			hs_trail_en = data_rate <= SENINF_HS_TRAIL_EN_CONDITION;
			if (ctx->csi_param.dphy_trail != 0) {
				hs_trail_en = 1;
				dev_info(ctx->dev, "hs_trail = %llu\n", hs_trail);
			}
		} else {
			if (ctx->csi_param.dphy_trail != 0 &&
				hs_trail != 0) {
				hs_trail_en = 1;
				dev_info(ctx->dev, "hs_trail = %llu\n", hs_trail);
			} else
				hs_trail_en = 0;
		}
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, hs_trail_en);

		/* Enable rx_deskew and disable rx_deskew delay for lane 0~3 */
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL,
				RG_DPHY_RX_DESKEW_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL,
				DPHY_RX_DESKEW_L0_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL,
				DPHY_RX_DESKEW_L1_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL,
				DPHY_RX_DESKEW_L2_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL,
				DPHY_RX_DESKEW_L3_DELAY_EN, 0);

		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_PREPARE_EN,
			    0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_PREPARE_EN,
			    0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_PREPARE_EN,
			    0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_PREPARE_EN,
			    0x01);

		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
			    RG_DPHY_RX_LC0_HS_PREPARE_PARAMETER,
			    0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
			    RG_DPHY_RX_LC0_HS_PREPARE_EN,
			    0x1);

		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
			    RG_DPHY_RX_LC1_HS_PREPARE_PARAMETER,
			    0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
			    RG_DPHY_RX_LC1_HS_PREPARE_EN,
			    0x1);
	} else {
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, 0);

		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
			    RG_DPHY_RX_LC0_HS_PREPARE_EN,
			    0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
			    RG_DPHY_RX_LC1_HS_PREPARE_EN,
			    0x0);
	}

	return 0;
}

static int csirx_cphy_init(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];

	SENINF_BITS(base, CPHY_RX_DETECT_CTRL_POST,
		    RG_CPHY_RX_DATA_VALID_POST_EN, 1);

	return 0;
}

static int csirx_phy_init(struct seninf_ctx *ctx)
{
	/* phyA init */
	csirx_phyA_init(ctx);

	/* phyD init */
	csirx_dphy_init(ctx);
	csirx_cphy_init(ctx);

	return 0;
}

static int mtk_cam_seninf_set_seninf_top_ctrl2(struct seninf_ctx *ctx, u32 val)
{
	void *pSeninf_top = ctx->reg_if_top;
	int port;
	u32 tmp;

	port = ctx->port;
	tmp = SENINF_READ_REG(pSeninf_top, SENINF_TOP_CTRL2);

	/* afifo pop out splited from 1T to 2T */
	switch (port) {
	case CSI_PORT_0:
	case CSI_PORT_0A:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 0)) : (tmp & ~(1 << 0)));
		break;
	case CSI_PORT_0B:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 8)) : (tmp & ~(1 << 8)));
		break;
	case CSI_PORT_1:
	case CSI_PORT_1A:
		SENINF_WRITE_REG(pSeninf_top,
						 SENINF_TOP_CTRL2,
						 val ? (tmp | (1 << 1)) : (tmp & ~(1 << 1)));
		break;
	case CSI_PORT_1B:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 9)) : (tmp & ~(1 << 9)));
		break;
	case CSI_PORT_2:
	case CSI_PORT_2A:
		SENINF_WRITE_REG(pSeninf_top,
						 SENINF_TOP_CTRL2,
						 val ? (tmp | (1 << 2)) : (tmp & ~(1 << 2)));
		break;
	case CSI_PORT_2B:
		SENINF_WRITE_REG(pSeninf_top,
						 SENINF_TOP_CTRL2,
						 val ? (tmp | (1 << 10)) : (tmp & ~(1 << 10)));
		break;
	case CSI_PORT_3:
	case CSI_PORT_3A:
		SENINF_WRITE_REG(pSeninf_top,
						 SENINF_TOP_CTRL2,
						 val ? (tmp | (1 << 3)) : (tmp & ~(1 << 3)));
		break;
	case CSI_PORT_3B:
		SENINF_WRITE_REG(pSeninf_top,
						 SENINF_TOP_CTRL2,
						 val ? (tmp | (1 << 11)) : (tmp & ~(1 << 11)));
		break;
	case CSI_PORT_4:
	case CSI_PORT_4A:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 4)) : (tmp & ~(1 << 4)));
		break;
	case CSI_PORT_4B:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 12)) : (tmp & ~(1 << 12)));
		break;
	case CSI_PORT_5:
	case CSI_PORT_5A:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 5)) : (tmp & ~(1 << 5)));
		break;
	case CSI_PORT_5B:
		SENINF_WRITE_REG(pSeninf_top,
						SENINF_TOP_CTRL2,
						val ? (tmp | (1 << 13)) : (tmp & ~(1 << 13)));
		break;
	default:
		dev_info(ctx->dev, "invalid port %d\n", port);
		return -EINVAL;
	}

	dev_info(ctx->dev, "[%s] port:%d,TOP_CTRL2(0x%x)", __func__, port,
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_CTRL2));

	return 0;
}

static int seninf1_setting(struct seninf_ctx *ctx)
{
	void *pSeninf = ctx->reg_if_ctrl[(unsigned int)ctx->seninfIdx];
	void *pSeninf_top = ctx->reg_if_top;
	int port = ctx->port;;

	mtk_cam_seninf_set_seninf_top_ctrl2(ctx, 1);

	// enable/disable seninf csi2
	SENINF_BITS(pSeninf, SENINF_CSI2_CTRL, RG_SENINF_CSI2_EN, 1);

	// enable/disable seninf, enable after csi2, testmdl is done.
	SENINF_BITS(pSeninf, SENINF_CTRL, SENINF_EN, 1);

	dev_info(ctx->dev,
		"[%s] port:%d,TOP_CTRL2(0x%x),SENINF_CSI2_CTRL(0x%x),SENINF_CTRL(0x%x)",
		__func__,
		port,
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_CTRL2),
		SENINF_READ_REG(pSeninf, SENINF_CSI2_CTRL),
		SENINF_READ_REG(pSeninf, SENINF_CTRL));

	return 0;
}

static int csirx_mac_top_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_top;

	if ((ctx->port < 0) || (ctx->port >= CSI_PORT_MAX_NUM)) {
		dev_info(ctx->dev, "[%s][Error] ctx->port (%d) is invalid\n",
			__func__, ctx->port);
		return 	-EINVAL;
	}

	csirx_mac_top = ctx->reg_csirx_mac_top[ctx->port];
	dev_info(ctx->dev, "[%s] ctx->port %d\n", __func__, ctx->port);

	/* Select share bus option */
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL,
				RG_8PIX_SHARE_16PIX_DATA,
				(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 1 : 0);//porting for rayas, after check can remove, rayas mac page13 15

	/* Enable C / D phy */
	if (ctx->is_cphy) {
		/* C-PHY */
		SENINF_BITS(csirx_mac_top,
					CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					PHY_SENINF_MUX0_DPHY_EN,//porting for rayas, after check can remove, rayas mac page13, 15
					0); //Disable Dphy
		SENINF_BITS(csirx_mac_top,
					CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					PHY_SENINF_MUX0_CPHY_EN,//porting for rayas, after check can remove, rayas mac page13, 15
					1);
		/* C-PHY split mode */
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					RG_PHY_SENINF_MUX0_CPHY_MODE,
					(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 2 : 0);//porting for rayas, after check can remove, rayas mac page13 15
	} else {
		/* D-PHY */
		SENINF_BITS(csirx_mac_top,
					CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					PHY_SENINF_MUX0_CPHY_EN,//porting for rayas, after check can remove, rayas mac page13, 15
					0); //Disable Cphy
		SENINF_BITS(csirx_mac_top,
					CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					PHY_SENINF_MUX0_DPHY_EN,//porting for rayas, after check can remove, rayas mac page13, 15
					1);
		/* D-PHY split mode */
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0,
					RG_PHY_SENINF_MUX0_CPHY_MODE, 0);//porting for rayas, after check can remove, rayas mac page13 15
	}
	return 0;
}

static int csirx_mac_csi_fixed_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi_A = NULL;
	void *csirx_mac_csi_B = NULL;

	/* get csi A/B base address */
	switch (ctx->port) {
	case CSI_PORT_0A:
	case CSI_PORT_1A:
	case CSI_PORT_2A:
	case CSI_PORT_3A:
	case CSI_PORT_4A:
	case CSI_PORT_5A:
		csirx_mac_csi_A = ctx->reg_csirx_mac_csi[(unsigned int)ctx->portA];
		break;
	case CSI_PORT_0B:
	case CSI_PORT_1B:
	case CSI_PORT_2B:
	case CSI_PORT_3B:
	case CSI_PORT_4B:
	case CSI_PORT_5B:
		csirx_mac_csi_B = ctx->reg_csirx_mac_csi[(unsigned int)ctx->portB];
		break;
	case CSI_PORT_0:
	case CSI_PORT_1:
	case CSI_PORT_2:
	case CSI_PORT_3:
	case CSI_PORT_4:
	case CSI_PORT_5:
		csirx_mac_csi_A = ctx->reg_csirx_mac_csi[(unsigned int)ctx->portA];
		csirx_mac_csi_B = ctx->reg_csirx_mac_csi[(unsigned int)ctx->portB];
		break;
	default:
		dev_info(ctx->dev, "[%s] ctx->port %d is invalid\n",
		__func__, ctx->port);
		break;
	}

	/* CSIRX_MAC_CSI2_A */
	if (csirx_mac_csi_A) {
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_ECC_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_B2P_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_IMG_PACKET_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_SPEC_V2P0_SEL, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_MULTI_FRAME_VLD_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_MODE, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_LEN_SEL, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_DMY_EN, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT_OPT, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT, 0x1F);//porting for rayas, after check can remove, rayas mac page9

		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L0_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L1_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L2_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L3_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9

		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_OPT, RG_RAW8_PIXEL_DOUBLE, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_CSI2_DESKEW_BYPASS, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_CSI2_SYNC_INIT_DESKEW_EN, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_A, CSIRX_MAC_CSI2_SPARE, RG_CSI2_SPARE_0, 0);//porting for rayas, after check can remove, rayas mac page9
	}

	/* CSIRX_MAC_CSI2_B */
	if (csirx_mac_csi_B) {
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_ECC_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_B2P_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_IMG_PACKET_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_SPEC_V2P0_SEL, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_MULTI_FRAME_VLD_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_MODE, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_LEN_SEL, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_DMY_EN, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT_OPT, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT, 0x1F);//porting for rayas, after check can remove, rayas mac page9

		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L0_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L1_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L2_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_IRQ_G1_EN, RG_CSI2_DESKEW_FIFO_OVERFLOW_L3_IRQ_EN, 1);//porting for rayas, after check can remove, rayas mac page9

		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_OPT, RG_RAW8_PIXEL_DOUBLE, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_CSI2_DESKEW_BYPASS, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_CSI2_SYNC_INIT_DESKEW_EN, 0);//porting for rayas, after check can remove, rayas mac page9
		SENINF_BITS(csirx_mac_csi_B, CSIRX_MAC_CSI2_SPARE, RG_CSI2_SPARE_0, 0);//porting for rayas, after check can remove, rayas mac page9
	}
	return 0;
}

static int csirx_mac_csi_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	int bit_per_pixel = 10;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	struct seninf_core *core = ctx->core;
	u64 csi_clk = SENINF_CK;

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	csi_clk =
		core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk ?
		(u64)core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk * CSI_CLK_FREQ_MULTIPLIER :
		SENINF_CK;

	/* select C / D phy */
	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_CPHY_SEL,
				(ctx->is_cphy) ? 1 : 0);//porting for rayas, after check can remove, rayas mac page13, 15

	/* enable csi2 lane */
	switch (ctx->port) {
	case CSI_PORT_0:
	case CSI_PORT_1:
	case CSI_PORT_2:
	case CSI_PORT_3:
	case CSI_PORT_4:
	case CSI_PORT_5:
		/* Enalbe 4 lane data lane */
		switch (ctx->num_data_lanes) {
		case 1:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		case 2:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		case 3:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		case 4:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
		}
		break;

	case CSI_PORT_0A:
	case CSI_PORT_1A:
	case CSI_PORT_2A:
	case CSI_PORT_3A:
	case CSI_PORT_4A:
	case CSI_PORT_5A:
		/* Enalbe 2 lane data lane */
		switch (ctx->num_data_lanes) {
		case 1:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		case 2:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);//porting for rayas, after check can remove, rayas mac page13, 15
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
		}
		break;

	case CSI_PORT_0B://rayas mac page13, 15 remove port B?
	case CSI_PORT_1B:
	case CSI_PORT_2B:
	case CSI_PORT_3B:
	case CSI_PORT_4B:
	case CSI_PORT_5B:
		/* Enalbe 1 lane data lane */
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 0);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
		break;

	default:
		dev_info(ctx->dev, "[%s] ctx->port %d is invalid\n",
		__func__, ctx->port);
		break;
	}

	/* Select CSI2 pixel mode */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT,
				RG_CSI2_8_16_PIXEL_SEL,
				(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 1 : 0);//porting for rayas, after check can remove, rayas mac page13, 15


	/* write fix setting */
	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_ECC_EN,
				1);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_B2P_EN,
				1);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_IMG_PACKET_EN,
				1);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_SPEC_V2P0_SEL,
				1);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_MULTI_FRAME_VLD_EN,
				1);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_VS_OUTPUT_MODE,
				0);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_OPT,
				RG_CSI2_VS_OUTPUT_LEN_SEL,
				0);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
				RG_CSI2_RESYNC_CYCLE_CNT,
				0x1F);

	/* MAC CSI CHECKER ENABLE */
	SENINF_WRITE_REG(csirx_mac_csi,
		CSIRX_MAC_CSI2_SIZE_CHK_CTRL0, 0x002B0011);
	SENINF_WRITE_REG(csirx_mac_csi,
		CSIRX_MAC_CSI2_SIZE_CHK_CTRL1, 0x002B0111);
	SENINF_WRITE_REG(csirx_mac_csi,
		CSIRX_MAC_CSI2_SIZE_CHK_CTRL2, 0x002B0211);
	SENINF_WRITE_REG(csirx_mac_csi,
		CSIRX_MAC_CSI2_SIZE_CHK_CTRL3, 0x002C0011);
	SENINF_WRITE_REG(csirx_mac_csi,
		CSIRX_MAC_CSI2_SIZE_CHK_CTRL4, 0x002D0011);

	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
				RG_CSI2_RESYNC_CYCLE_CNT_OPT,
				1);

	/* Enable CSI2 interrupt */
	SENINF_WRITE_REG(csirx_mac_csi,
					CSIRX_MAC_CSI2_IRQ_EN,
					0x80000000);


	SENINF_BITS(csirx_mac_csi,
				CSIRX_MAC_CSI2_DBG_CTRL,
				RG_CSI2_DBG_PACKET_CNT_EN,
				1);


	if (!ctx->is_cphy) { //Dphy

		u64 data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		u64 cycles = 64;

		cycles *= csi_clk;
		do_div(data_rate, ctx->num_data_lanes);
		do_div(cycles, data_rate);
		cycles += CYCLE_MARGIN;

		if (ctx->csi_param.dphy_csi2_resync_dmy_cycle)
			cycles = ctx->csi_param.dphy_csi2_resync_dmy_cycle;

		dev_info(ctx->dev,
		"%s data_rate %lld bps cycles %lld\n",
		__func__, data_rate, cycles);

		/* reset HDR_MODE_0 */
		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_HDR_MODE_0,
					RG_CSI2_HEADER_MODE,//porting for rayas, after check can remove, rayas mac page13, 15
					0);

		/* reset HDR_MODE_0 */
		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_HDR_MODE_0,
					RG_CSI2_HEADER_LEN,//porting for rayas, after check can remove, rayas mac page13, 15
					0);

		/* Setting DEFAULT RESYNC_MERGE SETTING */
		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_CYCLE,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_CNT,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_EN,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_LRTE_EN,
					0);

	} else { //Cphy
		u8 map_hdr_len[] = {0, 1, 2, 4, 5};
		u64 cycles = 64;
		u64 data_rate = ctx->mipi_pixel_rate * bit_per_pixel;

		cycles *= csi_clk;
		data_rate *= 7;
		do_div(data_rate, ctx->num_data_lanes*16);
		do_div(cycles, data_rate);
		cycles += CYCLE_MARGIN;

		if (ctx->csi_param.dphy_csi2_resync_dmy_cycle)
			cycles = ctx->csi_param.dphy_csi2_resync_dmy_cycle;

		dev_info(ctx->dev,
		"%s data_rate %lld pps cycles %lld, ctx->num_data_lanes %d\n",
		__func__, data_rate, cycles, ctx->num_data_lanes);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_HDR_MODE_0,
					RG_CSI2_HEADER_MODE,//porting for rayas, after check can remove, rayas mac page13, 15
					2); //cphy

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_HDR_MODE_0,
					RG_CSI2_HEADER_LEN,//porting for rayas, after check can remove, rayas mac page13, 15
					map_hdr_len[(unsigned int)ctx->num_data_lanes]);
		/* map_hdr_len[] = {0, 1, 2, 4, 5} */

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_CYCLE,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_CNT,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_DMY_EN,
					0);

		SENINF_BITS(csirx_mac_csi,
					CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL,
					RG_CSI2_RESYNC_LRTE_EN,
					0);

	}

	return 0;
}

static int csirx_mac_csi_lrte_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	void *cphy_base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];

	dev_info(ctx->dev, "[%s] lrte_support flag = %d\n",
			__func__, ctx->csi_param.cphy_lrte_support);

	if(!ctx->csi_param.cphy_lrte_support) {
		/* stability debug disable LRTE_EN */
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_LRTE_EN, 0x0);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN, 0x0);
		dev_info(ctx->dev, "[%s] lrte not support(%d), disable LRTE_EN ALP_EN, port:%d\n",
			__func__, ctx->csi_param.cphy_lrte_support, ctx->port);
		return 0;
	}

	if (!ctx->is_cphy) {
		/* DPHY Config */
		dev_info(ctx->dev, "[%s] Dphy sensor skip lrte mac_csi setting, port:%d\n",
			__func__, ctx->port);
	} else {
		/* CPHY Config */
		/* LRTE CPHY setting in mac, DPHY setting todo in DPHY reg */
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_LRTE_EN, 0x1);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_RESYNC_LRTE_WPTR_LENGTH, 0x1);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_RESYNC_LRTE_PKT_HSRST, 0x0);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_RESYNC_LRTE_WC_DMY_OPTION, 0x0);

		/* CPHY LRTE TX spacer must greater than 40 */
		dev_info(ctx->dev, "[%s] Cphy lrte support(%d), port:%d\n",
			__func__, ctx->csi_param.cphy_lrte_support, ctx->port);

		/* LRTE SW Workaround */
		SENINF_BITS(dphy_base, DPHY_RX_SPARE1, RG_POST_CNT, 0x1);
		SENINF_BITS(cphy_base, CPHY_RX_STATE_CHK_EN, RG_ALP_POS_DET_MASK, 0xFF);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_SETTLE_PARAMETER, 0x23);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_ALP_RX_EN_SEL, 0x0);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN, 0x1);
		SENINF_BITS(cphy_base, CPHY_RX_INIT, RG_CPHY_CSI2_TINIT_CNT_EN, 0x1);
		SENINF_BITS(cphy_base, CPHY_POST_ENCODE, CPHY_POST_REPLACE_EN, 0x0);
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(dphy_base, DPHY_RX_SPARE1, RG_POST_CNT));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_RX_STATE_CHK_EN, RG_ALP_POS_DET_MASK));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_SETTLE_PARAMETER));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_ALP_RX_EN_SEL));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_RX_INIT, RG_CPHY_CSI2_TINIT_CNT_EN));
		dev_info(ctx->dev, "[%s] LRTE DPHY_RX_SPARE1(0x%x)\n", __func__,
			SENINF_READ_BITS(cphy_base, CPHY_POST_ENCODE, CPHY_POST_REPLACE_EN));
	}

	return 0;
}

#define GET_DATA_RATE_FOR_PHYD(ctx) do { \
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);\
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);\
	int bit_per_pixel = 10;\
	if (vc)\
		bit_per_pixel = vc->bit_depth;\
	else if (vc1)\
		bit_per_pixel = vc1->bit_depth;\
	data_rate = ctx->mipi_pixel_rate * bit_per_pixel;\
	do_div(data_rate, ctx->num_data_lanes);\
} while (0)

#define GET_DATA_RATE_FOR_PHYC(ctx) do { \
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);\
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);\
	int bit_per_pixel = 10;\
	if (vc)\
		bit_per_pixel = vc->bit_depth;\
	else if (vc1)\
		bit_per_pixel = vc1->bit_depth;\
	data_rate = ctx->mipi_pixel_rate * bit_per_pixel;\
	data_rate *= 7;\
	do_div(data_rate, ctx->num_data_lanes*16);\
} while (0)

static int csirx_phyA_setting(struct seninf_ctx *ctx)
{
	void *base, *baseA, *baseB, *dphy_base;
	u64 data_rate = 0;

	base = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];
	baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];
	dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];

	//dev_info(ctx->dev, "port %d A %d B %d\n", ctx->port, ctx->portA, ctx->portB);

	if (!ctx->is_cphy) {
		/* DPHY Config */
		u32 pn_swap_en = SENINF_READ_BITS(dphy_base,
						DPHY_RX_HS_RX_EN_SW,
						RG_DPHY_PHY_PN_SWAP_EN);

		GET_DATA_RATE_FOR_PHYD(ctx);
		dev_info(ctx->dev, "DPHY data_rate %llu bps\n", data_rate);

		SENINF_BITS(baseA,
				CDPHY_RX_ANA_SETTING_1,
				RG_CSI0_ASYNC_OPTION,
				((!pn_swap_en) && (data_rate < 4000000000)) ? 0x4 : 0x0);

		SENINF_BITS(baseA,
					CDPHY_RX_ANA_SETTING_1,
					RG_AFIFO_DUMMY_VALID_NUM,
					(ctx->num_data_lanes > 1) ? 0x3 : 0x5);
		SENINF_BITS(baseB,
					CDPHY_RX_ANA_SETTING_1,
					RG_AFIFO_DUMMY_VALID_NUM,
					(ctx->num_data_lanes > 1) ? 0x3 : 0x5);

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
		    RG_AFIFO_DUMMY_VALID_EN, 0x1);

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
		    RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x4);

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
			AFIFO_DUMMY_VALID_GAP_NUM, 0x4);

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_0,
		    CSR_ASYNC_FIFO_GATING_SEL, 0x2);

		if (ctx->is_4d1c) {
			/* DPHY non-split mode */
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 0);
			// clear clk sel first
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 0);

			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKSEL, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKSEL, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKSEL, 1);

			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 0);

			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);

			if (data_rate < 1500000000) {
				/* DPHY non-split mode < 1.5G */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 2500000000) {
				/* DPHY non-split mode 1.5G ~ 2.5G */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 4500000000)  {
				/* DPHY non-split mode 2.5G ~ 4.5G */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x3);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x0);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x3);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
			} else {
				dev_info(ctx->dev, "[ERROR]data_rate %llu bps > 4.5G not support\n", data_rate);
			}

		} else {
			/* DPHY split mode */
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 0);
			// clear clk sel first
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 0);

			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKSEL, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKSEL, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKSEL, 0);

			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L1_CKMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_DPHY_L2_CKMODE_EN, 0);

			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);

			if (data_rate < 1500000000) {
				/* DPHY split mode < 1.5G */
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 2500000000) {
				/* DPHY split mode 1.5G ~ 2.5G */
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 4500000000)	{
				/* DPHY split mode 2.5G ~ 4.5G*/
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_BW, 0x2);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_SR0, 0x0);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
					    RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
					    RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
			} else {
				/* Not support > 4.5G*/
				dev_info(ctx->dev, "[ERROR]data_rate %llu bps > 4.5G not support\n", data_rate);
			}


		}
	} else {
		/* CPHY Config */
		GET_DATA_RATE_FOR_PHYC(ctx);
		dev_info(ctx->dev, "CPHY data_rate %llu sps\n", data_rate);

		if (ctx->is_4d1c) {
			/* CPHY non-split mode */
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 1);

			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);

			if (data_rate < 2500000000) {
				SENINF_WRITE_REG(baseA, CDPHY_RX_ANA_5, 0x55);
				SENINF_WRITE_REG(baseB, CDPHY_RX_ANA_5, 0x55);
			} else {
				SENINF_WRITE_REG(baseA, CDPHY_RX_ANA_5, 0x157);
				SENINF_WRITE_REG(baseB, CDPHY_RX_ANA_5, 0x157);
			}

			SENINF_WRITE_REG(baseA, CDPHY_RX_ANA_SETTING_0, 0x322);
			SENINF_WRITE_REG(baseB, CDPHY_RX_ANA_SETTING_0, 0x322);


			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
			    RG_AFIFO_DUMMY_VALID_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
			    RG_CSI0_ASYNC_OPTION, 0xC);
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
				RG_AFIFO_DUMMY_VALID_PREPARE_NUM,
				(ctx->num_data_lanes > 1) ? 0x5 : 0x6);

			SENINF_BITS(baseA,
					CDPHY_RX_ANA_SETTING_1,
					RG_AFIFO_DUMMY_VALID_NUM,
					(ctx->num_data_lanes > 1) ? 0x4 : 0x6);
			SENINF_BITS(baseB,
					CDPHY_RX_ANA_SETTING_1,
					RG_AFIFO_DUMMY_VALID_NUM,
					(ctx->num_data_lanes > 1) ? 0x4 : 0x6);

			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
			AFIFO_DUMMY_VALID_GAP_NUM, 0x4);
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_0,
		    CSR_ASYNC_FIFO_GATING_SEL, 0x0);

			SENINF_BITS(baseA, CDPHY_RX_ANA_3,
				RG_CSI0_EQ_DES_VREF_SEL, 0x20);

			SENINF_BITS(baseB, CDPHY_RX_ANA_3,
				RG_CSI0_EQ_DES_VREF_SEL, 0x20);
			if (data_rate < 1500000000) {
				/* CPHY non-split mode < 1.5G */
				/* baseA */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseA, CDPHY_RX_ANA_6,
					RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x14);
				SENINF_BITS(baseA, CDPHY_RX_ANA_7,
					RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x14);

				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
					RG_CSI0_CDPHY_EQ_OS_IS, 0x1);

				/* baseB */
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseB, CDPHY_RX_ANA_6,
					RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x14);
				SENINF_BITS(baseB, CDPHY_RX_ANA_7,
					RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x14);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
					RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
					RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 2500000000) {
				/* CPHY non-split mode 1.5G ~ 2.5G */
				/* baseA */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseA, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0xA);
				SENINF_BITS(baseA, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0xA);

				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x1);

				/* baseB */
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseB, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0xA);
				SENINF_BITS(baseB, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0xA);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 4500000000) {
				/* CPHY non-split mode 2.5G ~ 4.5G */
				/* baseA */
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x3);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseA, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x6);
				SENINF_BITS(baseA, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x6);

				SENINF_BITS(baseA, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
				/* baseB */
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x3);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(baseB, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x6);
				SENINF_BITS(baseB, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x6);

				SENINF_BITS(baseB, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
			} else {
				dev_info(ctx->dev, "[ERROR]data_rate %llu sps > 4.5G not support\n", data_rate);
			}

		} else {
			/* CPHY split mode */
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0,
				    RG_CSI0_CPHY_T1_HSMODE_EN, 1);

			if (data_rate < 2500000000)
				SENINF_WRITE_REG(base, CDPHY_RX_ANA_5, 0x55);
			else
				SENINF_WRITE_REG(base, CDPHY_RX_ANA_5, 0x157);

			SENINF_WRITE_REG(base, CDPHY_RX_ANA_SETTING_0, 0x322);

			/*baseA works for both A & B*/
			SENINF_BITS(base, CDPHY_RX_ANA_3,
				RG_CSI0_EQ_DES_VREF_SEL, 0x20);

			if (data_rate < 1500000000) {
				/* CPHY split mode < 1.5G */
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x14);
				SENINF_BITS(base, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x14);

				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			}else if (data_rate < 2500000000) {
				/* CPHY split mode 1.5G ~ 2.5G */
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0xA);
				SENINF_BITS(base, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0xA);

				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x2);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			} else if (data_rate < 4500000000) {
				/* CPHY split mode 2.5G ~ 4.5G */
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_BW, 0x3);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_SR0, 0x0);

				SENINF_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x6);
				SENINF_BITS(base, CDPHY_RX_ANA_7,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x6);

				SENINF_BITS(base, CDPHY_RX_ANA_5,
						RG_CSI0_CDPHY_EQ_IS, 0x1);
				SENINF_BITS(base, CDPHY_RX_ANA_14,
						RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
			} else {
				dev_info(ctx->dev, "[ERROR]data_rate %llu sps > 4.5G not support\n", data_rate);
			}
		}
	}

	/* phyA power on */

	if (ctx->is_4d1c) {
		csirx_phyA_power_on(ctx, ctx->portA, 1);
		csirx_phyA_power_on(ctx, ctx->portB, 1);
	} else {
		csirx_phyA_power_on(ctx, ctx->port, 1);
	}

	return 0;
}

#ifdef CDPHY_ULPS_MODE_SUPPORT
static int csirx_dphy_ulps_setting(struct seninf_ctx *ctx)
{
	void *baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	void *baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];

	if (ctx->is_4d1c) {
		/* DPHY non-split mode */
		switch(ctx->num_data_lanes) {
		/* DPHY non-split mode lane 1 */
		case 1:/* 1D1C must !is_4d1c */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			dev_info(ctx->dev, "[%s]: ERROR: 1D1C must split\n", __func__);
			break;

		/* DPHY non-split mode lane 2 */
		case 2:/* 2D1C-AB */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			break;

		/* DPHY non-split mode lane 4 */
		case 4:/* 4D1C */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	} else {
		/* DPHY split mode */
		switch(ctx->num_data_lanes) {
		/* DPHY split mode lane 1 */
		case 1:/* 1D1C-A or 1D1C-B */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;
			case CSI_PORT_0B:
			case CSI_PORT_1B:
			case CSI_PORT_2B:
			case CSI_PORT_3B:
			case CSI_PORT_4B:
			case CSI_PORT_5B:

				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;
			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}

			break;

		/* DPHY split mode lane 2 */
		case 2:/* 2D1C-A */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;

			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}
			break;

		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	}

	dev_info(ctx->dev,
			"[%s][Done] with is_4d1c(%d),num_data_lanes(%d),port(%d),ULPS_IRQ_EN\n",
			__func__,
			ctx->is_4d1c,
			ctx->num_data_lanes,
			ctx->port);

	return 0;
}

static int csirx_cphy_ulps_setting(struct seninf_ctx *ctx)
{
	void *baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	void *baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];

	if (ctx->is_4d1c) {
		/* CPHY non-split mode */
		switch(ctx->num_data_lanes) {
		/* CPHY non-split mode lane 1 */
		case 1:/* 1T must !is_4d1c */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			dev_info(ctx->dev, "[%s]: ERROR: 1D1C must split\n", __func__);
			break;

		/* CPHY non-split mode lane 2 */
		case 2:/* 2T-AB */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			break;

		/* CPHY non-split mode lane 4 */
		case 3:/* 3T */
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
			SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	} else {
		/* CPHY split mode */
		switch(ctx->num_data_lanes) {
		/* CPHY split mode lane 1 */
		case 1:/* 1T-A or 1T-B */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;
			case CSI_PORT_0B:
			case CSI_PORT_1B:
			case CSI_PORT_2B:
			case CSI_PORT_3B:
			case CSI_PORT_4B:
			case CSI_PORT_5B:
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;
			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}

			break;

		/* CPHY split mode lane 2 */
		case 2:/* 2T-A */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseA, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);

				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L0_T0_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_EN, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ULPS_CTRL_0, CSI_CDPHY_ULPS_L2_T1_CG_FORCE_ON, 0x0);
				SENINF_BITS(baseB, ULPS_IRQ_CTRL, RG_ULPS_IRQ_EN, 0x1);
				break;

			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}
			break;

		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	}

	dev_info(ctx->dev,
			"[%s][Done] with is_4d1c(%d),num_data_lanes(%d),port(%d),ULPS_IRQ_EN\n",
			__func__,
			ctx->is_4d1c,
			ctx->num_data_lanes,
			ctx->port);

	return 0;
}
#endif /* CDPHY_ULPS_MODE_SUPPORT */

static int debug_init_deskew_begin_end_apply_code(struct seninf_ctx *ctx)
{
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	unsigned int lane_idx;
	unsigned int apply_code = 0;
	unsigned int begin_end_code = 0;

	SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_MODE_SELECT, 0x1111);

	for (lane_idx = CSIRX_LANE_A0; lane_idx < CSIRX_LANE_MAX_NUM; lane_idx++) {
		if (lane_idx == CSIRX_LANE_A0) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_A0\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x03020100);
		} else if(lane_idx == CSIRX_LANE_A1) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_A1\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x1b1a1918);
		} else if(lane_idx == CSIRX_LANE_A2) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_A2\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x33323130);
		} else if(lane_idx == CSIRX_LANE_B0) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_B0\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x4b4a4948);
		} else if(lane_idx == CSIRX_LANE_B1) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_B1\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x63626160);
		} else if(lane_idx == CSIRX_LANE_B2) {
			dev_info(ctx->dev, "[%s] CSIRX_LANE_B2\n", __func__);
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x7b7a7978);
		} else {
			dev_info(ctx->dev, "[%s] ERROR lane\n", __func__);
		}
		SENINF_WRITE_REG(dphy_base, DPHY_RX_DESKEW_DBG_MUX, 0x1);
		udelay(5);
		apply_code = SENINF_READ_REG(dphy_base, CDPHY_DEBUG_OUT_READ);
		dev_info(ctx->dev, "[%s] read delay code CDPHY_DEBUG_OUT_READ = 0x%x, apply_code = %d\n",
			__func__,
			apply_code,
			(apply_code  >> 24 & 0x3f));

		SENINF_WRITE_REG(dphy_base, DPHY_RX_DESKEW_DBG_MUX, 0x8);
		udelay(5);
		begin_end_code = SENINF_READ_REG(dphy_base, CDPHY_DEBUG_OUT_READ);
		dev_info(ctx->dev, "[%s] read begin/end code CDPHY_DEBUG_OUT_READ = 0x%x, begin = %d, end = %d\n",
			__func__,
			begin_end_code,
			((begin_end_code) >> 8 & 0x3f),
			((begin_end_code) >> 16 & 0x3f));
	}
	return 0;
}

static int csirx_dphy_init_deskew_setting(struct seninf_ctx *ctx, u64 seninf_ck)
{
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	int bit_per_pixel = 10;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	u64 data_rate = 0;

	dev_info(ctx->dev, "[%s] dphy_init_deskew_support = %d\n",
			__func__, ctx->csi_param.dphy_init_deskew_support);

	if (!ctx->csi_param.dphy_init_deskew_support) {
		//porting for rayas, after check can remove, ponsot cdphy page13
		/* Disable DESKEW LANE0~3 CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL, DPHY_RX_DESKEW_L0_DELAY_EN, 0);//porting for rayas, after check can remove, rayas cdphy page13
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL, DPHY_RX_DESKEW_L1_DELAY_EN, 0);//porting for rayas, after check can remove, rayas cdphy page13
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL, DPHY_RX_DESKEW_L2_DELAY_EN, 0);//porting for rayas, after check can remove, rayas cdphy page13
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL, DPHY_RX_DESKEW_L3_DELAY_EN, 0);//porting for rayas, after check can remove, rayas cdphy page13
		return 0;
	}

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
	do_div(data_rate, ctx->num_data_lanes);


#ifdef INIT_DESKEW_UT
	if (adb_data_rate != 0) {
		data_rate = adb_data_rate;
		data_rate = data_rate * 1000000;
	}

	if (adb_seninf_clk != 0) {
		seninf_ck = adb_seninf_clk;
		seninf_ck = seninf_ck * 1000000;
	}
#endif /* INIT_DESKEW_UT */

	if (data_rate > SENINF_DESKEW_DATA_RATE_1500M) {
		/* Init deskew pattern calibration config */

#ifdef INIT_DESKEW_DEBUG
		dev_info(ctx->dev, "[%s] dump begin_end_apply_code before sw reset\n", __func__);
		debug_init_deskew_begin_end_apply_code(ctx);
#endif /* INIT_DESKEW_DEBUG */

		/* deskew sw reset */
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, DPHY_RX_DESKEW_SW_RST, 1);
		udelay(2);
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, DPHY_RX_DESKEW_SW_RST, 0);

#ifdef INIT_DESKEW_DEBUG
		dev_info(ctx->dev, "[%s] dump begin_end_apply_code after sw reset\n", __func__);
		debug_init_deskew_begin_end_apply_code(ctx);
#endif /* INIT_DESKEW_DEBUG */

		/* DESKEW CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_INITIAL_SETUP, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_EN, 1);

		if (data_rate < SENINF_DESKEW_DATA_RATE_3200M)
			SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_CODE_UNIT_SEL, 2);//porting for rayas, after check can remove, rayas cdphy page13
		else
			SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_CODE_UNIT_SEL, 1);//porting for rayas, after check can remove, rayas cdphy page13
		/* RG_DPHY_RX_DESKEW_CODE_UNIT_SEL */
		/* Date rate < 3.2G : set 'b10 */
		/* Date rate > 3.2G : set 'b01 */

		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_DELAY_APPLY_OPT, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_ACC_MODE, 0);//porting for rayas, after check can remove, rayas cdphy page13

		/* DESKEW LANE0~3 CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL, DPHY_RX_DESKEW_L0_DELAY_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL, DPHY_RX_DESKEW_L1_DELAY_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL, DPHY_RX_DESKEW_L2_DELAY_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL, DPHY_RX_DESKEW_L3_DELAY_EN, 1);

		/* DESKEW TIMING CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL, RG_DPHY_RX_DESKEW_DETECT_CNT, 2);

		SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL, RG_DPHY_RX_DESKEW_CMPLENGTH, 3);

		/* RG_DPHY_RX_DESKEW_SETUP_CNT setting by csi_clk default 312MHZ & data rate */
		switch (seninf_ck) {
		case SENINF_CLK_499_2MHZ:
			if (data_rate < SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 20);//porting for rayas, after check can remove, rayas cdphy page13
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 12);//porting for rayas, after check can remove, rayas cdphy page13
			}
			break;
		case SENINF_CLK_416_MHZ:
			if (data_rate < SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 18);//porting for rayas, after check can remove, rayas cdphy page13
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 11);//porting for rayas, after check can remove, rayas cdphy page13
			}
			break;
		case SENINF_CLK_343_MHZ://porting for rayas, after check can remove, rayas cdphy page13
			if (data_rate < SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 15);//porting for rayas, after check can remove, rayas cdphy page13
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 10);//porting for rayas, after check can remove, rayas cdphy page13
			}
			break;
		case SENINF_CLK_312_MHZ:
			if (data_rate < SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 14);//porting for rayas, after check can remove, rayas cdphy page13
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 9);//porting for rayas, after check can remove, rayas cdphy page13
			}
			break;
		default:
			dev_info(ctx->dev, "ERROR csi_clk no match\n");
			break;
		}

		SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL, RG_DPHY_RX_DESKEW_HOLD_CNT, 0);

		/* DESKEW LANE SWAP */
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE_SWAP, RG_APPLY_ONLY_1ST_PAT, 1);

		/* DESKEW LANE SYNC DETECT DESKEW */
		SENINF_BITS(base, DPHY_RX_DATA_LANE_SYNC_DETECT_DESKEW,
			RG_DPHY_RX_LD_SYNC_SEQ_PAT_DESKEW, 0xFF);
		SENINF_BITS(base, DPHY_RX_DATA_LANE_SYNC_DETECT_DESKEW,
			RG_DPHY_RX_LD_SYNC_SEQ_MASK_DESKEW, 0x00);

		/* lp11 idle en (new)*/
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL,
			DPHY_RX_DESKEW_LP11_IDLE_EN, 0x1);//porting for rayas, after check can remove, rayas cdphy page13

		/* enable deskew irq*/
		SENINF_WRITE_REG(base, DPHY_RX_DESKEW_IRQ_EN, 0xffffffff);
		SENINF_WRITE_REG(base, DPHY_RX_IRQ_EN, 0xffffffff);
		SENINF_WRITE_REG(base, DPHY_RX_IRQ_EN, 0x800000ff);

		dev_info(ctx->dev, "Data rate = %llu, SENINF CLK = %llu init deskew setting done\n",
			data_rate, seninf_ck);
#ifdef INIT_DESKEW_UT
		dev_info(ctx->dev, "DPHY_RX_DESKEW_CTRL = 0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_DESKEW_CTRL));
		dev_info(ctx->dev, "DPHY_RX_DESKEW_LANE_CTRL = 0x%x|0x%x|0x%x|0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_DESKEW_LANE0_CTRL),
			SENINF_READ_REG(base, DPHY_RX_DESKEW_LANE1_CTRL),
			SENINF_READ_REG(base, DPHY_RX_DESKEW_LANE2_CTRL),
			SENINF_READ_REG(base, DPHY_RX_DESKEW_LANE3_CTRL));
		dev_info(ctx->dev, "DPHY_RX_DESKEW_TIMING_CTRL = 0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_DESKEW_TIMING_CTRL));
		dev_info(ctx->dev, "DPHY_RX_DESKEW_LANE_SWAP = 0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_DESKEW_LANE_SWAP));
		dev_info(ctx->dev, "DPHY_RX_DATA_LANE_SYNC_DETECT_DESKEW = 0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_DATA_LANE_SYNC_DETECT_DESKEW));
		dev_info(ctx->dev, "DPHY_RX_IRQ_EN = 0x%x\n",
			SENINF_READ_REG(base, DPHY_RX_IRQ_EN));
#endif /* INIT_DESKEW_UT */

#ifdef INIT_DESKEW_DEBUG
		dev_info(ctx->dev, "[%s] after config\n", __func__);
		debug_init_deskew_begin_end_apply_code(ctx);
#endif /* INIT_DESKEW_DEBUG */
	} else {
		/* Disable DESKEW LANE0~3 CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL, DPHY_RX_DESKEW_L0_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL, DPHY_RX_DESKEW_L1_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL, DPHY_RX_DESKEW_L2_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL, DPHY_RX_DESKEW_L3_DELAY_EN, 0);
		seninf_aee_print("[AEE] error, [%s] Data rate (%llu) < 1.5G, no need deskew",
			__func__, data_rate);
	}
	return 0;
}


static int csirx_dphy_setting(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	void *baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];
	struct seninf_core *core = ctx->core;
	u64 csi_clk = SENINF_CK;
	int temp_RESERVE_0 = 0x0;

	csi_clk =
		core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk ?
		(u64)core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk * CSI_CLK_FREQ_MULTIPLIER :
		SENINF_CK;

	/* dummp_valid_num */
	SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
				RG_AFIFO_DUMMY_VALID_NUM,
				(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 3: 5) : 5);
	SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
				RG_AFIFO_DUMMY_VALID_NUM,
				(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 3: 5) : 5);

	/* dphy_ln*_flush_en all lane on */
	SENINF_BITS(base, CPHY_FSM_FLUSH_CTRL, CSI_DPHY_LN0_FLUSH_EN, 0x1);//porting for rayas, after check can remove, ponsot cdphy page9
	SENINF_BITS(base, CPHY_FSM_FLUSH_CTRL, CSI_DPHY_LN1_FLUSH_EN, 0x1);//porting for rayas, after check can remove, ponsot cdphy page9
	SENINF_BITS(base, CPHY_FSM_FLUSH_CTRL, CSI_DPHY_LN2_FLUSH_EN, 0x1);//porting for rayas, after check can remove, ponsot cdphy page9
	SENINF_BITS(base, CPHY_FSM_FLUSH_CTRL, CSI_DPHY_LN3_FLUSH_EN, 0x1);//porting for rayas, after check can remove, ponsot cdphy page9

	/* set only csiA RG_CSI0_RESERVE[0] */
	temp_RESERVE_0 = SENINF_READ_BITS(baseA, CDPHY_RX_ANA_8, RG_CSI0_RESERVE);
	temp_RESERVE_0 = temp_RESERVE_0 | 0x1;
	SENINF_BITS(baseA, CDPHY_RX_ANA_8,
						RG_CSI0_RESERVE, temp_RESERVE_0);

	if (ctx->is_4d1c) {
		/* DPHY non-split mode */
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x3);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							RG_SPLIT_EN, 0x0);

		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x3);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
		switch(ctx->num_data_lanes) {
		/* DPHY non-split mode lane 1 */
		case 1:/* 1D1C must !is_4d1c */
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);

			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 2);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 1);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);

			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);//porting for rayas, after check can remove, ponsot cdphy page11
			dev_info(ctx->dev, "[%s]: ERROR: 1D1C must split\n", __func__);
			break;

		/* DPHY non-split mode lane 2 */
		case 2:/* 2D1C-AB */
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);

			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 2);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 1);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 3);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);

			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L1_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);
			break;

		/* DPHY non-split mode lane 4 */
		case 4:/* 4D1C */
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 0);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 1);
			SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 1);

			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 2);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 1);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 3);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
			SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 4);

			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L1_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L1_AFIFO_FLUSH_EN, 0x1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	} else {
		/* DPHY split mode */
		switch(ctx->num_data_lanes) {
		/* DPHY split mode lane 1 */
		case 1:/* 1D1C-A or 1D1C-B */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);

				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 1);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);

				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);

				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_GAP_NUM, 0x3);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									RG_SPLIT_EN, 0x1);
				break;
			case CSI_PORT_0B:
			case CSI_PORT_1B:
			case CSI_PORT_2B:
			case CSI_PORT_3B:
			case CSI_PORT_4B:
			case CSI_PORT_5B:
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);

				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 4);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 3);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);

				SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);

				SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_GAP_NUM, 0x3);
				SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
									RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
				SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
				SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
									RG_SPLIT_EN, 0x1);
				break;
			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}

			break;

		/* DPHY split mode lane 2 */
		case 2:/* 2D1C-A */
			switch(ctx->port) {
			case CSI_PORT_0A:
			case CSI_PORT_1A:
			case CSI_PORT_2A:
			case CSI_PORT_3A:
			case CSI_PORT_4A:
			case CSI_PORT_5A:
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC0_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LC1_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD0_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);

				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 1);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 2);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);

				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L0_AFIFO_FLUSH_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0,
								L2_AFIFO_FLUSH_EN, 0x1);

				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_GAP_NUM, 0x3);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
									RG_SPLIT_EN, 0x1);
				break;

			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}
			break;

		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	}

	SENINF_BITS(base, DPHY_RX_LANE_SELECT, DPHY_RX_CK_DATA_MUX_EN, 1);
	SENINF_BITS(base, DPHY_RX_HS_RX_EN_SW, RG_DPHY_PHY_PN_SWAP_EN, 0);
	SENINF_BITS(base, DPHY_DPHYV21_CTRL, RG_DPHY_RX_SYNC_METH_SEL, 1);

	SENINF_WRITE_REG(base, DPHY_RX_SPARE0, 0xf1);

#ifdef CDPHY_ULPS_MODE_SUPPORT
	csirx_dphy_ulps_setting(ctx);
#endif /* CDPHY_ULPS_MODE_SUPPORT */

#ifdef INIT_DESKEW_SUPPORT
	csirx_dphy_init_deskew_setting(ctx, csi_clk);
#endif /* INIT_DESKEW_SUPPORT */

	/* DPHY_RX_IRQ_EN */
	SENINF_WRITE_REG(base, DPHY_RX_IRQ_EN, 0xF);

	dev_info(ctx->dev,
			"[%s][Done] with is_4d1c(%d),num_data_lanes(%d),port(%d),DPHY_RX_IRQ_EN\n",
			__func__,
			ctx->is_4d1c,
			ctx->num_data_lanes,
			ctx->port);

	return 0;
}

static int csirx_cphy_setting(struct seninf_ctx *ctx)
{
	void *cphy_base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	void *baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];

	switch (ctx->port) {
	case CSI_PORT_0:
	case CSI_PORT_1:
	case CSI_PORT_2:
	case CSI_PORT_3:
	case CSI_PORT_4:
	case CSI_PORT_5:
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_NUM,
						(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 0x4: 0x6) : 0x6);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_SPLIT_EN, 0x0);

		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_NUM,
						(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 0x4: 0x6) : 0x6);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x0);

		if (ctx->num_data_lanes == 3) {
			/* Lane AFIFO_FLUSH setting */
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_EN, 0X1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);

			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO1_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO2_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR1_LPRX_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR2_LPRX_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR3_LPRX_EN, 0);

		} else if (ctx->num_data_lanes == 2) {
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_EN, 0X1);

			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO1_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR1_LPRX_EN, 1);

		} else if (ctx->num_data_lanes == 1) {
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_SPLIT_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);

			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
		} else {
			dev_info(ctx->dev, "[error][%s] invalid ctx->num_data_lanes: %d\n",
				__func__, ctx->num_data_lanes);
		}
		break;
	case CSI_PORT_0A:
	case CSI_PORT_1A:
	case CSI_PORT_2A:
	case CSI_PORT_3A:
	case CSI_PORT_4A:
	case CSI_PORT_5A:
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_NUM,
						(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 0x4: 0x6) : 0x6);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);

		if (ctx->num_data_lanes == 2) {
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_SPLIT_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_EN, 0X1);

			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO1_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR1_LPRX_EN, 1);
		} else if (ctx->num_data_lanes == 1) {
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_SPLIT_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);

			SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);

		} else {
			dev_info(ctx->dev, "[error][%s] invalid ctx->num_data_lanes: %d\n",
				__func__, ctx->num_data_lanes);
		}
		break;
	case CSI_PORT_0B:
	case CSI_PORT_1B:
	case CSI_PORT_2B:
	case CSI_PORT_3B:
	case CSI_PORT_4B:
	case CSI_PORT_5B:

		if (ctx->num_data_lanes != 1) {
			dev_info(ctx->dev, "[error][%s] invalid ctx->num_data_lanes: %d\n",
				__func__, ctx->num_data_lanes);
			break;
		}

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_NUM,
						(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 0x4: 0x6) : 0x6);

		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);

		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
						RG_AFIFO_DUMMY_VALID_NUM,
						(ctx->is_4d1c) ? ((ctx->num_data_lanes > 1) ? 0x4: 0x6) : 0x6);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseB, CDPHY_RX_ANA_SETTING_1,
							AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);

		SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0X1);

		SENINF_BITS(cphy_base, CPHY_VALID_PIPE_EN, CPHY_TRIO2_VALID_PIPE_EN, 1);
		SENINF_BITS(cphy_base, CPHY_RX_CTRL, CPHY_RX_TR2_LPRX_EN, 1);

		break;
	default:
		dev_info(ctx->dev, "[error][%s] invalid ctx->port: %d\n",
				__func__, ctx->port);
		break;
	}

#ifdef CDPHY_ULPS_MODE_SUPPORT
	csirx_cphy_ulps_setting(ctx);
#endif /* CDPHY_ULPS_MODE_SUPPORT */

	if (!ctx->csi_param.legacy_phy)
		SENINF_WRITE_REG(dphy_base, DPHY_RX_SPARE0, 0xf1);
	else
		SENINF_WRITE_REG(dphy_base, DPHY_RX_SPARE0, 0xf0);

	return 0;
}

static int csirx_phy_setting(struct seninf_ctx *ctx)
{
	/* phyA */
	csirx_phyA_setting(ctx);

	if (!ctx->is_cphy)
		csirx_dphy_setting(ctx);
	else
		csirx_cphy_setting(ctx);

	return 0;
}

static int mtk_cam_seninf_set_csi_mipi(struct seninf_ctx *ctx)
{
	int ret = 0;

	csirx_phy_init(ctx);

	/* csi_mac_top */
	ret = csirx_mac_top_setting(ctx);
	if (ret) {
		dev_info(ctx->dev, "[%s][Error] ret(%d)\n", __func__, ret);
		return ret;
	}

	/* csi_mac_CSI2 */
	csirx_mac_csi_fixed_setting(ctx);
	csirx_mac_csi_setting(ctx);
	csirx_mac_csi_lrte_setting(ctx);

	/* seninf1 */
	seninf1_setting(ctx);

	/* phy */
	csirx_phy_setting(ctx);

	return 0;
}

static int mtk_cam_seninf_poweroff(struct seninf_ctx *ctx)
{
	void *pSeninf_csi2;

	pSeninf_csi2 = ctx->reg_if_csi2[(unsigned int)ctx->seninfIdx];

	SENINF_WRITE_REG(pSeninf_csi2, SENINF_CSI2_EN, 0x0);

	if (ctx->is_4d1c) {
		csirx_phyA_power_on(ctx, ctx->portA, 0);
		csirx_phyA_power_on(ctx, ctx->portB, 0);
	} else {
		csirx_phyA_power_on(ctx, ctx->port, 0);
	}

	return 0;
}

static int mtk_cam_seninf_reset(struct seninf_ctx *ctx, int seninfIdx)
{
	int i;
	void *pSeninf_mux;
	void *pSeninf = ctx->reg_if_ctrl[(unsigned int)seninfIdx];
	void *csirx_mac_top = ctx->reg_csirx_mac_top[(unsigned int)ctx->port];
	//void *mipi_csi_top_ctrl_0 = ctx->reg_mipi_csi_top_ctrl[0];
	//void *mipi_csi_top_ctrl_1 = ctx->reg_mipi_csi_top_ctrl[1];

	/* Reset seninf */
	SENINF_BITS(pSeninf, SENINF_CSI2_CTRL, SENINF_CSI2_SW_RST, 1);
	udelay(1);
	SENINF_BITS(pSeninf, SENINF_CSI2_CTRL, SENINF_CSI2_SW_RST, 0);

	/* Reset csi */
	/* SENINF_TOP_SW_RST : reset most of the CSIRX functional blocks*/
	/* CSR_TOP_SW_RESET  : reset entire csirx_mac_top */
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL, SENINF_TOP_SW_RST, 1);
	//SENINF_BITS(mipi_csi_top_ctrl_0, CSI_CSR_TOP_SW_RESET_B, CSR_TOP_SW_RESET, 0xffffffff);//porting for rayas, after check can remove, rayas mac page26
	//SENINF_BITS(mipi_csi_top_ctrl_1, CSI_CSR_TOP_SW_RESET_B, CSR_TOP_SW_RESET, 0xffffffff);//porting for rayas, after check can remove, rayas mac page26
	udelay(1);
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL, SENINF_TOP_SW_RST, 0);
	//SENINF_BITS(mipi_csi_top_ctrl_0, CSI_CSR_TOP_SW_RESET_B, CSR_TOP_SW_RESET, 0);//porting for rayas, after check can remove, rayas mac page26
	//SENINF_BITS(mipi_csi_top_ctrl_1, CSI_CSR_TOP_SW_RESET_B, CSR_TOP_SW_RESET, 0);//porting for rayas, after check can remove, rayas mac page26

	//dev_info(ctx->dev, "reset seninf %d\n", seninfIdx);

	for (i = SENINF_MUX1; i < _seninf_ops->mux_num; i++)
		if (mtk_cam_seninf_get_top_mux_ctrl(ctx, i) == seninfIdx &&
		    mtk_cam_seninf_is_mux_used(ctx, i)) {
			pSeninf_mux = ctx->reg_if_mux[i];
			SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_0,
				    SENINF_MUX_SW_RST, 1);
			udelay(1);
			SENINF_BITS(pSeninf_mux, SENINF_MUX_CTRL_0,
				    SENINF_MUX_SW_RST, 0);
			dev_info(ctx->dev, "reset mux %d\n", i);
		}

	return 0;
}

static int mtk_cam_seninf_set_idle(struct seninf_ctx *ctx)
{
	int i, j;
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	struct seninf_vc *vc;

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		if (vc->enable) {
			for (j = 0; j < vc->dest_cnt; j++) {
				mtk_cam_seninf_disable_mux(ctx, vc->dest[j].mux);
				mtk_cam_seninf_disable_cammux(ctx, vc->dest[j].cam);
			}
			vc->dest_cnt = 0;
		}
	}
	for (i = 0; i < PAD_MAXCNT; i++)
		for (j = 0; j < MAX_DEST_NUM; j++)
			ctx->pad2cam[i][j] = 0xff;

	seninf_logd(ctx, "release all mux & cam mux set all pd2cam to 0xff\n");

	return 0;
}

static int mtk_cam_seninf_get_mux_meter(struct seninf_ctx *ctx, int mux,
				 struct mtk_cam_seninf_mux_meter *meter)
{
	void *pSeninf_mux;
	s64 hv, hb, vv, vb, w, h, mipi_pixel_rate;
	s64 vb_in_us, hb_in_us, line_time_in_us;
	u32 res;

	pSeninf_mux = ctx->reg_if_mux[(unsigned int)mux];

	SENINF_BITS(pSeninf_mux, SENINF_MUX_FRAME_SIZE_MON_CTRL,
		    RG_SENINF_MUX_FRAME_SIZE_MON_EN, 1);

	hv = SENINF_READ_REG(pSeninf_mux,
			     SENINF_MUX_FRAME_SIZE_MON_H_VALID);
	hb = SENINF_READ_REG(pSeninf_mux,
			     SENINF_MUX_FRAME_SIZE_MON_H_BLANK);
	vv = SENINF_READ_REG(pSeninf_mux,
			     SENINF_MUX_FRAME_SIZE_MON_V_VALID);
	vb = SENINF_READ_REG(pSeninf_mux,
			     SENINF_MUX_FRAME_SIZE_MON_V_BLANK);
	res = SENINF_READ_REG(pSeninf_mux,
			      SENINF_MUX_SIZE);

	w = res & 0xffff;
	h = res >> 16;

	if (ctx->fps_n && ctx->fps_d) {
		mipi_pixel_rate = w * ctx->fps_n * (vv + vb);
		do_div(mipi_pixel_rate, ctx->fps_d);
		do_div(mipi_pixel_rate, hv);

		vb_in_us = vb * ctx->fps_d * 1000000;
		do_div(vb_in_us, vv + vb);
		do_div(vb_in_us, ctx->fps_n);

		hb_in_us = hb * ctx->fps_d * 1000000;
		do_div(hb_in_us, vv + vb);
		do_div(hb_in_us, ctx->fps_n);

		line_time_in_us = (hv + hb) * ctx->fps_d * 1000000;
		do_div(line_time_in_us, vv + vb);
		do_div(line_time_in_us, ctx->fps_n);
	} else {
		mipi_pixel_rate = -1;
		vb_in_us = -1;
		hb_in_us = -1;
		line_time_in_us = -1;
	}

	meter->width = w;
	meter->height = h;

	meter->h_valid = hv;
	meter->h_blank = hb;
	meter->v_valid = vv;
	meter->v_blank = vb;

	meter->mipi_pixel_rate = mipi_pixel_rate;
	meter->vb_in_us = vb_in_us;
	meter->hb_in_us = hb_in_us;
	meter->line_time_in_us = line_time_in_us;

	return 0;
}

static int mtk_cam_seninf_debug_core_dump(struct seninf_ctx *ctx,
				   struct mtk_cam_seninf_debug *debug_result)
{
	int i, j, dbg_idx, dbg_timeout = 0;
	int val = 0;
	struct seninf_vc *vc;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct seninf_vc_out_dest *dest;
	struct mtk_cam_seninf_mux_meter *meter;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	void *seninf, *csi_mac, *rx, *pmux, *pcammux, *base_ana;
	unsigned long debug_ft = FT_30_FPS;
	const unsigned int ft_delay_margin = 3;

	csi_mac = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfIdx];
	rx = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	base_ana = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];

	ctrl =
		v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_MTK_SOF_TIMEOUT_VALUE);
	if (ctrl) {
		val = v4l2_ctrl_g_ctrl(ctrl);
		if (val > 0)
			dbg_timeout = val + (val / 10);
	}

	if (dbg_timeout != 0)
		debug_ft = dbg_timeout / 1000;

	debug_ft += ft_delay_margin;

	mdelay(debug_ft);

	debug_result->csi_irq_status =
		SENINF_READ_REG(seninf, SENINF_CSI2_IRQ_STATUS);

	debug_result->csi_mac_irq_status =
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);

	dev_info(ctx->dev,
		"csirx_mac_csi irq_stat 0x%08x, seninf irq_stat 0x%08x\n",
		debug_result->csi_mac_irq_status,
		debug_result->csi_irq_status);

	/* wtire clear for enxt frame */
	SENINF_WRITE_REG(seninf, SENINF_CSI2_IRQ_STATUS, 0xffffffff);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);

	debug_result->packet_cnt_status =
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_PACKET_CNT_STATUS);

	dev_info(ctx->dev,
		"csi2 packet_cnt_status 0x%08x\n",
		debug_result->packet_cnt_status);

	dev_info(ctx->dev,
		"CSIRX_MAC_CSI2_SIZE_CHK_CTRL0/_CTRL1/_CTRL2/_CTRL3/_CTRL4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL0),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL1),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL2),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL3),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL4));
	dev_info(ctx->dev,
		"CSIRX_MAC_CSI2_SIZE_CHK_RCV0/_RCV1/_RCV2/_RCV3/_RCV4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3),
		SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4));
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0, 0xFFFFFFFF);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1, 0xFFFFFFFF);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2, 0xFFFFFFFF);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3, 0xFFFFFFFF);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4, 0xFFFFFFFF);

	for (i = 0; i < ctx->vcinfo.cnt; i++) {
		vc = &ctx->vcinfo.vc[i];

		for (j = 0; j < vc->dest_cnt; j++) {
			dbg_idx = i * vc->dest_cnt + j;

			if (dbg_idx >= MAX_MUX_VCINFO_DEBUG) {
				dev_info(ctx->dev,
				"[%s] vcinfo.cnt is exceed debug limit (%u)\n",
				__func__, MAX_MUX_VCINFO_DEBUG);
				return -1;
			}

			debug_result->mux_result_cnt = dbg_idx;
			vcinfo_debug = &debug_result->vcinfo_debug[dbg_idx];
			dest = &vc->dest[j];
			pmux = ctx->reg_if_mux[dest->mux];
			if (dest->cam < _seninf_ops->cam_mux_num)
				pcammux = ctx->reg_if_cam_mux_pcsr[dest->cam];
			else
				pcammux = NULL;

			vcinfo_debug->vc_feature = vc->feature;
			vcinfo_debug->vc = vc->vc;
			vcinfo_debug->dt = vc->dt;
			vcinfo_debug->seninf_mux = dest->mux;
			vcinfo_debug->cam_mux = dest->cam;
			vcinfo_debug->seninf_mux_en =
				mtk_cam_seninf_is_mux_used(ctx, dest->mux);
			vcinfo_debug->seninf_mux_src =
				mtk_cam_seninf_get_top_mux_ctrl(ctx, dest->mux);
			vcinfo_debug->seninf_mux_irq =
				SENINF_READ_REG(pmux, SENINF_MUX_IRQ_STATUS);

			SENINF_WRITE_REG(pmux,
					SENINF_MUX_IRQ_STATUS, 0xFFFFFFFF);

			dev_dbg(ctx->dev,
					"[%d] vc_feature %d vc 0x%x dt 0x%x mux %d cam %d\n",
					i,
					vcinfo_debug->vc_feature,
					vcinfo_debug->vc,
					vcinfo_debug->dt,
					vcinfo_debug->seninf_mux,
					vcinfo_debug->cam_mux);
			dev_dbg(ctx->dev,
				     "\tmux[%d] en %d src %d irq_stat 0x%x\n",
					vcinfo_debug->seninf_mux,
					vcinfo_debug->seninf_mux_en,
					vcinfo_debug->seninf_mux_src,
					vcinfo_debug->seninf_mux_irq);

			if (pcammux) {
				vcinfo_debug->cam_mux_en =
					_seninf_ops->_is_cammux_used(ctx, dest->cam);
				vcinfo_debug->cam_mux_src =
					_seninf_ops->_get_cammux_ctrl(ctx, dest->cam);
				vcinfo_debug->exp_size =
					mtk_cam_seninf_get_cammux_exp(ctx, dest->cam);
				vcinfo_debug->rec_size =
					mtk_cam_seninf_get_cammux_res(ctx, dest->cam);
				vcinfo_debug->frame_mointor_err =
					mtk_cam_seninf_get_cammux_err(ctx, dest->cam);
				vcinfo_debug->cam_mux_irq =
					SENINF_READ_REG(pcammux, SENINF_CAM_MUX_PCSR_IRQ_STATUS);
				dev_dbg(ctx->dev, "cam[%d] irq 0x%x\n",
					dest->cam,
					SENINF_READ_REG(pcammux, SENINF_CAM_MUX_PCSR_IRQ_STATUS));
				/*  write clear  */
				SENINF_WRITE_REG(pcammux,
					SENINF_CAM_MUX_PCSR_IRQ_STATUS, 0xFFFFFFFF);
			}

			if (vc->feature == VC_RAW_DATA ||
				vc->feature == VC_STAGGER_NE ||
				vc->feature == VC_STAGGER_ME ||
				vc->feature == VC_STAGGER_SE) {
				meter = &debug_result->meter[i];
				mtk_cam_seninf_get_mux_meter(ctx,
							     dest->mux, meter);

				dev_dbg(ctx->dev, "\t--- mux meter ---\n");
				dev_dbg(ctx->dev, "\twidth %d height %d\n",
				     meter->width, meter->height);
				dev_dbg(ctx->dev, "\th_valid %d, h_blank %d\n",
				     meter->h_valid, meter->h_blank);
				dev_dbg(ctx->dev, "\tv_valid %d, v_blank %d\n",
				     meter->v_valid, meter->v_blank);
				dev_dbg(ctx->dev, "\tmipi_pixel_rate %lld\n",
				     meter->mipi_pixel_rate);
				dev_dbg(ctx->dev, "\tv_blank %lld us\n",
				     meter->vb_in_us);
				dev_dbg(ctx->dev, "\th_blank %lld us\n",
				     meter->hb_in_us);
				dev_dbg(ctx->dev, "\tline_time %lld us\n",
				     meter->line_time_in_us);
			}
		}
	}
	return 1;
}

static int mtk_cam_dbg_fmeter(struct seninf_core *core, char *out_str, size_t out_str_sz)
{
	int i, ret;
	unsigned int result;
	int ofs = 0;
	const char * const clk_fmeter_names[] = {
		CLK_FMETER_NAMES
	};

	if (!core || !out_str)
		return -EINVAL;

	memset(out_str, 0, out_str_sz);

	for (i = 0; i < CLK_FMETER_MAX && i < ARRAY_SIZE(clk_fmeter_names); i++) {
		if (seninf_get_fmeter_clk(core, i, &result) < 0)
			continue;

		ret = snprintf(out_str + ofs, out_str_sz - 1,
			"%s(%u) ", clk_fmeter_names[i], result);

		if (ret < 0)
			return -EINVAL;

		ofs += ret;
	}

	return (ofs > 0) ? 0 : -EPERM;
}

static ssize_t mtk_cam_seninf_show_status(struct device *dev,
				   struct device_attribute *attr,
		char *buf)
{
	int i, j, len, dbg_idx;
	struct seninf_core *core;
	struct seninf_ctx *ctx;
	struct seninf_vc *vc;
	struct seninf_vc_out_dest *dest;
	struct media_link *link;
	struct media_pad *pad;
	struct mtk_cam_seninf_mux_meter meter;
	struct mtk_cam_seninf_debug debug_result;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	void *pmux, *pcammux, *rx, *base_ana, *csi_mac, *base_seninf;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);

	core = dev_get_drvdata(dev);
	len = 0;

	memset(&debug_result, 0, sizeof(struct mtk_cam_seninf_debug));
	mutex_lock(&core->mutex);

	if (fmeter_dbg && mtk_cam_dbg_fmeter(core, fmeter_dbg, sizeof(char) * 256) == 0)
		SHOW(buf, len, "\n%s\n", fmeter_dbg);
	kfree(fmeter_dbg);

	list_for_each_entry(ctx, &core->list, list) {
		SHOW(buf, len,
			"\n[%s] port %d intf %d test %d cphy %d lanes %d\n",
			ctx->subdev.name,
			ctx->port,
			ctx->seninfIdx,
			ctx->is_test_model,
			ctx->is_cphy,
			ctx->num_data_lanes);

		pad = &ctx->pads[PAD_SINK];
		list_for_each_entry(link, &pad->entity->links, list) {
			if (link->sink == pad) {
				SHOW(buf, len, "source %s flags 0x%lx\n",
				     link->source->entity->name,
					link->flags);
			}
		}

		if (!ctx->streaming)
			continue;

		mtk_cam_seninf_debug_core_dump(ctx, &debug_result);

		base_seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfIdx];
		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		rx = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
		base_ana = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];
		SHOW(buf, len,
			"csirx_mac_csi irq_stat 0x%08x, seninf irq_stat 0x%08x\n",
		     debug_result.csi_mac_irq_status,
			 debug_result.csi_irq_status);
		SHOW(buf, len, "csi2 line_frame_num 0x%08x\n",
		     SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_LINE_FRAME_NUM));
		SHOW(buf, len, "csi2 packet_status 0x%08x\n",
		     SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_PACKET_STATUS));
		SHOW(buf, len, "csi2 packet_cnt_status 0x%08x\n",
		     debug_result.packet_cnt_status);

		SHOW(buf, len, "rx-ana settle ck 0x%02x dt 0x%02x\n",
		     SENINF_READ_BITS(rx, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
				      RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER),
		     SENINF_READ_BITS(rx, DPHY_RX_DATA_LANE0_HS_PARAMETER,
				      RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER));
		SHOW(buf, len, "rx-ana trail en %u param 0x%02x\n",
		     SENINF_READ_BITS(rx, DPHY_RX_DATA_LANE0_HS_PARAMETER,
				      RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN),
		     SENINF_READ_BITS(rx, DPHY_RX_DATA_LANE0_HS_PARAMETER,
				      RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER));
		SHOW(buf, len,
			"CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL 0x%08x CDPHY_RX_ANA_SETTING_1 0x%08x DPHY_RX_SPARE0 0x%08x\n",
			SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_SETTING_1),
			SENINF_READ_REG(rx, DPHY_RX_SPARE0));

		SHOW(buf, len, "data_not_enough_cnt : <%d>\n",
			ctx->data_not_enough_cnt);
		SHOW(buf, len, "err_lane_resync_cnt : <%d>\n",
			ctx->err_lane_resync_cnt);
		SHOW(buf, len, "crc_err_cnt : <%d>\n",
			ctx->crc_err_flag);
		SHOW(buf, len, "ecc_err_double_cnt : <%d>\n",
			ctx->ecc_err_double_cnt);
		SHOW(buf, len, "ecc_err_corrected_cnt : <%d>\n",
			ctx->ecc_err_corrected_cnt);

		if ((SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS) & ~(0x324)) ||
			(SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS)&
			~(0x10000000))) {

			SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
			SENINF_WRITE_REG(base_seninf, SENINF_CSI2_IRQ_STATUS, 0xffffffff);
			SHOW(buf, len,
			     "after write clear csi irq 0x%x seninf irq_stat 0x%08x\n",
			     SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS),
				 SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS));
		}

		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];

			for (j = 0; j < vc->dest_cnt; j++) {
				dbg_idx = i * vc->dest_cnt + j;
				if (dbg_idx >= MAX_MUX_VCINFO_DEBUG) {
					dev_info(ctx->dev,
					"[%s] vcinfo.cnt is exceed debug limit (%u)\n",
					__func__, MAX_MUX_VCINFO_DEBUG);
					return -1;
				}


				dest = &vc->dest[j];
				pmux = ctx->reg_if_mux[dest->mux];
				vcinfo_debug = &debug_result.vcinfo_debug[dbg_idx];
				meter = debug_result.meter[j];

				if (dest->cam < _seninf_ops->cam_mux_num)
					pcammux = ctx->reg_if_cam_mux_pcsr[dest->cam];
				else
					pcammux = NULL;
				SHOW(buf, len, "[%d] vc 0x%x dt 0x%x mux %d cam %d\n",
					i,
					vcinfo_debug->vc,
					vcinfo_debug->dt,
					vcinfo_debug->seninf_mux,
					vcinfo_debug->cam_mux);
				SHOW(buf, len,
				     "\tmux[%d] en %d src %d irq_stat 0x%x\n",
					vcinfo_debug->seninf_mux,
					vcinfo_debug->seninf_mux_en,
					vcinfo_debug->seninf_mux_src,
					vcinfo_debug->seninf_mux_irq);
				SHOW(buf, len, "\t\tfifo_overrun_cnt : <%d>\n",
					ctx->fifo_overrun_cnt);
				if (pcammux) {
					SHOW(buf, len,
						"\tcam[%d] en %d src %d exp 0x%x res 0x%x err 0x%x irq_stat 0x%x\n",
						vcinfo_debug->cam_mux,
						vcinfo_debug->cam_mux_en,
						vcinfo_debug->cam_mux_src,
						vcinfo_debug->exp_size,
						vcinfo_debug->rec_size,
						vcinfo_debug->frame_mointor_err,
						vcinfo_debug->cam_mux_irq);
					SENINF_WRITE_REG(pcammux,
						SENINF_CAM_MUX_PCSR_IRQ_STATUS, 0x103);
				}
				SHOW(buf, len, "\t\tsize_err_cnt : <%d>\n",
					ctx->size_err_cnt);

				if (vc->feature == VC_RAW_DATA ||
					vc->feature == VC_STAGGER_NE ||
					vc->feature == VC_STAGGER_ME ||
					vc->feature == VC_STAGGER_SE) {
					mtk_cam_seninf_get_mux_meter(ctx,
								     dest->mux, &meter);
					SHOW(buf, len, "\t--- mux meter ---\n");
					SHOW(buf, len, "\twidth %d height %d\n",
					     meter.width, meter.height);
					SHOW(buf, len, "\th_valid %d, h_blank %d\n",
					     meter.h_valid, meter.h_blank);
					SHOW(buf, len, "\tv_valid %d, v_blank %d\n",
					     meter.v_valid, meter.v_blank);
					SHOW(buf, len, "\tmipi_pixel_rate %lld\n",
					     meter.mipi_pixel_rate);
					SHOW(buf, len, "\tv_blank %lld us\n",
					     meter.vb_in_us);
					SHOW(buf, len, "\th_blank %lld us\n",
					     meter.hb_in_us);
					SHOW(buf, len, "\tline_time %lld us\n",
					     meter.line_time_in_us);
				}
			}
		}
	}

	mutex_unlock(&core->mutex);

	return len;
}

#ifdef SCAN_SETTLE
#define SCAN_TIME 3
#define SENINF_DRV_DEBUG_MAX_DELAY 1200
#else
#define SCAN_TIME 1
#define SENINF_DRV_DEBUG_MAX_DELAY 400
#endif


#define PKT_CNT_CHK_MARGIN 110

#define MAX_DELAY_STEP 100

/**
 * Delay with stream off check
 *
 * @param ctx seninf_ctx
 * @param delay Total delay
 *
 * @return {@code false} if it has been streamed off and delay partically, otherwise (@code true}
 */
static bool delay_with_stream_check(struct seninf_ctx *ctx, unsigned long delay)
{
	unsigned long delay_step, delay_inc;

	delay_inc = 0;
	delay_step = 1;
	do {
		if (!ctx->streaming)
			break;

		delay_step = min((unsigned long)MAX_DELAY_STEP, delay - delay_inc);
		mdelay(delay_step);
		delay_inc += delay_step;
	} while (delay > delay_inc);

	if (delay > delay_inc) {
		dev_info(ctx->dev, "delay = %lu, inc = %lu, seninf streamed-off\n",
			 delay, delay_inc);
		return false;
	}

	return true;
}

static int mtk_cam_seninf_debug(struct seninf_ctx *ctx)
{
	void *base_ana, *base_cphy, *base_dphy, *base_seninf, *base_csi_mac, *base_mux;
	unsigned int mac_irq = 0;
	unsigned int seninf_irq = 0;
	int pkg_cnt_changed = 0;
	unsigned int mipi_packet_cnt = 0;
	unsigned int tmp_mipi_packet_cnt = 0;
	unsigned long total_delay = 0;
	unsigned long max_delay = 0;
	unsigned long long enabled = 0;
	int ret = 0;
	int j, i, k;
	unsigned long debug_ft = FT_30_FPS * SCAN_TIME;	// FIXME
	unsigned long debug_vb = 1;	// 1ms for min readout time
	enum CSI_PORT csi_port = CSI_PORT_0;
	unsigned int tag_03_vc, tag_03_dt, tag_47_vc, tag_47_dt;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);
	unsigned int frame_cnt1 = 0, frame_cnt2 = 0;
	unsigned int dphy_irq = 0;
	unsigned int cphy_irq = 0;
	unsigned int temp = 0;

	mtk_cam_sensor_get_frame_cnt(ctx, &frame_cnt1);

	if (ctx->dbg_timeout != 0)
		debug_ft = ctx->dbg_timeout / 1000;

	if (fmeter_dbg && mtk_cam_dbg_fmeter(ctx->core, fmeter_dbg, sizeof(char) * 256) == 0)
		dev_info(ctx->dev, "%s\n", fmeter_dbg);
	kfree(fmeter_dbg);

	for (csi_port = CSI_PORT_0A; csi_port <= CSI_PORT_5B; csi_port++) {
		if (csi_port != ctx->portA &&
			csi_port != ctx->portB)
			continue;

		base_ana = ctx->reg_ana_csi_rx[csi_port];

		dev_info(ctx->dev,
			"MipiRx_ANA%d:CDPHY_RX_ANA_SETTING_1:(0x%08x),CDPHY_RX_ANA_0/_1/_2/_3/_4/_5/_6/_7/_8:(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_SETTING_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_2),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_3),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_4),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_5),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_6),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_7),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_8));
		dev_info(ctx->dev,
			"MipiRx_ANA%d:CDPHY_RX_ANA_AD_0/_1:(0x%x)/(0x%x),AD_HS_0/_1/_2:(0x%x)/(0x%x)/(0x%x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_2));
	}

	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);

		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:LANE_EN/_SELECT:(0x%x)/(0x%x),CLK_LANE0_HS/1_HS:(0x%x)/(0x%x),DATA_LANE0_HS/1_HS/2_HS/3_HS:(0x%x)/(0x%x)/(0x%x)/(0x%x),DPHY_RX_SPARE0:(0x%x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_SELECT),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_CLOCK_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_CLOCK_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE2_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE3_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_SPARE0));
		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_CTRL/_TIMING_CTRL/_LANE0_CTRL/_LANE1_CTRL/_LANE2_CTRL/_LANE3_CTRL:(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_TIMING_CTRL),
				SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE0_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE1_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE2_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE3_CTRL));
		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_IRQ_EN/_CLR/_STATUS:(0x%08x)/(0x%08x)/(0x%08x),DPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_CLR),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_STATUS),
			SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_EN),
			dphy_irq);
		dev_info(ctx->dev,
			"Csi%d_Cphy_Top:CPHY_RX_CTRL:(0x%x),CPHY_RX_DETECT_CTRL_POST:(0x%x),CPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_cphy, CPHY_RX_CTRL),
			SENINF_READ_REG(base_cphy, CPHY_RX_DETECT_CTRL_POST),
			SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_EN),
			cphy_irq);
		/* WRITE CLEAR C/DPHY_IRQ_STATUS */
		SENINF_WRITE_REG(base_dphy, DPHY_RX_IRQ_CLR, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_cphy, CPHY_RX_IRQ_CLR, 0xFF0000);
	}

	dev_info(ctx->dev,
		"TOP_CTRL2:(0x%x),TOP_MUX_CTRL_0/_1/_2/_3/_4/_5:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x),debug_vb:%lu,debug_ft:%lu\n",
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_CTRL2),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_0),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_1),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_2),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_3),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_4),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_5),
		debug_vb, debug_ft);

	enabled = ((unsigned long long)SENINF_READ_REG(ctx->reg_if_cam_mux_gcsr,
						SENINF_CAM_MUX_GCSR_MUX_EN_H) << 32) |
		SENINF_READ_REG(ctx->reg_if_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_MUX_EN);
	/* clear cam mux irq */
	for (j = 0; j < ctx->vcinfo.cnt; j++) {
		if (ctx->vcinfo.vc[j].enable) {
			for (k = 0; k < ctx->vcinfo.vc[j].dest_cnt; k++) {
				unsigned int used_cammux = ctx->vcinfo.vc[j].dest[k].cam;

				for (i = 0; i < _seninf_ops->cam_mux_num; i++) {
					if ((used_cammux == i) && ((enabled >> i) & 1)) {
						u32 res, irq_st;

						res = SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_CHK_RES);
						irq_st = SENINF_READ_REG(
								ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_IRQ_STATUS);
						SENINF_WRITE_REG(ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_IRQ_STATUS, 0x103);
						dev_info(ctx->dev,
							"before clear cam mux%u,recSize = 0x%x,irq = 0x%x|0x%x",
							i, res, irq_st,
							SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_IRQ_STATUS));
					}
				}
			}
		}
	}

	/* Seninf_csi status IRQ */
	base_seninf = ctx->reg_if_csi2[(uint32_t)ctx->seninfIdx];
	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	mac_irq = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	seninf_irq = SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS);
	temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_MAC_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);
	if ((mac_irq & ~(0x324)) || (seninf_irq & ~(0x10000000))) {
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
		SENINF_WRITE_REG(base_seninf, SENINF_CSI2_IRQ_STATUS, 0xffffffff);
	}
	dev_info(ctx->dev,
		"SENINF%d_CSI2_PRBS_EN/_OPT:(0x%x)/(0x%x),CSIRX_MAC_CSI2_EN/_OPT/_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x)/(0x%x)/(0x%x),SENINF_CSI2_IRQ_STATUS:(0x%x),CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL:(0x%x)\n",
		(uint32_t)ctx->seninfIdx,
		SENINF_READ_REG(base_seninf, SENINF_CSI2_EN),
		SENINF_READ_REG(base_seninf, SENINF_CSI2_OPT),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_OPT),
		mac_irq,
		temp,
		seninf_irq,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL));

	if (_seninf_ops->iomem_ver == NULL) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6878_IOMOM_VERSIONS)) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6989_IOMOM_VERSIONS)) {
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_CTRL0/_CTRL1/_CTRL2/_CTRL3/_CTRL4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL0),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL1),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL2),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL3),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL4));
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_RCV0/_RCV1/_RCV2/_RCV3/_RCV4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4));
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4, 0xFFFFFFFF);
	} else {
		dev_info(ctx->dev, "iomem_ver is invalid\n");
		return -EINVAL;
	}

	/* Seninf_csi packet count */
	pkg_cnt_changed = 0;
	if (SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN) & 0x1) {
		SENINF_BITS(base_csi_mac, CSIRX_MAC_CSI2_DBG_CTRL,
			    RG_CSI2_DBG_PACKET_CNT_EN, 1);
		mipi_packet_cnt = SENINF_READ_REG(base_csi_mac,
					CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
		max_delay = debug_ft * PKT_CNT_CHK_MARGIN / 100;
		dev_info(ctx->dev,
			"total_delay:%lums/%lums,SENINF%d_PkCnt:(0x%x),ret=%d\n",
			total_delay, max_delay, ctx->seninfIdx, mipi_packet_cnt, ret);

		while (total_delay < max_delay) {
			tmp_mipi_packet_cnt = mipi_packet_cnt & 0xFFFF;
			if (!delay_with_stream_check(ctx, debug_vb))
				return ret; // has been stream off
			total_delay += debug_vb;
			mipi_packet_cnt = SENINF_READ_REG(base_csi_mac,
						CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
			if (tmp_mipi_packet_cnt != (mipi_packet_cnt & 0xFFFF)) {
				dev_info(ctx->dev,
					"total_delay:%lums/%lums,SENINF%d_PkCnt:(0x%x),ret=%d\n",
					total_delay, max_delay, ctx->seninfIdx, mipi_packet_cnt, ret);
				pkg_cnt_changed = 1;
				break;
			}
		}
	}
	if (!pkg_cnt_changed) {
		ret = -1;
		dev_info(ctx->dev,
			"total_delay:%lums/%lums,SENINF%d_PkCnt:(0x%x),ret=%d\n",
			total_delay, max_delay, ctx->seninfIdx, mipi_packet_cnt, ret);
	}

	/* Check csi status again */
	if (debug_ft > total_delay) {
		if (!delay_with_stream_check(ctx, debug_ft - total_delay))
			return ret; // has been stream off
	}

	mac_irq = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	seninf_irq = SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS);
	temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_MAC_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);
	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);
	}

	seninf_logi(ctx,
		"CSI_MAC%d_CSI2_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x),SENINF_CSI2_IRQ_STATUS:(0x%x),C/DPHY_RX_IRQ_STATUS:(0x%x)/(0x%x)\n",
		ctx->seninfIdx, mac_irq, temp, seninf_irq, cphy_irq, dphy_irq);
	if ((mac_irq & 0xD0) || (seninf_irq & 0x10000000))
		ret = -2; //multi lanes sync error, crc error, ecc error

	if (_seninf_ops->iomem_ver == NULL) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6878_IOMOM_VERSIONS)) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6989_IOMOM_VERSIONS)) {
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_CTRL0/_CTRL1/_CTRL2/_CTRL3/_CTRL4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL0),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL1),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL2),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL3),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL4));
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_RCV0/_RCV1/_RCV2/_RCV3/_RCV4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3),
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4));
	} else {
		dev_info(ctx->dev, "iomem_ver is invalid\n");
		return -EINVAL;
	}

	/* SENINF_MUX */
	for (j = SENINF_MUX1; j < _seninf_ops->mux_num; j++) {
		base_mux = ctx->reg_if_mux[j];
		if (SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_0) & 0x1) {
			dev_info(ctx->dev,
				"%sSENINF%d_MUX_CTRL0/_CTRL1/_IRQ_STATUS/_MUX_SIZE/_ERR_SIZE/_EXP_SIZE:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
				(mtk_cam_seninf_get_top_mux_ctrl(ctx, j) == ctx->seninfIdx) ?
				"*" : "",
				j,
				SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_0),
				SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_1),
				SENINF_READ_REG(base_mux, SENINF_MUX_IRQ_STATUS),
				SENINF_READ_REG(base_mux, SENINF_MUX_SIZE),
				SENINF_READ_REG(base_mux, SENINF_MUX_ERR_SIZE),
				SENINF_READ_REG(base_mux, SENINF_MUX_IMG_SIZE));

			if (SENINF_READ_REG(base_mux, SENINF_MUX_IRQ_STATUS) & 0x1) {
				SENINF_WRITE_REG(base_mux, SENINF_MUX_IRQ_STATUS,
						 0xffffffff);
				if (debug_ft > FT_30_FPS) {
					if (!delay_with_stream_check(ctx, FT_30_FPS))
						return ret; // has been stream off
				} else {
					if (!delay_with_stream_check(ctx, debug_ft))
						return ret; // has been stream off
				}
				dev_info(ctx->dev,
					"after reset overrun,SENINF%d_MUX_IRQ_STATUS/_MUX_SIZE:(0x%x)/(0x%x)\n",
					j,
					SENINF_READ_REG(base_mux,
							SENINF_MUX_IRQ_STATUS),
					SENINF_READ_REG(base_mux, SENINF_MUX_SIZE));
			}
		}
	}

	/* check SENINF_CAM_MUX size */
	for (j = 0; j < ctx->vcinfo.cnt; j++) {
		if (ctx->vcinfo.vc[j].enable) {
			for (k = 0; k < ctx->vcinfo.vc[j].dest_cnt; k++) {
				for (i = 0; i < _seninf_ops->cam_mux_num; i++) {
					unsigned int used_cammux = ctx->vcinfo.vc[j].dest[k].cam;

					if ((used_cammux == i) && ((enabled >> i) & 1)) {

						SENINF_BITS(ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_OPT,
							RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL,
							0);
						tag_03_vc = SENINF_READ_REG(
							ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
						tag_03_dt = SENINF_READ_REG(
							ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_TAG_DT_SEL);
						SENINF_BITS(ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_OPT,
							RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL,
							1);
						tag_47_vc = SENINF_READ_REG(
							ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
						tag_47_dt = SENINF_READ_REG(
							ctx->reg_if_cam_mux_pcsr[i],
							SENINF_CAM_MUX_PCSR_TAG_DT_SEL);

						dev_info(ctx->dev,
						"cam_mux_%d_CTRL/RES/EXP/ERR/OPT/IRQ:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x),tag03_vc/_dt(0x%x/0x%x),tag47_vc/_dt(0x%x/0x%x)\n",
						i,
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_CTRL),
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_CHK_RES),
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_CHK_CTL),
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_CHK_ERR_RES),
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_OPT),
						SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
								SENINF_CAM_MUX_PCSR_IRQ_STATUS),
						tag_03_vc, tag_03_dt, tag_47_vc, tag_47_dt);
					}
				}
			}
		}
	}
	mtk_cam_sensor_get_frame_cnt(ctx, &frame_cnt2);
	if (frame_cnt2 == frame_cnt1) {
		dev_info(ctx->dev,
			"frame cnt(%d) doesn't update, please check sensor status.\n", frame_cnt2);
	} else {
		dev_info(ctx->dev,
			"sensor is streaming, frame_cnt1: %d, frame_cnt2: %d\n", frame_cnt1, frame_cnt2);
	}
	dev_info(ctx->dev, "ret = %d\n", ret);

	/* dump debug cur status function in case of log dropping */
	if (ctx->debug_cur_sys_time_in_ns) {
		dev_info(ctx->dev,
		"[_debug_current_status] sys_time_ns:%llu,D/CPHY_RX_IRQ_STATUS:(0x%08x)/(0x%08x),CSIRX_MAC_CSI2_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x),SENINF_CSI2_IRQ_STATUS:(0x%x)\n",
		ctx->debug_cur_sys_time_in_ns,
		ctx->debug_cur_dphy_irq,
		ctx->debug_cur_cphy_irq,
		ctx->debug_cur_mac_irq,
		ctx->debug_cur_temp,
		ctx->debug_cur_seninf_irq);
		dev_info(ctx->dev,
		"[_debug_current_status] CSIRX_MAC_CSI2_SIZE_CHK_CTRL0/_CTRL1/_CTRL2/_CTRL3/_CTRL4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
		ctx->debug_cur_mac_csi2_size_chk_ctrl0,
		ctx->debug_cur_mac_csi2_size_chk_ctrl1,
		ctx->debug_cur_mac_csi2_size_chk_ctrl2,
		ctx->debug_cur_mac_csi2_size_chk_ctrl3,
		ctx->debug_cur_mac_csi2_size_chk_ctrl4);
		dev_info(ctx->dev,
		"[_debug_current_status] CSIRX_MAC_CSI2_SIZE_CHK_RCV0/_RCV1/_RCV2/_RCV3/_RCV4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
		ctx->debug_cur_mac_csi2_size_chk_rcv0,
		ctx->debug_cur_mac_csi2_size_chk_rcv1,
		ctx->debug_cur_mac_csi2_size_chk_rcv2,
		ctx->debug_cur_mac_csi2_size_chk_rcv3,
		ctx->debug_cur_mac_csi2_size_chk_rcv4);
	}

	return ret;
}

static int mtk_cam_seninf_debug_current_status(struct seninf_ctx *ctx)
{
	void *base_ana, *base_cphy, *base_dphy, *base_seninf, *base_csi_mac, *base_mux;
	unsigned long long enabled = 0;
	int ret = 0;
	int j, i;
	enum CSI_PORT csi_port = CSI_PORT_0;
	unsigned int tag_03_vc, tag_03_dt, tag_47_vc, tag_47_dt;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);

	ctx->debug_cur_sys_time_in_ns = ktime_get_boottime_ns();

	if (fmeter_dbg && mtk_cam_dbg_fmeter(ctx->core, fmeter_dbg, sizeof(char) * 256) == 0)
		dev_info(ctx->dev, "%s\n", fmeter_dbg);
	kfree(fmeter_dbg);

	for (csi_port = CSI_PORT_0A; csi_port <= CSI_PORT_5B; csi_port++) {
		if (csi_port != ctx->portA &&
			csi_port != ctx->portB)
			continue;

		base_ana = ctx->reg_ana_csi_rx[csi_port];

		dev_info(ctx->dev,
			"MipiRx_ANA%d:CDPHY_RX_ANA_SETTING_1:(0x%08x),CDPHY_RX_ANA_0/_1/_2/_3/_4/_5/_6/_7/_8:(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_SETTING_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_2),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_3),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_4),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_5),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_6),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_7),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_8));
		dev_info(ctx->dev,
			"MipiRx_ANA%d:CDPHY_RX_ANA_AD_0/_1:(0x%x)/(0x%x),AD_HS_0/_1/_2:(0x%x)/(0x%x)/(0x%x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_2));
	}

	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		ctx->debug_cur_cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		ctx->debug_cur_dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);

		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:LANE_EN/_SELECT:(0x%x)/(0x%x),CLK_LANE0_HS/1_HS:(0x%x)/(0x%x),DATA_LANE0_HS/1_HS/2_HS/3_HS:(0x%x)/(0x%x)/(0x%x)/(0x%x),DPHY_RX_SPARE0:(0x%x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_SELECT),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_CLOCK_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_CLOCK_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE2_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DATA_LANE3_HS_PARAMETER),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_SPARE0));
		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_CTRL/_TIMING_CTRL/_LANE0_CTRL/_LANE1_CTRL/_LANE2_CTRL/_LANE3_CTRL:(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_TIMING_CTRL),
				SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE0_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE1_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE2_CTRL),
			SENINF_READ_REG(base_dphy,
					DPHY_RX_DESKEW_LANE3_CTRL));
		dev_info(ctx->dev,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_IRQ_EN/_CLR/_STATUS:(0x%08x)/(0x%08x)/(0x%08x),DPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_CLR),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_STATUS),
			SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_EN),
			ctx->debug_cur_dphy_irq);
		dev_info(ctx->dev,
			"Csi%d_Cphy_Top:CPHY_RX_CTRL:(0x%x),CPHY_RX_DETECT_CTRL_POST:(0x%x),CPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_cphy, CPHY_RX_CTRL),
			SENINF_READ_REG(base_cphy, CPHY_RX_DETECT_CTRL_POST),
			SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_EN),
			ctx->debug_cur_cphy_irq);
		/* WRITE CLEAR C/DPHY_IRQ_STATUS */
		SENINF_WRITE_REG(base_dphy, DPHY_RX_IRQ_CLR, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_cphy, CPHY_RX_IRQ_CLR, 0xFF0000);
	}

	dev_info(ctx->dev,
		"TOP_CTRL2:(0x%x),TOP_MUX_CTRL_0/_1/_2/_3/_4/_5:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_CTRL2),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_0),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_1),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_2),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_3),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_4),
		SENINF_READ_REG(ctx->reg_if_top, SENINF_TOP_MUX_CTRL_5));

	enabled = ((unsigned long long)SENINF_READ_REG(ctx->reg_if_cam_mux_gcsr,
						SENINF_CAM_MUX_GCSR_MUX_EN_H) << 32) |
		SENINF_READ_REG(ctx->reg_if_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_MUX_EN);

	/* Seninf_csi status IRQ */
	base_seninf = ctx->reg_if_csi2[(uint32_t)ctx->seninfIdx];
	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	ctx->debug_cur_mac_irq = SENINF_READ_REG(base_csi_mac,
		CSIRX_MAC_CSI2_IRQ_STATUS);
	ctx->debug_cur_seninf_irq = SENINF_READ_REG(base_seninf,
		SENINF_CSI2_IRQ_STATUS);
	ctx->debug_cur_temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_MAC_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);

	if ((ctx->debug_cur_mac_irq & ~(0x324)) ||
		(ctx->debug_cur_seninf_irq & ~(0x10000000))) {
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_seninf, SENINF_CSI2_IRQ_STATUS, 0xFFFFFFFF);
	}

	dev_info(ctx->dev,
		"SENINF%d_CSI2_PRBS_EN/_OPT:(0x%x)/(0x%x),CSIRX_MAC_CSI2_EN/_OPT/_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x)/(0x%x)/(0x%x),SENINF_CSI2_IRQ_STATUS:(0x%x),CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL:(0x%x)\n",
		(uint32_t)ctx->seninfIdx,
		SENINF_READ_REG(base_seninf, SENINF_CSI2_EN),
		SENINF_READ_REG(base_seninf, SENINF_CSI2_OPT),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_OPT),
		ctx->debug_cur_mac_irq,
		ctx->debug_cur_temp,
		ctx->debug_cur_seninf_irq,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL));

	if (_seninf_ops->iomem_ver == NULL) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6878_IOMOM_VERSIONS)) {
		dev_dbg(ctx->dev, "no mac checker implementation\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6989_IOMOM_VERSIONS)) {
		ctx->debug_cur_mac_csi2_size_chk_ctrl0 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL0);
		ctx->debug_cur_mac_csi2_size_chk_ctrl1 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL1);
		ctx->debug_cur_mac_csi2_size_chk_ctrl2 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL2);
		ctx->debug_cur_mac_csi2_size_chk_ctrl3 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL3);
		ctx->debug_cur_mac_csi2_size_chk_ctrl4 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_CTRL4);
		ctx->debug_cur_mac_csi2_size_chk_rcv0 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV0);
		ctx->debug_cur_mac_csi2_size_chk_rcv1 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV1);
		ctx->debug_cur_mac_csi2_size_chk_rcv2 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV2);
		ctx->debug_cur_mac_csi2_size_chk_rcv3 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV3);
		ctx->debug_cur_mac_csi2_size_chk_rcv4 =
			SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_SIZE_CHK_RCV4);
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_CTRL0/_CTRL1/_CTRL2/_CTRL3/_CTRL4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			ctx->debug_cur_mac_csi2_size_chk_ctrl0,
			ctx->debug_cur_mac_csi2_size_chk_ctrl1,
			ctx->debug_cur_mac_csi2_size_chk_ctrl2,
			ctx->debug_cur_mac_csi2_size_chk_ctrl3,
			ctx->debug_cur_mac_csi2_size_chk_ctrl4);
		dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_SIZE_CHK_RCV0/_RCV1/_RCV2/_RCV3/_RCV4:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
			ctx->debug_cur_mac_csi2_size_chk_rcv0,
			ctx->debug_cur_mac_csi2_size_chk_rcv1,
			ctx->debug_cur_mac_csi2_size_chk_rcv2,
			ctx->debug_cur_mac_csi2_size_chk_rcv3,
			ctx->debug_cur_mac_csi2_size_chk_rcv4);
		SENINF_WRITE_REG(base_csi_mac,
			CSIRX_MAC_CSI2_SIZE_CHK_RCV0, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac,
			CSIRX_MAC_CSI2_SIZE_CHK_RCV1, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac,
			CSIRX_MAC_CSI2_SIZE_CHK_RCV2, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac,
			CSIRX_MAC_CSI2_SIZE_CHK_RCV3, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac,
			CSIRX_MAC_CSI2_SIZE_CHK_RCV4, 0xFFFFFFFF);
	} else {
		dev_info(ctx->dev, "iomem_ver is invalid\n");
		return -EINVAL;
	}

	if ((ctx->debug_cur_mac_irq & 0xD0) ||
		(ctx->debug_cur_seninf_irq & 0x10000000))
		ret = -2; //multi lanes sync error, crc error, ecc error

	/* SENINF_MUX */
	for (j = SENINF_MUX1; j < _seninf_ops->mux_num; j++) {
		base_mux = ctx->reg_if_mux[j];
		if (SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_0) & 0x1) {
			dev_info(ctx->dev,
				"%sSENINF%d_MUX_CTRL0/_CTRL1/_IRQ_STATUS/_MUX_SIZE/_ERR_SIZE/_EXP_SIZE:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)\n",
				(mtk_cam_seninf_get_top_mux_ctrl(ctx, j) == ctx->seninfIdx) ?
				"*" : "",
				j,
				SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_0),
				SENINF_READ_REG(base_mux, SENINF_MUX_CTRL_1),
				SENINF_READ_REG(base_mux, SENINF_MUX_IRQ_STATUS),
				SENINF_READ_REG(base_mux, SENINF_MUX_SIZE),
				SENINF_READ_REG(base_mux, SENINF_MUX_ERR_SIZE),
				SENINF_READ_REG(base_mux, SENINF_MUX_IMG_SIZE));
		}
	}

	/* check SENINF_CAM_MUX size, dump all cam mux */
	for (i = 0; i < _seninf_ops->cam_mux_num; i++) {
		SENINF_BITS(ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL,
			0);
		tag_03_vc = SENINF_READ_REG(
			ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
		tag_03_dt = SENINF_READ_REG(
			ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_TAG_DT_SEL);
		SENINF_BITS(ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_OPT,
			RG_SENINF_CAM_MUX_PCSR_TAG_VC_DT_PAGE_SEL,
			1);
		tag_47_vc = SENINF_READ_REG(
			ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
		tag_47_dt = SENINF_READ_REG(
			ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_TAG_DT_SEL);

		dev_info(ctx->dev,
		"cam_mux_%d_CTRL/RES/EXP/ERR/OPT/IRQ:(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x)/(0x%x),tag03_vc/_dt(0x%x/0x%x),tag47_vc/_dt(0x%x/0x%x)\n",
		i,
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
			SENINF_CAM_MUX_PCSR_CTRL),
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
				SENINF_CAM_MUX_PCSR_CHK_RES),
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
				SENINF_CAM_MUX_PCSR_CHK_CTL),
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
				SENINF_CAM_MUX_PCSR_CHK_ERR_RES),
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
				SENINF_CAM_MUX_PCSR_OPT),
		SENINF_READ_REG(ctx->reg_if_cam_mux_pcsr[i],
				SENINF_CAM_MUX_PCSR_IRQ_STATUS),
		tag_03_vc, tag_03_dt, tag_47_vc, tag_47_dt);
	}
	dev_info(ctx->dev, "ret = %d", ret);

	return ret;
}

static int mtk_cam_get_csi_irq_status(struct seninf_ctx *ctx)
{
	void *base_csi_mac;
	int ret = 0;

	if (!ctx->streaming)
		return 0;

	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	ret = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	//dev_info(ctx->dev,"CSI_RX%d_MAC_CSI2_IRQ_STATUS(0x%x)\n", ctx->port, ret);
	SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);

	return ret;
}

static int mtk_cam_seninf_get_tsrec_timestamp(struct seninf_ctx *ctx, void *arg)
{
	int ret = 0;
	unsigned int tsrec_no = 0;
	struct seninf_mux *mux_target;
	struct mtk_tsrec_timestamp_by_sensor_id *info = arg;
	struct mtk_cam_seninf_tsrec_timestamp_info ts_info = {0};

	if (info == NULL) {
		dev_info(ctx->dev, "%s arg is null", __func__);
		return -1;
	}

	mutex_lock(&ctx->mutex);
	list_for_each_entry(mux_target, &ctx->list_mux, list) {
		tsrec_no = mux_target->idx;
	}
	mutex_unlock(&ctx->mutex);

	mtk_cam_seninf_tsrec_get_timestamp_info(tsrec_no, &ts_info);

	info->ts_us[0] = ts_info.exp_recs[0].ts_us[0];
	info->ts_us[1] = ts_info.exp_recs[0].ts_us[1];
	info->ts_us[2] = ts_info.exp_recs[0].ts_us[2];
	info->ts_us[3] = ts_info.exp_recs[0].ts_us[3];
	return ret;
}

static int mtk_cam_seninf_get_debug_reg_result(struct seninf_ctx *ctx, void *arg)
{
	int ret = 0;
	int i;
	struct seninf_core *core = ctx->core;
	struct mtk_seninf_debug_result *target = arg;
	struct mtk_cam_seninf_debug debug_result;
	struct mtk_cam_seninf_mux_meter *meter;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	struct mux_debug_result *mux_result;
	static __u16 last_pkCnt;

	if (target == NULL) {
		dev_info(ctx->dev, "mux_debug_result arg is null");
		return -1;
	}

	memset(&debug_result, 0, sizeof(struct mtk_cam_seninf_debug));
	mutex_lock(&core->mutex);

	list_for_each_entry(ctx, &core->list, list) {
		if (!ctx->streaming)
			continue;

		target->is_cphy = ctx->is_cphy;
		target->csi_port = ctx->port;
		target->seninf = ctx->seninfIdx;
		target->data_lanes = ctx->num_data_lanes;

		mtk_cam_seninf_debug_core_dump(ctx, &debug_result);
		target->mux_result_cnt = debug_result.mux_result_cnt;
		target->csi_irq_status = debug_result.csi_irq_status;
		target->csi_mac_irq_status = debug_result.csi_mac_irq_status;

		if (last_pkCnt != debug_result.packet_cnt_status) {
			last_pkCnt = debug_result.packet_cnt_status;
			target->packet_status_err = 0;
		} else {
			target->packet_status_err = 1;
		}

		for (i = 0; i <= debug_result.mux_result_cnt; i++) {
			vcinfo_debug = &debug_result.vcinfo_debug[i];
			meter = &debug_result.meter[i];
			mux_result = &target->mux_result[i];

			mux_result->vc_feature = vcinfo_debug->vc_feature;
			mux_result->vc = vcinfo_debug->vc;
			mux_result->dt = vcinfo_debug->dt;
			mux_result->seninf_mux = vcinfo_debug->seninf_mux;
			mux_result->seninf_mux_en = vcinfo_debug->seninf_mux_en;
			mux_result->seninf_mux_src = vcinfo_debug->seninf_mux_src;
			mux_result->seninf_mux_irq = vcinfo_debug->seninf_mux_irq;
			mux_result->cam_mux = vcinfo_debug->cam_mux;
			mux_result->cam_mux_en = vcinfo_debug->cam_mux_en;
			mux_result->cam_mux_src = vcinfo_debug->cam_mux_src;
			mux_result->cam_mux_irq = vcinfo_debug->cam_mux_irq;
			mux_result->frame_mointor_err = vcinfo_debug->frame_mointor_err;
			mux_result->exp_size = vcinfo_debug->exp_size;
			mux_result->rec_size = vcinfo_debug->rec_size;
			mux_result->v_valid = meter->v_valid;
			mux_result->h_valid = meter->h_valid;
			mux_result->v_blank = meter->v_blank;
			mux_result->h_blank = meter->h_blank;
			mux_result->mipi_pixel_rate = meter->mipi_pixel_rate;
			mux_result->vb_in_us = meter->vb_in_us;
			mux_result->hb_in_us = meter->hb_in_us;
			mux_result->line_time_in_us = meter->line_time_in_us;
		}
	}
	mutex_unlock(&core->mutex);
	return ret;
}

static ssize_t mtk_cam_seninf_show_err_status(struct device *dev,
				   struct device_attribute *attr,
		char *buf)
{
	int i, k, len;
	struct seninf_core *core;
	struct seninf_ctx *ctx;
	struct seninf_vc *vc;
	void *pmux;

	core = dev_get_drvdata(dev);
	len = 0;

	mutex_lock(&core->mutex);

	list_for_each_entry(ctx, &core->list, list) {
		SHOW(buf, len,
		     "\n[%s] port %d intf %d test %d cphy %d lanes %d\n",
			ctx->subdev.name,
			ctx->port,
			ctx->seninfIdx,
			ctx->is_test_model,
			ctx->is_cphy,
			ctx->num_data_lanes);

		SHOW(buf, len, "---flag = errs exceed theshhold ? 1 : 0---\n");
		SHOW(buf, len, "\tdata_not_enough error flag : <%d>",
			ctx->data_not_enough_flag);
		SHOW(buf, len, "\terr_lane_resync error flag : <%d>",
			ctx->err_lane_resync_flag);
		SHOW(buf, len, "\tcrc_err error flag : <%d>",
			ctx->crc_err_flag);
		SHOW(buf, len, "\tecc_err_double error flag : <%d>",
			ctx->ecc_err_double_flag);
		SHOW(buf, len, "\tecc_err_corrected error flag : <%d>\n",
			ctx->ecc_err_corrected_flag);

		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];
			for (k = 0; k < vc->dest_cnt; i++) {
				pmux = ctx->reg_if_mux[vc->dest[k].mux];
				SHOW(buf, len,
				     "[%d] vc 0x%x dt 0x%x mux %d cam %d\n",
					i, vc->vc, vc->dt,
					vc->dest[k].mux, vc->dest[k].cam);
				SHOW(buf, len, "\tfifo_overrun error flag : <%d>",
					ctx->fifo_overrun_flag);
				SHOW(buf, len, "\tsize_err error flag : <%d>\n",
					ctx->size_err_flag);
			}
		}
	}

	mutex_unlock(&core->mutex);

	return len;
}

static int seninf_push_vsync_info_msgfifo(struct mtk_cam_seninf_vsync_info *vsync_info)
{
	int len = 0;

	if (unlikely(kfifo_avail(
			&vsync_detect_seninf_irq_event.msg_fifo) < sizeof(*vsync_info))) {
		atomic_set(&vsync_detect_seninf_irq_event.is_fifo_overflow, 1);
		pr_info("%s fifo over flow\n", __func__);
		return -1;
	}

	len = kfifo_in(&vsync_detect_seninf_irq_event.msg_fifo,
				vsync_info, sizeof(*vsync_info));
	WARN_ON(len != sizeof(*vsync_info));

	return 0;
}

static unsigned int seninf_chk_vsync_info_msgfifo_not_empty(void)
{
	return (kfifo_len(&vsync_detect_seninf_irq_event.msg_fifo)
		>= sizeof(struct mtk_cam_seninf_vsync_info));
}

static void seninf_pop_vsync_info_msgfifo(struct mtk_cam_seninf_vsync_info *vsync_info)
{
	unsigned int len = 0;

	len = kfifo_out(
		&vsync_detect_seninf_irq_event.msg_fifo,
		vsync_info, sizeof(*vsync_info));

	WARN_ON(len != sizeof(*vsync_info));

}

static void calculate_mipi_error_cnt(struct seninf_core *core,
	struct seninf_ctx *ctx, unsigned int csi_irq_st)
{
	switch (csi_irq_st) {
	case 804:
	case 805:
		break;
	default:
		if (csi_irq_st & RO_CSI2_ECC_ERR_CORRECTED_IRQ_MASK)
			ctx->ecc_err_corrected_cnt++;
		if (csi_irq_st & RO_CSI2_ECC_ERR_DOUBLE_IRQ_MASK)
			ctx->ecc_err_double_cnt++;
		if (csi_irq_st & RO_CSI2_CRC_ERR_IRQ_MASK)
			ctx->crc_err_cnt++;
		if (csi_irq_st & RO_CSI2_ERR_LANE_RESYNC_IRQ_MASK)
			ctx->err_lane_resync_cnt++;
		if (csi_irq_st & RO_CSI2_RECEIVE_DATA_NOT_ENOUGH_IRQ_MASK)
			ctx->data_not_enough_cnt++;
		break;
	}
#ifdef ERR_DETECT_TEST
	if (core->err_detect_test_flag)
		ctx->test_cnt++;
#endif
}

static void dump_current_mipi_error_cnt(struct seninf_core *core,
	struct seninf_ctx *ctx, struct mtk_cam_seninf_vsync_info *vsync_info)
{
	char *buf = NULL;
	void *pcammux_gcsr;
	int len = 0;

	buf = kmalloc(sizeof(char) * VSYNC_DUMP_BUF_MAX_LEN, GFP_ATOMIC);
	if (buf == NULL)
		return;

	SENINF_SNPRINTF(buf, len,
		"%s detection_cnt(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x),",
		 __func__,
		core->size_err_detection_cnt,
		core->fifo_overrun_detection_cnt,
		core->ecc_err_corrected_detection_cnt,
		core->ecc_err_double_detection_cnt,
		core->crc_err_detection_cnt,
		core->err_lane_resync_detection_cnt,
		core->data_not_enough_detection_cnt);

#ifdef ERR_DETECT_TEST
	SENINF_SNPRINTF(buf, len,
		" test enable:%d test cnt:%d,",
		core->err_detect_test_flag,
		ctx->test_cnt);
#endif
	SENINF_SNPRINTF(buf, len,
		"port%d: error_cnt: %d; %d; %d; %d; %d; %d; %d tMono %llu",
		ctx->port,
		ctx->data_not_enough_cnt,
		ctx->err_lane_resync_cnt,
		ctx->crc_err_cnt,
		ctx->ecc_err_double_cnt,
		ctx->ecc_err_corrected_cnt,
		ctx->fifo_overrun_cnt,
		ctx->size_err_cnt,
		vsync_info->time_mono/1000000);
	dev_info(ctx->dev, "%s\n", buf);

	if (ctx->streaming)
		if ((ctx->data_not_enough_cnt) >= (core->data_not_enough_detection_cnt) || // 2
		(ctx->err_lane_resync_cnt) >= (core->err_lane_resync_detection_cnt) || // val = 300
		(ctx->crc_err_cnt) >= (core->crc_err_detection_cnt) || // 2
		(ctx->ecc_err_double_cnt) >= (core->ecc_err_double_detection_cnt) || // 2
		(ctx->ecc_err_corrected_cnt) >= (core->ecc_err_corrected_detection_cnt) || //300
		(ctx->fifo_overrun_cnt) >= (core->fifo_overrun_detection_cnt) || // 2
		(ctx->size_err_cnt) >= (core->size_err_detection_cnt) // val = 300
#ifdef ERR_DETECT_TEST
		|| (ctx->test_cnt) >= (300)
#endif
		) {
			pcammux_gcsr = ctx->reg_if_cam_mux_gcsr;
			SENINF_WRITE_REG(
				pcammux_gcsr,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
				0);
			SENINF_WRITE_REG(
				pcammux_gcsr,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
				0);

			core->err_detect_termination_flag = 1;
			len = 0;

			if ((ctx->data_not_enough_cnt) >= (core->data_not_enough_detection_cnt))
				ctx->data_not_enough_flag = 1;
			if ((ctx->err_lane_resync_cnt) >= (core->err_lane_resync_detection_cnt))
				ctx->err_lane_resync_flag = 1;
			if ((ctx->crc_err_cnt) >= (core->crc_err_detection_cnt))
				ctx->crc_err_flag = 1;
			if ((ctx->ecc_err_double_cnt) >= (core->ecc_err_double_detection_cnt))
				ctx->ecc_err_double_flag = 1;
			if ((ctx->ecc_err_corrected_cnt) >=
				(core->ecc_err_corrected_detection_cnt))
				ctx->ecc_err_corrected_flag = 1;
			if ((ctx->fifo_overrun_cnt) >= (core->fifo_overrun_detection_cnt))
				ctx->fifo_overrun_flag = 1;
			if ((ctx->size_err_cnt) >= (core->size_err_detection_cnt))
				ctx->size_err_flag = 1;
			SENINF_SNPRINTF(buf, len,
				"data_not_enough_count: %d, err_lane_resync_count: %d,",
				ctx->data_not_enough_cnt,
				ctx->err_lane_resync_cnt);

			SENINF_SNPRINTF(buf, len,
				"crc_err_count: %d, ecc_err_double_count: %d,",
				ctx->crc_err_cnt,
				ctx->ecc_err_double_cnt);

			SENINF_SNPRINTF(buf, len,
				"ecc_err_corrected_count: %d, fifo_overrun_count: %d,",
				ctx->ecc_err_corrected_cnt,
				ctx->fifo_overrun_cnt);

			SENINF_SNPRINTF(buf, len,
				"size_err_count: %d,",
				ctx->size_err_cnt);
#ifdef ERR_DETECT_TEST
			SENINF_SNPRINTF(buf, len,
				" test_count: %d",
				ctx->test_cnt);
#endif
			seninf_aee_print("[AEE] %s", buf);
			// if(ctx->pid)
			// kill_pid(ctx->pid, SIGKILL, 1);
	}
	kfree(buf);
}

static void dump_mipi_error_detect_info(struct seninf_core *core,
				struct mtk_cam_seninf_vsync_info *vsync_info)
{
	struct seninf_ctx *ctx_;
	int i;

	for (i = 0; i < vsync_info->used_csi_port_num; i++) {
		list_for_each_entry(ctx_, &core->list, list) {
			if (vsync_info->ctx_port[i] == ctx_->port) {
				calculate_mipi_error_cnt(core, ctx_, vsync_info->csi_irq_st[i]);
				dump_current_mipi_error_cnt(core, ctx_, vsync_info);
			}
		}
	}
}

static void dump_vsync_info(struct seninf_core *core,
			struct mtk_cam_seninf_vsync_info *vsync_info)
{
	int i, len = 0;
	char *buf = NULL;

	buf = kmalloc(sizeof(char) * VSYNC_DUMP_BUF_MAX_LEN, GFP_ATOMIC);
	if (buf == NULL)
		return;

	SENINF_SNPRINTF(buf, len,
		"tMono:%llu,vsync_irq:(0x%x/0x%x), cammux_used:%d, csiport_used:%d",
		vsync_info->time_mono/1000000,
		vsync_info->vsync_irq_st_h,
		vsync_info->vsync_irq_st,
		vsync_info->used_cammux_num,
		vsync_info->used_csi_port_num);

	for (i = 0; i < vsync_info->used_csi_port_num; i++) {

		SENINF_SNPRINTF(buf, len,
			" csi_port%d:(csi_irq:0x%x, csi_packet_cnt:0x%x),",
			vsync_info->ctx_port[i],
			vsync_info->csi_irq_st[i],
			vsync_info->csi_packet_cnt_st[i]);

		if (*(core->seninf_vsync_debug_flag) ==
			ENABLE_VSYNC_DETECT_ONLY_CSI_IRQ_STATUS_ERROR_INFO)
			core->vsync_irq_detect_csi_irq_error_flag |=
				(vsync_info->csi_irq_st[i] & (0x324));
	}

	switch (*(core->seninf_vsync_debug_flag)) {
	case ENABLE_VSYNC_DETECT_PER_FRAME_INFO:
		dev_info(core->dev, "%s:%s", __func__, buf);
		break;
	case ENABLE_VSYNC_DETECT_ONLY_CSI_IRQ_STATUS_ERROR_INFO:
		if (core->vsync_irq_detect_csi_irq_error_flag != (0x324))
			dev_info(core->dev, "%s:%s", __func__, buf);
		break;
	default:
		break;
	}

	len = 0;

	for (i = 0; i < vsync_info->used_cammux_num; i++) {
		SENINF_SNPRINTF(buf, len,
			" cam_mux%d :(ctrl:0x%x, res:0x%x, exp:0x%x, err:0x%x",
			vsync_info->used_cammux[i],
			vsync_info->cammux_ctrl_st[i],
			vsync_info->cammux_chk_res_st[i],
			vsync_info->cammux_chk_ctrl_st[i],
			vsync_info->cammux_chk_err_res_st[i]);
		SENINF_SNPRINTF(buf, len,
			" opt:0x%x, irq:0x%x, tag_vc:0x%x, tag_dt:0x%x)",
			vsync_info->cammux_opt_st[i],
			vsync_info->cammux_irq_st[i],
			vsync_info->cammux_tag_vc_sel_st[i],
			vsync_info->cammux_tag_dt_sel_st[i]);
	}

	switch (*(core->seninf_vsync_debug_flag)) {
	case ENABLE_VSYNC_DETECT_PER_FRAME_INFO:
		dev_info(core->dev, "%s:%s", __func__, buf);
		break;
	case ENABLE_VSYNC_DETECT_ONLY_CSI_IRQ_STATUS_ERROR_INFO:
		if (core->vsync_irq_detect_csi_irq_error_flag != (0x324))
			dev_info(core->dev, "%s:%s", __func__, buf);
		break;
	default:
		break;
	}

	kfree(buf);
}

static void mtk_notify_frame_end_fn(struct kthread_work *work)
{
	struct mtk_cam_seninf_vsync_work *vsync_work =
		container_of(work, struct mtk_cam_seninf_vsync_work, work);
	struct seninf_core *core = vsync_work->core;
	struct mtk_cam_seninf_vsync_info *vsync_info = &vsync_work->vsync_info;

	if (core->vsync_irq_en_flag)
		dump_vsync_info(core, vsync_info);

	if (core->csi_irq_en_flag)
		dump_mipi_error_detect_info(core, vsync_info);

	kfree(vsync_work);
}

static void seninf_dump_vsync_info(struct seninf_core *core,
			struct mtk_cam_seninf_vsync_info *vsync_info)
{
	struct mtk_cam_seninf_vsync_work *vsync_work = NULL;

	vsync_work = kmalloc(
			sizeof(struct mtk_cam_seninf_vsync_work),
			GFP_ATOMIC);
	if (vsync_work) {
		kthread_init_work(&vsync_work->work, mtk_notify_frame_end_fn);
		vsync_work->core = core;
		memcpy(&vsync_work->vsync_info, vsync_info, sizeof(*vsync_info));
		kthread_queue_work(&core->seninf_worker, &vsync_work->work);
	} else
		dev_info(core->dev, "%s malloc fail\n", __func__);
}

static int mtk_thread_cam_seninf_irq_handler(int irq, void *data)
{
	struct seninf_core *core = (struct seninf_core *)data;
	struct mtk_cam_seninf_vsync_info vsync_info;

	/* error handling (check case and print information) */
	if (unlikely(atomic_cmpxchg(&vsync_detect_seninf_irq_event.is_fifo_overflow, 1, 0)))
		dev_info(core->dev,
			"WARNING:Vsync detect irq msg fifo overflow\n");

	while (seninf_chk_vsync_info_msgfifo_not_empty()) {
		seninf_pop_vsync_info_msgfifo(&vsync_info);
		seninf_dump_vsync_info(core, &vsync_info);
	}

	return 0;
}

static void mtk_cam_seninf_irq_event_st_init(struct seninf_core *core)
{
	int ret = 0;

	/* init/setup fifo size for below dynamic mem alloc using */
	vsync_detect_seninf_irq_event.fifo_size = roundup_pow_of_two(
		SENINF_IRQ_FIFO_LEN * sizeof(struct mtk_cam_seninf_vsync_info));

	if (likely(vsync_detect_seninf_irq_event.msg_buffer == NULL)) {
		vsync_detect_seninf_irq_event.msg_buffer = devm_kzalloc(core->dev,
					vsync_detect_seninf_irq_event.fifo_size, GFP_ATOMIC);

		if (unlikely(vsync_detect_seninf_irq_event.msg_buffer == NULL))
			dev_info(core->dev,
				"ERROR: irq msg_buffer:%p allocate memory failed, fifo_size:%u\n",
				vsync_detect_seninf_irq_event.msg_buffer,
				vsync_detect_seninf_irq_event.fifo_size);
		else {
			ret = kfifo_init(&vsync_detect_seninf_irq_event.msg_fifo,
				vsync_detect_seninf_irq_event.msg_buffer,
				vsync_detect_seninf_irq_event.fifo_size);

			if (unlikely(ret != 0)) {
				dev_info(core->dev,
					"%s init failed,ret:%d,msg_buffer:%p,fifo_size:(%u/%lu)\n",
					__func__, ret, vsync_detect_seninf_irq_event.msg_buffer,
					vsync_detect_seninf_irq_event.fifo_size,
					sizeof(struct mtk_cam_seninf_vsync_info));
				return;
			}

			atomic_set(&vsync_detect_seninf_irq_event.is_fifo_overflow, 0);

			dev_info(core->dev,
				"%s init done,ret:%d,msg_buffer:%p,fifo_size:%u(%lu)\n",
				__func__, ret, vsync_detect_seninf_irq_event.msg_buffer,
				vsync_detect_seninf_irq_event.fifo_size,
				sizeof(struct mtk_cam_seninf_vsync_info));
		}
	}
}

static void mtk_cam_seninf_irq_event_st_uninit(struct seninf_core *core)
{
	kfifo_free(&vsync_detect_seninf_irq_event.msg_fifo);

	if (likely(vsync_detect_seninf_irq_event.msg_buffer != NULL)) {
		devm_kfree(core->dev, vsync_detect_seninf_irq_event.msg_buffer);
		vsync_detect_seninf_irq_event.msg_buffer = NULL;
		dev_info(core->dev,
			"irq msg_buffer:%p is freed\n",
			vsync_detect_seninf_irq_event.msg_buffer);
	}

}

static int mtk_cam_enable_stream_err_detect(struct seninf_ctx *ctx)
{
	struct seninf_core *core;
	struct seninf_ctx *ctx_;
	void *pSeninf_cam_mux_gcsr;

	core = dev_get_drvdata(ctx->dev->parent);

	list_for_each_entry(ctx_, &core->list, list) {
		if (core->err_detect_termination_flag)
			core->err_detect_termination_flag = 0;

		ctx_->data_not_enough_flag = 0;
		ctx_->err_lane_resync_flag = 0;
		ctx_->crc_err_flag = 0;
		ctx_->ecc_err_double_flag = 0;
		ctx_->ecc_err_corrected_flag = 0;
		ctx_->fifo_overrun_flag = 0;
		ctx_->size_err_flag = 0;
		ctx_->data_not_enough_cnt = 0;
		ctx_->err_lane_resync_cnt = 0;
		ctx_->crc_err_cnt = 0;
		ctx_->ecc_err_double_cnt = 0;
		ctx_->ecc_err_corrected_cnt = 0;
		ctx_->fifo_overrun_cnt = 0;
		ctx_->size_err_cnt = 0;
#ifdef ERR_DETECT_TEST
		ctx_->test_cnt = 0;
#endif
		pSeninf_cam_mux_gcsr = ctx_->reg_if_cam_mux_gcsr;
		dev_info(
			ctx_->dev,
			"mwj VSYNC_IRQ_EN by stream_err_detect!");
			SENINF_WRITE_REG(
				pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
				0xFFFFFFFF);
			SENINF_WRITE_REG(
				pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
				0xFFFFFFFF);
	}
	return 0;
}


static void seninf_record_cammux_irq(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
	struct seninf_ctx *ctx_;
	int i, j, index = 0;
	void *pSeninf_cam_mux_pcsr;
	struct seninf_vc *vc;
	struct seninf_vc_out_dest *dest;

	list_for_each_entry(ctx_, &core->list, list) {
		if (ctx_->streaming) {
			for (i = 0; i < ctx_->vcinfo.cnt; i++) {
				vc = &ctx_->vcinfo.vc[i];
				for (j = 0; j < vc->dest_cnt; j++) {
					dest = &vc->dest[j];
					vsync_info->used_cammux[index] = dest->cam;
					if (dest->cam < _seninf_ops->cam_mux_num) {
						pSeninf_cam_mux_pcsr =
							ctx_->reg_if_cam_mux_pcsr[dest->cam];
					} else
						pSeninf_cam_mux_pcsr = NULL;

					if (pSeninf_cam_mux_pcsr) {
						vsync_info->cammux_irq_st[index] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_IRQ_STATUS);
						index++;
					}
				}
			}
		}
	}
	vsync_info->used_cammux_num = index;
}

static void seninf_record_cammux_info(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
	struct seninf_ctx *ctx_;
	int i, j, used_cammux = 0;
	void *csirx_mac_csi, *pSeninf_cam_mux_pcsr, *pmux;
	struct seninf_vc *vc;
	struct seninf_vc_out_dest *dest;

	 list_for_each_entry(ctx_, &core->list, list) {
		if (ctx_->streaming && ctx_->power_status_flag) {
			csirx_mac_csi = ctx_->reg_csirx_mac_csi[(uint32_t)ctx_->port];
			vsync_info->csi_irq_st[vsync_info->used_csi_port_num] =
				SENINF_READ_REG(csirx_mac_csi, CSIRX_MAC_CSI2_IRQ_STATUS);
			vsync_info->csi_packet_cnt_st[vsync_info->used_csi_port_num] =
				SENINF_READ_REG(csirx_mac_csi, CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
			vsync_info->ctx_port[vsync_info->used_csi_port_num++] = ctx_->port;

			for (i = 0; i < ctx_->vcinfo.cnt; i++) {
				vc = &ctx_->vcinfo.vc[i];
				for (j = 0; j < vc->dest_cnt; j++) {
					dest = &vc->dest[j];
					pmux = ctx_->reg_if_mux[dest->mux];
					vsync_info->seninf_mux_irq_st[used_cammux] =
						SENINF_READ_REG(pmux, SENINF_MUX_IRQ_STATUS);
					if (core->csi_irq_en_flag &&
					(vsync_info->seninf_mux_irq_st[used_cammux] & (0x3))) {
						ctx_->fifo_overrun_cnt++;
						SENINF_WRITE_REG(pmux,
							SENINF_MUX_IRQ_STATUS, 0x103);
					}
					if (dest->cam < _seninf_ops->cam_mux_num &&
						core->vsync_irq_en_flag) {
						pSeninf_cam_mux_pcsr =
							ctx_->reg_if_cam_mux_pcsr[dest->cam];
					} else
						pSeninf_cam_mux_pcsr = NULL;

					if (pSeninf_cam_mux_pcsr) {
						vsync_info->cammux_chk_res_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_CHK_RES);
						vsync_info->cammux_tag_vc_sel_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_TAG_VC_SEL);
						vsync_info->cammux_tag_dt_sel_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_TAG_DT_SEL);
						vsync_info->cammux_ctrl_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_CTRL);
						vsync_info->cammux_chk_ctrl_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_CHK_CTL);
						vsync_info->cammux_chk_err_res_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_CHK_ERR_RES);
						vsync_info->cammux_opt_st[used_cammux] =
							SENINF_READ_REG(pSeninf_cam_mux_pcsr,
								SENINF_CAM_MUX_PCSR_OPT);
						used_cammux++;
					}
				}
			}
		}
	}
}

static void seninf_record_vsync_info(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
	void *pcammux_gcsr;
	struct seninf_ctx *ctx_;

	ctx_ = list_first_entry_or_null(&core->list, struct seninf_ctx, list);
	if (ctx_ != NULL) {
		vsync_info->time_mono = ktime_get_ns();
		pcammux_gcsr = ctx_->reg_if_cam_mux_gcsr;
		vsync_info->vsync_irq_st = SENINF_READ_REG(pcammux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_STS);
		vsync_info->vsync_irq_st_h = SENINF_READ_REG(pcammux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_STS_H);
	} else {
		dev_info(core->dev, "%s [ERROR] ctx_ is NULL", __func__);
		return;
	}

	if (core->vsync_irq_en_flag)
		seninf_record_cammux_irq(core, vsync_info);

	if (vsync_info->vsync_irq_st)
		SENINF_WRITE_REG(pcammux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_STS, 0xFFFFFFFF);

	if (vsync_info->vsync_irq_st_h)
		SENINF_WRITE_REG(pcammux_gcsr,
			SENINF_CAM_MUX_GCSR_VSYNC_IRQ_STS_H, 0xFFFFFFFF);

	seninf_record_cammux_info(core, vsync_info);
}

static int mtk_cam_seninf_irq_handler(int irq, void *data)
{
	struct seninf_core *core = (struct seninf_core *)data;
	struct mtk_cam_seninf_vsync_info vsync_info  = {0};
	unsigned int wake_thread = 0;
	unsigned long flags;

	spin_lock_irqsave(&core->spinlock_irq, flags);

	if (core->vsync_irq_en_flag || core->csi_irq_en_flag) {
		seninf_record_vsync_info(core, &vsync_info);
		if (seninf_push_vsync_info_msgfifo(&vsync_info) == 0)
			wake_thread = 1;
	}

	spin_unlock_irqrestore(&core->spinlock_irq, flags);

	return (wake_thread) ? 1 : 0;
}

static int mtk_cam_seninf_set_sw_cfg_busy(struct seninf_ctx *ctx, bool enable, int index)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;

	if (index == 0)
		SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_DYN_CTRL,
			RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_BSY0, enable);
	else
		SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_DYN_CTRL,
			 RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_BSY0, enable);
	return 0;
}

static int mtk_cam_seninf_set_cam_mux_dyn_en(
	struct seninf_ctx *ctx, bool enable, int cam_mux, int index)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	int tmp = 0;

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		    SENINF_CAM_MUX_GCSR_DYN_CTRL, RG_SENINF_CAM_MUX_DYN_SAT_SWITCH_EN, 0);

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		    SENINF_CAM_MUX_GCSR_DYN_CTRL, RG_SENINF_CAM_MUX_DYN_SKIP_CURR_EN, enable);

	seninf_logd(ctx, "skip curr_en enbled. cam_mux:%d\n", cam_mux);

	if ((cam_mux >= 0) && (cam_mux <= 31)) {
		if (index == 0) {
			tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN0, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN0);
			if (enable)
				tmp |= (1 << cam_mux);
			else
				tmp &= ~(1 << cam_mux);
			SENINF_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN0, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN0,
				tmp);
		} else {
			tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN1, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN1);
			if (enable)
				tmp = tmp | (1 << cam_mux);
			else
				tmp = tmp & ~(1 << cam_mux);
			SENINF_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN1, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN1,
				tmp);
		}
	} else if ((cam_mux >= 32) && (cam_mux <= 63)) {
		cam_mux -= 32;
		if (index == 0) {
			tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN0_H,
				RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN0_H);
			if (enable)
				tmp |= (1 << cam_mux);
			else
				tmp &= ~(1 << cam_mux);
			SENINF_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN0_H,
				RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN0_H, tmp);
		} else {
			tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN1_H,
				RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN1_H);
			if (enable)
				tmp |= (1 << cam_mux);
			else
				tmp &= ~(1 << cam_mux);
			SENINF_BITS(pSeninf_cam_mux_gcsr,
				SENINF_CAM_MUX_GCSR_DYN_EN1_H,
				RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN1_H, tmp);
		}
	}
	return 0;

}

static int mtk_cam_seninf_reset_cam_mux_dyn_en(struct seninf_ctx *ctx, int index)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_DYN_CTRL, RG_SENINF_CAM_MUX_DYN_SAT_SWITCH_EN, 0);

	if (index == 0)
		SENINF_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_DYN_EN0, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN0, 0);
	else
		SENINF_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_DYN_EN1, RG_SENINF_CAM_MUX_GCSR_DYN_SWITCH_EN1, 0);
	return 0;
}


static int mtk_cam_seninf_enable_global_drop_irq(struct seninf_ctx *ctx, bool enable, int index)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	int tmp = 0;

	tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr,
			SENINF_CAM_MUX_GCSR_IRQ_EN, RG_SENINF_CAM_MUX_GCSR_SKIP_NEXT_FRAME_IRQ_EN);
	if (enable)
		tmp |= 1 << index;
	else
		tmp &= ~(1 << index);

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_IRQ_EN, RG_SENINF_CAM_MUX_GCSR_SKIP_NEXT_FRAME_IRQ_EN, tmp);

	return 0;

}

static int mtk_cam_seninf_set_all_cam_mux_vsync_irq(struct seninf_ctx *ctx, bool enable)
{
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	u32 val = enable ? 0xFFFFFFFF : 0;

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, val);
	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H, RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, val);
	return 0;

}
#ifdef SCAN_SETTLE
static int mtk_cam_scan_settle(struct seninf_ctx *ctx)
{
	u16 settle = 0, trail = 0;
	int ret = 0, ret_old = -1;

	if (!ctx->is_cphy) {
		for (trail = 0; trail <= 0xff; trail++) {
			set_trail(ctx, trail);
			msleep(30);
			ret = mtk_cam_seninf_debug(ctx);
			if (ret == 0) {
				if (ret != ret_old)
					dev_info(ctx->dev,
							"%s valid trail = 0x%x ret = %d ret_old = %d trail enbled\n",
							__func__, trail, ret, ret_old);

				dev_info(ctx->dev,
					"%s valid trail = 0x%x ret_detail = %dtrail enbled\n",
					__func__, trail, ret);

			} else {
				if (ret != ret_old)
					dev_info(ctx->dev,
						"%s invalid trail = 0x%x ret = %d ret_old = %d trail enbled\n",
						__func__, trail, ret, ret_old);
				dev_info(ctx->dev,
					"%s invalid trail = 0x%x ret_detail = %d trail enbled\n",
					__func__, trail, ret);
			}
			ret_old = ret;
		}
	} else {
		ret = 0;
		ret_old = -1;
		for (settle = 0; settle <= 0xff; settle++) {
			set_settle(ctx, settle, false);
			msleep(30);
			ret = mtk_cam_seninf_debug(ctx);
			if (ret == 0) {
				if (ret != ret_old)
					dev_info(ctx->dev,
							"%s valid settle = 0x%x ret = %d ret_old = %d\n",
							__func__, settle, ret, ret_old);
					dev_info(ctx->dev,
						"%s valid settle = 0x%x ret_detail = %d\n",
						__func__, settle, ret);
			} else {
				if (ret != ret_old)
					dev_info(ctx->dev,
							"%s invalid settle = 0x%x ret = %d ret_old = %d\n",
							__func__, settle, ret, ret_old);
				dev_info(ctx->dev,
						"%s invalid settle = 0x%x ret_detail = %d\n",
						__func__, settle, ret);
			}
			ret_old = ret;
		}
	}

	return 0;
}
#endif

static void update_vsync_irq_en_flag(struct seninf_ctx *ctx)
{
	void *p_gcammux = ctx->reg_if_cam_mux_gcsr;
	struct seninf_core *core = ctx->core;

	core->vsync_irq_en_flag =
		SENINF_READ_REG(p_gcammux, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN) ||
		SENINF_READ_REG(p_gcammux, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H);

	dev_info(ctx->dev, "vsync enable = %u\n", core->vsync_irq_en_flag);
}

static int mtk_cam_seninf_eye_scan(struct seninf_ctx *ctx, u32 key, int val_signed, char *plog, int logbuf_size)
{
	int i, port, log_len = 0;
    void *base, *base_seninf, *csi_mac;
	u32 temp_base_seninf = 0, temp_csi_mac = 0;
	u32 eq_offset_val;
	u32 val = (val_signed < 0) ? -val_signed : val_signed;
	u32 mask;
	u32 get_rg_val = 0;
	dev_info(ctx->dev, "[EYE_SCAN] key=%u val_signed=%d\n",key,val_signed);

	log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN SUCCESS] set register:\n");
	if (!ctx->streaming) {
		log_len = 0;
		log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] is not streaming\n");
		dev_info(ctx->dev, "[EYE_SCAN FAIL] is not streaming\n");
		return 0;
	}

	switch (key) {
	case EYE_SCAN_KEYS_EQ_DG0_EN:
		if (!((val == 0x1) || (val == 0x0))) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_DG0_EN value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_DG0_EN value(%d) illegal\n", val);
			break;
		}


		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_5,
				    RG_CSI0_CDPHY_EQ_DG0_EN, val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CDPHY_EQ_DG0_EN, val=0x%x\n",val);
			dev_info(ctx->dev, "SENINF_BITS set RG_CSI0_CDPHY_EQ_DG0_EN, val=0x%x\n",val);
		}
		break;
	case EYE_SCAN_KEYS_EQ_SR0:
		if (val > 15) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_SR0 value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_SR0 value(%d) illegal\n", val);
			break;
		}
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_5,
				    RG_CSI0_CDPHY_EQ_SR0, val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CDPHY_EQ_SR0, val=0x%x\n", val);
			dev_info(ctx->dev, "SENINF_BITS set RG_CSI0_CDPHY_EQ_SR0, val=0x%x\n", val);
		}
		break;
	case EYE_SCAN_KEYS_EQ_DG1_EN:
		dev_info(ctx->dev, "this platofrm is not support EQ_DG1 setting\n");
		break;
	case EYE_SCAN_KEYS_EQ_SR1:
		dev_info(ctx->dev, "this platofrm is not support EQ_SR1 setting\n");
		break;
	case EYE_SCAN_KEYS_EQ_BW:
		if (!((val == 0x1) || ((val == 0x0) || (val == 0x3)))) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_BW value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_BW value(%d) illegal\n", val);
			break;
		}

		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_5,
				RG_CSI0_CDPHY_EQ_BW, val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CDPHY_EQ_BW, val=0x%x\n", val);
			dev_info(ctx->dev, "SENINF_BITS set RG_CSI0_CDPHY_EQ_BW, val=0x%x\n", val);
		}
		break;
	case EYE_SCAN_KEYS_CDR_DELAY:
		if (!ctx->is_cphy) {
			if (val > 254) {
				log_len = 0;
				log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] CDR_DELAY value(%d) illegal\n", val);
				dev_info(ctx->dev, "[EYE_SCAN FAIL] CDR_DELAY value(%d) illegal\n", val);
				break;
			}
			for (i = 0; i <= ctx->is_4d1c; i++) {
				port = i ? ctx->portB : ctx->port;
				base = ctx->reg_ana_csi_rx[(unsigned int)port];
				// L0
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_6,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L0_DELAY_CODE, (val & 0b11111111));
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_6,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L0_DELAY_APPLY, 0x0);
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_6,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L0_DELAY_APPLY, 0x1);
				// L1
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_7,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L1_DELAY_CODE, (val & 0b11111111));
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_7,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L1_DELAY_APPLY, 0x0);
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_7,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L1_DELAY_APPLY, 0x1);
				// L2
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_8,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L2_DELAY_CODE, (val & 0b11111111));
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_8,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L2_DELAY_APPLY, 0x0);
				mdelay(1);
				SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_8,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L2_DELAY_APPLY, 0x1);

				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"EYE_SCAN_KEYS_CDR_DELAY input val_signed=%d, write to reg val=0x%x\n",
					val, (val & 0b11111111));
				dev_info(ctx->dev,
				"EYE_SCAN_KEYS_CDR_DELAY input val_signed=%d, write to reg val=0x%x\n",
					val, (val & 0b11111111));
			}

		} else {
			if (val > 31) {
				log_len = 0;
				log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] CDR_DELAY value(%d) illegal\n", val);
				dev_info(ctx->dev, "[EYE_SCAN FAIL] CDR_DELAY value(%d) illegal\n", val);
				break;
			}
			for (i = 0; i <= ctx->is_4d1c; i++) {
				port = i ? ctx->portB : ctx->port;
				base = ctx->reg_ana_csi_rx[(unsigned int)port];
				// T0
				SENINF_BITS(base, CDPHY_RX_ANA_4,
						RG_CSI0_CPHY_T0_CDR_RSTB_CODE, ((val & 0b111000) >> 3));
				SENINF_BITS(base, CDPHY_RX_ANA_4,
						RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, (val & 0b111));
				SENINF_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY, val);
				// T1
				SENINF_BITS(base, CDPHY_RX_ANA_4,
						RG_CSI0_CPHY_T1_CDR_RSTB_CODE, ((val & 0b111000) >> 3));
				SENINF_BITS(base, CDPHY_RX_ANA_4,
						RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, (val & 0b111));
				SENINF_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY, val);

				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_RSTB_CODE, ((val=0x%x & 0b111000) >> 3)\n"
				"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, (val=0x%x & 0b111)\n"
				"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_CK_DELAY, val=0x%x\n"
				"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_RSTB_CODE, ((val=0x%x & 0b111000) >> 3)\n"
				"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, (val=0x%x & 0b111)\n"
				"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_CK_DELAY, val=0x%x\n",
				val, val, val, val, val, val);

				dev_info(ctx->dev,
				"EYE_SCAN_KEYS_CDR_DELAY input val_signed=%d, write to reg val=0x%x\n",
					val, val);
			}
		}
		break;
	case EYE_SCAN_KEYS_GET_CRC_STATUS:
		base_seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfIdx];
		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		temp_csi_mac = SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
		temp_base_seninf = SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS);

		mask = 0xcfff0001;
		if ((temp_csi_mac&(~mask)) == 0x324) {
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"CSIRX_MAC_CSI2_IRQ_STATUS=0x%8x\nSENINF_CSI2_IRQ_STATUS=0x%8x\ncheck CSI2_IRQ_STATUS CORRECT\n",
				temp_csi_mac, temp_base_seninf);
			dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_IRQ_STATUS=0x%8x\nSENINF_CSI2_IRQ_STATUS=0x%8x\ncheck CSI2_IRQ_STATUS CORRECT\n",
			temp_csi_mac, temp_base_seninf);
		} else {
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"CSIRX_MAC_CSI2_IRQ_STATUS=0x%8x\nSENINF_CSI2_IRQ_STATUS=0x%8x\ncheck CSI2_IRQ_STATUS WRONG\n",
				temp_csi_mac, temp_base_seninf);
			dev_info(ctx->dev,
			"CSIRX_MAC_CSI2_IRQ_STATUS=0x%8x\nSENINF_CSI2_IRQ_STATUS=0x%8x\ncheck CSI2_IRQ_STATUS WRONG\n",
			temp_csi_mac, temp_base_seninf);
		}
		break;
	case EYE_SCAN_KEYS_CDR_DELAY_DPHY_EN:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			// L0
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_6,
           		RG_SW_FORCE_EN_DA_CSI0_DPHY_L0_DELAY_EN, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_6,
				RG_SW_FORCE_EN_DA_CSI0_DPHY_L0_DELAY_CODE, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_6,
					RG_SW_FORCE_EN_DA_CSI0_DPHY_L0_DELAY_APPLY, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_6,
           		RG_SW_FORCE_VAL_DA_CSI0_DPHY_L0_DELAY_EN, 0x1);
			// L1
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_7,
           		RG_SW_FORCE_EN_DA_CSI0_DPHY_L1_DELAY_EN, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_7,
				RG_SW_FORCE_EN_DA_CSI0_DPHY_L1_DELAY_CODE, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_7,
					RG_SW_FORCE_EN_DA_CSI0_DPHY_L1_DELAY_APPLY, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_7,
           		RG_SW_FORCE_VAL_DA_CSI0_DPHY_L1_DELAY_EN, 0x1);
			// L2
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_8,
           		RG_SW_FORCE_EN_DA_CSI0_DPHY_L2_DELAY_EN, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_8,
				RG_SW_FORCE_EN_DA_CSI0_DPHY_L2_DELAY_CODE, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_EN_8,
					RG_SW_FORCE_EN_DA_CSI0_DPHY_L2_DELAY_APPLY, 0x1);
			mdelay(1);
			SENINF_BITS(base, CDPHY_RX_ANA_FORCE_MODE_8,
           		RG_SW_FORCE_VAL_DA_CSI0_DPHY_L2_DELAY_EN, 0x1);

			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set CDR_DELAY_DPHY_EN\n");
			dev_info(ctx->dev, "SENINF_BITS set CDR_DELAY_DPHY_EN\n");
		}
		break;
	case EYE_SCAN_KEYS_FLUSH_CRC_STATUS:
		base_seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfIdx];
		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
		SENINF_WRITE_REG(base_seninf, SENINF_CSI2_IRQ_STATUS, 0xffffffff);
		log_len += snprintf(plog + log_len, logbuf_size - log_len,
			"SENINF_WRITE_REG set CSIRX_MAC_CSI2_IRQ_STATUS 0xffffffff\nSENINF_WRITE_REG set SENINF_CSI2_IRQ_STATUS 0xffffffff\n");
		dev_info(ctx->dev, "SENINF_WRITE_REG set FLUSH_CRC_STATUS\n");
		break;
	case EYE_SCAN_KEYS_EQ_OFFSET:
		if ((val_signed > 31) || (val_signed < -31)) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_OFFSET value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_OFFSET value(%d) illegal\n", val);
			break;
		}
		eq_offset_val = (val_signed < 0) ? ((0b11111 & val) + 0b100000) : (0b11111 & val);
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L0_T0AB_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L0_T0AB_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_XX_T0CA_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_XX_T0CA_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_XX_T0BC_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_XX_T0BC_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L1_T1AB_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L1_T1AB_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			SENINF_BITS(base, CDPHY_RX_ANA_11,
				RG_CSI0_CDPHY_XX_T1CA_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_11,
				RG_CSI0_CDPHY_XX_T1CA_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			SENINF_BITS(base, CDPHY_RX_ANA_11,
				RG_CSI0_CDPHY_L2_T1BC_EQ_OS_CAL_FORCE_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_11,
				RG_CSI0_CDPHY_L2_T1BC_EQ_OS_CAL_FORCE_CODE, eq_offset_val);

			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"EYE_SCAN_KEYS_EQ_OFFSET input val_signed=%d, write to reg val=0x%x\n",
					val_signed, eq_offset_val);
			dev_info(ctx->dev,
				"EYE_SCAN_KEYS_EQ_OFFSET input val_signed=%d, write to reg val=0x%x\n",
					val_signed, eq_offset_val);
		}
		break;

	case EYE_SCAN_KEYS_GET_EQ_DG0_EN:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_5,
				    RG_CSI0_CDPHY_EQ_DG0_EN);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_DG0_EN, val=0x%x\n",get_rg_val);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_DG0_EN, val=0x%x\n",get_rg_val);
		}
		break;
	case EYE_SCAN_KEYS_GET_EQ_SR0:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_5,
				    RG_CSI0_CDPHY_EQ_SR0);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_SR0, val=0x%x\n", get_rg_val);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_SR0, val=0x%x\n", get_rg_val);
		}
		break;
	case EYE_SCAN_KEYS_GET_EQ_DG1_EN:
		dev_info(ctx->dev, "this platofrm is not support EQ_DG1 setting\n");
		break;
	case EYE_SCAN_KEYS_GET_EQ_SR1:
		dev_info(ctx->dev, "this platofrm is not support EQ_SR1 setting\n");
		break;
	case EYE_SCAN_KEYS_GET_EQ_BW:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_5,
				RG_CSI0_CDPHY_EQ_BW);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_BW, val=0x%x\n", get_rg_val);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_BW, val=0x%x\n", get_rg_val);
		}
		break;
	case EYE_SCAN_KEYS_GET_CDR_DELAY:
		if (!ctx->is_cphy) {
			for (i = 0; i <= ctx->is_4d1c; i++) {
				port = i ? ctx->portB : ctx->port;
				base = ctx->reg_ana_csi_rx[(unsigned int)port];
				// L0
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_FORCE_MODE_6,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L0_DELAY_CODE);
				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L0 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
				dev_info(ctx->dev,
				"SENINF_READ_BITS get L0 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
				// L1
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_FORCE_MODE_7,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L1_DELAY_CODE);
				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L1 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
				dev_info(ctx->dev,
				"SENINF_READ_BITS get L1 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
				// L2
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_FORCE_MODE_8,
						RG_SW_FORCE_VAL_DA_CSI0_DPHY_L2_DELAY_CODE);
				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L2 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
				dev_info(ctx->dev,
				"SENINF_READ_BITS get L2 CDR_DELAY, val=0x%x\n", get_rg_val & 0b11111111);
			}
		} else {
			for (i = 0; i <= ctx->is_4d1c; i++) {
				port = i ? ctx->portB : ctx->port;
				base = ctx->reg_ana_csi_rx[(unsigned int)port];
				// T0
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T0_CDR_CK_DELAY);
				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get T0 CDR_DELAY, val=0x%x\n", get_rg_val & 0b111111);
				dev_info(ctx->dev,
				"SENINF_READ_BITS get T0 CDR_DELAY, val=0x%x\n", get_rg_val & 0b111111);

				// T1
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_6,
						RG_CSI0_CPHY_T1_CDR_CK_DELAY);
				log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get T1 CDR_DELAY, val=0x%x\n", get_rg_val & 0b111111);
				dev_info(ctx->dev,
				"SENINF_READ_BITS get T1 CDR_DELAY, val=0x%x\n", get_rg_val & 0b111111);
			}
		}
		break;
	case EYE_SCAN_KEYS_GET_EQ_OFFSET:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L0_T0AB_EQ_OS_CAL_FORCE_CODE);
			val_signed = (0b100000 & get_rg_val) ? ((-1) * (0b11111 & get_rg_val)) : (0b11111 & get_rg_val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L0 EQ_OFFSET= %d\n", val_signed);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get L0 EQ_OFFSET= %d\n", val_signed);

			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_10,
				RG_CSI0_CDPHY_L1_T1AB_EQ_OS_CAL_FORCE_CODE);
			val_signed = (0b100000 & get_rg_val) ? ((-1) * (0b11111 & get_rg_val)) : (0b11111 & get_rg_val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L1 EQ_OFFSET= %d\n", val_signed);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get L1 EQ_OFFSET= %d\n", val_signed);

			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_11,
				RG_CSI0_CDPHY_L2_T1BC_EQ_OS_CAL_FORCE_CODE);
			val_signed = (0b100000 & get_rg_val) ? ((-1) * (0b11111 & get_rg_val)) : (0b11111 & get_rg_val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get L2 EQ_OFFSET= %d\n", val_signed);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get L2 EQ_OFFSET= %d\n", val_signed);
		}
		break;
	}
	return 0;
}

static int mtk_cam_seninf_set_reg(struct seninf_ctx *ctx, u32 key, u64 val)
{
	int i, j;
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	void *pmux, *pcammux, *p_gcammux;
	struct seninf_vc *vc;
	struct seninf_core *core;
	struct seninf_ctx *ctx_;

	core = dev_get_drvdata(ctx->dev->parent);

	dev_info(ctx->dev, "%s key = 0x%x, val = %llu\n",
		 __func__, key, val);

	switch (key) {
	case REG_KEY_SETTLE_CK:
		if (!ctx->streaming)
			return 0;
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER,
			    RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER,
			    val);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER,
			    RG_DPHY_RX_LC1_HS_SETTLE_PARAMETER,
			    val);
		break;
	case REG_KEY_SETTLE_DT:
		if (!ctx->streaming)
			return 0;
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER,
			    val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_SETTLE_PARAMETER,
			    val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_SETTLE_PARAMETER,
			    val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_SETTLE_PARAMETER,
			    val);
		break;
	case REG_KEY_HS_TRAIL_EN:
		if (!ctx->streaming)
			return 0;
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, val);
		break;
	case REG_KEY_HS_TRAIL_PARAM:
		if (!ctx->streaming)
			return 0;
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER,
			    RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER,
			    RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_PARAMETER, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER,
			    RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_PARAMETER, val);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER,
			    RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_PARAMETER, val);
		break;
	case REG_KEY_CSI_IRQ_STAT:
		if (!ctx->streaming)
			return 0;
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS,
				 val & 0xFFFFFFFF);
		break;
	case REG_KEY_MUX_IRQ_STAT:
		if (!ctx->streaming)
			return 0;
		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];
			for (j = 0; j < vc->dest_cnt; j++) {
				pmux = ctx->reg_if_mux[vc->dest[j].mux];
				SENINF_WRITE_REG(pmux, SENINF_MUX_IRQ_STATUS,
						 val & 0xFFFFFFFF);
			}
		}
		break;
	case REG_KEY_CAMMUX_IRQ_STAT:
		if (!ctx->streaming)
			return 0;
		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];
			for (j = 0; j < vc->dest_cnt; j++) {
				pcammux = ctx->reg_if_cam_mux_pcsr[vc->dest[j].cam];
				SENINF_WRITE_REG(pcammux,
						 SENINF_CAM_MUX_PCSR_IRQ_STATUS,
						 val & 0xFFFFFFFF);
			}
		}
		break;
	case REG_KEY_CAMMUX_VSYNC_IRQ_EN:
		if (!ctx->streaming)
			return 0;
		p_gcammux = ctx->reg_if_cam_mux_gcsr;
		SENINF_WRITE_REG(p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
				val & 0xFFFFFFFF);

		update_vsync_irq_en_flag(ctx);
		break;
	case REG_KEY_CAMMUX_VSYNC_IRQ_EN_H:
		if (!ctx->streaming)
			return 0;
		p_gcammux = ctx->reg_if_cam_mux_gcsr;
		SENINF_WRITE_REG(p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
				val & 0xFFFFFFFF);

		update_vsync_irq_en_flag(ctx);
		break;
	case REG_KEY_MIPI_ERROR_DETECT_EN:
		dev_info(ctx->dev,
			"%s REG_KEY_MIPI_ERROR_DETECT_EN start val=%lld ctx->streaming=%d",
			__func__, val, ctx->streaming);
		if (!ctx->streaming) {
			dev_info(ctx->dev, "%s no streaming, return ", __func__);
			return 0;
		}

		if (val == 1) { // stop/restart err detection in stream
			core->csi_irq_en_flag = 0;
			core->err_detect_init_flag = 0;
			core->err_detect_termination_flag = 1;
			list_for_each_entry(ctx_, &core->list, list) {
				dev_info(ctx_->dev,
					"%s MIPI_ERROR_DETECT_EN terminated by user!",
					__func__);
				p_gcammux = ctx_->reg_if_cam_mux_gcsr;
				SENINF_WRITE_REG(
					p_gcammux,
					SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
					0);
				SENINF_WRITE_REG(
					p_gcammux,
					SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
					0);
				ctx_->data_not_enough_flag = 0;
				ctx_->err_lane_resync_flag = 0;
				ctx_->crc_err_flag = 0;
				ctx_->ecc_err_double_flag = 0;
				ctx_->ecc_err_corrected_flag = 0;
				ctx_->fifo_overrun_flag = 0;
				ctx_->size_err_flag = 0;
				ctx_->data_not_enough_cnt = 0;
				ctx_->err_lane_resync_cnt = 0;
				ctx_->crc_err_cnt = 0;
				ctx_->ecc_err_double_cnt = 0;
				ctx_->ecc_err_corrected_cnt = 0;
				ctx_->fifo_overrun_cnt = 0;
				ctx_->size_err_cnt = 0;
#ifdef ERR_DETECT_TEST
				ctx_->test_cnt = 0;
				//stop err detect test
				core->err_detect_test_flag = 0;
#endif
				return 0;
			}
		}
		core->data_not_enough_detection_cnt = 50;
		core->err_lane_resync_detection_cnt = 50;
		core->crc_err_detection_cnt = 50;
		core->ecc_err_double_detection_cnt = 50;
		core->ecc_err_corrected_detection_cnt = 50;
		core->fifo_overrun_detection_cnt = 50;
		core->size_err_detection_cnt = 50;
		core->csi_irq_en_flag = 1;
		core->detection_cnt = val;
		core->err_detect_init_flag = 1;
		core->err_detect_termination_flag = 0;
#ifdef ERR_DETECT_TEST
		core->err_detect_test_flag = 0;
#endif
/*
 * data_not_enough_detection_cnt		[7:0]
 * err_lane_resync_detection_cnt		[15:8]
 * crc_err_detection_cnt			[23:16]
 * ecc_err_double_detection_cnt		[31:24]
 * ecc_err_corrected_detection_cnt	[39:32]
 * fifo_overrun_detection_cnt		[47:40]
 * size_err_detection_cnt		[55:48]
 * err_detect_test_flag			[56]
 */
		if (val & 0xFF)
			core->data_not_enough_detection_cnt = (val & 0xFF);
		if (((val >> 8) & 0xFF))
			core->err_lane_resync_detection_cnt = ((val >> 8) & 0xFF);
		if (((val >> 16) & 0xFF))
			core->crc_err_detection_cnt = ((val >> 16) & 0xFF);
		if (((val >> 24) & 0xFF))
			core->ecc_err_double_detection_cnt = ((val >> 24) & 0xFF);
		if (((val >> 32) & 0xFF))
			core->ecc_err_corrected_detection_cnt = ((val >> 32) & 0xFF);
		if (((val >> 40) & 0xFF))
			core->fifo_overrun_detection_cnt = ((val >> 40) & 0xFF);
		if (((val >> 48) & 0xFF))
			core->size_err_detection_cnt = ((val >> 48) & 0xFF);
#ifdef ERR_DETECT_TEST
		if (((val >> 56) & 0x01))
			core->err_detect_test_flag = 1;
#endif
		dev_info(ctx->dev,
			"%s detection_cnt(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,test enable:%d)",
			__func__,
			core->size_err_detection_cnt,
			core->fifo_overrun_detection_cnt,
			core->ecc_err_corrected_detection_cnt,
			core->ecc_err_double_detection_cnt,
			core->crc_err_detection_cnt,
			core->err_lane_resync_detection_cnt,
			core->data_not_enough_detection_cnt,
			core->err_detect_test_flag);
			dev_info(ctx->dev,
				"%s err detection enabled by user!", __func__);
		list_for_each_entry(ctx_, &core->list, list) {
			p_gcammux = ctx_->reg_if_cam_mux_gcsr;
			SENINF_WRITE_REG(
				p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
				0xFFFFFFFF);
			SENINF_WRITE_REG(
				p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
				0xFFFFFFFF);
		}
		break;
	case REG_KEY_AOV_CSI_CLK_SWITCH:
		switch (val) {
		case 130:
			core->aov_csi_clk_switch_flag = CSI_CLK_130;
			core->aov_ut_debug_for_get_csi_param = 1;
			break;
		case 242:
			core->aov_csi_clk_switch_flag = CSI_CLK_242;
			core->aov_ut_debug_for_get_csi_param = 0;
			break;
		case 312:
		default:
			core->aov_csi_clk_switch_flag = CSI_CLK_312;
			core->aov_ut_debug_for_get_csi_param = 0;
			break;
		}
		dev_info(ctx->dev,
			"[%s] set aov csi clk (%llu), ut_flag:%u\n",
			__func__, val, core->aov_ut_debug_for_get_csi_param);
		break;
	}

	return 0;
}

static int mtk_cam_set_phya_clock_src(struct seninf_ctx *ctx, u64 val)
{
	void *base = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];

	switch (val) {
	case 1:
		if (_seninf_ops->iomem_ver == NULL) {
			dev_info(ctx->dev, "[%s] phya clk set to 0\n", __func__);
			return 0;
		} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6878_IOMOM_VERSIONS)) {
			dev_info(ctx->dev, "[%s] phya clk set to 0\n", __func__);
			return 0;
		} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6989_IOMOM_VERSIONS))
			SENINF_BITS(base, CDPHY_RX_ANA_SETTING_0, CSR_ANA_REF_CK_SEL, val);
		else {
			dev_info(ctx->dev,
				"[%s] phya clk set to %llu fail, check platform ver\n",
				__func__, val);
			return -EINVAL;
		}
		break;
	case 0:
		SENINF_BITS(base, CDPHY_RX_ANA_SETTING_0, CSR_ANA_REF_CK_SEL, val);
		break;
	default:
		return -EINVAL;
	}
	dev_info(ctx->dev, "[%s] phya clk set to %llu\n", __func__, val);

	return 0;
}

static int debug_init_deskew_irq(struct seninf_ctx *ctx)
{
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *csi_mac_base = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	static int i = 0;
	dev_info(ctx->dev, "[%s] deskew_irq_en = 0x%x, deskew_irq_clr = 0x%x, deskew_irq_status = 0x%x\n",
		__func__,
		SENINF_READ_REG(dphy_base, DPHY_RX_DESKEW_IRQ_EN),
		SENINF_READ_REG(dphy_base, DPHY_RX_DESKEW_IRQ_CLR),
		SENINF_READ_REG(dphy_base, DPHY_RX_DESKEW_IRQ_STATUS));
	dev_info(ctx->dev, "[%s] csirx_mac_irq_status = 0x%x, \n",
		__func__,
		SENINF_READ_REG(csi_mac_base, CSIRX_MAC_CSI2_IRQ_STATUS));
	dev_info(ctx->dev, "[%s] DPHY_RX_IRQ_EN = 0x%x, DPHY_RX_IRQ_CLR = 0x%x, DPHY_RX_IRQ_STATUS = 0x%x\n",
		__func__,
		SENINF_READ_REG(dphy_base, DPHY_RX_IRQ_EN),
		SENINF_READ_REG(dphy_base, DPHY_RX_IRQ_CLR),
		SENINF_READ_REG(dphy_base, DPHY_RX_IRQ_STATUS));

	dev_info(ctx->dev, "[%s] dump i = %d\n", __func__, i);
	i++;
	debug_init_deskew_begin_end_apply_code(ctx);
	return 0;
}

static int mtk_cam_seninf_set_csi_afifo_pop(struct seninf_ctx *ctx)
{
	struct seninf_vcinfo *vcinfo;
	struct seninf_vc *vc;
	struct seninf_vc_out_dest *dest;
	int i, j;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	vcinfo = &ctx->vcinfo;
	if (unlikely(vcinfo == NULL)) {
		dev_info(ctx->dev, "[%s][ERROR] vcinfo is NULL\n", __func__);
		return -EINVAL;
	}

	/* scan all vc info */
	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];

		if (unlikely(vc == NULL)) {
			dev_info(ctx->dev, "[%s][ERROR] vc is NULL\n", __func__);
			return -EINVAL;
		}

		/* scan all dest_cnt in per vcinfo */
		for (j = 0; j < vc->dest_cnt; j++) {
			dest = &vc->dest[j];

			if (unlikely(dest == NULL)) {
				dev_info(ctx->dev, "[%s][ERROR] dest is NULL\n", __func__);
				return -EINVAL;
			}

			if (dest->pix_mode == PIX_MODE_16_REG_VAL) {
				mtk_cam_seninf_set_seninf_top_ctrl2(ctx, 0);
				dev_info(ctx->dev,
					"[%s] target 16 pix_mode at vc[%d].dest[%d]\n", __func__, i, j);
				return 0;
			}
		}
	}
	return 0;
}

struct mtk_cam_seninf_ops mtk_csi_phy_3_1 = {
	._init_iomem = mtk_cam_seninf_init_iomem,
	._init_port = mtk_cam_seninf_init_port,
	._is_cammux_used = mtk_cam_seninf_is_cammux_used,
	._cammux = mtk_cam_seninf_cammux,
	._disable_cammux = mtk_cam_seninf_disable_cammux,
	._disable_all_cammux = mtk_cam_seninf_disable_all_cammux,
	._set_top_mux_ctrl = mtk_cam_seninf_set_top_mux_ctrl,
	._get_top_mux_ctrl = mtk_cam_seninf_get_top_mux_ctrl,
	._get_cammux_ctrl = mtk_cam_seninf_get_cammux_ctrl,
	._get_cammux_res = mtk_cam_seninf_get_cammux_res,
	._set_cammux_vc = mtk_cam_seninf_set_cammux_vc,
	._set_cammux_tag = mtk_cam_seninf_set_cammux_tag,
	._set_cammux_src = mtk_cam_seninf_set_cammux_src,
	._set_vc = mtk_cam_seninf_set_vc,
	._set_mux_ctrl = mtk_cam_seninf_set_mux_ctrl,
	._set_mux_vc_split_all = mtk_cam_seninf_set_mux_vc_split_all,
	._set_mux_crop = mtk_cam_seninf_set_mux_crop,
	._is_mux_used = mtk_cam_seninf_is_mux_used,
	._mux = mtk_cam_seninf_mux,
	._disable_mux = mtk_cam_seninf_disable_mux,
	._disable_all_mux = mtk_cam_seninf_disable_all_mux,
	._set_cammux_chk_pixel_mode = mtk_cam_seninf_set_cammux_chk_pixel_mode,
	._set_test_model = mtk_cam_seninf_set_test_model,
	._set_csi_mipi = mtk_cam_seninf_set_csi_mipi,
	._poweroff = mtk_cam_seninf_poweroff,
	._reset = mtk_cam_seninf_reset,
	._set_idle = mtk_cam_seninf_set_idle,
	._get_mux_meter = mtk_cam_seninf_get_mux_meter,
	._show_status = mtk_cam_seninf_show_status,
	._switch_to_cammux_inner_page = mtk_cam_seninf_switch_to_cammux_inner_page,
	._set_cammux_next_ctrl = mtk_cam_seninf_set_cammux_next_ctrl,
	._update_mux_pixel_mode = mtk_cam_seninf_update_mux_pixel_mode,
	._irq_handler = mtk_cam_seninf_irq_handler,
	._thread_irq_handler = mtk_thread_cam_seninf_irq_handler,
	._init_irq_fifo = mtk_cam_seninf_irq_event_st_init,
	._uninit_irq_fifo = mtk_cam_seninf_irq_event_st_uninit,
	._set_sw_cfg_busy = mtk_cam_seninf_set_sw_cfg_busy,
	._set_cam_mux_dyn_en = mtk_cam_seninf_set_cam_mux_dyn_en,
	._reset_cam_mux_dyn_en = mtk_cam_seninf_reset_cam_mux_dyn_en,
	._enable_global_drop_irq = mtk_cam_seninf_enable_global_drop_irq,
	._enable_cam_mux_vsync_irq = mtk_cam_seninf_enable_cam_mux_vsync_irq,
	._set_all_cam_mux_vsync_irq = mtk_cam_seninf_set_all_cam_mux_vsync_irq,
#ifdef SCAN_SETTLE
	._debug = mtk_cam_scan_settle,

#else
	._debug = mtk_cam_seninf_debug,
#endif
	._get_debug_reg_result = mtk_cam_seninf_get_debug_reg_result,
	._get_tsrec_timestamp = mtk_cam_seninf_get_tsrec_timestamp,
	._eye_scan = mtk_cam_seninf_eye_scan,
	._set_reg = mtk_cam_seninf_set_reg,
	._set_phya_clock_src = mtk_cam_set_phya_clock_src,
	.seninf_num = 12,
	.mux_num = 22,
	.cam_mux_num = 41,
	.pref_mux_num = 17,
	.iomem_ver = NULL,
	._show_err_status = mtk_cam_seninf_show_err_status,
	._enable_stream_err_detect = mtk_cam_enable_stream_err_detect,
	._debug_init_deskew_irq = debug_init_deskew_irq,
	._debug_init_deskew_begin_end_apply_code = debug_init_deskew_begin_end_apply_code,
	._debug_current_status = mtk_cam_seninf_debug_current_status,
	._set_csi_afifo_pop = mtk_cam_seninf_set_csi_afifo_pop,
	._get_csi_irq_status = mtk_cam_get_csi_irq_status,
};
