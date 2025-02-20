// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/minmax.h>

#include "mtk_cam-seninf.h"
#include "mtk_cam-seninf-if.h"
#include "mtk_cam-seninf-hw.h"
#include "mtk_cam-seninf-regs.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-top.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-async-top.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-tg1.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-outmux.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-mipi-rx-ana-cdphy-csi0a.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-csi0-cphy.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-csi0-dphy.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-csirx_mac_csi0.h"
#include "mtk_csi_phy_3_0/mtk_cam-seninf-csirx_mac_top.h"

#include "mtk_cam-seninf_control-8.h"
#include "mtk_cam-seninf-route.h"
#include "mtk_cam-seninf-event-handle.h"
#include "mtk_cam-seninf-sentest-ioctrl.h"
#include "imgsensor-user.h"
#define SENINF_CK 312000000
#define CYCLE_MARGIN 1
#define RESYNC_DMY_CNT 4
#define DPHY_SETTLE_DEF 100  // ns
#define CPHY_SETTLE_DEF 70 //60~80ns
#define DPHY_TRAIL_SPEC 224
#define FT_30_FPS 33

#define PIX_MODE_16_REG_VAL 4
#define DEBUG_CAM_MUX_SWITCH 0
//#define SCAN_SETTLE

#define MT6899_IOMOM_VERSIONS "mt6899"
#define MT6991_IOMOM_VERSIONS "mt6991"

static struct mtk_cam_seninf_ops *_seninf_ops = &mtk_csi_phy_3_0;
static struct mtk_cam_seninf_irq_event_st vsync_detect_seninf_irq_event;
#define SENINF_IRQ_FIFO_LEN 36
#define VSYNC_DUMP_BUF_MAX_LEN 2048

#define SENINF_SNPRINTF(buf, len, fmt, ...) { \
	len += snprintf(buf + len, VSYNC_DUMP_BUF_MAX_LEN - len, fmt, ##__VA_ARGS__); \
}

#define SHOW(buf, len, fmt, ...) { \
	len += snprintf(buf + len, PAGE_SIZE - len, fmt, ##__VA_ARGS__); \
}

#define SET_MAC_CHECKER_V1(ptr, num, vc, dt, hsize, vsize) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_VC_SEL_SET##num, vc); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_DT_SEL_SET##num, dt); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num, RG_CSI2_CHK_EXPECT_HSIZE_SET##num, hsize); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num, RG_CSI2_CHK_EXPECT_VSIZE_SET##num, vsize); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_VC_EN_SET##num, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_DT_EN_SET##num, 1); \
} while (0)

#define RESET_MAC_CHECKER_V1(ptr, num) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_VC_EN_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_DT_EN_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_VC_SEL_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num, RG_CSI2_CHK_DT_SEL_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num, RG_CSI2_CHK_EXPECT_HSIZE_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num, RG_CSI2_CHK_EXPECT_VSIZE_SET##num, 0); \
} while (0)

#define CLEAR_MAC_CHECKER_IRQ_V1(ptr, num) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_RCV##num, RO_CSI2_CHK_RECEIVED_HSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_RCV##num, RO_CSI2_CHK_RECEIVED_VSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_ERRSIZE##num, RO_CSI2_CHK_ERR_HSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SIZE_CHK_ERRSIZE##num, RO_CSI2_CHK_ERR_VSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_IRQ_STATUS, RO_CSI2_HVSIZE_ERR_SET##num##_IRQ, 1); \
} while (0)

#define DUMP_MAC_CHECKER_V1(ctx, ptr, num) { \
seninf_logi(ctx, "CSIRX_MAC_CSI2_SIZE_CHK_CTRL/_EXP/_RCV/_ERR:[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_RCV##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_ERRSIZE##num)); \
}

#define DUMP_CUR_MAC_CHECKER_V1(ctx, num) { \
seninf_logi(ctx, "CSIRX_MAC_CSI2_SIZE_CHK_CTRL/_EXP/_RCV/_ERR:[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	ctx->debug_cur_mac_csi2_size_chk_ctrl##num, \
	ctx->debug_cur_mac_csi2_size_chk_exp##num, \
	ctx->debug_cur_mac_csi2_size_chk_rcv##num, \
	ctx->debug_cur_mac_csi2_size_chk_err##num); \
}

#define READ_CUR_MAC_CHECKER_V1(ptr, num) do { \
ctx->debug_cur_mac_csi2_size_chk_ctrl##num =  SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num); \
ctx->debug_cur_mac_csi2_size_chk_exp##num =  SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num); \
ctx->debug_cur_mac_csi2_size_chk_rcv##num =  SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_RCV##num); \
ctx->debug_cur_mac_csi2_size_chk_err##num =  SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_ERRSIZE##num); \
} while (0)

#define SHOW_MAC_CHECKER_V1(buf, len, ptr, num) { \
SHOW(buf, len, "csirx_mac_csi2 SIZE_CHK_CTRL/_EXP/_RCV/_ERR:[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_CTRL##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_EXP##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_RCV##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SIZE_CHK_ERRSIZE##num)); \
}

#define SET_MAC_CHECKER_V2(ptr, num, vc, dt, hsize, vsize) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_VC_SEL_SET##num, vc); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_DT_SEL_SET##num, dt); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num, RG_CSI2_SENINF_CHK_EXP_HSIZE_SET##num, hsize); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num, RG_CSI2_SENINF_CHK_EXP_VSIZE_SET##num, vsize); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_VC_EN_SET##num, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_DT_EN_SET##num, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VHS_ERR_SET##num##_IRQ_EN, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN1_ERR_SET##num##_IRQ_EN, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN4_ERR_SET##num##_IRQ_EN, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN8_ERR_SET##num##_IRQ_EN, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_HVSIZE_ERR_SET##num##_IRQ_EN, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_LP_HS_ERR_SET##num##_IRQ_EN, 1); \
} while (0)

#define RESET_MAC_CHECKER_V2(ptr, num) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_VC_EN_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_DT_EN_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_VC_SEL_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num, RG_CSI2_SENINF_CHK_DT_SEL_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num, RG_CSI2_SENINF_CHK_EXP_HSIZE_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num, RG_CSI2_SENINF_CHK_EXP_VSIZE_SET##num, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VHS_ERR_SET##num##_IRQ_EN, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN1_ERR_SET##num##_IRQ_EN, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN4_ERR_SET##num##_IRQ_EN, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_VS_LEN8_ERR_SET##num##_IRQ_EN, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_HVSIZE_ERR_SET##num##_IRQ_EN, 0); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN, RG_CSI2_SENINF_LP_HS_ERR_SET##num##_IRQ_EN, 0); \
} while (0)

#define CLEAR_MAC_CHECKER_IRQ_V2(ptr, num) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_RCV##num, RO_CSI2_SENINF_CHK_RCV_HSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_RCV##num, RO_CSI2_SENINF_CHK_RCV_VSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_ERRSIZE##num, RO_CSI2_SENINF_CHK_ERR_HSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_ERRSIZE##num, RO_CSI2_SENINF_CHK_ERR_VSIZE_SET##num, 0xffff); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_IRQ_STATUS, RO_CSI2_HVSIZE_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_VHS_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_VS_LEN1_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_VS_LEN4_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_VS_LEN8_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_HVSIZE_ERR_SET##num##_IRQ, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS, RO_CSI2_SENINF_LP_HS_ERR_SET##num##_IRQ, 1); \
} while (0)

#define DUMP_MAC_CHECKER_V2(ctx, ptr, num) \
seninf_logi(ctx, "CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL/_EXP/_RCV/_ERR/_IRQ_EN/_IRQ_STATUS:"\
	"[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_RCV##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_ERRSIZE##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS))

#define DUMP_CUR_MAC_CHECKER_V2(ctx, num) { \
seninf_logi(ctx, "CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL/_EXP/_RCV/_ERR/_IRQ_EN/_IRQ_STATUS:"\
	"[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	ctx->debug_cur_mac_csi2_size_chk_ctrl##num, \
	ctx->debug_cur_mac_csi2_size_chk_exp##num, \
	ctx->debug_cur_mac_csi2_size_chk_rcv##num, \
	ctx->debug_cur_mac_csi2_size_chk_err##num, \
	ctx->debug_cur_mac_csi2_size_irq_en##num, \
	ctx->debug_cur_mac_csi2_size_irq##num); \
}

#define READ_CUR_MAC_CHECKER_V2(ptr, num) do { \
ctx->debug_cur_mac_csi2_size_chk_ctrl##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num); \
ctx->debug_cur_mac_csi2_size_chk_exp##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num); \
ctx->debug_cur_mac_csi2_size_chk_rcv##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_RCV##num); \
ctx->debug_cur_mac_csi2_size_chk_err##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_ERRSIZE##num); \
ctx->debug_cur_mac_csi2_size_irq_en##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN); \
ctx->debug_cur_mac_csi2_size_irq##num = \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS); \
} while (0)

#define SHOW_MAC_CHECKER_V2(buf, len, ptr, num) { \
SHOW(buf, len, "csirx_mac_csi2 SENINF_SIZE_CHK_CTRL/_EXP/_RCV/_ERR/_IRQ_EN/_IRQ_STATUS:"\
	"[%d](0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n", \
	num, \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_CTRL##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_EXP##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_RCV##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_SIZE_CHK_ERRSIZE##num), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_EN), \
	SENINF_READ_REG(ptr, CSIRX_MAC_CSI2_SENINF_CHK_IRQ_SET##num##_STATUS)); \
}

#define SET_DI_CTRL(ptr, s, vc) do { \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_S##s##_DI_CTRL, RG_CSI2_S##s##_DT_SEL, vc->dt); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_S##s##_DI_CTRL, RG_CSI2_S##s##_VC_SEL, vc->vc); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_S##s##_DI_CTRL, RG_CSI2_S##s##_DT_INTERLEAVE_MODE, 1); \
SENINF_BITS(ptr, CSIRX_MAC_CSI2_S##s##_DI_CTRL, RG_CSI2_S##s##_VC_INTERLEAVE_EN, 1); \
} while (0)

#ifdef SENINF_IRQ_DBG_EN
/* IRQ debug enable, don't enable oversize and incomp irq en */
#define SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, tag_id, en) { \
	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_TAG_DONE_IRQ_EN_##tag_id, en); \
}
#else
/* IRQ debug disable */
#define SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, tag_id, en) do { \
SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_INCOMP_IRQ_EN_##tag_id, en); \
SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_OVERSIZE_IRQ_EN_##tag_id, en); \
SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_TAG_DONE_IRQ_EN_##tag_id, en); \
} while (0)
#endif

#define DUMP_DEBUG_REG_INFO_BY_TAG(tag_id) do { \
u32 irq_status; \
vcinfo_debug->exp_size_h =  SENINF_READ_BITS(outmux, SENINF_OUTMUX_TAG_SIZE_##tag_id, SENINF_OUTMUX_HSIZE_##tag_id); \
vcinfo_debug->exp_size_v =  SENINF_READ_BITS(outmux, SENINF_OUTMUX_TAG_SIZE_##tag_id, SENINF_OUTMUX_VSIZE_##tag_id); \
irq_status = SENINF_READ_REG(outmux, SENINF_OUTMUX_IRQ_STATUS); \
vcinfo_debug->done_irq_status =	0x01 &  (irq_status >> SENINF_OUTMUX_TAG_DONE_IRQ_STATUS_##tag_id##_SHIFT); \
irq_status = 0x00;\
irq_status |= SENINF_OUTMUX_TAG_DONE_IRQ_STATUS_##tag_id##_MASK; \
SENINF_WRITE_REG(outmux, SENINF_OUTMUX_IRQ_STATUS, irq_status); \
} while (0)


#define SET_TAG_V2(ctx, ptr, ptr_inout, cfgn, sel, refvc, vc, dt, hsize, vsize) do { \
SENINF_BITS(ptr_inout, SENINF_OUTMUX_SOURCE_CONFIG_##cfgn, SENINF_OUTMUX_TAG_VC_##sel, refvc); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_EN_##sel, 1); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_VC_##sel, vc); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_DT_##sel, dt); \
if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) \
	SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_HSIZE_##sel, hsize - 1); \
else \
	SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_HSIZE_##sel, hsize); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_VSIZE_##sel, vsize - 1); \
} while (0)

#define SET_TAG(ctx, ptr, cfgn, sel, vc, dt, hsize, vsize) do { \
SENINF_BITS(ptr, SENINF_OUTMUX_SOURCE_CONFIG_##cfgn, SENINF_OUTMUX_TAG_VC_##sel, vc); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_EN_##sel, 1); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_VC_##sel, vc); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel, SENINF_OUTMUX_FILT_DT_##sel, dt); \
if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) \
	SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_HSIZE_##sel, hsize - 1); \
else \
	SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_HSIZE_##sel, hsize); \
SENINF_BITS(ptr, SENINF_OUTMUX_TAG_SIZE_##sel, SENINF_OUTMUX_VSIZE_##sel, vsize - 1); \
seninf_logd(ctx, "reg SENINF_OUTMUX_SOURCE_CONFIG_%d::SENINF_OUTMUX_TAG_VC_%d = 0x%x", \
	    cfgn, sel, SENINF_READ_REG(ptr, SENINF_OUTMUX_SOURCE_CONFIG_##cfgn)); \
seninf_logd(ctx, "reg SENINF_OUTMUX_TAG_VCDT_FILT_%d = 0x%x", \
	    sel, SENINF_READ_REG(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##sel)); \
} while (0)

#define SET_TM_VC_DT(ptr, s, vc, dt) do { \
SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_DT##s, dt); \
SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_VSYNC_VC##s, vc); \
SENINF_BITS(ptr, TM_STAGGER_CON##s, TM_EXP_HSYNC_VC##s, vc); \
} while (0)

#define DUMP_TAG_REG(ptr, tag) { \
SHOW(buf, len, \
	"\tTag%d: VCDT(0x%08x),EXP_SZ(0x%08x),DBG(0x%08x)%s", \
	tag, \
	SENINF_READ_REG(ptr, SENINF_OUTMUX_TAG_VCDT_FILT_##tag), \
	SENINF_READ_REG(ptr, SENINF_OUTMUX_TAG_SIZE_##tag), \
	SENINF_READ_REG(ptr, SENINF_OUTMUX_TAG_DBG_PORT_##tag), \
	(tag % 2) ? "\n" : "\t"); \
}

#define SET_TG_TM0(ptr, num, vc, dt, vs_diff) do { \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_HSYNC_VC, vc); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_VSYNC_VC, vc); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_DT, dt); \
} while (0)

#define SET_TG_TM(ptr, num, vc, dt, vs_diff) do { \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_HSYNC_VC, vc); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_VSYNC_VC, vc); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_STAGGER_CON##num, SENINF_TG_SENINF_TG_TM_EXP##num##_DT, dt); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_EXP##num##_CTL, SENINF_TG_SENINF_TG_TM_EXP##num##_VC_MODE, 1); \
SENINF_BITS(ptr, SENINF_TG_SENINF_TG_TM_EXP##num##_CTL, SENINF_TG_SENINF_TG_TM_EXP##num##_DELAY, (vs_diff * num)); \
} while (0)

#define PORTING_FIXME 0

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
				void __iomem *if_top_base, void __iomem *if_async_base,
				void __iomem *if_tm_base, void __iomem *if_outmux[],
				void __iomem *if_outmux_inner[],
				struct csi_reg_base *csi_base)
{
	int i;

	ctx->reg_ana_csi_rx[CSI_PORT_0] =
	ctx->reg_ana_csi_rx[CSI_PORT_0A] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_0B] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_csi_rx[CSI_PORT_1] =
	ctx->reg_ana_csi_rx[CSI_PORT_1A] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_1B] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_csi_rx[CSI_PORT_2A] =
	ctx->reg_ana_csi_rx[CSI_PORT_2] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_2B] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_csi_rx[CSI_PORT_3A] =
	ctx->reg_ana_csi_rx[CSI_PORT_3] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_3B] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_csi_rx[CSI_PORT_4A] =
	ctx->reg_ana_csi_rx[CSI_PORT_4] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_4B] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_csi_rx[CSI_PORT_5A] =
	ctx->reg_ana_csi_rx[CSI_PORT_5] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_RXANA_CSIA];
	ctx->reg_ana_csi_rx[CSI_PORT_5B] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_RXANA_CSIB];

	ctx->reg_ana_dphy_top[CSI_PORT_0A] =
	ctx->reg_ana_dphy_top[CSI_PORT_0B] =
	ctx->reg_ana_dphy_top[CSI_PORT_0] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_dphy_top[CSI_PORT_1A] =
	ctx->reg_ana_dphy_top[CSI_PORT_1B] =
	ctx->reg_ana_dphy_top[CSI_PORT_1] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_dphy_top[CSI_PORT_2A] =
	ctx->reg_ana_dphy_top[CSI_PORT_2B] =
	ctx->reg_ana_dphy_top[CSI_PORT_2] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_dphy_top[CSI_PORT_3A] =
	ctx->reg_ana_dphy_top[CSI_PORT_3B] =
	ctx->reg_ana_dphy_top[CSI_PORT_3] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_dphy_top[CSI_PORT_4A] =
	ctx->reg_ana_dphy_top[CSI_PORT_4B] =
	ctx->reg_ana_dphy_top[CSI_PORT_4] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_dphy_top[CSI_PORT_5A] =
	ctx->reg_ana_dphy_top[CSI_PORT_5B] =
	ctx->reg_ana_dphy_top[CSI_PORT_5] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_DPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_0A] =
	ctx->reg_ana_cphy_top[CSI_PORT_0B] =
	ctx->reg_ana_cphy_top[CSI_PORT_0] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_1A] =
	ctx->reg_ana_cphy_top[CSI_PORT_1B] =
	ctx->reg_ana_cphy_top[CSI_PORT_1] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_2A] =
	ctx->reg_ana_cphy_top[CSI_PORT_2B] =
	ctx->reg_ana_cphy_top[CSI_PORT_2] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_3A] =
	ctx->reg_ana_cphy_top[CSI_PORT_3B] =
	ctx->reg_ana_cphy_top[CSI_PORT_3] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_4A] =
	ctx->reg_ana_cphy_top[CSI_PORT_4B] =
	ctx->reg_ana_cphy_top[CSI_PORT_4] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_ana_cphy_top[CSI_PORT_5A] =
	ctx->reg_ana_cphy_top[CSI_PORT_5B] =
	ctx->reg_ana_cphy_top[CSI_PORT_5] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_CPHY];

	ctx->reg_csirx_mac_csi[CSI_PORT_0]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_0A] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_0B] =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_csi[CSI_PORT_1]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_1A] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_1B] =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_csi[CSI_PORT_2]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_2A] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_2B] =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_csi[CSI_PORT_3]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_3A] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_3B] =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_csi[CSI_PORT_4]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_4A] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_4B] =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_csi[CSI_PORT_5]  =
	ctx->reg_csirx_mac_csi[CSI_PORT_5A] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_MAC_CSIA];
	ctx->reg_csirx_mac_csi[CSI_PORT_5B] =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_MAC_CSIB];

	ctx->reg_csirx_mac_top[CSI_PORT_0A] =
	ctx->reg_csirx_mac_top[CSI_PORT_0B] =
	ctx->reg_csirx_mac_top[CSI_PORT_0]  =
		csi_base[CSI_PORT_0].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_csirx_mac_top[CSI_PORT_1A] =
	ctx->reg_csirx_mac_top[CSI_PORT_1B] =
	ctx->reg_csirx_mac_top[CSI_PORT_1]  =
		csi_base[CSI_PORT_1].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_csirx_mac_top[CSI_PORT_2A] =
	ctx->reg_csirx_mac_top[CSI_PORT_2B] =
	ctx->reg_csirx_mac_top[CSI_PORT_2]  =
		csi_base[CSI_PORT_2].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_csirx_mac_top[CSI_PORT_3A] =
	ctx->reg_csirx_mac_top[CSI_PORT_3B] =
	ctx->reg_csirx_mac_top[CSI_PORT_3]  =
		csi_base[CSI_PORT_3].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_csirx_mac_top[CSI_PORT_4A] =
	ctx->reg_csirx_mac_top[CSI_PORT_4B] =
	ctx->reg_csirx_mac_top[CSI_PORT_4]  =
		csi_base[CSI_PORT_4].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_csirx_mac_top[CSI_PORT_5A] =
	ctx->reg_csirx_mac_top[CSI_PORT_5B] =
	ctx->reg_csirx_mac_top[CSI_PORT_5]  =
		csi_base[CSI_PORT_5].reg_csi_base[SENINF_CSI_MAC_TOP];

	ctx->reg_if_top = if_top_base;
	ctx->reg_if_async = if_async_base;

	for (i = SENINF_ASYNC_0; i < _seninf_ops->async_num; i++)
		ctx->reg_if_tg[i] = if_tm_base + (0x200 * i);

	for (i = SENINF_OUTMUX0; i < _seninf_ops->outmux_num; i++)
		ctx->reg_if_outmux[i] = if_outmux[i];

	for (i = SENINF_OUTMUX0; i < _seninf_ops->outmux_num; i++)
		ctx->reg_if_outmux_inner[i] = if_outmux_inner[i];

	return 0;
}

static int mtk_cam_seninf_init_port(struct seninf_ctx *ctx, int port, struct csi_reg_base *csi_base)
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

	/* init to none then it should be updated later */
	ctx->tsrec_idx = TSREC_NO_NONE;

	if (portNum < CSI_PORT_PHYSICAL_MAX_NUM)
		ctx->seninfAsyncIdx = csi_base[portNum].seninf_async_idx;
	else {
		seninf_logi(ctx, "invalid port %d\n", port);
		return -EINVAL;
	}

	if (ctx->is_4d1c)
		ctx->seninfSelSensor = 0;
	else if (ctx->port == ctx->portA)
		ctx->seninfSelSensor = 0;
	else if (ctx->port == ctx->portB)
		ctx->seninfSelSensor = 1;
	else {
		seninf_logi(ctx, "invalid sel sensor. port %d\n", port);
		return -EINVAL;
	}

	/* setup tsrec hw connected relationship */
	/* (2 is a hardcode value due to each seninf async have 2 tsrec hw) */
	ctx->tsrec_idx = (2 * ctx->seninfAsyncIdx + ctx->seninfSelSensor);

	return 0;
}

static int mtk_cam_seninf_enable_cam_mux_vsync_irq(struct seninf_ctx *ctx, bool enable, int cam_mux)
{
#if PORTING_FIXME
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	int tmp = 0;

	if ((cam_mux >= 0) && (cam_mux <= 31)) {
		tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN);
		if (enable)
			tmp |= (1 << cam_mux);
		else
			tmp &= ~(1 << cam_mux);
		SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, tmp);
	} else if ((cam_mux >= 32) && (cam_mux <= 63)) {
		cam_mux -= 32;
		tmp = SENINF_READ_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN);
		if (enable)
			tmp |= (1 << cam_mux);
		else
			tmp &= ~(1 << cam_mux);
		SENINF_BITS(pSeninf_cam_mux_gcsr, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
			RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, tmp);
	}
#endif
	return 0;
}

static int mtk_cam_seninf_set_outmux_cg(struct seninf_ctx *ctx, int outmux, int en)
{
	// Always on outmux cg to avoid write racing between aov scp side

	return 0;
}

static int mtk_cam_seninf_is_outmux_used(struct seninf_ctx *ctx, int outmux)
{
	void *pSeninf_top = ctx->reg_if_top;
	int val = 0;

	if (outmux >= _seninf_ops->outmux_num)
		return false;

	mutex_lock(&ctx->core->seninf_top_rg_mutex);
	val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_OUTMUX_CG_EN, SENINF_TOP_OUTMUX_CG_EN);
	mutex_unlock(&ctx->core->seninf_top_rg_mutex);

	return ((val >> outmux) & 0x1);
}

