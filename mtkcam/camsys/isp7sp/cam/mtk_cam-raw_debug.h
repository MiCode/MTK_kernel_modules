/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MTK_CAM_RAW_DEBUG_H
#define _MTK_CAM_RAW_DEBUG_H

struct mtk_raw_device;
struct mtk_yuv_device;

enum topdebug_event {
	ALL_THE_TIME	= 1 << 0,
	TG_OVERRUN	= 1 << 1,
	CQ_MAIN_VS_ERR	= 1 << 2,
	CQ_SUB_VS_ERR	= 1 << 3,
	RAW_DMA_ERR	= 1 << 4,
	YUV_DMA_ERR	= 1 << 5,
};

struct reg_to_dump {
	const char *name;
	unsigned int reg;
};

#define ADD_DMA_ERR(name) { #name, REG_ ## name ## _BASE + DMA_OFFSET_ERR_STAT }
#define ADD_DMA_FBC(name) { #name, REG_FBC_ ## name ## _CTL2 }

void dump_raw_dma_fbc(struct mtk_raw_device *raw);
void dump_yuv_dma_fbc(struct mtk_yuv_device *yuv);

void dump_raw_dma_err_st(struct mtk_raw_device *raw);
void dump_yuv_dma_err_st(struct mtk_yuv_device *yuv);

void dump_dmatop_dc_st(struct mtk_raw_device *raw);

void set_topdebug_rdyreq(struct mtk_raw_device *dev, u32 event);
void dump_topdebug_rdyreq(struct mtk_raw_device *dev);

struct dma_debug_item {
	unsigned int	debug_sel;
	const char	*msg;
};

void mtk_cam_dump_dma_debug(struct mtk_raw_device *dev,
			    void __iomem *dmatop_base,
			    const char *dma_name,
			    struct dma_debug_item *items, int n);

void mtk_cam_dump_ufd_debug(struct mtk_raw_device *raw_dev,
			    const char *mod_name,
			    struct dma_debug_item *items, int n);

#endif	/* _MTK_CAM_RAW_DEBUG_H */