static int mtk_cam_seninf_disable_outmux(struct seninf_ctx *ctx, int outmux, bool immed)
{
	void *pSeninf_outmux = NULL;
	unsigned int outmux_irq;

	if (outmux < 0 || outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %d invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];
	outmux_irq = _seninf_ops->_get_outmux_irq_st(ctx, outmux, 1);

	if (!immed)
		_seninf_ops->_wait_outmux_cfg_done(ctx, outmux);

	/* restore config mode to 0 */
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_SW_CONFIG_MODE, SENINF_OUTMUX_CONFIG_MODE, 0);

	/* clear tags */
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_0, SENINF_OUTMUX_FILT_EN_0, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_1, SENINF_OUTMUX_FILT_EN_1, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_2, SENINF_OUTMUX_FILT_EN_2, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_3, SENINF_OUTMUX_FILT_EN_3, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_4, SENINF_OUTMUX_FILT_EN_4, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_5, SENINF_OUTMUX_FILT_EN_5, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_6, SENINF_OUTMUX_FILT_EN_6, 0);
	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_7, SENINF_OUTMUX_FILT_EN_7, 0);

	if (g_seninf_ops->_is_outmux_used(ctx, outmux)) {
		if (immed) {
			/* sw rst */
			SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_SW_RST, SENINF_OUTMUX_LOCAL_SW_RST, 1);
			udelay(1);
			SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_SW_RST, SENINF_OUTMUX_LOCAL_SW_RST, 0);
			/* disable cg */
			mtk_cam_seninf_set_outmux_cg(ctx, outmux, 0);
		} else {
			ctx->outmux_disable_list[outmux] = true;
		}
	}

	seninf_logi(ctx, "clear outmux:%d (disable en:%d) immediately(%d),current irq(0x%x)\n",
		    outmux, ctx->outmux_disable_list[outmux], immed,
		    outmux_irq);

	return 0;
}

static int mtk_cam_seninf_get_outmux_irq_st(struct seninf_ctx *ctx, int outmux_idx, bool clear)
{
	void *pSeninf_outmux = NULL;
	u32 val = 0;

	/* test parameter */
	if (outmux_idx < 0 || outmux_idx >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "invalid outmux %d\n", outmux_idx);
		return -EINVAL;
	}

	pSeninf_outmux = ctx->reg_if_outmux[outmux_idx];

	val = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_IRQ_STATUS);

	if (clear)
		SENINF_WRITE_REG(pSeninf_outmux, SENINF_OUTMUX_IRQ_STATUS, val);

	seninf_logd(ctx, "get outmux%d irq_st 0x%x clear(%d)\n",
		outmux_idx, val, clear);

	return val;
}

static u32 seninf_get_outmux_rg_val(struct seninf_ctx *ctx, int outmux_idx, u32 rg)
{
	void *pSeninf_outmux = NULL;

	/* test parameter */
	if (outmux_idx < 0 || outmux_idx >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "invalid outmux %d\n", outmux_idx);
		return -EINVAL;
	}

	pSeninf_outmux = ctx->reg_if_outmux[outmux_idx];

	return SENINF_READ_REG(pSeninf_outmux, rg);
}

static u32 seninf_get_outmux_rg_val_inner(struct seninf_ctx *ctx, int outmux_idx, u32 rg)
{
	void *pSeninf_outmux = NULL;

	/* test parameter */
	if (outmux_idx < 0 || outmux_idx >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "invalid outmux %d\n", outmux_idx);
		return -EINVAL;
	}

	pSeninf_outmux = ctx->reg_if_outmux_inner[outmux_idx];

	return SENINF_READ_REG(pSeninf_outmux, rg);
}

static int mtk_cam_get_outmux_sel(struct seninf_ctx *ctx, int outmux_idx, int *asyncIdx, int *sensorSel)
{
	void *pSeninf_outmux = NULL;

	/* test parameter */
	if (unlikely(ctx == NULL)) {
		/* invalid seninf ctx */
		return -EINVAL;
	}
	if (outmux_idx < 0 || outmux_idx >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "invalid outmux %d\n", outmux_idx);
		return -EINVAL;
	}

	if (!asyncIdx || !sensorSel)
		return -EINVAL;

	pSeninf_outmux = ctx->reg_if_outmux_inner[outmux_idx];

	*asyncIdx = SENINF_READ_BITS(pSeninf_outmux, SENINF_OUTMUX_SRC_SEL,
				SENINF_OUTMUX_SRC_SEL_MIPI);
	*sensorSel = SENINF_READ_BITS(pSeninf_outmux, SENINF_OUTMUX_SRC_SEL,
				SENINF_OUTMUX_SRC_SEL_SEN);

	seninf_logd(ctx, "%s get asyncIdx %d sensorSel %d\n",
		__func__, *asyncIdx, *sensorSel);

	return 0;
}

static u32 mtk_cam_seninf_get_outmux_vcdt_filt(struct seninf_ctx *ctx, int outmux, int tag, bool inner)
{
	u32 ret = 0;
	void *pSeninf_outmux = NULL;

	if (outmux < 0 || outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %d invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}

	if (inner)
		pSeninf_outmux = ctx->reg_if_outmux_inner[outmux];
	else
		pSeninf_outmux = ctx->reg_if_outmux[outmux];

	switch (tag) {
	case 0:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_0);
		break;
	case 1:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_1);
		break;
	case 2:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_2);
		break;
	case 3:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_3);
		break;
	case 4:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_4);
		break;
	case 5:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_5);
		break;
	case 6:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_6);
		break;
	case 7:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_VCDT_FILT_7);
		break;
	default:
		seninf_logi(ctx, "err tag %d invalid\n", tag);
		break;
	}

	return ret;
}

static u32 mtk_cam_seninf_get_outmux_res(struct seninf_ctx *ctx, int outmux, int tag)
{
	u32 ret = 0;
	void *pSeninf_outmux = NULL;

	if (outmux < 0 || outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %d invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];

	switch (tag) {
	case 0:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_0);
		break;
	case 1:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_1);
		break;
	case 2:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_2);
		break;
	case 3:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_3);
		break;
	case 4:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_4);
		break;
	case 5:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_5);
		break;
	case 6:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_6);
		break;
	case 7:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_DBG_PORT_7);
		break;
	default:
		seninf_logi(ctx, "err tag %d invalid\n", tag);
		break;
	}

	return ret;
}

static u32 mtk_cam_seninf_get_outmux_exp(struct seninf_ctx *ctx, int outmux,
					     int tag)
{
	u32 ret = 0;
	void *pSeninf_outmux = NULL;

	if (outmux < 0 || outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %d invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];

	switch (tag) {
	case 0:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_0);
		break;
	case 1:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_1);
		break;
	case 2:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_2);
		break;
	case 3:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_3);
		break;
	case 4:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_4);
		break;
	case 5:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_5);
		break;
	case 6:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_6);
		break;
	case 7:
		ret = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_TAG_SIZE_7);
		break;
	default:
		seninf_logi(ctx, "err tag %d invalid\n", tag);
		break;
	}

	return ret;
}

static void mtk_cam_seninf_reset_dt_remap(void *pCsi2_mac)
{
	if (pCsi2_mac) {
		SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, 0);
		SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, 0);
		/* enable raw24 */
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_R24USERDEF_DT, RG_CSI2_USERDEF_DT_EN, 0x01);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_R24USERDEF_DT, RG_CSI2_RAW24LIKE_USERDEF_DT, 0x27);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT1, RG_CSI2_RAW20_DT, 0x2f);
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
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT0_SEL, vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT0_EN, 0x1);
		break;
	case 1:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1_SEL, vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT0, RG_FORCE_DT1_EN, 0x1);
		break;
	case 2:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2_SEL, vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT2_EN, 0x1);
		break;
	case 3:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3, vc->dt);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3_SEL, vc->dt_remap_to_type);
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT1, RG_FORCE_DT3_EN, 0x1);
		break;
	default:
		remap_ret = -1;
		break;
	}

	if (remap_ret >= 0) {
		switch (vc->dt) {
		case 0x27:
			/* map raw24 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_R24USERDEF_DT, RG_CSI2_RAW24LIKE_USERDEF_DT, 0x37);
			break;
		case 0x2f:
			/* map raw20 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT1, RG_CSI2_RAW20_DT, 0x37);
			break;
		case 0x2e:
			/* map raw16 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW16_DT, 0x37);
			break;
		case 0x2d:
			/* map raw14 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW14_DT, 0x37);
			break;
		case 0x2c:
			/* map raw12 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW12_DT, 0x37);
			break;
		case 0x2b:
			/* map raw10 dt to unused dt number */
			SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_DT0, RG_CSI2_RAW10_DT, 0x37);
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
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	int i, ret, dt_remap_index = 0, j, k = 0, seq_dt_flag = 0;
	struct seninf_vc *vc;
	int dt_remap_table[4] = {0};

	if (!vcinfo || !vcinfo->cnt)
		return 0;

	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S0_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S1_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S2_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S3_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S4_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S5_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S6_DI_CTRL, 0);
	SENINF_WRITE_REG(pCsi2_mac, CSIRX_MAC_CSI2_S7_DI_CTRL, 0);

	mtk_cam_seninf_reset_dt_remap(pCsi2_mac);

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		if (vc->dt_remap_to_type > MTK_MBUS_FRAME_DESC_REMAP_NONE &&
			vc->dt_remap_to_type <= MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14) {
			if (dt_remap_index == 0) {
				dt_remap_table[dt_remap_index] = vc->dt;
				ret = mtk_cam_seninf_remap_dt(pCsi2_mac, vc, dt_remap_index);
				seninf_logi(ctx, "ret(%d) idx(%d) vc[%d] dt 0x%x remap to %d\n",
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
					ret = mtk_cam_seninf_remap_dt(pCsi2_mac, vc, dt_remap_index);
					seninf_logi(ctx, "ret(%d) idx(%d) vc[%d] dt 0x%x remap to %d\n",
						ret, dt_remap_index, i, vc->dt, vc->dt_remap_to_type);
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
			seninf_logi(ctx, "CSIRX_MAC_CSI2_FORCEDT2: 0x%08x\n",
				SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_FORCEDT2));
		}

		switch (i) {
		case 0:
			SET_DI_CTRL(pCsi2_mac, 0, vc);
			break;
		case 1:
			SET_DI_CTRL(pCsi2_mac, 1, vc);
			break;
		case 2:
			SET_DI_CTRL(pCsi2_mac, 2, vc);
			break;
		case 3:
			SET_DI_CTRL(pCsi2_mac, 3, vc);
			break;
		case 4:
			SET_DI_CTRL(pCsi2_mac, 4, vc);
			break;
		case 5:
			SET_DI_CTRL(pCsi2_mac, 5, vc);
			break;
		case 6:
			SET_DI_CTRL(pCsi2_mac, 6, vc);
			break;
		case 7:
			SET_DI_CTRL(pCsi2_mac, 7, vc);
			break;
		}
	}

	/* General Long Packet Data Types: 0x10-0x17 */
	if (ctx->core->force_glp_en && !seq_dt_flag) {
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_CSI2_OPT, RG_CSI2_GENERIC_LONG_PACKET_EN, 1);
		seninf_logi(ctx, "enable generic long packet\n");
	}

	seninf_logd(ctx, "DI_CTRL 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		 SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S0_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S1_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S2_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S3_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S4_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S5_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S6_DI_CTRL),
		SENINF_READ_REG(pCsi2_mac, CSIRX_MAC_CSI2_S7_DI_CTRL));

	return 0;
}

static int mtk_cam_seninf_disable_all_outmux(struct seninf_ctx *ctx)
{
	int i;

	for (i = 0; i < _seninf_ops->outmux_num; i++)
		mtk_cam_seninf_set_outmux_cg(ctx, i, 0);

	return 0;
}

static int mtk_cam_seninf_set_outmux_pixel_mode(struct seninf_ctx *ctx,
					     int outmux, int pixel_mode)
{
	void *pSeninf_outmux = NULL;

	if (outmux < 0 || outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %d invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];

	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_PIX_MODE, SENINF_OUTMUX_PIX_MODE,
		    (pixel_mode == pix_mode_16p));

	return 0;
}

static int mtk_cam_seninf_set_outmux_grp_en(struct seninf_ctx *ctx,
					     u8 outmux, bool grp_en)
{
	void *pSeninf_outmux = NULL;

	if (outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %u invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];

	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_CSR_CFG_CTRL, SENINF_OUTMUX_CAM_RDY_GRP_EN,
		    grp_en);

	seninf_logd(ctx, "outmux%u, grp_en=%u", outmux, grp_en);

	return 0;
}

static int mtk_cam_seninf_set_outmux_cfg_rdy(struct seninf_ctx *ctx,
					     u8 outmux, bool cfg_rdy)
{
	void *pSeninf_outmux = NULL;

	if (outmux >= _seninf_ops->outmux_num) {
		seninf_logi(ctx, "err outmux %u invalid (0~SENINF_OUTMUX_NUM:%d)\n", outmux, _seninf_ops->outmux_num);
		return 0;
	}
	pSeninf_outmux = ctx->reg_if_outmux[outmux];

	SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_CAM_CFG_RDY, SENINF_OUTMUX_CAM_CFG_RDY, cfg_rdy);

	seninf_logi(ctx, "outmux%u, cfg_rdy=%u", outmux, cfg_rdy);

	return 0;
}

static int mtk_cam_seninf_set_async_cg(struct seninf_ctx *ctx, int async, int en)
{
	// Always on async cg to avoid write racing between aov scp side

	return 0;
}

static int mtk_cam_seninf_en_async_overrun_irq(struct seninf_ctx *ctx, int async)
{
	// Always on async overrun irq to avoid write racing between aov scp side

	return 0;
}

static int mtk_cam_seninf_set_async(struct seninf_ctx *ctx, int async, int split, int tm)
{
	void *pSeninf;
	int val = 0;
	struct mtk_cam_seninf_async_split split_info = {
		.async_idx = async,
		.is_split = split,
	};

	if (async >= _seninf_ops->async_num)
		return false;

	pSeninf = ctx->reg_if_async;

	mutex_lock(&ctx->core->seninf_top_rg_mutex);

	/* set if split */
	val = SENINF_READ_BITS(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG, SENINF_ASYTOP_MIPI_SPLIT);
	if (((val >> (async << 1)) & 0x3) != split)
		/* modify only when the split need to update */
		mtk_cam_seninf_rproc_ccu_ctrl_with_para(ctx->dev, &ctx->core->ccu_rproc_ctrl,
					MSG_TO_CCU_SENINF_MIPI_SPLIT_CTRL, &split_info, __func__);
	/* set if test model */
	val = SENINF_READ_BITS(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG, SENINF_ASYTOP_TESTMDL_SEL);
	if (tm)
		val |= (0x1 << async);
	else
		val &= (~(0x1 << async));

	SENINF_BITS(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG,
		    SENINF_ASYTOP_TESTMDL_SEL, val);

	/* enable debug */
	val = SENINF_READ_BITS(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG, SENINF_ASYTOP_DEBUG_EN);
	val |= (0x1 << async);
	SENINF_BITS(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG, SENINF_ASYTOP_DEBUG_EN, val);

	mutex_unlock(&ctx->core->seninf_top_rg_mutex);

	seninf_logd(ctx, "input async:%d, ASYNC CFG = 0x%x\n",
		async,
		SENINF_READ_REG(pSeninf, SENINF_ASYTOP_SENINF_ASYNC_CFG));

	mtk_cam_seninf_en_async_overrun_irq(ctx, async);

	return true;
}

static int mtk_cam_seninf_get_async_irq_st(struct seninf_ctx *ctx, int async, bool clear)
{
	void *pSeninf_top = ctx->reg_if_top;
	int val = 0;

	if (async >= _seninf_ops->async_num)
		return -1;

	mutex_lock(&ctx->core->seninf_top_rg_mutex);
	switch (async) {
	case 0:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_0);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_0, val);
		break;
	case 1:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_1);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_1, val);
		break;
	case 2:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_2);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_2, val);
		break;
	case 3:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_3);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_3, val);
		break;
	case 4:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_4);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_4, val);
		break;
	case 5:
		val = SENINF_READ_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_5);
		if (clear)
			SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS,
				SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS_5, val);
		break;
	default:
		mutex_unlock(&ctx->core->seninf_top_rg_mutex);
		return -1;
	}
	mutex_unlock(&ctx->core->seninf_top_rg_mutex);

	seninf_logd(ctx, "async%d overrun:%d\n", async, val);

	return val;
}

static int mtk_cam_seninf_set_test_model(struct seninf_ctx *ctx, int intf)
{
	void *pSeninf;
	void *pSeninf_tg;
	int i;
	int tm_width = TEST_MODEL_HSIZE;
	int tm_height = TEST_MODEL_VSIZE;
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	struct seninf_vc *vc;
	int vs_diff = 0x10;

	pSeninf = ctx->reg_if_async;
	pSeninf_tg = ctx->reg_if_tg[(unsigned int)intf];

	mtk_cam_seninf_set_async_cg(ctx, intf, 1);
	_seninf_ops->_reset(ctx, intf);
	mtk_cam_seninf_set_async(ctx, intf, 0, 1);

	/* tm size */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_SIZE,SENINF_TG_SENINF_TG_TM_LINE, tm_height);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_SIZE, SENINF_TG_SENINF_TG_TM_PXL, (tm_width >> 1));
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_CLK_CNT, 0);
	/* tm vb hb */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_DUM, SENINF_TG_SENINF_TG_TM_VB, 100);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_DUM, SENINF_TG_SENINF_TG_TM_HB, 100);
	/* vc dt setup */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_EXP_NUM, (vcinfo->cnt - 1));
	for (i = 0 ; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		dev_info(ctx->dev, "%s: vc[%d] vcid:%d vcdt:0x%x\n", __func__, i, vc->vc, vc->dt);
		switch (i) {
		case 0:
			SET_TG_TM0(pSeninf_tg, 0, vc->vc, vc->dt, vs_diff * i);
			break;
		case 1:
			SET_TG_TM(pSeninf_tg, 1, vc->vc, vc->dt, vs_diff * i);
			break;
		case 2:
			SET_TG_TM(pSeninf_tg, 2, vc->vc, vc->dt, vs_diff * i);
			break;
		case 3:
			SET_TG_TM(pSeninf_tg, 3, vc->vc, vc->dt, vs_diff * i);
			break;
		case 4:
			SET_TG_TM(pSeninf_tg, 4, vc->vc, vc->dt, vs_diff * i);
			break;
		case 5:
			SET_TG_TM(pSeninf_tg, 5, vc->vc, vc->dt, vs_diff * i);
			break;
		case 6:
			SET_TG_TM(pSeninf_tg, 6, vc->vc, vc->dt, vs_diff * i);
			break;
		case 7:
			SET_TG_TM(pSeninf_tg, 7, vc->vc, vc->dt, vs_diff * i);
			break;
		default:
			break;
		}
	}

	/* tm pattern */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_PAT, 0x8);
	/* tm rst */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_RST, 1);
	udelay(1);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_RST, 0);
	/* tm enable */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_EN, 1);
	/* Set as non single mode */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL, SENINF_TG_SENINF_TG_TM_CORE0_SINGLE, 0);

	return 0;
}


static int mtk_cam_seninf_set_test_model_fake_sensor(struct seninf_ctx *ctx, int intf)
{
	void *pSeninf_tg;
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	struct seninf_vc *vc = &vcinfo->vc[0];
	int i;
	u64 tm_width = vc->exp_hsize;
	u64 tm_height = vc->exp_vsize;
	u32 vs_diff = 0;

	/* default isp clk: 564 Mhz */
	u32 tm_vsync = (tm_height >= 3072) ? 256 : 1024; /* HB */
	u32 tm_dummypxl = 2048; /* VB */
	u8 clk_cnt = 2;

	if (ctx->fake_sensor_info.fps == 4800) {
		clk_cnt = 0;
		tm_dummypxl = 360;
		tm_vsync = 128;
	}

	switch (ctx->fake_sensor_info.hdr_mode) {
	case HDR_NONE:
		break;
	case HDR_RAW_DCG_RAW:
		vs_diff = 0;
		tm_dummypxl = 632;
		break;
	case HDR_RAW_STAGGER:
		vs_diff = 0x10;
		tm_dummypxl = 632;
		break;
	case HDR_RAW_LBMF:
		vs_diff = tm_height / 2;
		tm_dummypxl = 632;
		break;
	default:
		dev_err(ctx->dev,
			"fake sensor not support: ctx->fake_sensor_info.hdr_mode:%u\n",
			ctx->fake_sensor_info.hdr_mode);
		break;
	}

	pSeninf_tg = ctx->reg_if_tg[(unsigned int)intf];
	mtk_cam_seninf_set_async(ctx, intf, 0, 1);

	/* tm size */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_SIZE,
		    SENINF_TG_SENINF_TG_TM_LINE, tm_height);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_SIZE,
		    SENINF_TG_SENINF_TG_TM_PXL, (tm_width >> 1));

	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_CLK_CNT, clk_cnt);

	/* tm vb hb */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_DUM,
		    SENINF_TG_SENINF_TG_TM_VB, tm_vsync );
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_DUM,
		    SENINF_TG_SENINF_TG_TM_HB, tm_dummypxl);

	/* vc dt setup */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_EXP_NUM,
		    (vcinfo->cnt - 1));

	/* 2-line interleave */
	if (ctx->fake_sensor_info.hdr_mode == HDR_RAW_DCG_RAW)
		SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
			    SENINF_TG_SENINF_TG_TM_CORE0_INTLV_NUM, 0x1);

	for (i = 0 ; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		switch (i) {
		case 0:
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON0,
				    SENINF_TG_SENINF_TG_TM_EXP0_HSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON0,
				    SENINF_TG_SENINF_TG_TM_EXP0_VSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON0,
				    SENINF_TG_SENINF_TG_TM_EXP0_DT, vc->dt);
			break;
		case 1:
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON1,
					SENINF_TG_SENINF_TG_TM_EXP1_HSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON1,
					SENINF_TG_SENINF_TG_TM_EXP1_VSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON1,
					SENINF_TG_SENINF_TG_TM_EXP1_DT, vc->dt);

			if ((vc->feature >= VC_PDAF_STATS) && (vc->feature < VC_PDAF_MAX_NUM)) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_DELAY, (vs_diff * (i-1)));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_VC_MODE, 0);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_DATA_MODE, 1);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_HT_RATIO, 2);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_SLICE_SIZE, 8);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_SLICE_INTERVAL, 10);
			} else if (vc->feature == VC_RAW_DATA) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_DELAY, (vs_diff * i));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP1_CTL,
						SENINF_TG_SENINF_TG_TM_EXP1_VC_MODE, 1);
			} else {
				dev_err(ctx->dev,
					"fake sensor not support: vc[%d] vcid:%d vcdt:0x%x vcfeature:0x%x\n",
					i, vc->vc, vc->dt, vc->feature);
			}
			break;
		case 2:
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON2,
					SENINF_TG_SENINF_TG_TM_EXP2_HSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON2,
					SENINF_TG_SENINF_TG_TM_EXP2_VSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON2,
					SENINF_TG_SENINF_TG_TM_EXP2_DT, vc->dt);

			if ((vc->feature >= VC_PDAF_STATS) && (vc->feature < VC_PDAF_MAX_NUM)) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_DELAY, (vs_diff * (i-1)));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_VC_MODE, 0);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_DATA_MODE, 1);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_HT_RATIO, 2);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_SLICE_SIZE, 8);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_SLICE_INTERVAL, 10);
			} else if (vc->feature == VC_RAW_DATA) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_DELAY, (vs_diff * i));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP2_CTL,
						SENINF_TG_SENINF_TG_TM_EXP2_VC_MODE, 1);
			} else {
				dev_err(ctx->dev,
					"fake sensor not support: vc[%d] vcid:%d vcdt:0x%x vcfeature:0x%x\n",
					i, vc->vc, vc->dt, vc->feature);
			}
			break;
		case 3:
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON3,
					SENINF_TG_SENINF_TG_TM_EXP3_HSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON3,
					SENINF_TG_SENINF_TG_TM_EXP3_VSYNC_VC, vc->vc);
			SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_STAGGER_CON3,
					SENINF_TG_SENINF_TG_TM_EXP3_DT, vc->dt);

			if ((vc->feature >= VC_PDAF_STATS) && (vc->feature < VC_PDAF_MAX_NUM)) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_DELAY, (vs_diff * (i-1)));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_VC_MODE, 0);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
					SENINF_TG_SENINF_TG_TM_EXP3_DATA_MODE, 1);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_HT_RATIO, 2);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_SLICE_SIZE, 8);
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_SLICE_INTERVAL, 10);
			} else if (vc->feature == VC_RAW_DATA) {
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_DELAY, (vs_diff * i));
				SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_EXP3_CTL,
						SENINF_TG_SENINF_TG_TM_EXP3_VC_MODE, 1);
			} else {
				dev_err(ctx->dev,
					"fake sensor not support: vc[%d] vcid:%d vcdt:0x%x vcfeature:0x%x\n",
					i, vc->vc, vc->dt, vc->feature);
			}
			break;
		default:
			dev_err(ctx->dev,
					"fake sensor not support: vc[%d] vcid:%d vcdt:0x%x vcfeature:0x%x\n",
					i, vc->vc, vc->dt, vc->feature);
			break;
		}
	}

	/* tm pattern Color Bar */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_PAT, 0x8);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_RAND_SEED,
		    SENINF_TG_SENINF_TG_TM_SEED, 0x1);

	/* tm rst */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_RST, 1);
	udelay(300);
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_RST, 0);
	/* tm enable */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_EN, 1);

	/* Set as non single mode */
	SENINF_BITS(pSeninf_tg, SENINF_TG_SENINF_TG_TM_CORE0_CTL,
		    SENINF_TG_SENINF_TG_TM_CORE0_SINGLE, 0);


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
	SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CDPHY_OS_CAL_RSTB, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_LPF_EN, 0);
	SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_CORE_EN, 0);
	udelay(200);

	if (en) {
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_CORE_EN, 1);
		udelay(30);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_BG_LPF_EN, 1);
		udelay(5);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CDPHY_OS_CAL_RSTB, 1);
		udelay(1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L0_T0AB_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L1_T1AB_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_L2_T1BC_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T0BC_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T0CA_EQ_OS_CAL_EN, 1);
		SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_XX_T1CA_EQ_OS_CAL_EN, 1);
		udelay(1);
	}

	seninf_logd(ctx, "portIdx %d en %d CDPHY_RX_ANA_0 0x%x ANA_8 0x%x\n",
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
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0P_T0A_HSRT_CODE, (m_csi_efuse >> 27) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0N_T0B_HSRT_CODE, (m_csi_efuse >> 27) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1P_T0C_HSRT_CODE, (m_csi_efuse >> 22) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1N_T1A_HSRT_CODE, (m_csi_efuse >> 22) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_L2P_T1B_HSRT_CODE, (m_csi_efuse >> 17) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_L2N_T1C_HSRT_CODE, (m_csi_efuse >> 17) & 0x1f);
		seninf_logd(ctx, "CSI%dA,CDPHY_RX_ANA_2/3/4:(0x%x)/(0x%x)/(0x%x),Efuse Data:(0x%08x)",
			ctx->portNum,
			SENINF_READ_REG(base, CDPHY_RX_ANA_2),
			SENINF_READ_REG(base, CDPHY_RX_ANA_3),
			SENINF_READ_REG(base, CDPHY_RX_ANA_4),
			ctx->m_csi_efuse);
	}

	if (ctx->is_4d1c || (ctx->port == ctx->portB)) {
		port = ctx->portB;
		base = ctx->reg_ana_csi_rx[(unsigned int)port];
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0P_T0A_HSRT_CODE, (m_csi_efuse >> 12) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0N_T0B_HSRT_CODE, (m_csi_efuse >> 12) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1P_T0C_HSRT_CODE, (m_csi_efuse >> 7) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1N_T1A_HSRT_CODE, (m_csi_efuse >> 7) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_L2P_T1B_HSRT_CODE, (m_csi_efuse >> 2) & 0x1f);
		SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_L2N_T1C_HSRT_CODE, (m_csi_efuse >> 2) & 0x1f);
		seninf_logd(ctx, "CSI%dB,CDPHY_RX_ANA_2/3/4:(0x%x)/(0x%x)/(0x%x),Efuse Data:(0x%08x)",
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
		SENINF_BITS(base, CDPHY_RX_ANA_1, RG_CSI0_BG_LPRX_VTL_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_1, RG_CSI0_BG_LPRX_VTH_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_1, RG_CSI0_CDPHY_EQ_DES_VREF_SEL, 0x2);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_BG_ALP_RX_VTL_SEL, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_BG_ALP_RX_VTH_SEL, 0x4);
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			SENINF_BITS(base, CDPHY_RX_ANA_1, RG_CSI0_BG_VREF_SEL, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_EQ_DES_VREF_SEL, 0x20);
		} else {
			SENINF_BITS(base, CDPHY_RX_ANA_1, RG_CSI0_BG_VREF_SEL, 0x4);
			SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_EQ_DES_VREF_SEL, 0x10);
		}
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_LATCH_EN, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
		//r50 termination
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0P_T0A_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_L0N_T0B_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1P_T0C_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_L1N_T1A_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_L2P_T1B_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_4,  RG_CSI0_L2N_T1C_HSRT_CODE, 0x10);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_CDR_FIRST_EDGE_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_CDR_FIRST_EDGE_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_CPHY_T0_CDR_SELF_CAL_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_2, RG_CSI0_CPHY_T1_CDR_SELF_CAL_EN, 0x0);

		SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x4);
		SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x9);
		SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x9);
		/* Disable non-ULPS mode */
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS))
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x400);
		else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS))
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x700);
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

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, 1);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, 1);

	return 0;

}

static int set_settle(struct seninf_ctx *ctx, u16 settle, bool hs_trail_en)
{

	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *csi_mac = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	int settle_delay_dt, settle_delay_ck;

	settle_delay_dt = settle;

	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_G1_STATUS, 0xffffffff);
	SENINF_WRITE_REG(base, DPHY_RX_IRQ_CLR, 0xffffffff);
	dev_info(ctx->dev, "[%s]ctx->port (%d) reset status\n",
			__func__, ctx->port);

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_SETTLE_PARAMETER, settle_delay_dt);

	settle_delay_ck = settle;

	SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER, settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER, RG_DPHY_RX_LC1_HS_SETTLE_PARAMETER, settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, hs_trail_en);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, hs_trail_en);

	return 0;
}
#endif

static int csirx_dphy_init(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	u64 settle_delay_dt, settle_delay_ck, hs_trail, hs_trail_en, hs_option;
	int bit_per_pixel = 10;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	u64 data_rate = 0;
	struct seninf_core *core = ctx->core;
	u64 csi_clk = SENINF_CK;
	u64 temp;
	u64 ui_224;

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	csi_clk = core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk ?
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
				dev_info(ctx->dev, "cphy settle val: (%u, %llu) => %llu\n",
					 CPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
			} else {
				temp = csi_clk * settle_delay_dt;

				if (temp % 1000000000)
					settle_delay_dt = 1 + (temp / 1000000000);
				else
					settle_delay_dt = (temp / 1000000000);
			}
		}
		settle_delay_ck = settle_delay_dt;
	} else {
		if (!ctx->csi_param.not_fixed_dphy_settle) {
			settle_delay_dt = settle_delay_ck = settle_formula(DPHY_SETTLE_DEF, csi_clk);
			settle_delay_ck = 0;
			seninf_logi(ctx, "dphy settle val: (%u, %llu) => %llu\n",
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
					settle_delay_dt = settle_formula(DPHY_SETTLE_DEF, csi_clk);
					seninf_logi(ctx, "dphy settle val: (%u, %llu) => %llu\n",
						 DPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
				} else {
					temp = csi_clk * settle_delay_dt;

					if (temp % 1000000000)
						settle_delay_dt = 1 + (temp / 1000000000);
					else
						settle_delay_dt = (temp / 1000000000);
				}
				settle_delay_ck = ctx->csi_param.dphy_clk_settle;
				if (settle_delay_ck == 0) {
					settle_delay_ck = settle_formula(DPHY_SETTLE_DEF, csi_clk);
					seninf_logi(ctx, "dphy settle val: (%u, %llu) => %llu\n",
						 DPHY_SETTLE_DEF, csi_clk, settle_delay_dt);
				} else {
					temp = csi_clk * settle_delay_ck;

					if (temp % 1000000000)
						settle_delay_ck = 1 + (temp / 1000000000);
					else
						settle_delay_ck = (temp / 1000000000);
				}
			}
		}
	}

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_SETTLE_PARAMETER, settle_delay_dt);
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_SETTLE_PARAMETER, settle_delay_ck);
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER, RG_DPHY_RX_LC1_HS_SETTLE_PARAMETER, settle_delay_ck);

	hs_option = ctx->csi_param.clk_lane_no_initial_flow;
	SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_OPTION, hs_option);

	/*Settle delay by lane*/
	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_PREPARE_PARAMETER, 0);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_PREPARE_PARAMETER, 0);

	if (ctx->csi_param.not_fixed_trail_settle) {
		hs_trail = ctx->csi_param.dphy_trail
			? ctx->csi_param.dphy_trail
			: ctx->hs_trail_parameter;
	} else {
		temp = 0;
		ui_224 = 0;
		hs_trail = 0;
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		do_div(data_rate, ctx->num_data_lanes);
		ui_224 = (DPHY_TRAIL_SPEC * 1000) / (data_rate / 1000000);

		if (ctx->csi_param.dphy_trail == 0 || ctx->csi_param.dphy_trail > ui_224)
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

	SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_PARAMETER, hs_trail);
	SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_PARAMETER, hs_trail);

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
			if (ctx->csi_param.dphy_trail != 0 && hs_trail != 0) {
				hs_trail_en = 1;
				dev_info(ctx->dev, "hs_trail = %llu\n", hs_trail);
			} else
				hs_trail_en = 0;
		}
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, hs_trail_en);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, hs_trail_en);

		/* Enable rx_deskew and disable rx_deskew delay for lane 0~3 */
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, DPHY_RX_DESKEW_LP11_IDLE_EN, 1);
		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_ACC_MODE, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL, DPHY_RX_DESKEW_L0_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL, DPHY_RX_DESKEW_L1_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL, DPHY_RX_DESKEW_L2_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL, DPHY_RX_DESKEW_L3_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_PREPARE_EN, 0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_PREPARE_EN, 0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_PREPARE_EN, 0x01);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_PREPARE_EN, 0x01);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_PREPARE_PARAMETER, 0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_PREPARE_EN, 0x1);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER, RG_DPHY_RX_LC1_HS_PREPARE_PARAMETER, 0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER, RG_DPHY_RX_LC1_HS_PREPARE_EN, 0x1);
	} else {
		SENINF_BITS(base, DPHY_RX_DATA_LANE0_HS_PARAMETER, RG_CDPHY_RX_LD0_TRIO0_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE1_HS_PARAMETER, RG_CDPHY_RX_LD1_TRIO1_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE2_HS_PARAMETER, RG_CDPHY_RX_LD2_TRIO2_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_DATA_LANE3_HS_PARAMETER, RG_CDPHY_RX_LD3_TRIO3_HS_TRAIL_EN, 0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE0_HS_PARAMETER, RG_DPHY_RX_LC0_HS_PREPARE_EN, 0x0);
		SENINF_BITS(base, DPHY_RX_CLOCK_LANE1_HS_PARAMETER, RG_DPHY_RX_LC1_HS_PREPARE_EN, 0x0);
	}

	return 0;
}

static int csirx_cphy_init(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];

	SENINF_BITS(base, CPHY_RX_DETECT_CTRL_POST, RG_CPHY_RX_DATA_VALID_POST_EN, 1);

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

static int seninf_async_setting(struct seninf_ctx *ctx)
{
	// Always on async cg to avoid write racing between aov scp side

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
	seninf_logd(ctx, "ctx->port %d\n", ctx->port);

	/* Select share bus option */
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL, RG_8PIX_SHARE_16PIX_DATA,
				(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 1 : 0);

	/* Enable C / D phy */
	if (ctx->is_cphy) {
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0, PHY_SENINF_MUX0_DPHY_EN, 0);
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0, PHY_SENINF_MUX0_CPHY_EN, 1);
		/* Select split mode */
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0, RG_PHY_SENINF_MUX0_CPHY_MODE,
					(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 2 : 0);
	} else {
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0, PHY_SENINF_MUX0_CPHY_EN, 0);
		SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_PHY_CTRL_CSI0, PHY_SENINF_MUX0_DPHY_EN, 1);
	}

	return 0;
}

static int csirx_mac_csi_checker_v1(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	/* Reset VC/DT selection and disable */
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 0);
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 1);
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 2);
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 3);
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 4);
	RESET_MAC_CHECKER_V1(csirx_mac_csi, 5);

	/* Clear status and IRQ status */
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 0);
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 1);
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 2);
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 3);
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 4);
	CLEAR_MAC_CHECKER_IRQ_V1(csirx_mac_csi, 5);

	/* Set VC/DT selection and enable */
	SET_MAC_CHECKER_V1(csirx_mac_csi, 0, 0, 0x2b, 0, 0);
	SET_MAC_CHECKER_V1(csirx_mac_csi, 1, 1, 0x2b, 0, 0);
	SET_MAC_CHECKER_V1(csirx_mac_csi, 2, 2, 0x2b, 0, 0);
	SET_MAC_CHECKER_V1(csirx_mac_csi, 3, 3, 0x2b, 0, 0);
	SET_MAC_CHECKER_V1(csirx_mac_csi, 4, 0, 0x30, 0, 0);
	SET_MAC_CHECKER_V1(csirx_mac_csi, 5, 1, 0x30, 0, 0);

	return 0;
}

static int csirx_mac_csi_checker_v2(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	/* Reset VC/DT selection and disable */
	RESET_MAC_CHECKER_V2(csirx_mac_csi, 0);
	RESET_MAC_CHECKER_V2(csirx_mac_csi, 1);
	RESET_MAC_CHECKER_V2(csirx_mac_csi, 2);
	RESET_MAC_CHECKER_V2(csirx_mac_csi, 3);
	RESET_MAC_CHECKER_V2(csirx_mac_csi, 5);

	/* Clear status and IRQ status */
	CLEAR_MAC_CHECKER_IRQ_V2(csirx_mac_csi, 0);
	CLEAR_MAC_CHECKER_IRQ_V2(csirx_mac_csi, 1);
	CLEAR_MAC_CHECKER_IRQ_V2(csirx_mac_csi, 2);
	CLEAR_MAC_CHECKER_IRQ_V2(csirx_mac_csi, 3);
	CLEAR_MAC_CHECKER_IRQ_V2(csirx_mac_csi, 5);

	/* Set VC/DT selection and enable */
	SET_MAC_CHECKER_V2(csirx_mac_csi, 0, 0, 0x2b, 0, 0);
	SET_MAC_CHECKER_V2(csirx_mac_csi, 1, 1, 0x2b, 0, 0);
	SET_MAC_CHECKER_V2(csirx_mac_csi, 2, 2, 0x2b, 0, 0);
	SET_MAC_CHECKER_V2(csirx_mac_csi, 3, 0, 0x30, 0, 0);
	SET_MAC_CHECKER_V2(csirx_mac_csi, 5, 1, 0x30, 0, 0);

	return 0;
}

static int csirx_mac_csi_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	int bit_per_pixel = 10;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	u64 data_rate = 0;
	u8 map_hdr_len[] = {0, 1, 2, 4, 5};
	u8 dt = 0;

	if (vc) {
		bit_per_pixel = vc->bit_depth;
		dt = vc->dt;
	} else if (vc1) {
		bit_per_pixel = vc1->bit_depth;
		dt = vc1->dt;
	}

#ifdef DOUBLE_PIXEL_EN
	/* enable raw8 pixel double for raw8 data, only impact dt = 0x2a */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_RAW8_PIXEL_DOUBLE, 1);
#endif

	/* select C / D phy */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_CPHY_SEL, (ctx->is_cphy) ? 1 : 0);
	/* Select CSI2 8p/16p pixel mode */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_8_16_PIXEL_SEL,
				(ctx->port >= CSI_PORT_MIN_SPLIT_PORT) ? 1 : 0);

	/* Write fixed setting */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_ECC_EN, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_B2P_EN, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_IMG_PACKET_EN, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_SPEC_V2P0_SEL, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_MULTI_FRAME_VLD_EN, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_MODE, 0);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_OUTPUT_LEN_SEL, 0);
	/* mt6899 only start */
	if (dt == 0x27) {
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_R24USERDEF_DT, RG_CSI2_RAW24LIKE_USERDEF_DT, 0x27);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_R24USERDEF_DT, RG_CSI2_USERDEF_DT_EN, 1);
	}
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_VS_1T, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_CSI2_EARLY_VSYNC, 1);
#ifdef DOUBLE_PIXEL_EN
	/* enable pixel double for ext dt data, impact dt = 0x18/1a/1c/1e/20~26/28/29 */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_OPT, RG_EXTDT_PIXEL_DOUBLE, 1);
#endif
	/* mt6899 only end */

	/* enable resync cycle cnt */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT, 0x1f);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_CYCLE_CNT_OPT, 1);

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
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		case 2:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		case 3:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		case 4:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n", __func__, ctx->num_data_lanes);
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
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		case 2:
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE3_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE2_EN, 0);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE1_EN, 1);
			SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_EN, CSI2_LANE0_EN, 1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n", __func__, ctx->num_data_lanes);
		}
		break;

	case CSI_PORT_0B:
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
		dev_info(ctx->dev, "[%s] ctx->port %d is invalid\n", __func__, ctx->port);
		break;
	}

	/* Disable CSI2 interrupt */
	SENINF_WRITE_REG(csirx_mac_csi, CSIRX_MAC_CSI2_IRQ_EN, 0x80000000);
	/* Enable packet cnt */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_DBG_CTRL, RG_CSI2_DBG_PACKET_CNT_EN, 1);

	/* Disable resync dummy valid */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_DMY_CYCLE, 0);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_DMY_CNT, 0);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_DMY_EN, 0);

	if (!ctx->is_cphy) { //Dphy
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		do_div(data_rate, ctx->num_data_lanes);
		dev_info(ctx->dev, "[%s] pixel_rate(%lldpps) data_rate(%lldbps/lane)\n",
			__func__, ctx->mipi_pixel_rate, data_rate);

		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_HDR_MODE_0, RG_CSI2_HEADER_MODE, 0);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_HDR_MODE_0, RG_CSI2_HEADER_LEN, 0);
	} else { //Cphy
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel * 7;
		do_div(data_rate, ctx->num_data_lanes * 16);
		dev_info(ctx->dev, "[%s] pixel_rate(%lldpps) data_rate(%lldsps/trio)\n",
			__func__, ctx->mipi_pixel_rate, data_rate);

		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_HDR_MODE_0, RG_CSI2_HEADER_MODE, 2);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_HDR_MODE_0, RG_CSI2_HEADER_LEN,
			map_hdr_len[(unsigned int)ctx->num_data_lanes]);
	}

	/* Set mac checker */
	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		csirx_mac_csi_checker_v1(ctx);
		dev_info(ctx->dev, "[%s] mac checker v1\n", __func__);
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		csirx_mac_csi_checker_v2(ctx);
		dev_info(ctx->dev, "[%s] mac checker v2\n", __func__);
	} else
		dev_info(ctx->dev, "[%s] warning: iomem_ver is invalid. mac checker is not set.\n", __func__);

	/* Enable BER */
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_EN, 0);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_CLR, 1);
	mdelay(1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_CLR, 0);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_EN, 1);
	SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_IRQ_THRESHOLD, 5);

	return 0;
}

static int csirx_mac_csi_lrte_setting(struct seninf_ctx *ctx)
{
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	void *cphy_base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];

	seninf_logd(ctx, "lrte_support flag = %d\n",
			ctx->csi_param.cphy_lrte_support);

	if (ctx->is_cphy && ctx->csi_param.cphy_lrte_support) {
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_LRTE_EN, 1);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_RESYNC_LRTE_PKT_HSRST, 0);
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2, RG_RESYNC_LRTE_WPTR_LENGTH, 2);
		/* LRTE SW Workaround */
		SENINF_BITS(dphy_base, DPHY_RX_SPARE1, RG_POST_CNT, 0x1);
		SENINF_BITS(cphy_base, CPHY_RX_STATE_CHK_EN, RG_ALP_POS_DET_MASK, 0xFF);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_SETTLE_PARAMETER, 0x23);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_ALP_RX_EN_SEL, 0x0);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN, 0x1);
		SENINF_BITS(cphy_base, CPHY_RX_INIT, RG_CPHY_CSI2_TINIT_CNT_EN, 0x1);
		SENINF_BITS(cphy_base, CPHY_POST_ENCODE, CPHY_POST_REPLACE_EN, 0x0);
		seninf_logi(ctx,
			"LRTE POST_CNT(0x%x),ALP_POS_DET_MASK(0x%x),ALP_SETTLE_PARAMETER(0x%x),ALP_RX_EN_SEL(0x%x),CPHY_ALP_EN(0x%x),CPHY_CSI2_TINIT_CNT_EN(0x%x)\n",
			SENINF_READ_BITS(dphy_base, DPHY_RX_SPARE1, RG_POST_CNT),
			SENINF_READ_BITS(cphy_base, CPHY_RX_STATE_CHK_EN, RG_ALP_POS_DET_MASK),
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_SETTLE_PARAMETER),
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_ALP_RX_EN_SEL),
			SENINF_READ_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN),
			SENINF_READ_BITS(cphy_base, CPHY_RX_INIT, RG_CPHY_CSI2_TINIT_CNT_EN));
	} else {
		SENINF_BITS(csirx_mac_csi, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL, RG_CSI2_RESYNC_LRTE_EN, 0);
		SENINF_BITS(cphy_base, CPHY_RX_CAL_ALP_CTRL, RG_CPHY_ALP_EN, 0x0);
		seninf_logd(ctx, "lrte not support, disable LRTE_EN ALP_EN, port:%d\n",
			 ctx->port);
	}

	return 0;
}

static void csirx_phyA_dphy_setting(void *base, u64 data_rate)
{
	u32 en_16bit_mode = (data_rate > 4500000000) ? 0 : 1;
	/* set CDPHY 16/32bit mode */
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_CDPHY_16BIT_SEL, en_16bit_mode);
	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		/* data rate < 1.5 Gbps */
		if (data_rate < 1500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
		}
		/* 1.5 Gbps <= data date < 2.5 Gbps */
		else if (data_rate < 2500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
		}
		/* 2.5 Gbps <= data date < 4.5 Gbps */
		else if (data_rate < 4500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
		}
		/* 4.5 Gbps <= data date < 6.5 Gbps */
		else if (data_rate < 6500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
		}
		/* 6.5 Gbps <= data date */
		else {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
		}
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* data rate < 2.5 Gbps */
		if (data_rate < 2500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x680 | (en_16bit_mode<<8));
		}
		/* 2.5 Gbps <= data date < 4.5 Gbps */
		else if (data_rate < 4500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x680 | (en_16bit_mode<<8));
		}
		/* 4.5 Gbps <= data date */
		else {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x600 | (en_16bit_mode<<8));
		}
	}
}

static void csirx_phyA_cphy_setting(void *base, u64 data_rate)
{
	u32 en_16bit_mode = (data_rate > 4500000000) ? 0 : 1;
	/* set CDPHY 16/32bit mode */
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_16BIT_EN, en_16bit_mode);
	SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_CDPHY_16BIT_SEL, en_16bit_mode);
	SENINF_WRITE_REG(base, CDPHY_RX_ANA_SETTING_0, 0x322);
	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_LDO_X26M_EN, 0x0);
		SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_LDO_LP_EN, 0x1);
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_EQ_DES_VREF_SEL, 0x14);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_EN, 1);
		/* data rate < 2.5 Gsps */
		if (data_rate < 2500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T0_CDR_SEL_CODE, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T1_CDR_SEL_CODE, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x1);
		}
		/* 2.5 Gsps<= data rate < 4.5 Gsps */
		else if (data_rate < 4500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T0_CDR_SEL_CODE, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T1_CDR_SEL_CODE, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
		}
		/* 4.5 Gsps<= data rate < 6.37 Gsps */
		else {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T0_CDR_SEL_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_13, RG_CSI0_CPHY_T1_CDR_SEL_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_CDPHY_EQ_OS_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS_RDC, 0x0);
		}
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		SENINF_BITS(base, CDPHY_RX_ANA_3, RG_CSI0_EQ_DES_VREF_SEL, 0x2E);
		SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_EN, 1);
		/* data rate < 2.5 Gsps */
		if (data_rate < 2500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x9);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0xA);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_RSTB_CODE, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_RSTB_CODE, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x600 | (en_16bit_mode<<8));
		}
		/* 2.5 Gsps<= data rate < 4.5 Gsps */
		else if (data_rate < 4500000000) {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_RSTB_CODE, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_RSTB_CODE, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, 0x6);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x680 | (en_16bit_mode<<8));
		}
		/* 4.5 Gsps<= data rate < 6.37 Gsps */
		else {
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_BW, 0x3);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG0_EN, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_DG1_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR0, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_SR1, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_6, RG_CSI0_CPHY_T0_CDR_CK_DELAY, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_AB_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_BC_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CA_WIDTH, 0x8);
			SENINF_BITS(base, CDPHY_RX_ANA_7, RG_CSI0_CPHY_T1_CDR_CK_DELAY, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_RSTB_CODE, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_RSTB_CODE, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_4, RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, 0x2);
			SENINF_BITS(base, CDPHY_RX_ANA_5, RG_CSI0_CDPHY_EQ_IS, 0x1);
			SENINF_BITS(base, CDPHY_RX_ANA_8, RG_CSI0_RESERVE, 0x600 | (en_16bit_mode<<8));
		}
	}
}

static int csirx_phyA_setting(struct seninf_ctx *ctx)
{
	void *base, *baseA, *baseB, *dphy_base;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	int bit_per_pixel = 10;
	u64 data_rate;
	u32 pn_swap_en;

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	base = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];
	baseA = ctx->reg_ana_csi_rx[(unsigned int)ctx->portA];
	baseB = ctx->reg_ana_csi_rx[(unsigned int)ctx->portB];
	dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];

	if (!ctx->is_cphy) {
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		pn_swap_en = SENINF_READ_BITS(dphy_base, DPHY_RX_HS_RX_EN_SW, RG_DPHY_PHY_PN_SWAP_EN);
		do_div(data_rate, ctx->num_data_lanes);

		/* baseA works for both A & B */
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_CSI0_ASYNC_OPTION,
				((!pn_swap_en) && (data_rate < 4000000000)) ? 0x4 : 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_NUM,
				((ctx->num_data_lanes > 1)&&(ctx->is_4d1c)) ? 0x6 : 0x9);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x3);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, AFIFO_DUMMY_VALID_GAP_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_SPLIT_EN,
				((ctx->num_data_lanes > 1)&&(ctx->is_4d1c)) ? 0x0 : 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_0, CSR_ASYNC_FIFO_GATING_SEL, 0x2);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_0, DPHY_SYNC_INIT_EXTEND_EN, 0x0);

		if (ctx->is_4d1c) {
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_CPHY_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_CPHY_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKSEL, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKSEL, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKSEL, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKSEL, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(baseA, CDPHY_RX_ANA_14, RG_CSI0_LDO_X26M_EN, 0x0);
			SENINF_BITS(baseA, CDPHY_RX_ANA_14, RG_CSI0_LDO_LP_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ANA_14, RG_CSI0_LDO_X26M_EN, 0x0);
			SENINF_BITS(baseB, CDPHY_RX_ANA_14, RG_CSI0_LDO_LP_EN, 0x1);
			/* DPHY configuration(A/B same) */
			csirx_phyA_dphy_setting(baseA, data_rate);
			csirx_phyA_dphy_setting(baseB, data_rate);
		} else {
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKSEL, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKSEL, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKSEL, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L0_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L1_CKMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_DPHY_L2_CKMODE_EN, 0);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T0_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_0, RG_CSI0_CPHY_T1_HSMODE_EN, 1);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_LDO_X26M_EN, 0x0);
			SENINF_BITS(base, CDPHY_RX_ANA_14, RG_CSI0_LDO_LP_EN, 0x1);
			/* DPHY configuration(A/B same) */
			csirx_phyA_dphy_setting(base, data_rate);
		}
	} else {
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		data_rate *= 7;
		do_div(data_rate, ctx->num_data_lanes*16);
		/* baseA works for both A & B */
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_CSI0_ASYNC_OPTION, 0xC);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_EN, 0x1);
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS))
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_NUM,
					(ctx->num_data_lanes > 1) ? 0x6 : 0x0A);
		else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS))
			SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_NUM,
					((ctx->num_data_lanes > 1) && ctx->is_4d1c) ? 0x6 : 0x0A);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, AFIFO_DUMMY_VALID_GAP_NUM, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_AFIFO_DUMMY_VALID_PREPARE_NUM, 0x0);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, AFIFO_DUMMY_VALID_DESKEW_EN, 0x1);
		SENINF_BITS(baseA, CDPHY_RX_ANA_SETTING_1, RG_SPLIT_EN, (ctx->is_4d1c) ? 0x0 : 0x1);
		if (ctx->is_4d1c) {
			/* CPHY configuration(A/B same) */
			csirx_phyA_cphy_setting(baseA, data_rate);
			csirx_phyA_cphy_setting(baseB, data_rate);
		} else {
			/* CPHY configuration(A/B same) */
			csirx_phyA_cphy_setting(base, data_rate);
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

static int debug_init_deskew_begin_end_apply_code(struct seninf_ctx *ctx)
{
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	unsigned int lane_idx;
	unsigned int apply_code = 0;
	unsigned int begin_end_code = 0;

	SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_MODE_SELECT, 0x1111);

	for (lane_idx = CSIRX_LANE_A0; lane_idx < CSIRX_LANE_MAX_NUM; lane_idx++) {
		if (lane_idx == CSIRX_LANE_A0) {
			seninf_logi(ctx, "CSIRX_LANE_A0\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x03020100);
		} else if(lane_idx == CSIRX_LANE_A1) {
			seninf_logi(ctx, "CSIRX_LANE_A1\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x1b1a1918);
		} else if(lane_idx == CSIRX_LANE_A2) {
			seninf_logi(ctx, "CSIRX_LANE_A2\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x33323130);
		} else if(lane_idx == CSIRX_LANE_B0) {
			seninf_logi(ctx, "CSIRX_LANE_B0\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x4b4a4948);
		} else if(lane_idx == CSIRX_LANE_B1) {
			seninf_logi(ctx, "CSIRX_LANE_B1\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x63626160);
		} else if(lane_idx == CSIRX_LANE_B2) {
			seninf_logi(ctx, "CSIRX_LANE_B2\n");
			SENINF_WRITE_REG(dphy_base, CDPHY_DEBUG_PORT_SELECT, 0x7b7a7978);
		} else {
			seninf_logi(ctx, "ERROR lane\n");
		}
		SENINF_WRITE_REG(dphy_base, DPHY_RX_DESKEW_DBG_MUX, 0x1);
		udelay(5);
		apply_code = SENINF_READ_REG(dphy_base, CDPHY_DEBUG_OUT_READ);
		seninf_logi(ctx, "read delay code CDPHY_DEBUG_OUT_READ = 0x%x, apply_code = %d\n",
			apply_code, (apply_code  >> 24 & 0x3f));
		SENINF_WRITE_REG(dphy_base, DPHY_RX_DESKEW_DBG_MUX, 0x8);
		udelay(5);
		begin_end_code = SENINF_READ_REG(dphy_base, CDPHY_DEBUG_OUT_READ);
		seninf_logi(ctx, "read begin/end code CDPHY_DEBUG_OUT_READ = 0x%x, begin = %d, end = %d\n",
			begin_end_code, ((begin_end_code) >> 8 & 0x3f), ((begin_end_code) >> 16 & 0x3f));
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

	seninf_logd(ctx, "dphy_init_deskew_support = %d\n", ctx->csi_param.dphy_init_deskew_support);

	if (!ctx->csi_param.dphy_init_deskew_support) {
		/* Disable DESKEW LANE0~3 CTRL */
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE0_CTRL, DPHY_RX_DESKEW_L0_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE1_CTRL, DPHY_RX_DESKEW_L1_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE2_CTRL, DPHY_RX_DESKEW_L2_DELAY_EN, 0);
		SENINF_BITS(base, DPHY_RX_DESKEW_LANE3_CTRL, DPHY_RX_DESKEW_L3_DELAY_EN, 0);
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

		if (data_rate > SENINF_DESKEW_DATA_RATE_6500M)
			SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_CODE_UNIT_SEL, 0);
		else if (data_rate > SENINF_DESKEW_DATA_RATE_3200M)
			SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_CODE_UNIT_SEL, 1);
		else
			SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_CODE_UNIT_SEL, 2);
		/* RG_DPHY_RX_DESKEW_CODE_UNIT_SEL */
		/* Date rate > 6.5G :                    set 'b0 */
		/* Date rate < 6.5G & Date rate > 3.2G : set 'b01 */
		/* Date rate < 3.2G :                    set 'b10 */

		SENINF_BITS(base, DPHY_RX_DESKEW_CTRL, RG_DPHY_RX_DESKEW_DELAY_APPLY_OPT, 1);

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
			if (data_rate > SENINF_DESKEW_DATA_RATE_6500M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 13);
			} else if (data_rate > SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 19);
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 36);
			}
			break;
		case SENINF_CLK_416_MHZ:
			if (data_rate > SENINF_DESKEW_DATA_RATE_6500M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 12);
			} else if (data_rate > SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 17);
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 31);
			}
			break;
		case SENINF_CLK_364_MHZ:
			if (data_rate > SENINF_DESKEW_DATA_RATE_6500M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 11);
			} else if (data_rate > SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 16);
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 29);
			}
			break;
		case SENINF_CLK_312_MHZ:
			if (data_rate > SENINF_DESKEW_DATA_RATE_6500M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 10);
			} else if (data_rate > SENINF_DESKEW_DATA_RATE_3200M) {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 14);
			} else {
				SENINF_BITS(base, DPHY_RX_DESKEW_TIMING_CTRL,
					RG_DPHY_RX_DESKEW_SETUP_CNT, 24);
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

		/* enable deskew irq*/
		SENINF_WRITE_REG(base, DPHY_RX_DESKEW_IRQ_EN, 0xffffffff);
		SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
		SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
		SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
		SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);

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
		seninf_aee_print(SENINF_AEE_GENERAL,
			"[%s] Data rate (%llu) < 1.5G, no need deskew", __func__, data_rate);
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

	csi_clk = core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk ?
		(u64)core->cdphy_dvfs_step[ctx->vcore_step_index].csi_clk * CSI_CLK_FREQ_MULTIPLIER :
		SENINF_CK;

	if (ctx->is_4d1c) {
		switch(ctx->num_data_lanes) {
		case 1:
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
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_EN, 0x1);
			break;
		case 2:
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
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
			break;
		case 4:
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
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
			SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_EN, 0x1);
			break;
		default:
			dev_info(ctx->dev, "[%s][ERROR] invalid lane num(%d)\n",
				__func__,
				ctx->num_data_lanes);
			break;
		}
	} else {
		switch(ctx->num_data_lanes) {
		case 1:
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
				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
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
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD1_EN, 1);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD2_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_EN, DPHY_RX_LD3_EN, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LC1_SEL, 4);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD0_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD1_SEL, 3);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD2_SEL, 0);
				SENINF_BITS(base, DPHY_RX_LANE_SELECT, RG_DPHY_RX_LD3_SEL, 0);
				SENINF_BITS(baseB, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
				break;
			default:
				dev_info(ctx->dev, "[%s][ERROR] invalid port(%d on lane %d\n",
				__func__,
				ctx->port,
				ctx->num_data_lanes);
				break;
			}
			break;
		case 2:
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
				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_EN, 0x1);
				SENINF_BITS(baseA, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_EN, 0x1);
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

#ifdef INIT_DESKEW_SUPPORT
	csirx_dphy_init_deskew_setting(ctx, csi_clk);
#endif /* INIT_DESKEW_SUPPORT */

	/* DPHY_RX_IRQ_EN */
	SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
	SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
	SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);
	SENINF_BITS(base, DPHY_RX_IRQ_EN, RG_DPHY_RX_ERR_SOT_SYNC_HS_L0_IRQ_EN, 1);

	dev_info(ctx->dev,
			"[%s][Done] with is_4d1c(%d),num_data_lanes(%d),port(%d),DPHY_RX_IRQ_EN(0x%08x)\n",
			__func__, ctx->is_4d1c, ctx->num_data_lanes, ctx->port,
			SENINF_READ_REG(base, DPHY_RX_IRQ_EN));

	return 0;
}

static int csirx_cphy_setting(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_cphy_top[(unsigned int)ctx->port];
	void *dphy_base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];

	switch (ctx->port) {
	case CSI_PORT_0:
	case CSI_PORT_1:
	case CSI_PORT_2:
	case CSI_PORT_3:
	case CSI_PORT_4:
	case CSI_PORT_5:
	case CSI_PORT_0A:
	case CSI_PORT_1A:
	case CSI_PORT_2A:
	case CSI_PORT_3A:
	case CSI_PORT_4A:
	case CSI_PORT_5A:

		if (ctx->num_data_lanes == 3) {
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR1_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR2_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO1_VALID_PIPE_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO2_VALID_PIPE_EN, 1);
		} else if (ctx->num_data_lanes == 2) {
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR1_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO1_VALID_PIPE_EN, 1);
		} else {
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR0_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO0_VALID_PIPE_EN, 1);
		}
		break;
	case CSI_PORT_0B:
	case CSI_PORT_1B:
	case CSI_PORT_2B:
	case CSI_PORT_3B:
	case CSI_PORT_4B:
	case CSI_PORT_5B:

		if (ctx->num_data_lanes == 1) {
			SENINF_BITS(base, CPHY_RX_CTRL, CPHY_RX_TR2_LPRX_EN, 1);
			SENINF_BITS(base, CPHY_VALID_PIPE_EN, CPHY_TRIO2_VALID_PIPE_EN, 1);
		} else {
			dev_info(ctx->dev, "[error][%s] invalid ctx->num_data_lanes: %d\n",
				__func__, ctx->num_data_lanes);
		}
		break;
	default:
		dev_info(ctx->dev, "[error][%s] invalid ctx->port: %d\n",
				__func__, ctx->port);
		break;
	}
	if (!ctx->csi_param.legacy_phy)
		SENINF_WRITE_REG(dphy_base, DPHY_RX_SPARE0, 0xf1);
	else
		SENINF_WRITE_REG(dphy_base, DPHY_RX_SPARE0, 0xf0);

	/* CPHY_RX_IRQ_EN */
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR0_ERR_SOT_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR1_ERR_SOT_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR2_ERR_SOT_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR3_ERR_SOT_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR0_ERR_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR1_ERR_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR2_ERR_SYNC_HS_IRQ_EN, 1);
	SENINF_BITS(base, CPHY_RX_IRQ_EN, RG_CPHY_RX_TR3_ERR_SYNC_HS_IRQ_EN, 1);

	dev_info(ctx->dev,
			"[%s][Done] with is_4d1c(%d),num_data_lanes(%d),port(%d),CPHY_RX_IRQ_EN(0x%08x)\n",
			__func__, ctx->is_4d1c, ctx->num_data_lanes, ctx->port,
			SENINF_READ_REG(base, CPHY_RX_IRQ_EN));

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
	csirx_mac_csi_setting(ctx);

	/* cphy lrte */
	csirx_mac_csi_lrte_setting(ctx);

	/* seninf async */
	seninf_async_setting(ctx);
	mtk_cam_seninf_set_async(ctx, ctx->seninfAsyncIdx, !ctx->is_4d1c, ctx->is_test_model);

	/* phy */
	csirx_phy_setting(ctx);

	return 0;
}

static int mtk_cam_seninf_poweroff(struct seninf_ctx *ctx)
{
	mtk_cam_seninf_set_async_cg(ctx, ctx->seninfAsyncIdx, 0);

	if (ctx->is_4d1c) {
		csirx_phyA_power_on(ctx, ctx->portA, 0);
		csirx_phyA_power_on(ctx, ctx->portB, 0);
	} else {
		csirx_phyA_power_on(ctx, ctx->port, 0);
	}

	return 0;
}

static int _reset_seninf(struct seninf_ctx *ctx, int seninfAsyncIdx)
{
	int i;
	//void *pSeninfAsync;
	void *pSeninf_outmux;
	int selAsync, selSensor;

	/* Reset async seninf */
	// ignore reset async

	seninf_logd(ctx, "reset seninf %d\n", seninfAsyncIdx);

	/* Reset outmux */
	for (i = SENINF_OUTMUX0; i < _seninf_ops->outmux_num; i++)
		if (!mtk_cam_get_outmux_sel(ctx, i, &selAsync, &selSensor) &&
			selAsync == seninfAsyncIdx && selSensor == ctx->seninfSelSensor &&
			mtk_cam_seninf_is_outmux_used(ctx, i)) {
			pSeninf_outmux = ctx->reg_if_outmux[i];
			SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_SW_RST,
				    SENINF_OUTMUX_LOCAL_SW_RST, 1);
			udelay(1);
			SENINF_BITS(pSeninf_outmux, SENINF_OUTMUX_SW_RST,
				    SENINF_OUTMUX_LOCAL_SW_RST, 0);
			dev_info(ctx->dev, "Async%d reset outmux %d, selSensor:%d\n",
					selAsync, i, selSensor);
		}

	return 0;
}

static int _reset_csi(struct seninf_ctx *ctx)
{
	void *csirx_mac_top = ctx->reg_csirx_mac_top[(unsigned int)ctx->port];
	void *csirx_mac_csi = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	unsigned int csi_irq = 0;

	/* Reset csi */
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL, SENINF_TOP_SW_RST, 1);
	udelay(1);
	SENINF_BITS(csirx_mac_top, CSIRX_MAC_TOP_CTRL, SENINF_TOP_SW_RST, 0);

	/* clear CSI IRQ status */
	csi_irq = SENINF_READ_REG(csirx_mac_csi, CSIRX_MAC_CSI2_IRQ_STATUS);
	if (csi_irq)
		seninf_logi(ctx, "Current csi irq status(0x%x) non-zero, clear it\n", csi_irq);
	SENINF_WRITE_REG(csirx_mac_csi,
			 CSIRX_MAC_CSI2_IRQ_STATUS,
			 0xffffffff);

	return 0;
}

static int mtk_cam_seninf_reset(struct seninf_ctx *ctx, int seninfAsyncIdx)
{
	_reset_seninf(ctx, seninfAsyncIdx);

	if (!ctx->is_test_model) {
		seninf_logd(ctx, "start reset csi\n");
		_reset_csi(ctx);
	} else
		seninf_logd(ctx, "skip reset csi due to tm mode\n");

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
				mtk_cam_seninf_disable_outmux(ctx, vc->dest[j].outmux, true);
			}
			vc->dest_cnt = 0;
		}
	}
	for (i = 0; i < PAD_MAXCNT; i++)
		for (j = 0; j < MAX_DEST_NUM; j++)
			ctx->pad2cam[i][j] = 0xff;

	/* Clear disabled outmux */
	for (i = 0; i < SENINF_OUTMUX_NUM; i++) {
		if (ctx->outmux_disable_list[i]) {
			// disable outmux if already disabled
			mtk_cam_seninf_disable_outmux(ctx, i, true);
			ctx->outmux_disable_list[i] = false;
		}
	}

	seninf_logi(ctx, "release all outmux set all pd2cam to 0xff\n");

	return 0;
}

static int mtk_cam_seninf_get_mux_meter(struct seninf_ctx *ctx, int mux,
				 struct mtk_cam_seninf_mux_meter *meter)
{
#if PORTING_FIXME
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
#endif

	return 0;
}

static int mtk_cam_seninf_debug_core_dump(struct seninf_ctx *ctx,
				   struct mtk_cam_seninf_debug *debug_result)
{
	int i, j, dbg_timeout = 0;
	int val = 0;
	struct seninf_vc *vc;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct seninf_vc_out_dest *dest;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	void *seninf_top, *csi_mac, *outmux;
	unsigned long debug_ft = FT_30_FPS;
	const unsigned int ft_delay_margin = 3;
	unsigned int vc_vaild_cnt = 0;

	if (unlikely(debug_result == NULL)) {
		seninf_logi(ctx, "debug_result is NULL\n");
		return -EINVAL;
	}

	if (ctx->is_test_model) {
		debug_ft = FT_30_FPS;
	} else {
		ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
							V4L2_CID_MTK_SOF_TIMEOUT_VALUE);
		if (ctrl) {
			val = v4l2_ctrl_g_ctrl(ctrl);
			if (val > 0)
				dbg_timeout = val + (val / 10);
		}

		if (dbg_timeout != 0)
			debug_ft = dbg_timeout / 1000;
	}

	debug_ft += ft_delay_margin;
	mdelay(debug_ft);
	csi_mac = ctx->reg_csirx_mac_csi[(unsigned int)ctx->port];
	seninf_top = ctx->core->reg_seninf_top;
	memset(debug_result, 0, sizeof(struct mtk_cam_seninf_debug));
	debug_result->csi_mac_irq_status = SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	debug_result->seninf_async_irq = SENINF_READ_REG(seninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_STATUS);
	/* wtire clear for enxt frame */
	SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
	debug_result->packet_cnt_status = SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_PACKET_CNT_STATUS);

	for (i = 0; i < ctx->vcinfo.cnt; i++) {
		vc = &ctx->vcinfo.vc[i];
		for (j = 0; (j < vc->dest_cnt) && (j < MAX_DEST_NUM); j++) {
			if (vc_vaild_cnt >= MAX_MUX_VCINFO_DEBUG) {
				seninf_logi(ctx, "vcinfo.cnt is exceed debug limit (%u)\n", MAX_MUX_VCINFO_DEBUG);
				return -EINVAL;
			}
			dest = &vc->dest[j];
			if (dest->outmux >= SENINF_OUTMUX_NUM) {
				seninf_logi(ctx,
					"dest->outmux (%u) is invalid and larger than SENINF_OUTMUX_NUM (%u)\n",
					dest->outmux, SENINF_OUTMUX_NUM);
				return -EINVAL;
			}
			outmux = ctx->reg_if_outmux[dest->outmux];
			if (!outmux) {
				seninf_logi(ctx, "dest->outmux (%u) is invalid\n", dest->outmux);
				return -EINVAL;
			}
			vcinfo_debug = &debug_result->vcinfo_debug[vc_vaild_cnt];
			vcinfo_debug->outmux_id = dest->outmux;
			vcinfo_debug->vc_feature = vc->feature;
			vcinfo_debug->tag_id = dest->tag;
			vcinfo_debug->vc = vc->vc;
			vcinfo_debug->dt = vc->dt;
			vcinfo_debug->ref_vsync_irq_status = SENINF_READ_BITS(outmux,
				SENINF_OUTMUX_IRQ_STATUS, SENINF_OUTMUX_REF_VSYNC_IRQ_STATUS);
			switch(dest->tag) {
			case 0:
				DUMP_DEBUG_REG_INFO_BY_TAG(0);
				break;
			case 1:
				DUMP_DEBUG_REG_INFO_BY_TAG(1);
				break;
			case 2:
				DUMP_DEBUG_REG_INFO_BY_TAG(2);
				break;
			case 3:
				DUMP_DEBUG_REG_INFO_BY_TAG(3);
				break;
			case 4:
				DUMP_DEBUG_REG_INFO_BY_TAG(4);
				break;
			case 5:
				DUMP_DEBUG_REG_INFO_BY_TAG(5);
				break;
			case 6:
				DUMP_DEBUG_REG_INFO_BY_TAG(6);
				break;
			case 7:
				DUMP_DEBUG_REG_INFO_BY_TAG(7);
				break;
			}
			seninf_logi(ctx, "[%d] vc_feature %d vc 0x%x dt 0x%x outmux %d, tag %d pixmode:0x%08x\n",
					i,
					vcinfo_debug->vc_feature,
					vcinfo_debug->vc,
					vcinfo_debug->dt,
					vcinfo_debug->outmux_id,
					vcinfo_debug->tag_id,
					SENINF_READ_REG(outmux, SENINF_OUTMUX_PIX_MODE)
					);
			seninf_logi(ctx, "done_irq %d, exp %dx%d\n",
					vcinfo_debug->done_irq_status,
					vcinfo_debug->exp_size_h,
					vcinfo_debug->exp_size_v);
			vc_vaild_cnt++;
			debug_result->valid_result_cnt = vc_vaild_cnt;
			SENINF_WRITE_REG(outmux, SENINF_OUTMUX_IRQ_STATUS, 0x00000002);
		}
	}

	return 0;
}

/*static */int mtk_cam_dbg_fmeter(struct seninf_core *core, char *out_str, size_t out_str_sz)
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

static int calculate_bit_error_rate(struct seninf_ctx *ctx)
{
	void *base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	struct mtk_cam_seninf_bit_error *ber = &ctx->ber;
	u64 min_cycle = 0;
	u64 max_cycle = 0;
	int bit_per_pixel = 10;
	u16 hsize = 0;
	u16 vsize = 0;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);

	if (!SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_CTRL, RG_CSI2_BIT_ERR_CNT_EN)) {
		dev_info(ctx->dev, "warning: BER disabled\n");
		return -1;
	}

	memset(ber, 0, sizeof(struct mtk_cam_seninf_bit_error));
	ber->seninf_clk_mhz = SENINF_CK / 1000000;

	if (vc) {
		bit_per_pixel = vc->bit_depth;
		hsize = vc->exp_hsize;
		vsize = vc->exp_vsize;
	} else if (vc1) {
		bit_per_pixel = vc1->bit_depth;
		hsize = vc1->exp_hsize;
		vsize = vc1->exp_vsize;
	}

	ber->bit_err_ctrl = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_CTRL);
	ber->bit_err_cnt = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_CNT);
	ber->min_cycle_lsb = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_MIN_CYCLE_LSB);
	ber->min_cycle_msb = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_MIN_CYCLE_MSB);
	ber->max_cycle_lsb = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_MAX_CYCLE_LSB);
	ber->max_cycle_msb = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_BIT_ERR_MAX_CYCLE_MSB);

	min_cycle = ((u64)ber->min_cycle_msb << 32) | ber->min_cycle_lsb;
	max_cycle = ((u64)ber->max_cycle_msb << 32) | ber->max_cycle_lsb;

	if (!ctx->is_cphy)
		ber->bit_rate_mhz = hsize * vsize / 1000 * ctx->fps_n / ctx->fps_d * bit_per_pixel / 1000;
	else
		ber->bit_rate_mhz = hsize * vsize / 1000 * ctx->fps_n / ctx->fps_d * bit_per_pixel * 7 / 16 / 1000;

	if (min_cycle == 0xFFFFFFFFFFFF)
		min_cycle = 0;

	ber->min_bit = min_cycle * ber->bit_rate_mhz;
	do_div(ber->min_bit, ber->seninf_clk_mhz);

	ber->max_bit = max_cycle * ber->bit_rate_mhz;
	do_div(ber->max_bit, ber->seninf_clk_mhz);

	return 0;
}

static int calculate_cphy_lrte_spacer(struct seninf_ctx *ctx)
{
	void *base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	struct mtk_cam_seninf_spacer_detector *lrte_sd = &ctx->lrte_sd;

	lrte_sd->valid_cnt = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_2, RO_CSI2_SPACER_DET_VALID_CNT);
	lrte_sd->num_hs1 = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_1, RO_CSI2_SPACER_DET_HS1);
	lrte_sd->num_hs2 = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_1, RO_CSI2_SPACER_DET_HS2);
	lrte_sd->wc = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_1, RO_CSI2_SPACER_DET_WC);
	lrte_sd->vc = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_2, RO_CSI2_SPACER_DET_VC);
	lrte_sd->dt = SENINF_READ_BITS(base_csi_mac, CSIRX_MAC_SPACER_DET_1, RO_CSI2_SPACER_DET_DT);
	lrte_sd->trio = ctx->num_data_lanes;
	lrte_sd->spacer =
		(lrte_sd->valid_cnt - 1) * 4 + lrte_sd->num_hs2 + lrte_sd->num_hs1 - 4 -
		(lrte_sd->wc + 1) / 2 / lrte_sd->trio;

	return 0;
}

static ssize_t mtk_cam_seninf_show_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, len;
	struct seninf_core *core;
	struct seninf_ctx *ctx;
	struct media_link *link;
	struct media_pad *pad;
	struct mtk_cam_seninf_debug debug_result;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	void *rx, *base_ana, *csi_mac, *OutMux;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);

	core = dev_get_drvdata(dev);
	len = 0;

	mutex_lock(&core->mutex);

	if (fmeter_dbg && mtk_cam_dbg_fmeter(core, fmeter_dbg, sizeof(char) * 256) == 0)
		SHOW(buf, len, "\n%s\n", fmeter_dbg);

	kfree(fmeter_dbg);

	list_for_each_entry(ctx, &core->list, list) {
		SHOW(buf, len,
			"\n[%s] port %d intf %d test %d cphy %d lanes %d\n",
			ctx->subdev.name,
			ctx->port,
			ctx->seninfAsyncIdx,
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

		if (mtk_cam_seninf_debug_core_dump(ctx, &debug_result)) {
			SHOW(buf, len, "mtk_cam_seninf_debug_core_dump return failed\n");
			return len;
		}

		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		rx = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
		base_ana = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];
		SHOW(buf, len,
			"csirx_mac_csi2 irq_stat 0x%08x, irq_g1_stat 0x%08x, seninf_async_irq 0x%08x\n",
		     debug_result.csi_mac_irq_status,
			 SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_G1_STATUS),
			 debug_result.seninf_async_irq);

		if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 0);
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 1);
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 2);
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 3);
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 4);
			SHOW_MAC_CHECKER_V1(buf, len, csi_mac, 5);
		} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			SHOW_MAC_CHECKER_V2(buf, len, csi_mac, 0);
			SHOW_MAC_CHECKER_V2(buf, len, csi_mac, 1);
			SHOW_MAC_CHECKER_V2(buf, len, csi_mac, 2);
			SHOW_MAC_CHECKER_V2(buf, len, csi_mac, 3);
			SHOW_MAC_CHECKER_V2(buf, len, csi_mac, 5);
		}

		if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			/* Bit Error Rate (BER) */
			calculate_bit_error_rate(ctx);
			SHOW(buf, len,
				"csirx_mac_csi2 BIT_ERR_CTRL/MIN_CYCLE/MAX_CYCLE:(0x%08x)/(0x%08x%08x)/(%08x%08x)\n",
				ctx->ber.bit_err_ctrl,
				ctx->ber.min_cycle_msb, ctx->ber.min_cycle_lsb,
				ctx->ber.max_cycle_msb, ctx->ber.max_cycle_lsb);
			SHOW(buf, len, "seninf_ck(%uMHz) bit_rate(%uMHz)\n",
				ctx->ber.seninf_clk_mhz, ctx->ber.bit_rate_mhz);
			SHOW(buf, len, "bit_err_cnt <%u> bit_err_rate(1/[%llu,%llu])\n",
				ctx->ber.bit_err_cnt, ctx->ber.min_bit, ctx->ber.max_bit);
			if (ctx->ber.min_bit && ctx->ber.min_bit < 1000000000000)
				SHOW(buf, len, "WARN: max(BER) > 10^-12\n");
			if (ctx->ber.max_bit && ctx->ber.max_bit < 1000000000000)
				SHOW(buf, len, "WARN: min(BER) > 10^-12\n");
			/* CPHY LRTE Spacer */
			if (ctx->is_cphy && ctx->csi_param.cphy_lrte_support) {
				calculate_cphy_lrte_spacer(ctx);
				SHOW(buf, len,
					"cphy_lrte_spacer <%u> vc(0x%02x) dt(0x%02x) valid_cnt/num_hs1/num_hs2/wc/trio: %u/%u/%u/%u/%u\n",
					ctx->lrte_sd.spacer, ctx->lrte_sd.vc, ctx->lrte_sd.dt, ctx->lrte_sd.valid_cnt,
					ctx->lrte_sd.num_hs1, ctx->lrte_sd.num_hs2, ctx->lrte_sd.wc, ctx->lrte_sd.trio);
			}
		}

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
			"CDPHY_RX_ANA_SETTING_0 0x%08x ANA_SETTING_1 0x%08x  DPHY_RX_SPARE0 0x%08x\n",
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_SETTING_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_SETTING_1),
			SENINF_READ_REG(rx, DPHY_RX_SPARE0));

		SHOW(buf, len,
			"CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL 0x%08x MERGE_CTRL2 0x%08x\n",
			SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL),
			SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL2));

		SHOW(buf, len,
			"MipiRx_ANA%d:CDPHY_RX_ANA_AD_0/_1:(0x%x)/(0x%x),AD_HS_0/_1/_2:(0x%x)/(0x%x)/(0x%x)\n",
			ctx->port,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_2));

		if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			SHOW(buf, len,
				"MipiRx_ANA%d:CDPHY_RX_ASYM_AFIFO_CTRL_0/_1:(0x%x)/(0x%x)\n",
				ctx->port,
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0),
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_1));
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_FLAG_CLEAR, 1);
		}

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

		if ((SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS) & ~(0x324))) {

			SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
			SHOW(buf, len, "after write clear csi irq 0x%x\n",
			     SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS));
		}

		for (i = 0; i < debug_result.valid_result_cnt; i++) {
			vcinfo_debug = &debug_result.vcinfo_debug[i];
			OutMux = ctx->reg_if_outmux[vcinfo_debug->outmux_id];

			SHOW(buf, len, "[%d] vc 0x%x dt 0x%x outmux %d tag: %d\n",
					i,
					vcinfo_debug->vc,
					vcinfo_debug->dt,
					vcinfo_debug->outmux_id,
					vcinfo_debug->tag_id);

			SHOW(buf, len, "\texp %dx%d\n",
					vcinfo_debug->exp_size_h, vcinfo_debug->exp_size_v);

			SHOW(buf, len, "\tdone_irq 0x%x\n", vcinfo_debug->done_irq_status);
			SHOW(buf, len, "\tref_vsync_irq 0x%x\n", vcinfo_debug->ref_vsync_irq_status);

			SHOW(buf, len, "\tTAG_DBG_PORT 0x%x TAG_CRC_PORT 0x%x, OUTMUX_SW_CFG_DONE 0x%x\n",
				SENINF_READ_REG(OutMux, SENINF_OUTMUX_TAG_DBG_PORT_0),
				SENINF_READ_REG(OutMux, SENINF_OUTMUX_TAG_CRC_PORT_0),
				SENINF_READ_REG(OutMux, SENINF_OUTMUX_SW_CFG_DONE));
		}
	}

	mutex_unlock(&core->mutex);

	return len;
}

static ssize_t mtk_cam_seninf_show_outmux_status(struct device *dev,
				   struct device_attribute *attr, char *buf,
				   unsigned int *outmuxs, unsigned int cnt)
{
	unsigned int i;
	int len;
	struct seninf_core *core;
	void *outmux_outer, *outmux_inner;
	unsigned int outmux_id = 0;

	core = dev_get_drvdata(dev);
	len = 0;

	if (!core->refcnt)
		return len;

	for (i = 0; i < cnt; i++) {
		outmux_id = outmuxs[i];
		if (outmux_id >= _seninf_ops->outmux_num)
			continue;

		outmux_outer = core->reg_seninf_outmux[outmux_id];
		outmux_inner = core->reg_seninf_outmux_inner[outmux_id];

		SHOW(buf, len,
		     "\n[Outer] Outmux%d: CFG_MODE(0x%x),PIX_MODE(0x%x),CFG0/1/2(0x%x/0x%x/0x%x),SRC_SEL(0x%x),CFG_DONE(0x%x),CFG_RDY(0x%x),IRQ_EN(0x%x),IRQ_ST(0x%x)\n",
		     outmux_id,
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SW_CONFIG_MODE),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_PIX_MODE),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SOURCE_CONFIG_0),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SOURCE_CONFIG_1),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SOURCE_CONFIG_2),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SRC_SEL),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_SW_CFG_DONE),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_CAM_CFG_RDY),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_IRQ_EN),
		     SENINF_READ_REG(outmux_outer, SENINF_OUTMUX_IRQ_STATUS));

		DUMP_TAG_REG(outmux_outer, 0);
		DUMP_TAG_REG(outmux_outer, 1);
		DUMP_TAG_REG(outmux_outer, 2);
		DUMP_TAG_REG(outmux_outer, 3);
		DUMP_TAG_REG(outmux_outer, 4);
		DUMP_TAG_REG(outmux_outer, 5);
		DUMP_TAG_REG(outmux_outer, 6);
		DUMP_TAG_REG(outmux_outer, 7);

		SHOW(buf, len,
		     "\n[Inner] Outmux%d: CFG_MODE(0x%x),PIX_MODE(0x%x),CFG0/1/2(0x%x/0x%x/0x%x),SRC_SEL(0x%x),CFG_DONE(0x%x),CFG_RDY(0x%x),IRQ_EN(0x%x),IRQ_ST(0x%x)\n",
		     outmux_id,
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SW_CONFIG_MODE),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_PIX_MODE),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SOURCE_CONFIG_0),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SOURCE_CONFIG_1),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SOURCE_CONFIG_2),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SRC_SEL),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_SW_CFG_DONE),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_CAM_CFG_RDY),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_IRQ_EN),
		     SENINF_READ_REG(outmux_inner, SENINF_OUTMUX_IRQ_STATUS));

		DUMP_TAG_REG(outmux_inner, 0);
		DUMP_TAG_REG(outmux_inner, 1);
		DUMP_TAG_REG(outmux_inner, 2);
		DUMP_TAG_REG(outmux_inner, 3);
		DUMP_TAG_REG(outmux_inner, 4);
		DUMP_TAG_REG(outmux_inner, 5);
		DUMP_TAG_REG(outmux_inner, 6);
		DUMP_TAG_REG(outmux_inner, 7);
	}

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
/*static */bool delay_with_stream_check(struct seninf_ctx *ctx, unsigned long delay)
{
	unsigned long delay_step, delay_inc;

	delay_inc = 0;
	delay_step = 1;
	do {
		if (!ctx->streaming)
			break;

		if (ctx->set_abort_flag) {
			dev_info(ctx->dev, "%s abort\n", __func__);
			return false;
		}
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
	void *base_ana, *base_cphy, *base_dphy, *base_csi_mac;
	unsigned int mac_irq = 0;
	unsigned int seninf_irq = 0;
	int pkg_cnt_changed = 0;
	unsigned int mipi_packet_cnt = 0;
	unsigned int tmp_mipi_packet_cnt = 0;
	unsigned long total_delay = 0;
	unsigned long max_delay = 0;
	int ret = 0;
	int j, i, k;
	unsigned long debug_ft = FT_30_FPS * SCAN_TIME;	// FIXME
	unsigned long debug_vb = 1;	// 1ms for min readout time
	enum CSI_PORT csi_port = CSI_PORT_0;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);
	unsigned int frame_cnt1 = 0, frame_cnt2 = 0;
	unsigned int dphy_irq = 0;
	unsigned int cphy_irq = 0;
	unsigned int temp = 0;
	void *pSeninf_top = ctx->reg_if_top;
	void *pSeninf_asytop = ctx->reg_if_async;

	mtk_cam_sensor_get_frame_cnt(ctx, &frame_cnt1);

	if (ctx->dbg_timeout != 0)
		debug_ft = ctx->dbg_timeout / 1000;

	if (fmeter_dbg && mtk_cam_dbg_fmeter(ctx->core, fmeter_dbg, sizeof(char) * 256) == 0)
		seninf_logi(ctx, "%s\n", fmeter_dbg);
	kfree(fmeter_dbg);

	for (csi_port = CSI_PORT_0A; csi_port <= CSI_PORT_5B; csi_port++) {
		if (csi_port != ctx->portA &&
			csi_port != ctx->portB)
			continue;

		base_ana = ctx->reg_ana_csi_rx[csi_port];

		seninf_logi(ctx,
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
		seninf_logi(ctx,
			"MipiRx_ANA%d:CDPHY_RX_ANA_AD_0/_1:(0x%x)/(0x%x),AD_HS_0/_1/_2:(0x%x)/(0x%x)/(0x%x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_2));
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			seninf_logi(ctx,
				"MipiRx_ANA%d:CDPHY_RX_ASYM_AFIFO_CTRL_0/_1:(0x%x)/(0x%x)\n",
				csi_port - CSI_PORT_0A,
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0),
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_1));
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_FLAG_CLEAR, 1);
		}
	}

	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);

		seninf_logi(ctx,
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
		seninf_logi(ctx,
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
		seninf_logi(ctx,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_IRQ_EN/_CLR/_STATUS:(0x%08x)/(0x%08x)/(0x%08x),DPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_CLR),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_STATUS),
			SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_EN),
			dphy_irq);
		seninf_logi(ctx,
			"Csi%d_Cphy_Top:CPHY_RX_CTRL:(0x%x),CPHY_RX_DETECT_CTRL_POST:(0x%x),CPHY_RX_IRQ_EN/_CLR:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_cphy, CPHY_RX_CTRL),
			SENINF_READ_REG(base_cphy, CPHY_RX_DETECT_CTRL_POST),
			SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_EN),
			cphy_irq);
		/* Clear C/DPHY_IRQ status */
		SENINF_WRITE_REG(base_dphy, DPHY_RX_IRQ_CLR, 0xffffffff);
		SENINF_WRITE_REG(base_cphy, CPHY_RX_IRQ_CLR, 0x0f0f0000);
	}

	/* Check if SENINF_TOP_SW_CFG_LEVEL is 1 or not */
	if (SENINF_READ_BITS(pSeninf_top, SENINF_TOP_CTRL, SENINF_TOP_SW_CFG_LEVEL) == 0)
		seninf_logi(ctx, "warning: SENINF_TOP_SW_CFG_LEVEL is zero\n");

	seninf_logi(ctx,
		"SENINF_TOP:ASYNC_SW_RST(0x%x),OUTMUX_SW_RST(0x%x),ASYNC_CG_EN(0x%x),OUTMUX_CG_EN(0x%x),ASYNC_OVERRUN_EN(0x%x)\n",
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_ASYNC_SW_RST),
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_OUTMUX_SW_RST),
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_ASYNC_CG_EN),
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_OUTMUX_CG_EN),
		SENINF_READ_REG(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_EN));
	seninf_logi(ctx,
		"current async%d:ASYNC_CFG(0x%x),ASYNC0_DBG0(0x%x),ASYNC1_DBG0(0x%x),ASYNC2_DBG0(0x%x),ASYNC3_DBG0(0x%x),ASYNC4_DBG0(0x%x),ASYNC5_DBG0(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_CFG),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_0),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_1),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_2),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_3),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_4),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_5));
	seninf_logi(ctx,
		"current async%d:ASYNC0_DBG1(0x%x),ASYNC1_DBG1(0x%x),ASYNC2_DBG1(0x%x),ASYNC3_DBG1(0x%x),ASYNC4_DBG1(0x%x),ASYNC5_DBG1(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_0),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_1),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_2),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_3),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_4),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_5));
	seninf_logi(ctx,
		"current async%d:BIST_RST0(0x%x),BIST_RST1(0x%x),BIST_RST2(0x%x),BIST_RST3(0x%x),BIST_RST4(0x%x),BIST_RST5(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_0,
				 SENINF_ASYTOP_AFIFO_BIST_RST_0),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_1,
				 SENINF_ASYTOP_AFIFO_BIST_RST_1),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_2,
				 SENINF_ASYTOP_AFIFO_BIST_RST_2),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_3,
				 SENINF_ASYTOP_AFIFO_BIST_RST_3),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_4,
				 SENINF_ASYTOP_AFIFO_BIST_RST_4),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_5,
				 SENINF_ASYTOP_AFIFO_BIST_RST_5));

	/* clear outmux irq */
	for (j = 0; j < ctx->vcinfo.cnt; j++) {
		if (ctx->vcinfo.vc[j].enable) {
			for (k = 0; k < ctx->vcinfo.vc[j].dest_cnt; k++) {
				unsigned int used_outmux = ctx->vcinfo.vc[j].dest[k].outmux;
				unsigned int used_tag = ctx->vcinfo.vc[j].dest[k].tag;

				for (i = 0; i < _seninf_ops->outmux_num; i++) {
					if ((used_outmux == i) && mtk_cam_seninf_is_outmux_used(ctx, i)) {
						u32 filt, filt_in, res, exp_sz, irq_st;

						filt = mtk_cam_seninf_get_outmux_vcdt_filt(ctx, used_outmux,
								used_tag, false);
						filt_in = mtk_cam_seninf_get_outmux_vcdt_filt(ctx, used_outmux,
								used_tag, true);
						res = mtk_cam_seninf_get_outmux_res(ctx, used_outmux, used_tag);
						exp_sz = mtk_cam_seninf_get_outmux_exp(ctx, used_outmux, used_tag);

						irq_st = mtk_cam_seninf_get_outmux_irq_st(ctx, used_outmux, 0);
						seninf_logi(ctx,
							"dump outmux%d,tag%u,CFG_M/PIX_M/CFG0_out-in/CFG1_out-in/CFG2_out-in/SRC_out-in/CFG_DONE/CFG_CTL/CFG_RDY/DBG_PORT0/DBG_PORT1:(0x%x/0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x/0x%x/0x%x/0x%x/0x%x),filt_out/in=(0x%x/0x%x),expSize=0x%x,dbgRecSize=0x%x,irq=0x%x\n",
							i, used_tag,
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SW_CONFIG_MODE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PIX_MODE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_0),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_0),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_1),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_1),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_2),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_2),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SRC_SEL),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SRC_SEL),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SW_CFG_DONE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_CSR_CFG_CTRL),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_CAM_CFG_RDY),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PATH_DBG_PORT_0),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PATH_DBG_PORT_1),
							filt, filt_in, exp_sz, res, irq_st);
					}
				}
			}
		}
	}
	for (j = 0; j < _seninf_ops->outmux_num; j++) {
		unsigned int rdy = 0;
		u32 irq_st = mtk_cam_seninf_get_outmux_irq_st(ctx, j, 1);

		rdy = seninf_get_outmux_rg_val(ctx, j, SENINF_OUTMUX_CAM_CFG_RDY);
		if (ctx->outmux_disable_list[j]) {
			seninf_logi(ctx,
				 "outmux%d marked disable but not cfg done: CFG_M/PIX_M/CFG0/CFG1/CFG2/SRC/CFG_DONE/CFG_CTL/CFG_RDY/DBG_PORT0/DBG_PORT1:(0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x),irq=0x%x\n",
				 j,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CONFIG_MODE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PIX_MODE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_0),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_1),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_2),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SRC_SEL),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CFG_DONE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_CSR_CFG_CTRL),
				 rdy,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PATH_DBG_PORT_0),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PATH_DBG_PORT_1),
				 irq_st);
		} else if (!rdy) {
			seninf_logi(ctx,
				 "outmux%u,CFG_DONE/CFG_RDY:(0x%x/0x%x),irq=0x%x",
				 j,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CFG_DONE),
				 rdy, irq_st);
		}
	}

	/* Seninf_csi status IRQ */
	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	mac_irq = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	seninf_irq = mtk_cam_seninf_get_async_irq_st(ctx, ctx->seninfAsyncIdx, 1);
	temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);
	// always clear irq status and multi-framesync status
	SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
	SENINF_WRITE_REG(base_csi_mac, CSIRX_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS, temp);

	seninf_logi(ctx,
		"CSI-%d,CSIRX_MAC_CSI2_EN/_OPT/_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x)/(0x%x)/(0x%x),SENINF_ASYNC%d_OVERRUN:(0x%x),CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL:(0x%x)\n",
		(uint32_t)ctx->portNum,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_OPT),
		mac_irq,
		temp,
		(uint32_t)ctx->seninfAsyncIdx,
		seninf_irq,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL));

	/* Set mac checker */
	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 4);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 5);
		/* Clear MAC CHECKER status and IRQ status */
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 4);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 5);
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 5);
		/* Clear MAC CHECKER status and IRQ status */
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 5);
	} else
		seninf_logi(ctx, "warning: iomem_ver is invalid. mac checker is not set.\n");

	if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* Bit Error Rate (BER) */
		calculate_bit_error_rate(ctx);
		seninf_logi(ctx,
			"CSI-%d,CSIRX_MAC_CSI2_BIT_ERR_CTRL/MIN_CYCLE/MAX_CYCLE:(0x%08x)/(0x%08x%08x)/(%08x%08x)\n",
			(uint32_t)ctx->portNum, ctx->ber.bit_err_ctrl,
			ctx->ber.min_cycle_msb, ctx->ber.min_cycle_lsb,
			ctx->ber.max_cycle_msb, ctx->ber.max_cycle_lsb);
		seninf_logi(ctx,
			"seninf_ck(%uMHz) bit_rate(%uMHz) bit_err_cnt(%u) bit_err_rate(1/[%llu,%llu])\n",
			ctx->ber.seninf_clk_mhz, ctx->ber.bit_rate_mhz,
			ctx->ber.bit_err_cnt, ctx->ber.min_bit, ctx->ber.max_bit);
		if (ctx->ber.min_bit && ctx->ber.min_bit < 1000000000000)
			seninf_logi(ctx, "WARN: max(BER) > 10^-12\n");
		if (ctx->ber.max_bit && ctx->ber.max_bit < 1000000000000)
			seninf_logi(ctx, "WARN: min(BER) > 10^-12\n");
		/* CPHY LRTE Spacer */
		if (ctx->is_cphy && ctx->csi_param.cphy_lrte_support) {
			calculate_cphy_lrte_spacer(ctx);
			seninf_logi(ctx, "CSI-%d,cphy_lrte_spacer(%u) vc(0x%02x) dt(0x%02x)\n",
				(uint32_t)ctx->portNum, ctx->lrte_sd.spacer, ctx->lrte_sd.vc, ctx->lrte_sd.dt);
			seninf_logi(ctx, "CSI-%d,valid_cnt/num_hs1/num_hs2/wc/trio: %u/%u/%u/%u/%u\n",
				(uint32_t)ctx->portNum, ctx->lrte_sd.valid_cnt,
				ctx->lrte_sd.num_hs1, ctx->lrte_sd.num_hs2, ctx->lrte_sd.wc, ctx->lrte_sd.trio);
		}
	}

	/* Seninf_csi packet count */
	pkg_cnt_changed = 0;
	if (SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN) & 0x1) {
		SENINF_BITS(base_csi_mac, CSIRX_MAC_CSI2_DBG_CTRL,
			    RG_CSI2_DBG_PACKET_CNT_EN, 1);
		mipi_packet_cnt = SENINF_READ_REG(base_csi_mac,
					CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
		max_delay = debug_ft * PKT_CNT_CHK_MARGIN / 100;
		seninf_logi(ctx,
			"total_delay:%lums/%lums,CSI-%d_PkCnt:(0x%x),ret=%d\n",
			total_delay, max_delay, ctx->portNum, mipi_packet_cnt, ret);

		while (total_delay < max_delay) {
			tmp_mipi_packet_cnt = mipi_packet_cnt & 0xFFFF;
			if (!delay_with_stream_check(ctx, debug_vb))
				return ret; // has been stream off
			total_delay += debug_vb;
			mipi_packet_cnt = SENINF_READ_REG(base_csi_mac,
						CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
			if (tmp_mipi_packet_cnt != (mipi_packet_cnt & 0xFFFF)) {
				seninf_logi(ctx,
					"total_delay:%lums/%lums,CSI-%d_PkCnt:(0x%x),ret=%d\n",
					total_delay, max_delay, ctx->portNum, mipi_packet_cnt, ret);
				pkg_cnt_changed = 1;
				break;
			}
		}
	}
	if (!pkg_cnt_changed) {
		ret = -1;
		seninf_logi(ctx,
			"total_delay:%lums/%lums,CSI-%d_PkCnt:(0x%x),ret=%d\n",
			total_delay, max_delay, ctx->portNum, mipi_packet_cnt, ret);
	}

	/* Check csi status again */
	if (debug_ft > total_delay) {
		if (!delay_with_stream_check(ctx, debug_ft - total_delay))
			return ret; // has been stream off
	}

	mac_irq = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	seninf_irq = mtk_cam_seninf_get_async_irq_st(ctx, ctx->seninfAsyncIdx, 0);
	temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);
	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);
	}

	seninf_logi(ctx,
		"CSI-%d_CSI2_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x),SENINF_ASYNC%d_OVERRUN:(0x%x),C/DPHY_RX_IRQ_STATUS:(0x%x)/(0x%x)\n",
		ctx->portNum, mac_irq, temp, ctx->seninfAsyncIdx, seninf_irq, cphy_irq, dphy_irq);
	if ((mac_irq & 0xD0) || seninf_irq)
		ret = -2; //multi lanes sync error, crc error, ecc error

	if ((ret == -1) && (mac_irq & 0x324)) {
		seninf_logi(ctx,
			"packet count is not changed but IRQ status raised still, so it would be false alarm due to all checking are in vb");
		ret = 0;
	}

	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 4);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 5);
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 5);
	} else
		seninf_logi(ctx, "[%s] warning: iomem_ver is invalid. mac checker is not set.\n", __func__);

	/* check OUTMUX irq status */
	for (j = 0; j < ctx->vcinfo.cnt; j++) {
		if (ctx->vcinfo.vc[j].enable) {
			for (k = 0; k < ctx->vcinfo.vc[j].dest_cnt; k++) {
				unsigned int used_outmux = ctx->vcinfo.vc[j].dest[k].outmux;
				unsigned int used_tag = ctx->vcinfo.vc[j].dest[k].tag;

				for (i = 0; i < _seninf_ops->outmux_num; i++) {
					if ((used_outmux == i) && mtk_cam_seninf_is_outmux_used(ctx, i)) {
						u32 filt, filt_in, res, exp_sz, irq_st;

						filt = mtk_cam_seninf_get_outmux_vcdt_filt(ctx, used_outmux,
								used_tag, false);
						filt_in = mtk_cam_seninf_get_outmux_vcdt_filt(ctx, used_outmux,
								used_tag, true);
						res = mtk_cam_seninf_get_outmux_res(ctx, used_outmux, used_tag);
						exp_sz = mtk_cam_seninf_get_outmux_exp(ctx, used_outmux, used_tag);

						irq_st = mtk_cam_seninf_get_outmux_irq_st(ctx, used_outmux, 0);
						seninf_logi(ctx,
							"dump outmux%d,tag%u,CFG_M/PIX_M/CFG0_out-in/CFG1_out-in/CFG2_out-in/SRC_out-in/CFG_DONE/CFG_CTL/CFG_RDY/DBG_PORT0/DBG_PORT1:(0x%x/0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x/0x%x/0x%x/0x%x/0x%x),filt_out/in=(0x%x/0x%x),expSize=0x%x,dbgRecSize=0x%x,irq=0x%x\n",
							i, used_tag,
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SW_CONFIG_MODE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PIX_MODE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_0),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_0),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_1),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_1),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_2),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SOURCE_CONFIG_2),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SRC_SEL),
							seninf_get_outmux_rg_val_inner(ctx, used_outmux,
								SENINF_OUTMUX_SRC_SEL),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_SW_CFG_DONE),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_CSR_CFG_CTRL),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_CAM_CFG_RDY),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PATH_DBG_PORT_0),
							seninf_get_outmux_rg_val(ctx, used_outmux,
								SENINF_OUTMUX_PATH_DBG_PORT_1),
							filt, filt_in, exp_sz, res, irq_st);
					}
				}
			}
		}
	}
	for (j = 0; j < _seninf_ops->outmux_num; j++) {
		unsigned int rdy = 0;
		u32 irq_st = mtk_cam_seninf_get_outmux_irq_st(ctx, j, 1);

		rdy = seninf_get_outmux_rg_val(ctx, j, SENINF_OUTMUX_CAM_CFG_RDY);
		if (ctx->outmux_disable_list[j]) {
			seninf_logi(ctx,
				 "outmux%d marked disable but not cfg done: CFG_M/PIX_M/CFG0/CFG1/CFG2/SRC/CFG_DONE/CFG_CTL/CFG_RDY/DBG_PORT0/DBG_PORT1:(0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x/0x%x),irq=0x%x\n",
				 j,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CONFIG_MODE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PIX_MODE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_0),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_1),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SOURCE_CONFIG_2),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SRC_SEL),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CFG_DONE),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_CSR_CFG_CTRL),
				 rdy,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PATH_DBG_PORT_0),
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_PATH_DBG_PORT_1),
				 irq_st);
		} else if (!rdy) {
			seninf_logi(ctx,
				 "outmux%u,CFG_DONE/CFG_RDY:(0x%x/0x%x),irq=0x%x",
				 j,
				 seninf_get_outmux_rg_val(ctx, j,
							  SENINF_OUTMUX_SW_CFG_DONE),
				 rdy, irq_st);
		}
	}

	mtk_cam_sensor_get_frame_cnt(ctx, &frame_cnt2);
	if (frame_cnt2 == frame_cnt1) {
		seninf_logi(ctx,
			"frame cnt(%d) doesn't update, please check sensor status.\n", frame_cnt2);
	} else {
		seninf_logi(ctx,
			"sensor is streaming, frame_cnt1: %d, frame_cnt2: %d\n", frame_cnt1, frame_cnt2);
	}
	seninf_logi(ctx, "ret = %d\n", ret);

	/* dump debug cur status function in case of log dropping */
	if (ctx->debug_cur_sys_time_in_ns) {
		seninf_logi(ctx,
		"sys_time_ns:%llu,D/CPHY_RX_IRQ_STATUS:(0x%08x)/(0x%08x),CSIRX_MAC_CSI2_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x),SENINF_CSI2_IRQ_STATUS:(0x%x)\n",
		ctx->debug_cur_sys_time_in_ns,
		ctx->debug_cur_dphy_irq,
		ctx->debug_cur_cphy_irq,
		ctx->debug_cur_mac_irq,
		ctx->debug_cur_temp,
		ctx->debug_cur_seninf_irq);
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
			DUMP_CUR_MAC_CHECKER_V1(ctx, 0);
			DUMP_CUR_MAC_CHECKER_V1(ctx, 1);
			DUMP_CUR_MAC_CHECKER_V1(ctx, 2);
			DUMP_CUR_MAC_CHECKER_V1(ctx, 3);
			DUMP_CUR_MAC_CHECKER_V1(ctx, 4);
			DUMP_CUR_MAC_CHECKER_V1(ctx, 5);
		} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			DUMP_CUR_MAC_CHECKER_V2(ctx, 0);
			DUMP_CUR_MAC_CHECKER_V2(ctx, 1);
			DUMP_CUR_MAC_CHECKER_V2(ctx, 2);
			DUMP_CUR_MAC_CHECKER_V2(ctx, 3);
			DUMP_CUR_MAC_CHECKER_V2(ctx, 5);
		} else
			seninf_logi(ctx, "[%s] warning: iomem_ver is invalid. mac checker is not set.\n", __func__);
	}

	return ret;
}

static int mtk_cam_seninf_debug_current_status(struct seninf_ctx *ctx)
{
	void *base_ana, *base_cphy, *base_dphy, *base_csi_mac;
	int i, ret = 0;
	enum CSI_PORT csi_port = CSI_PORT_0;
	char *fmeter_dbg = kzalloc(sizeof(char) * 256, GFP_KERNEL);
	void *pSeninf_asytop = ctx->reg_if_async;

	ctx->debug_cur_sys_time_in_ns = ktime_get_boottime_ns();

	if (fmeter_dbg && mtk_cam_dbg_fmeter(ctx->core, fmeter_dbg, sizeof(char) * 256) == 0)
		seninf_logi(ctx, "%s\n", fmeter_dbg);
	kfree(fmeter_dbg);

	for (csi_port = CSI_PORT_0A; csi_port <= CSI_PORT_5B; csi_port++) {
		if (csi_port != ctx->portA &&
			csi_port != ctx->portB)
			continue;

		base_ana = ctx->reg_ana_csi_rx[csi_port];

		seninf_logi(ctx,
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
		seninf_logi(ctx,
			"MipiRx_ANA%d:CDPHY_RX_ANA_AD_0/_1:(0x%x)/(0x%x),AD_HS_0/_1/_2:(0x%x)/(0x%x)/(0x%x)\n",
			csi_port - CSI_PORT_0A,
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_0),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_1),
			SENINF_READ_REG(base_ana, CDPHY_RX_ANA_AD_HS_2));
		if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
			seninf_logi(ctx,
				"MipiRx_ANA%d:CDPHY_RX_ASYM_AFIFO_CTRL_0/_1:(0x%x)/(0x%x)\n",
				csi_port - CSI_PORT_0A,
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0),
				SENINF_READ_REG(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_1));
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L0_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L1_AFIFO_FLUSH_FLAG_CLEAR, 1);
			SENINF_BITS(base_ana, CDPHY_RX_ASYM_AFIFO_CTRL_0, L2_AFIFO_FLUSH_FLAG_CLEAR, 1);
		}
	}

	for (csi_port = CSI_PORT_0; csi_port <= CSI_PORT_5; csi_port++) {
		if (csi_port != ctx->port)
			continue;

		base_cphy = ctx->reg_ana_cphy_top[csi_port];
		base_dphy = ctx->reg_ana_dphy_top[csi_port];
		ctx->debug_cur_cphy_irq = SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_CLR);
		ctx->debug_cur_dphy_irq = SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_STATUS);

		seninf_logi(ctx,
			"Csi%d_Dphy_Top:LANE_EN/_SELECT:(0x%x)/(0x%x),CLK_LANE0_HS/1_HS:(0x%x)/(0x%x),DATA_LANE0_HS/1_HS/2_HS/3_HS:(0x%x)/(0x%x)/(0x%x)/(0x%x),DPHY_RX_SPARE0:(0x%x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_LANE_SELECT),
			SENINF_READ_REG(base_dphy, DPHY_RX_CLOCK_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_CLOCK_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_DATA_LANE0_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_DATA_LANE1_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_DATA_LANE2_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_DATA_LANE3_HS_PARAMETER),
			SENINF_READ_REG(base_dphy, DPHY_RX_SPARE0));
		seninf_logi(ctx,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_CTRL/_TIMING_CTRL/_LANE0_CTRL/_LANE1_CTRL/_LANE2_CTRL/_LANE3_CTRL:(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_TIMING_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_LANE0_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_LANE1_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_LANE2_CTRL),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_LANE3_CTRL));
		seninf_logi(ctx,
			"Csi%d_Dphy_Top:DPHY_RX_DESKEW_IRQ_EN/_CLR/_STATUS:(0x%08x)/(0x%08x)/(0x%08x),DPHY_RX_IRQ_EN/_STATUS:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_EN),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_CLR),
			SENINF_READ_REG(base_dphy, DPHY_RX_DESKEW_IRQ_STATUS),
			SENINF_READ_REG(base_dphy, DPHY_RX_IRQ_EN),
			ctx->debug_cur_dphy_irq);
		seninf_logi(ctx,
			"Csi%d_Cphy_Top:CPHY_RX_CTRL:(0x%x),CPHY_RX_DETECT_CTRL_POST:(0x%x),CPHY_RX_IRQ_EN/_CLR:(0x%08x)/(0x%08x)\n",
			csi_port,
			SENINF_READ_REG(base_cphy, CPHY_RX_CTRL),
			SENINF_READ_REG(base_cphy, CPHY_RX_DETECT_CTRL_POST),
			SENINF_READ_REG(base_cphy, CPHY_RX_IRQ_EN),
			ctx->debug_cur_cphy_irq);
		/* Clear C/DPHY_IRQ status */
		SENINF_WRITE_REG(base_dphy, DPHY_RX_IRQ_CLR, 0xffffffff);
		SENINF_WRITE_REG(base_cphy, CPHY_RX_IRQ_CLR, 0x0f0f0000);
	}

	/* Seninf_csi status IRQ */
	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	ctx->debug_cur_mac_irq = SENINF_READ_REG(base_csi_mac,
		CSIRX_MAC_CSI2_IRQ_STATUS);
	ctx->debug_cur_seninf_irq = mtk_cam_seninf_get_async_irq_st(ctx, ctx->seninfAsyncIdx, 1);
	ctx->debug_cur_temp = SENINF_READ_REG(base_csi_mac,
		CSIRX_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS);

	if (ctx->debug_cur_mac_irq & ~(0x324)) {
		SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xFFFFFFFF);
		SENINF_WRITE_REG(base_csi_mac, CSIRX_CSI2_IRQ_MULTI_ERR_FRAME_SYNC_STATUS, ctx->debug_cur_temp);
	}

	seninf_logi(ctx,
		"CSI-%d,CSIRX_MAC_CSI2_EN/_OPT/_IRQ_STATUS/_MULTI_ERR_F_STATUS:(0x%x)/(0x%x)/(0x%x)/(0x%x),SENINF_ASYNC%d_OVERRUN:(0x%x),CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL:(0x%x)\n",
		(uint32_t)ctx->portNum,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_EN),
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_OPT),
		ctx->debug_cur_mac_irq,
		ctx->debug_cur_temp,
		(uint32_t)ctx->seninfAsyncIdx,
		ctx->debug_cur_seninf_irq,
		SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_RESYNC_MERGE_CTRL));

	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 0);
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 1);
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 2);
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 3);
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 4);
		READ_CUR_MAC_CHECKER_V1(base_csi_mac, 5);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 0);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 1);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 2);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 3);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 4);
		DUMP_CUR_MAC_CHECKER_V1(ctx, 5);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 4);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 5);
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		READ_CUR_MAC_CHECKER_V2(base_csi_mac, 0);
		READ_CUR_MAC_CHECKER_V2(base_csi_mac, 1);
		READ_CUR_MAC_CHECKER_V2(base_csi_mac, 2);
		READ_CUR_MAC_CHECKER_V2(base_csi_mac, 3);
		READ_CUR_MAC_CHECKER_V2(base_csi_mac, 5);
		DUMP_CUR_MAC_CHECKER_V2(ctx, 0);
		DUMP_CUR_MAC_CHECKER_V2(ctx, 1);
		DUMP_CUR_MAC_CHECKER_V2(ctx, 2);
		DUMP_CUR_MAC_CHECKER_V2(ctx, 3);
		DUMP_CUR_MAC_CHECKER_V2(ctx, 5);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 5);
	} else
		seninf_logi(ctx, "warning: iomem_ver is invalid. mac checker is not set.\n");

	if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* Bit Error Rate (BER) */
		calculate_bit_error_rate(ctx);
		seninf_logi(ctx,
			"CSI-%d,CSIRX_MAC_CSI2_BIT_ERR_CTRL/MIN_CYCLE/MAX_CYCLE:(0x%08x)/(0x%08x%08x)/(%08x%08x)\n",
			(uint32_t)ctx->portNum, ctx->ber.bit_err_ctrl,
			ctx->ber.min_cycle_msb, ctx->ber.min_cycle_lsb,
			ctx->ber.max_cycle_msb, ctx->ber.max_cycle_lsb);
		seninf_logi(ctx,
			"seninf_ck(%uMHz) bit_rate(%uMHz) bit_err_cnt(%u) bit_err_rate(1/[%llu,%llu])\n",
			ctx->ber.seninf_clk_mhz, ctx->ber.bit_rate_mhz,
			ctx->ber.bit_err_cnt, ctx->ber.min_bit, ctx->ber.max_bit);
		if (ctx->ber.min_bit && ctx->ber.min_bit < 1000000000000)
			seninf_logi(ctx, "WARN: max(BER) > 10^-12\n");
		if (ctx->ber.max_bit && ctx->ber.max_bit < 1000000000000)
			seninf_logi(ctx, "WARN: min(BER) > 10^-12\n");
		/* CPHY LRTE Spacer */
		if (ctx->is_cphy && ctx->csi_param.cphy_lrte_support) {
			calculate_cphy_lrte_spacer(ctx);
			seninf_logi(ctx, "CSI-%d,cphy_lrte_spacer(%u) vc(0x%02x) dt(0x%02x)\n",
				(uint32_t)ctx->portNum, ctx->lrte_sd.spacer, ctx->lrte_sd.vc, ctx->lrte_sd.dt);
			seninf_logi(ctx, "CSI-%d,valid_cnt/num_hs1/num_hs2/wc/trio: %u/%u/%u/%u/%u\n",
				(uint32_t)ctx->portNum, ctx->lrte_sd.valid_cnt,
				ctx->lrte_sd.num_hs1, ctx->lrte_sd.num_hs2, ctx->lrte_sd.wc, ctx->lrte_sd.trio);
		}
	}

	if ((ctx->debug_cur_mac_irq & 0xD0) ||
		(ctx->debug_cur_seninf_irq & 0x10000000))
		ret = -2; //multi lanes sync error, crc error, ecc error

	dev_info(ctx->dev,
		"current async%d:ASYNC_CFG(0x%x),ASYNC0_DBG0(0x%x),ASYNC1_DBG0(0x%x),ASYNC2_DBG0(0x%x),ASYNC3_DBG0(0x%x),ASYNC4_DBG0(0x%x),ASYNC5_DBG0(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_CFG),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_0),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_1),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_2),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_3),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_4),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT0_5));
	dev_info(ctx->dev,
		"current async%d:ASYNC0_DBG1(0x%x),ASYNC1_DBG1(0x%x),ASYNC2_DBG1(0x%x),ASYNC3_DBG1(0x%x),ASYNC4_DBG1(0x%x),ASYNC5_DBG1(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_0),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_1),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_2),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_3),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_4),
		SENINF_READ_REG(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_DBG_PORT1_5));
	dev_info(ctx->dev,
		"current async%d:BIST_RST0(0x%x),BIST_RST1(0x%x),BIST_RST2(0x%x),BIST_RST3(0x%x),BIST_RST4(0x%x),BIST_RST5(0x%x)\n",
		ctx->seninfAsyncIdx,
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_0,
				 SENINF_ASYTOP_AFIFO_BIST_RST_0),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_1,
				 SENINF_ASYTOP_AFIFO_BIST_RST_1),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_2,
				 SENINF_ASYTOP_AFIFO_BIST_RST_2),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_3,
				 SENINF_ASYTOP_AFIFO_BIST_RST_3),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_4,
				 SENINF_ASYTOP_AFIFO_BIST_RST_4),
		SENINF_READ_BITS(pSeninf_asytop, SENINF_ASYTOP_SENINF_ASYNC_FIFO_BIST_CTRL_5,
				 SENINF_ASYTOP_AFIFO_BIST_RST_5));

	/* dump all outmux */
	for (i = 0; i < _seninf_ops->outmux_num; i++) {
		u32 irq_st = mtk_cam_seninf_get_outmux_irq_st(ctx, i, 1);

		seninf_logi(ctx,
			 "dump outmux%d with irq clear,CFG_M/PIX_M/CFG0_out-in/CFG1_out-in/CFG2_out-in/SRC_out-in/CFG_DONE/CFG_CTL/CFG_RDY/DBG_PORT0/DBG_PORT1:(0x%x/0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x-0x%x/0x%x/0x%x/0x%x/0x%x/0x%x),tag0_filt_out-in/exp(0x%x-0x%x/0x%x-0x%x),tag1_filt_out-in/exp(0x%x-0x%x/0x%x-0x%x),tag4_filt_out-in/exp(0x%x-0x%x/0x%x-0x%x),irq=0x%x\n",
			 i,
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SW_CONFIG_MODE),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_PIX_MODE),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_0),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_0),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_1),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_1),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_2),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_SOURCE_CONFIG_2),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SRC_SEL),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_SRC_SEL),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_SW_CFG_DONE),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_CSR_CFG_CTRL),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_CAM_CFG_RDY),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_PATH_DBG_PORT_0),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_PATH_DBG_PORT_1),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_0),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_0),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_SIZE_0),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_SIZE_0),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_1),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_1),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_SIZE_1),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_SIZE_1),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_4),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_VCDT_FILT_4),
			 seninf_get_outmux_rg_val(ctx, i, SENINF_OUTMUX_TAG_SIZE_4),
			 seninf_get_outmux_rg_val_inner(ctx, i, SENINF_OUTMUX_TAG_SIZE_4),
			 irq_st);
	}
	seninf_logi(ctx, "ret = %d", ret);

	return ret;
}

static int mtk_cam_get_csi_irq_status(struct seninf_ctx *ctx)
{
#ifdef __XIAOMI_CAMERA__
	void *base_csi_mac;
	int csi_irq_st = 0;
	int ecc_err_corrected_cnt = 0, ecc_err_double_cnt = 0, crc_err_cnt = 0, err_lane_resync_cnt = 0, data_not_enough_cnt = 0, fifo_overrun_cnt = 0;

	if (!ctx->streaming)
		return 0;

	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	csi_irq_st = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);

	switch (csi_irq_st) {
		case 804:
		case 805:
			break;
		default:
			if (csi_irq_st & RO_CSI2_ECC_ERR_CORRECTED_IRQ_MASK)
				ecc_err_corrected_cnt++;
			if (csi_irq_st & RO_CSI2_ECC_ERR_DOUBLE_IRQ_MASK)
				ecc_err_double_cnt++;
			if (csi_irq_st & RO_CSI2_CRC_ERR_IRQ_MASK)
				crc_err_cnt++;
			if (csi_irq_st & RO_CSI2_ERR_LANE_RESYNC_IRQ_MASK)
				err_lane_resync_cnt++;
			if (csi_irq_st & RO_CSI2_RECEIVE_DATA_NOT_ENOUGH_IRQ_MASK)
				data_not_enough_cnt++;
			break;
		}

	dev_info(ctx->dev,"update mipi status: %d, %d, %d, %d, %d, %d, %d, %d\n",
		ecc_err_corrected_cnt,
		ecc_err_double_cnt,
		crc_err_cnt,
		err_lane_resync_cnt,
		data_not_enough_cnt,
		fifo_overrun_cnt,
		ctx->esd_status_flag,
		ctx->test_cnt);

	return (ecc_err_double_cnt)     |
		(crc_err_cnt          << 1) |
		(data_not_enough_cnt  << 2) |
		(fifo_overrun_cnt     << 3) |
		(ctx->test_cnt        << 8);
#else
	void *base_csi_mac;
	int ret = 0;

	if (!ctx->streaming)
		return 0;

	base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	ret = SENINF_READ_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
	//dev_info(ctx->dev,"CSI_RX%d_MAC_CSI2_IRQ_STATUS(0x%x)\n", ctx->port, ret);
	SENINF_WRITE_REG(base_csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);

	return ret;
#endif
}

static int mtk_cam_seninf_get_tsrec_timestamp(struct seninf_ctx *ctx, void *arg)
{
	int ret = 0;
	unsigned int tsrec_no = 0;
	struct seninf_outmux *mux_target;
	struct mtk_tsrec_timestamp_by_sensor_id *info = arg;
	struct mtk_cam_seninf_tsrec_timestamp_info ts_info = {0};

	if (info == NULL) {
		dev_info(ctx->dev, "%s arg is null", __func__);
		return -1;
	}

	mutex_lock(&ctx->mutex);
	list_for_each_entry(mux_target, &ctx->list_outmux, list) {
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
static ssize_t mtk_cam_seninf_show_err_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if PORTING_FIXME
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
			ctx->seninfAsyncIdx,
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
#else
	return 0;
#endif
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
			seninf_aee_print(SENINF_AEE_GENERAL, "[AEE] %s", buf);
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

static void seninf_record_vsync_info(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info);

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
		seninf_record_vsync_info(core, vsync_info);
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
#if PORTING_FIXME
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
#endif
	return 0;
}


/*static */void seninf_record_cammux_irq(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
#if PORTING_FIXME
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
#endif
}

void seninf_record_cammux_info(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
	struct seninf_ctx *ctx_;
	void *csirx_mac_csi;

	list_for_each_entry(ctx_, &core->list, list) {
		if (ctx_->streaming && ctx_->power_status_flag) {
			csirx_mac_csi = ctx_->reg_csirx_mac_csi[(uint32_t)ctx_->port];
			vsync_info->csi_irq_st[vsync_info->used_csi_port_num] =
				SENINF_READ_REG(csirx_mac_csi, CSIRX_MAC_CSI2_IRQ_STATUS);

			SENINF_WRITE_REG(csirx_mac_csi,
							 CSIRX_MAC_CSI2_IRQ_STATUS,
							 0xffffffff);

			vsync_info->csi_packet_cnt_st[vsync_info->used_csi_port_num] =
				SENINF_READ_REG(csirx_mac_csi, CSIRX_MAC_CSI2_PACKET_CNT_STATUS);
			vsync_info->ctx_port[vsync_info->used_csi_port_num++] = ctx_->port;
		}
	}
}

static void seninf_record_vsync_info(struct seninf_core *core,
	struct mtk_cam_seninf_vsync_info *vsync_info)
{
	vsync_info->time_mono = ktime_get_ns();

	if (core->vsync_irq_en_flag)
		seninf_record_cammux_irq(core, vsync_info);

	seninf_record_cammux_info(core, vsync_info);
}

static void handle_all_outmux_irq(struct seninf_core *core)
{
	void *pSeninf_outmux = NULL;
	u32 rg_val;
	int i;

	for (i = 0; i < _seninf_ops->outmux_num; i++) {
		pSeninf_outmux = core->reg_seninf_outmux[i];

		rg_val = SENINF_READ_REG(pSeninf_outmux, SENINF_OUTMUX_IRQ_STATUS);
		if (!rg_val)
			continue;

		SENINF_WRITE_REG(pSeninf_outmux, SENINF_OUTMUX_IRQ_STATUS, rg_val);

		/*
		 * Add code to observe what you want to check here
		 *
		 * dev_info(core->dev, "[%s] read outmux%d irq st 0x%x\n",
		 * __func__, i, rg_val);
		 */
	}
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
	handle_all_outmux_irq(core);

	spin_unlock_irqrestore(&core->spinlock_irq, flags);

	return (wake_thread) ? 1 : 0;
}

static int mtk_cam_seninf_set_all_cam_mux_vsync_irq(struct seninf_ctx *ctx, bool enable)
{
#if PORTING_FIXME
	void *pSeninf_cam_mux_gcsr = ctx->reg_if_cam_mux_gcsr;
	u32 val = enable ? 0xFFFFFFFF : 0;

	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, val);
	SENINF_BITS(pSeninf_cam_mux_gcsr,
		SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H, RG_SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN, val);
#endif
	return 0;

}
#ifdef SCAN_SETTLE
static int mtk_cam_scan_settle(struct seninf_ctx *ctx)
{
	u16 settle = 0, trail = 0;
	int ret = 0, ret_old = -1;

	if (!ctx->is_cphy) {
		for (trail = 0; trail <= 0xff; trail+= 4) {
			set_trail(ctx, trail);
			msleep(30);
			for (settle = 0; settle <= 0xff; settle += 8) {
				set_settle(ctx, settle, true);
				ret = mtk_cam_seninf_debug(ctx);
				if (ret == 0) {
					if (ret != ret_old)
						dev_info(ctx->dev,
								"%s valid trail = 0x%x settle =  0x%x ret = %d ret_old = %d trail enbled\n",
								__func__, trail, settle, ret, ret_old);

					dev_info(ctx->dev,
						"%s valid trail = 0x%x settle =  0x%x ret_detail = %dtrail enbled\n",
						__func__, trail, settle, ret);

				} else {
					if (ret != ret_old)
						dev_info(ctx->dev,
							"%s invalid trail = 0x%x  settle =  0x%x ret = %d ret_old = %d trail enbled\n",
							__func__, trail, settle, ret, ret_old);
					dev_info(ctx->dev,
						"%s invalid trail = 0x%x settle =  0x%x ret_detail = %d trail enbled\n",
						__func__, trail, settle, ret);
				}
				ret_old = ret;
			}
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

/*static */void update_vsync_irq_en_flag(struct seninf_ctx *ctx)
{
#if PORTING_FIXME
	void *p_gcammux = ctx->reg_if_cam_mux_gcsr;
	struct seninf_core *core = ctx->core;

	core->vsync_irq_en_flag =
		SENINF_READ_REG(p_gcammux, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN) ||
		SENINF_READ_REG(p_gcammux, SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H);

	dev_info(ctx->dev, "vsync enable = %u\n", core->vsync_irq_en_flag);
#endif
}

static int mtk_cam_seninf_eye_scan(struct seninf_ctx *ctx, u32 key, int val_signed, char *plog, int logbuf_size)
{
	int i, port, log_len = 0;
	void *base,/* *base_seninf,*/ *csi_mac;
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
		if (!((val == 0x1) || (val == 0x0))) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_DG1_EN value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_DG1_EN value(%d) illegal\n", val);
			break;
		}

		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_5,
                    RG_CSI0_CDPHY_EQ_DG1_EN, val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CDPHY_EQ_DG1_EN, val=0x%x\n", val);
			dev_info(ctx->dev, "SENINF_BITS set RG_CSI0_CDPHY_EQ_DG1_EN, val=0x%x\n", val);
		}
		break;
	case EYE_SCAN_KEYS_EQ_SR1:
		if (val > 15) {
			log_len = 0;
			log_len += snprintf(plog + log_len, logbuf_size - log_len, "[EYE_SCAN FAIL] EQ_SR1 value(%d) illegal\n", val);
			dev_info(ctx->dev, "[EYE_SCAN FAIL] EQ_SR1 value(%d) illegal\n", val);
			break;
		}
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			SENINF_BITS(base, CDPHY_RX_ANA_5,
                    RG_CSI0_CDPHY_EQ_SR1, val);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_BITS set RG_CSI0_CDPHY_EQ_SR1, val=0x%x\n", val);
			dev_info(ctx->dev, "SENINF_BITS set RG_CSI0_CDPHY_EQ_SR1, val=0x%x\n", val);
		}
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
				if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
					SENINF_BITS(base, CDPHY_RX_ANA_6,
							RG_CSI0_CPHY_T0_CDR_CK_DELAY, val);
					SENINF_BITS(base, CDPHY_RX_ANA_13,
							RG_CSI0_CPHY_T0_CDR_SEL_CODE, val);
					SENINF_BITS(base, CDPHY_RX_ANA_7,
							RG_CSI0_CPHY_T1_CDR_CK_DELAY, val);
					SENINF_BITS(base, CDPHY_RX_ANA_13,
							RG_CSI0_CPHY_T1_CDR_SEL_CODE, val);

					log_len += snprintf(plog + log_len, logbuf_size - log_len,
					"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_CK_DELAY, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_SEL_CODE, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_CK_DELAY, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_SEL_CODE, val=0x%x\n",
					val, val, val, val);
				} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
					SENINF_BITS(base, CDPHY_RX_ANA_4,
							RG_CSI0_CPHY_T0_CDR_RSTB_CODE, ((val & 0x38) >> 3));
					SENINF_BITS(base, CDPHY_RX_ANA_4,
							RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, (val & 0x7));
					SENINF_BITS(base, CDPHY_RX_ANA_6,
							RG_CSI0_CPHY_T0_CDR_CK_DELAY, val);
					SENINF_BITS(base, CDPHY_RX_ANA_4,
							RG_CSI0_CPHY_T1_CDR_RSTB_CODE, ((val & 0x38) >> 3));
					SENINF_BITS(base, CDPHY_RX_ANA_4,
							RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, (val & 0x7));
					SENINF_BITS(base, CDPHY_RX_ANA_6,
							RG_CSI0_CPHY_T1_CDR_CK_DELAY, val);

					log_len += snprintf(plog + log_len, logbuf_size - log_len,
					"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_RSTB_CODE, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_SEC_EDGE_CODE, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T0_CDR_CK_DELAY, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_RSTB_CODE, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_SEC_EDGE_CODE, val=0x%x\n"
					"SENINF_BITS set RG_CSI0_CPHY_T1_CDR_CK_DELAY, val=0x%x\n",
					((val & 0x38) >> 3), (val & 0x7), val,
					((val & 0x38) >> 3), (val & 0x7), val);
				}
				dev_info(ctx->dev,
				"EYE_SCAN_KEYS_CDR_DELAY input val_signed=%d, write to reg val=0x%x\n",
					val, val);
			}
		}
		break;
	case EYE_SCAN_KEYS_GET_CRC_STATUS:
		//base_seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfAsyncIdx];
		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		temp_csi_mac = SENINF_READ_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS);
		//temp_base_seninf = SENINF_READ_REG(base_seninf, SENINF_CSI2_IRQ_STATUS);

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
		//base_seninf = ctx->reg_if_csi2[(unsigned int)ctx->seninfAsyncIdx];
		csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
		SENINF_WRITE_REG(csi_mac, CSIRX_MAC_CSI2_IRQ_STATUS, 0xffffffff);
		//SENINF_WRITE_REG(base_seninf, SENINF_CSI2_IRQ_STATUS, 0xffffffff);
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
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_5,
                    RG_CSI0_CDPHY_EQ_DG1_EN);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_DG1_EN, val=0x%x\n", get_rg_val);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_DG1_EN, val=0x%x\n", get_rg_val);
		}
		break;
	case EYE_SCAN_KEYS_GET_EQ_SR1:
		for (i = 0; i <= ctx->is_4d1c; i++) {
			port = i ? ctx->portB : ctx->port;
			base = ctx->reg_ana_csi_rx[(unsigned int)port];
			get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_5,
                    RG_CSI0_CDPHY_EQ_SR1);
			log_len += snprintf(plog + log_len, logbuf_size - log_len,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_SR1, val=0x%x\n", get_rg_val);
			dev_info(ctx->dev,
				"SENINF_READ_BITS get RG_CSI0_CDPHY_EQ_SR1, val=0x%x\n", get_rg_val);
		}
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
				get_rg_val = SENINF_READ_BITS(base, CDPHY_RX_ANA_7,
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
//	int i, j;
	void *base = ctx->reg_ana_dphy_top[(unsigned int)ctx->port];
	void *base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
//	void *pmux, *pcammux, *p_gcammux;
//	struct seninf_vc *vc;
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
#if PORTING_FIXME
		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];
			for (j = 0; j < vc->dest_cnt; j++) {
				pmux = ctx->reg_if_mux[vc->dest[j].mux];
				SENINF_WRITE_REG(pmux, SENINF_MUX_IRQ_STATUS,
						 val & 0xFFFFFFFF);
			}
		}
#endif
		break;
	case REG_KEY_CAMMUX_IRQ_STAT:
		if (!ctx->streaming)
			return 0;
#if PORTING_FIXME
		for (i = 0; i < ctx->vcinfo.cnt; i++) {
			vc = &ctx->vcinfo.vc[i];
			for (j = 0; j < vc->dest_cnt; j++) {
				pcammux = ctx->reg_if_cam_mux_pcsr[vc->dest[j].cam];
				SENINF_WRITE_REG(pcammux,
						 SENINF_CAM_MUX_PCSR_IRQ_STATUS,
						 val & 0xFFFFFFFF);
			}
		}
#endif
		break;
	case REG_KEY_CAMMUX_VSYNC_IRQ_EN:
		if (!ctx->streaming)
			return 0;
#if PORTING_FIXME
		p_gcammux = ctx->reg_if_cam_mux_gcsr;
		SENINF_WRITE_REG(p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
				val & 0xFFFFFFFF);

		update_vsync_irq_en_flag(ctx);
#endif
		break;
	case REG_KEY_CAMMUX_VSYNC_IRQ_EN_H:
		if (!ctx->streaming)
			return 0;
#if PORTING_FIXME
		p_gcammux = ctx->reg_if_cam_mux_gcsr;
		SENINF_WRITE_REG(p_gcammux,
				SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
				val & 0xFFFFFFFF);

		update_vsync_irq_en_flag(ctx);
#endif
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
#if PORTING_FIXME
				p_gcammux = ctx_->reg_if_cam_mux_gcsr;
				SENINF_WRITE_REG(
					p_gcammux,
					SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN,
					0);
				SENINF_WRITE_REG(
					p_gcammux,
					SENINF_CAM_MUX_GCSR_VSYNC_IRQ_EN_H,
					0);
#endif
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
#if PORTING_FIXME
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
#endif
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

static int mtk_cam_set_phya_clock_src(struct seninf_ctx *ctx)
{
	void *base = ctx->reg_ana_csi_rx[(unsigned int)ctx->port];

	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		SENINF_BITS(base, CDPHY_RX_ANA_SETTING_0, CSR_ANA_REF_CK_SEL, 1);
		seninf_logd(ctx, "set phya clk source => osc\n");
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		SENINF_BITS(base, CDPHY_RX_ANA_SETTING_0, CSR_ANA_REF_CK_SEL, 0);
		seninf_logd(ctx, "set phya clk source => pll\n");
	} else {
		seninf_logd(ctx, "warning: iomem_ver is invalid. phya clk source is not set. default pll.\n");
		return -EINVAL;
	}

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
				//mtk_cam_seninf_set_seninf_top_ctrl2(ctx, 0);
				dev_info(ctx->dev,
					"[%s] target 16 pix_mode at vc[%d].dest[%d]\n", __func__, i, j);
				return 0;
			}
		}
	}
	return 0;
}

int mtk_cam_seninf_wait_outmux_cfg_done(struct seninf_ctx *ctx, u8 outmux_idx)
{
	void *pSeninf_mux;
	const u64 max_wait = ctx->cfg_done_max_delay;
	u64 waited = 0;

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	if (SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE)) {
		seninf_logi(ctx,
			"outmux idx %u, read CFG_M/FILT_M/CFG_CTL/CFG0/DBG0(0x%x/0x%x/0x%x/0x%x/0x%x), max wait %llu us\n",
			outmux_idx,
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_SW_CONFIG_MODE),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_FILT_MODE),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_CSR_CFG_CTRL),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0),
			max_wait);
	}

	while ((waited < max_wait) &&
	       SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE)) {
		udelay(100);
		waited += 100;
		if (waited < 100) {
			/* overflow. never here */
			seninf_aee_print(SENINF_AEE_OUTMUX,
				"Outmux%d doesn't cfg done\n", outmux_idx);
			break;
		}
	}

	if (waited > max_wait) {
		/* Assert */
		seninf_aee_print(SENINF_AEE_OUTMUX,
			"Outmux%d doesn't cfg done within %llu msec\n", outmux_idx, max_wait);
	}

	return 0;
}

int mtk_cam_seninf_set_out_mux_irq_en_by_tag(struct seninf_ctx *ctx, u8 outmux_idx, u8 tag_id, bool en)
{
	void *pOutMux;

	if (unlikely(outmux_idx > _seninf_ops->outmux_num)) {
		dev_info(ctx->dev, "[Error][%s] invalid tag_id (%d)\n", __func__, tag_id);
		return -EINVAL;
	}

	pOutMux = ctx->reg_if_outmux[outmux_idx];

	if (unlikely(pOutMux == NULL)) {
		dev_info(ctx->dev, "[Error][%s] pOutMux is NULL\n", __func__);
		return -EINVAL;
	}


	/* SET IRQ STATUS AS WRITE CLEAR*/
	SENINF_BITS(pOutMux, SENINF_OUTMUX_CSR_CFG_CTRL, SENINF_OUTMUX_INT_WCLR_EN, 1);

	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_CFG_DONE_IRQ_EN, en);
	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_REF_VSYNC_IRQ_EN, en);
	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_CFG_OVERLAP_IRQ_EN, en);
	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_CFG_LAG_HANG_IRQ_EN, en);
	SENINF_BITS(pOutMux, SENINF_OUTMUX_IRQ_EN, SENINF_OUTMUX_PIXCVT_OVERRUN_IRQ_EN, en);


	switch(tag_id) {
	case 0:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 0, en);
		break;
	case 1:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 1, en);
		break;
	case 2:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 2, en);
		break;
	case 3:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 3, en);
		break;
	case 4:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 4, en);
		break;
	case 5:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 5, en);
		break;
	case 6:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 6, en);
		break;
	case 7:
		SET_OUT_MUX_IRQ_EN_BY_TAG(pOutMux, 7, en);
		break;
	default:
		dev_info(ctx->dev, "[Error][%s] tag_id(%d) is invalid", __func__, tag_id);
		return -EINVAL;

	}
	return 0;
}

static bool chk_sensor_delay_with_wait(struct seninf_ctx *ctx, u8 outmux_idx, bool *sensor_delay)
{
	void *pSeninf_mux;
	int ret = true;
	const u64 max_wait = ctx->cfg_done_max_delay;
	u64 waited = 0;

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	if (!SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_IRQ_STATUS, SENINF_OUTMUX_REF_VSYNC_IRQ_STATUS)) {
		if (sensor_delay)
			*sensor_delay = false;
		return false;
	}

	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_IRQ_STATUS, SENINF_OUTMUX_REF_VSYNC_IRQ_STATUS, 1);

	if (SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE)) {
		seninf_logi(ctx,
			"outmux idx %u, read CFG_M/FILT_M/CFG_CTL/CFG0/DBG0(0x%x/0x%x/0x%x/0x%x/0x%x), max wait %llu us\n",
			outmux_idx,
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_SW_CONFIG_MODE),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_FILT_MODE),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_CSR_CFG_CTRL),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0),
			SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0),
			max_wait);
	}

	while ((waited < max_wait) &&
	       SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE)) {
		udelay(100);
		waited += 100;
		if (waited < 100) {
			/* overflow. never here */
			seninf_aee_print(SENINF_AEE_OUTMUX,
				"Outmux%d doesn't cfg done\n", outmux_idx);
			break;
		}
		if (SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_IRQ_STATUS, SENINF_OUTMUX_REF_VSYNC_IRQ_STATUS)) {
			// sensor no delay, but outmux change delay
			seninf_logi(ctx, "ref irq raised, sensor cfg is not completed, reset to recover\n");
			break;
		}
	}

	if (waited > max_wait) {
		/* Assert */
		seninf_aee_print(SENINF_AEE_OUTMUX,
			"Outmux%d doesn't cfg done within %llu msec\n", outmux_idx, max_wait);
	}

	if (sensor_delay)
		*sensor_delay = ret;

	return ret;
}

int mtk_cam_seninf_apply_outmux_for_v2(struct seninf_ctx *ctx, u8 outmux_idx,
			u8 cfg_mode, struct outmux_tag_cfg *tag_cfg, bool is_sensor_delay)
{
	void *pSeninf_mux, *pSeninf_mux_inout;
	int i;

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	if (is_sensor_delay) {
		// sensor delay apply
		pSeninf_mux_inout = pSeninf_mux;
		cfg_mode = MTK_CAM_OUTMUX_CFG_MODE_NORMAL_CFG; // using normal config to enable tag

		SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_RST,
			    SENINF_OUTMUX_LOCAL_SW_RST, 1);
		udelay(1);
		SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_RST,
			    SENINF_OUTMUX_LOCAL_SW_RST, 0);
		seninf_logd(ctx, "outmux%d force reset, DBG0 (0x%x)", outmux_idx,
			    SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0));
	} else {
		// normal apply
		pSeninf_mux_inout = ctx->reg_if_outmux_inner[outmux_idx];
	}

	seninf_logi(ctx, "outmux%d set %s first/last vs %u/%u, set outer cfg_mode %d, cfg_done st=%d, DBG0(0x%x)",
		    outmux_idx, (is_sensor_delay ? "outer" : "inner"),
		    ctx->cur_first_vs, ctx->cur_last_vs, cfg_mode,
		    SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE),
		    SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0));

	SENINF_BITS(pSeninf_mux_inout, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_REF_VC, ctx->cur_first_vs);
	SENINF_BITS(pSeninf_mux_inout, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_LAST_VC, ctx->cur_last_vs);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CONFIG_MODE,
					SENINF_OUTMUX_CONFIG_MODE, cfg_mode);

	for (i = 0; i < MAX_OUTMUX_TAG_NUM; i++) {
		if (tag_cfg[i].enable) {
			switch (i) {
			case 0:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						1, 0,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 1:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						1, 1,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 2:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						1, 2,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 3:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						1, 3,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 4:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						2, 4,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 5:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						2, 5,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 6:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						2, 6,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 7:
				SET_TAG_V2(ctx, pSeninf_mux, pSeninf_mux_inout,
						2, 7,
						(is_sensor_delay ? tag_cfg[i].filt_vc : ctx->cur_first_vs),
						tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
						tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			default:
				break;
			}
		}
	}

	return 0;
}

int mtk_cam_seninf_set_outmux_ref_vsync_inner(struct seninf_ctx *ctx, u8 outmux_idx)
{
	void *pSeninf_mux_outer, *pSeninf_mux_inner;

	if (unlikely(outmux_idx > _seninf_ops->outmux_num)) {
		dev_info(ctx->dev, "[Error][%s] invalid outmux_idx (%d)\n", __func__, outmux_idx);
		return -EINVAL;
	}

	pSeninf_mux_outer = ctx->reg_if_outmux[outmux_idx];
	pSeninf_mux_inner = ctx->reg_if_outmux_inner[outmux_idx];

	if (SENINF_READ_BITS(pSeninf_mux_outer, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE)) {
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_0,
			    SENINF_OUTMUX_REF_VC, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_0,
			    SENINF_OUTMUX_LAST_VC, ctx->cur_last_vs);

		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_1,
			    SENINF_OUTMUX_TAG_VC_0, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_1,
			    SENINF_OUTMUX_TAG_VC_1, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_1,
			    SENINF_OUTMUX_TAG_VC_2, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_1,
			    SENINF_OUTMUX_TAG_VC_3, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_2,
			    SENINF_OUTMUX_TAG_VC_4, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_2,
			    SENINF_OUTMUX_TAG_VC_5, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_2,
			    SENINF_OUTMUX_TAG_VC_6, ctx->cur_first_vs);
		SENINF_BITS(pSeninf_mux_inner, SENINF_OUTMUX_SOURCE_CONFIG_2,
			    SENINF_OUTMUX_TAG_VC_7, ctx->cur_first_vs);
	}

	dev_info(ctx->dev, "[%s] outmux%d update ref vc to first/last %d/%d\n",
		 __func__, outmux_idx, ctx->cur_first_vs, ctx->cur_last_vs);

	return 0;
}

int mtk_cam_seninf_config_outmux(struct seninf_ctx *ctx, u8 outmux_idx, u8 src_mipi, u8 src_sen,
			u8 cfg_mode, struct outmux_tag_cfg *tag_cfg)
{
	void *pSeninf_mux;
	int i;
	int is_tag_en = 0;

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	if (cfg_mode == 0) {
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_0,
					      SENINF_OUTMUX_FILT_EN_0);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_1,
					      SENINF_OUTMUX_FILT_EN_1);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_2,
					      SENINF_OUTMUX_FILT_EN_2);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_3,
					      SENINF_OUTMUX_FILT_EN_3);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_4,
					      SENINF_OUTMUX_FILT_EN_4);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_5,
					      SENINF_OUTMUX_FILT_EN_5);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_6,
					      SENINF_OUTMUX_FILT_EN_6);
		is_tag_en |= SENINF_READ_BITS(pSeninf_mux, SENINF_OUTMUX_TAG_VCDT_FILT_7,
					      SENINF_OUTMUX_FILT_EN_7);
		if (!is_tag_en) {
			seninf_logd(ctx, "outmux%d force reset, DBG0 (0x%x)", outmux_idx,
				SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0));
			SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_RST,
				    SENINF_OUTMUX_LOCAL_SW_RST, 1);
			udelay(1);
			SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_RST,
				    SENINF_OUTMUX_LOCAL_SW_RST, 0);
		}
	}

	seninf_logi(ctx, "outmux%d set outer src/sen %u/%u with cfg mode %d, DBG0(0x%x)",
		    outmux_idx, src_mipi, src_sen, cfg_mode,
		    SENINF_READ_REG(pSeninf_mux, SENINF_OUTMUX_PATH_DBG_PORT_0));

	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_VSYNC_SRC_SEL_MIPI, src_mipi);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_VSYNC_SRC_SEL_SEN, src_sen);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_REF_VC, ctx->cur_first_vs);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_LAST_VC, ctx->cur_last_vs);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SRC_SEL,
					SENINF_OUTMUX_SRC_SEL_MIPI, src_mipi);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SRC_SEL,
					SENINF_OUTMUX_SRC_SEL_SEN, src_sen);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CONFIG_MODE,
					SENINF_OUTMUX_CONFIG_MODE, cfg_mode);

	for (i = 0; i < MAX_OUTMUX_TAG_NUM; i++) {
		if (tag_cfg[i].enable) {
			mtk_cam_seninf_set_out_mux_irq_en_by_tag(ctx, outmux_idx, i, true);

			switch (i) {
			case 0:
				SET_TAG(ctx, pSeninf_mux, 1, 0, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 1:
				SET_TAG(ctx, pSeninf_mux, 1, 1, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 2:
				SET_TAG(ctx, pSeninf_mux, 1, 2, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 3:
				SET_TAG(ctx, pSeninf_mux, 1, 3, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 4:
				SET_TAG(ctx, pSeninf_mux, 2, 4, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 5:
				SET_TAG(ctx, pSeninf_mux, 2, 5, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 6:
				SET_TAG(ctx, pSeninf_mux, 2, 6, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			case 7:
				SET_TAG(ctx, pSeninf_mux, 2, 7, tag_cfg[i].filt_vc, tag_cfg[i].filt_dt,
					tag_cfg[i].exp_hsize, tag_cfg[i].exp_vsize);
				break;
			default:
				break;
			}
		}
	}

	return 0;
}

int mtk_cam_seninf_set_outmux_ref_vsync(struct seninf_ctx *ctx, u8 outmux_idx)
{
	void *pSeninf_mux;
	int first_vs, last_vs;

	if (unlikely(outmux_idx > _seninf_ops->outmux_num)) {
		dev_info(ctx->dev, "[Error][%s] invalid outmux_idx (%d)\n", __func__, outmux_idx);
		return -EINVAL;
	}

	first_vs = ctx->cur_first_vs;
	last_vs = ctx->cur_last_vs;

	seninf_logi(ctx, "set outmux%u reference vsync to first/last: %d/%d", outmux_idx, first_vs, last_vs);

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_REF_VC, first_vs);
	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SOURCE_CONFIG_0,
					SENINF_OUTMUX_LAST_VC, last_vs);

	return 0;
}

int mtk_cam_seninf_set_outmux_cfg_done(struct seninf_ctx *ctx, u8 outmux_idx)
{
	void *pSeninf_mux;

	if (unlikely(ctx == NULL))
		return -EINVAL;

	if (unlikely(outmux_idx > _seninf_ops->outmux_num)) {
		dev_info(ctx->dev, "[Error][%s] invalid outmux_idx (%d)\n", __func__, outmux_idx);
		return -EINVAL;
	}

	seninf_logi(ctx, "raise outmux%u cfg done, curr irq st:0x%x", outmux_idx,
		_seninf_ops->_get_outmux_irq_st(ctx, outmux_idx, 0));

	pSeninf_mux = ctx->reg_if_outmux[outmux_idx];

	SENINF_BITS(pSeninf_mux, SENINF_OUTMUX_SW_CFG_DONE, SENINF_OUTMUX_SW_CFG_DONE, 1);

	return 0;
}

static int mtk_cam_seninf_common_reg_setup(struct seninf_ctx *ctx)
{
	void *pSeninf_top = ctx->reg_if_top;

	mutex_lock(&ctx->core->seninf_top_rg_mutex);
	SENINF_BITS(pSeninf_top, SENINF_TOP_CTRL, SENINF_TOP_SW_CFG_LEVEL, 1);

	// enable all async and outmux cg
	SENINF_BITS(pSeninf_top, SENINF_TOP_ASYNC_CG_EN, SENINF_TOP_ASYNC_CG_EN, 0xffffffff);
	SENINF_BITS(pSeninf_top, SENINF_TOP_OUTMUX_CG_EN, SENINF_TOP_OUTMUX_CG_EN, 0xffffffff);

	// async overrun irq en
	SENINF_WRITE_REG(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_EN, 0xffffffff);

	mutex_unlock(&ctx->core->seninf_top_rg_mutex);

	seninf_logi(ctx, "setup common reg. TOP_CTL(0x%x),ASY_CG(0x%x),OUTMUX_CG(0x%x),ASY_OVERRUN_IRQEN(0x%x)",
		    SENINF_READ_REG(pSeninf_top, SENINF_TOP_CTRL),
		    SENINF_READ_REG(pSeninf_top, SENINF_TOP_ASYNC_CG_EN),
		    SENINF_READ_REG(pSeninf_top, SENINF_TOP_OUTMUX_CG_EN),
		    SENINF_READ_REG(pSeninf_top, SENINF_TOP_ASYNC_OVERRUN_IRQ_EN));

	return 0;
}

static int mtk_cam_seninf_device_sel_setting(struct device *dev,
			struct mtk_cam_seninf_dev *dev_setting)
{
	if (!dev_setting) {
		dev_info(dev, "[%s] parameter dev_setting is null", __func__);
		return -EINVAL;
	}

	// Allocate all outmux to dev0
	dev_setting->count = 1;
	dev_setting->val[0] = 0x7FFFFF;

	return 0;
}

static int mtk_cam_show_mac_chk_status(struct seninf_ctx *ctx, int is_clear)
{
	void *base_csi_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	/* Set mac checker */
	if (!strcasecmp(_seninf_ops->iomem_ver, MT6991_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 4);
		DUMP_MAC_CHECKER_V1(ctx, base_csi_mac, 5);
		/* Clear MAC CHECKER status and IRQ status */
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 4);
		CLEAR_MAC_CHECKER_IRQ_V1(base_csi_mac, 5);
	} else if (!strcasecmp(_seninf_ops->iomem_ver, MT6899_IOMOM_VERSIONS)) {
		/* Dump MAC CHECKER status and IRQ status */
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 0);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 1);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 2);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 3);
		DUMP_MAC_CHECKER_V2(ctx, base_csi_mac, 5);
		/* Clear MAC CHECKER status and IRQ status */
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 0);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 1);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 2);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 3);
		CLEAR_MAC_CHECKER_IRQ_V2(base_csi_mac, 5);
	} else
		seninf_logi(ctx, "warning: iomem_ver is invalid. mac checker is not set.\n");
	return 0;
}

static inline int mtk_cam_csi_mac_get_measure_probe_id_by_req(const int measure_req)
{
	return measure_req % CSIMAC_MEASURE_MAX_NUM;
}

static int mtk_cam_csi_mac_hv_hb_config(struct seninf_ctx *ctx,
	const struct mtk_cam_seninf_meter_info *pInfo, const int measure_req)
{
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];
	u8 probe_id = mtk_cam_csi_mac_get_measure_probe_id_by_req(measure_req);
	/* implement config csi mac hw */

	if (unlikely(pCsi2_mac == NULL)) {
		pr_info("[%s][ERROR] pCsi2_mac is NULL\n", __func__);
		return -EINVAL;
	}

	if (probe_id >= CSIMAC_MEASURE_MAX_NUM) {
		pr_info("[%s][ERROR] probe_id %d is invald\n", __func__, probe_id);
		return -EINVAL;
	}

	switch (probe_id) {
	case 1:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT3, RG_CSI2_VC_FOR_MEASURE1,
					pInfo->target_vc);

		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT3, RG_CSI2_DT_FOR_MEASURE1,
					pInfo->target_dt);

		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT3, RG_CSI2_V_LINE1,
					pInfo->probes[1].measure_line);
		break;

	default:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT1, RG_CSI2_VC_FOR_MEASURE0,
					pInfo->target_vc);

		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT1, RG_CSI2_DT_FOR_MEASURE0,
					pInfo->target_dt);

		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT1, RG_CSI2_V_LINE0,
					pInfo->probes[0].measure_line);
		break;
	}

	pr_info("[%s] vc 0x%x dt 0x%x target_line %d", __func__,
			pInfo->target_vc,
			pInfo->target_dt,
			pInfo->probes[probe_id].measure_line);

	pr_info("[%s] csi_mac_mipi measure probe %d cfg done", __func__,
			probe_id);

	return 0;
}

static int mtk_cam_csi_mac_hv_hb_measure_en(struct seninf_ctx *ctx, bool en, int measure_req)
{
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	if (unlikely(pCsi2_mac == NULL)) {
		pr_info("[%s][ERROR] pCsi2_mac is NULL\n", __func__);
		return -EINVAL;
	}

	switch (mtk_cam_csi_mac_get_measure_probe_id_by_req(measure_req)) {
	case 1:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_EN1, en);
		break;
	default:
		SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_EN0, en);
		break;
	}

	pr_info("[%s] probe %d en %d\n",
		__func__,
		mtk_cam_csi_mac_get_measure_probe_id_by_req(measure_req),
		en);

	return 0;
}

static int mtk_cam_csi_mac_hv_hb_wait_measure_done(struct seninf_ctx *ctx, int measure_req)
{

	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	if (unlikely(pCsi2_mac == NULL)) {
		pr_info("[%s][ERROR] pCsi2_mac is NULL\n", __func__);
		return -EINVAL;
	}

	switch (mtk_cam_csi_mac_get_measure_probe_id_by_req(measure_req)) {
	case 1:
		while(!SENINF_READ_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RO_CSI2_MIPI_MEASURE_DONE1))
			mdelay(5);

		mtk_cam_csi_mac_hv_hb_measure_en(ctx, false, measure_req);
		break;

	default:
		while(!SENINF_READ_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RO_CSI2_MIPI_MEASURE_DONE0))
			mdelay(5);

		mtk_cam_csi_mac_hv_hb_measure_en(ctx, false, measure_req);
		break;

	}
	return 0;
}

static int mtk_cam_csi_mac_hv_hb_get_result_by_line(struct seninf_ctx *ctx,
	struct mtk_cam_seninf_meter_info *pInfo, const int measure_req)
{
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	if (unlikely(pCsi2_mac == NULL)) {
		pr_info("[%s][ERROR] pCsi2_mac is NULL\n", __func__);
		return -EINVAL;
	}

	switch (mtk_cam_csi_mac_get_measure_probe_id_by_req(measure_req)) {
	case 1:
		mtk_cam_csi_mac_hv_hb_wait_measure_done(ctx, measure_req);
		pInfo->probes[1].measure_HV_cnt = SENINF_READ_BITS(pCsi2_mac,
											CSIRX_MAC_MIPI_MEASUREMENT4,
											RO_CSI2_H_VALID_CNT1);

		pInfo->probes[1].measure_HB_cnt = SENINF_READ_BITS(pCsi2_mac,
											CSIRX_MAC_MIPI_MEASUREMENT4,
											RO_CSI2_H_BLANKING_CNT1);
		break;
	default:
		mtk_cam_csi_mac_hv_hb_wait_measure_done(ctx, measure_req);
		pInfo->probes[0].measure_HV_cnt = SENINF_READ_BITS(pCsi2_mac,
											CSIRX_MAC_MIPI_MEASUREMENT2,
											RO_CSI2_H_VALID_CNT0);

		pInfo->probes[0].measure_HB_cnt = SENINF_READ_BITS(pCsi2_mac,
											CSIRX_MAC_MIPI_MEASUREMENT2,
											RO_CSI2_H_BLANKING_CNT0);
		break;
	}

	return 0;
}

static int mtk_cam_csi_mac_hv_hb_reset(struct seninf_ctx *ctx)
{
	void *pCsi2_mac = ctx->reg_csirx_mac_csi[(uint32_t)ctx->port];

	if (unlikely(pCsi2_mac == NULL)) {
		pr_info("[%s][ERROR] pCsi2_mac is NULL\n", __func__);
		return -EINVAL;
	}

	/* CLR the previous measure en */
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_EN0, false);
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_EN1, false);

	/* CLR the previous measure result */
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_CLR0, true);
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_CLR1, true);
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_CLR0, false);
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT5, RG_CSI2_MIPI_MEASURE_CLR1, false);

	/* set measure line as default line: 0 */
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT1, RG_CSI2_V_LINE0, 0);
	SENINF_BITS(pCsi2_mac, CSIRX_MAC_MIPI_MEASUREMENT3, RG_CSI2_V_LINE1, 0);


	return 0;
}

static int mtk_cam_csi_mac_get_hv_hb(struct seninf_ctx *ctx,
	struct mtk_cam_seninf_meter_info *pInfo, const int valid_measure_req)
{
	int i, ret = 0;

	if (mtk_cam_csi_mac_hv_hb_reset(ctx)) {
		pr_info("[%s][ERROR] mtk_cam_csi_mac_hv_hb_reset fail\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < valid_measure_req; i++) {
		if (mtk_cam_csi_mac_hv_hb_config(ctx, pInfo, i)) {
			pr_info("[%s][ERROR] mtk_cam_csi_mac_hv_hb_config fail\n", __func__);
			return -EINVAL;
		}
	}

	for (i = 0; i < valid_measure_req; i++)
		ret |= mtk_cam_csi_mac_hv_hb_measure_en(ctx, true, i);

	for (i = 0; i < valid_measure_req; i++) {
		if (mtk_cam_csi_mac_hv_hb_get_result_by_line(ctx, pInfo, i)) {
			pr_info("[%s][ERROR] mtk_cam_csi_mac_hv_hb_get_result_by_line fail\n", __func__);
			return -EINVAL;
		}
	}

	 return 0;
}

struct mtk_cam_seninf_ops mtk_csi_phy_3_0 = {
	._init_iomem = mtk_cam_seninf_init_iomem,
	._init_port = mtk_cam_seninf_init_port,
	._set_vc = mtk_cam_seninf_set_vc,
	._disable_outmux = mtk_cam_seninf_disable_outmux,
	._get_outmux_irq_st = mtk_cam_seninf_get_outmux_irq_st,
	._get_outmux_sel = mtk_cam_get_outmux_sel,
	._get_outmux_res = mtk_cam_seninf_get_outmux_res,
	._set_outmux_cg = mtk_cam_seninf_set_outmux_cg,
	._is_outmux_used = mtk_cam_seninf_is_outmux_used,
	._disable_all_outmux = mtk_cam_seninf_disable_all_outmux,
	._wait_outmux_cfg_done = mtk_cam_seninf_wait_outmux_cfg_done,
	._config_outmux = mtk_cam_seninf_config_outmux,
	._apply_outmux_for_v2 = mtk_cam_seninf_apply_outmux_for_v2,
	._set_outmux_ref_vsync_inner = mtk_cam_seninf_set_outmux_ref_vsync_inner,
	._chk_sensor_delay_with_wait = chk_sensor_delay_with_wait,
	._set_outmux_ref_vsync = mtk_cam_seninf_set_outmux_ref_vsync,
	._set_outmux_cfg_done = mtk_cam_seninf_set_outmux_cfg_done,
	._set_outmux_pixel_mode = mtk_cam_seninf_set_outmux_pixel_mode,
	._set_outmux_grp_en = mtk_cam_seninf_set_outmux_grp_en,
	._set_outmux_cfg_rdy = mtk_cam_seninf_set_outmux_cfg_rdy,
	._set_test_model = mtk_cam_seninf_set_test_model,
	._set_test_model_fake_sensor = mtk_cam_seninf_set_test_model_fake_sensor,
	._get_async_irq_st = mtk_cam_seninf_get_async_irq_st,
	._set_csi_mipi = mtk_cam_seninf_set_csi_mipi,
	._poweroff = mtk_cam_seninf_poweroff,
	._reset = mtk_cam_seninf_reset,
	._set_idle = mtk_cam_seninf_set_idle,
	._get_mux_meter = mtk_cam_seninf_get_mux_meter,
	._show_status = mtk_cam_seninf_show_status,
	._show_outmux_status = mtk_cam_seninf_show_outmux_status,
	._irq_handler = mtk_cam_seninf_irq_handler,
	._thread_irq_handler = mtk_thread_cam_seninf_irq_handler,
	._init_irq_fifo = mtk_cam_seninf_irq_event_st_init,
	._uninit_irq_fifo = mtk_cam_seninf_irq_event_st_uninit,
	._enable_cam_mux_vsync_irq = mtk_cam_seninf_enable_cam_mux_vsync_irq,
	._set_all_cam_mux_vsync_irq = mtk_cam_seninf_set_all_cam_mux_vsync_irq,
#ifdef SCAN_SETTLE
	._debug = mtk_cam_scan_settle,

#else
	._debug = mtk_cam_seninf_debug,
#endif
	.get_seninf_debug_core_dump = mtk_cam_seninf_debug_core_dump,
	._get_tsrec_timestamp = mtk_cam_seninf_get_tsrec_timestamp,
	._eye_scan = mtk_cam_seninf_eye_scan,
	._set_reg = mtk_cam_seninf_set_reg,
	._set_phya_clock_src = mtk_cam_set_phya_clock_src,
	.async_num = 6,
	.outmux_num = 14,
	.iomem_ver = NULL,
	._show_err_status = mtk_cam_seninf_show_err_status,
	._enable_stream_err_detect = mtk_cam_enable_stream_err_detect,
	._debug_init_deskew_irq = debug_init_deskew_irq,
	._debug_init_deskew_begin_end_apply_code = debug_init_deskew_begin_end_apply_code,
	._debug_current_status = mtk_cam_seninf_debug_current_status,
	._set_csi_afifo_pop = mtk_cam_seninf_set_csi_afifo_pop,
	._get_csi_irq_status = mtk_cam_get_csi_irq_status,
	._common_reg_setup = mtk_cam_seninf_common_reg_setup,
	._get_device_sel_setting = mtk_cam_seninf_device_sel_setting,
	._seninf_dump_mipi_err = seninf_dump_vsync_info,
	._show_mac_chk_status = mtk_cam_show_mac_chk_status,
	._get_csi_HV_HB_meter = mtk_cam_csi_mac_get_hv_hb,
};
