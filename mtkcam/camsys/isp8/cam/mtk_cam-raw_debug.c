// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/device.h>
#include <linux/io.h>

#include "mtk_cam-raw.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-raw_regs.h"
#include "mtk_cam-reg_utils.h"
#include "mtk_cam-raw_debug.h"

#define LOGGER_PREFIX_SIZE 16
#define LOGGER_BUFSIZE 128
struct buffered_logger {
	struct device *dev;

	char prefix[LOGGER_PREFIX_SIZE];
	char buf[LOGGER_BUFSIZE + 1];
	int size;
};

#define INIT_LOGGER(logger, _dev)	\
({						\
	(logger)->dev = _dev;			\
	(logger)->prefix[0] = '\0';		\
	(logger)->buf[LOGGER_BUFSIZE] = '\0';	\
	(logger)->size = 0;			\
})

static __printf(2, 3)
void mtk_cam_log_set_prefix(struct buffered_logger *log, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vscnprintf(log->prefix, LOGGER_PREFIX_SIZE, fmt, args);
	va_end(args);
}

static void mtk_cam_log_flush(struct buffered_logger *log)
{
	dev_info(log->dev, "%s: %.*s\n",
		 log->prefix, log->size, log->buf);
	log->size = 0;
}

static __printf(2, 3)
void mtk_cam_log_push(struct buffered_logger *log, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = vscnprintf(log->buf + log->size, LOGGER_BUFSIZE - log->size + 1,
			 fmt, args);
	va_end(args);

	if (len + log->size < LOGGER_BUFSIZE) {
		log->size += len;
		return;
	}

	mtk_cam_log_flush(log);

	va_start(args, fmt);
	len = vscnprintf(log->buf + log->size, LOGGER_BUFSIZE - log->size + 1,
			 fmt, args);
	va_end(args);

	log->size += len;

	if (len == LOGGER_BUFSIZE)
		dev_info(log->dev, "log buffer size not enough: %d\n",
			 LOGGER_BUFSIZE);
}

void dump_raw_dma_err_st(struct mtk_raw_device *raw)
{
	size_t list_size = 0;
	struct reg_to_dump *raw_dma_list = NULL;
	struct buffered_logger log;
	int i = 0, err_st;

	CALL_PLAT_HW(query_raw_dma_list, &list_size, &raw_dma_list);

	INIT_LOGGER(&log, raw->dev);
	mtk_cam_log_set_prefix(&log, "%s", "RAW DMA ERR: ");
	for (i = 0; i < list_size; i++) {
		err_st = readl_relaxed(raw->dmatop_base + raw_dma_list[i].reg);
		if (err_st & 0xffff) {
			mtk_cam_log_push(&log, " %s: 0x%08x",
					raw_dma_list[i].name, err_st);
		}
	}
	mtk_cam_log_flush(&log);
}

void dump_yuv_dma_err_st(struct mtk_yuv_device *yuv)
{
	static const struct reg_to_dump yuv_dma_list[] = {
		ADD_DMA_ERR(YUVO_R1), ADD_DMA_ERR(YUVBO_R1),
		ADD_DMA_ERR(YUVCO_R1), ADD_DMA_ERR(YUVDO_R1),
		ADD_DMA_ERR(YUVO_R3), ADD_DMA_ERR(YUVBO_R3),
		ADD_DMA_ERR(YUVCO_R3), ADD_DMA_ERR(YUVDO_R3),
		ADD_DMA_ERR(YUVO_R2), ADD_DMA_ERR(YUVBO_R2),
		ADD_DMA_ERR(YUVO_R4), ADD_DMA_ERR(YUVBO_R4),
		ADD_DMA_ERR(YUVO_R5), ADD_DMA_ERR(YUVBO_R5),
		ADD_DMA_ERR(TCYSO_R1),
		ADD_DMA_ERR(RZH1N2TO_R2),
		ADD_DMA_ERR(DRZH2NO_R1),
		ADD_DMA_ERR(DRZH2NO_R8),
		ADD_DMA_ERR(DRZS4NO_R3),
		ADD_DMA_ERR(DRZH1NO_R1),
		ADD_DMA_ERR(DRZH1NO_R3),
		ADD_DMA_ERR(DRZH1NBO_R1),
		ADD_DMA_ERR(DRZH1NBO_R3),
		ADD_DMA_ERR(FHO_R3),
		ADD_DMA_ERR(STG_R3),
	};
	struct buffered_logger log;
	int i = 0, err_st;

	INIT_LOGGER(&log, yuv->dev);
	mtk_cam_log_set_prefix(&log, "%s", "YUV DMA ERR: ");
	for (i = 0; i < ARRAY_SIZE(yuv_dma_list); i++) {
		err_st = readl_relaxed(yuv->dmatop_base + yuv_dma_list[i].reg);
		if (err_st & 0xffff) {
			mtk_cam_log_push(&log, " %s: 0x%08x",
					yuv_dma_list[i].name, err_st);
		}
	}
	mtk_cam_log_flush(&log);
}
void dump_dmatop_dc_st(struct mtk_raw_device *raw)
{
	char str[256];
	int n;

	str[0] = '\0';
	n = scnprintf(str, sizeof(str), "chasing status 0x%08x/0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS),
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_CHASING_STATUS2));

	/*n += scnprintf(str + n, sizeof(str) - n, " src_sel 0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_CHASING_SRC_SEL));*/

	n += scnprintf(str + n, sizeof(str) - n, ", DC DBG LINE CNT:");
	n += scnprintf(str + n, sizeof(str) - n, " RAWI_R5:0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R5));
	n += scnprintf(str + n, sizeof(str) - n, " UFDI_R5:0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R5));
	n += scnprintf(str + n, sizeof(str) - n, " RAWI_R2:0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_RAWI_R2));
	n += scnprintf(str + n, sizeof(str) - n, " UFDI_R2:0x%08x",
		      readl(raw->dmatop_base + REG_CAMRAWDMATOP_DC_DBG_LINE_CNT_UFDI_R2));

	dev_info(raw->dev, "%s: %s\n", __func__, str);
}

void set_topdebug_rdyreq(struct mtk_raw_device *dev, u32 event)
{
	u32 val = event << 16 | 0xa << 12;

	writel(val, dev->base + REG_CAMCTL_DBG_SET);
	writel(event, dev->base + REG_CAMCTL_DBG_SET2);
	writel(val, dev->yuv_base + REG_CAMCTL_DBG_SET);
	writel(event, dev->yuv_base + REG_CAMCTL_DBG_SET2);
	/*ISP8 new*/
	writel(event, dev->base + REG_CAMCTL_DBG_SET3);
	writel(event, dev->yuv_base + REG_CAMCTL2_DBG_SET3);
}

void dump_topdebug_rdyreq(struct mtk_raw_device *dev)
{
	static const u32 debug_sel[] = {
		/* req group 1~7 */
		0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6,
		/* rdy group 1~7 */
		0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE,
		/* latched_events */
		0xF,
	};
	void __iomem *dbg_set, *dbg_port;
	u32 set;
	int i;

	dbg_set = dev->base + REG_CAMCTL_DBG_SET;
	dbg_port = dev->base + REG_CAMCTL_DBG_PORT;

	set = (readl(dbg_set) & 0xfff000) | 0x1;
	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		writel(set | debug_sel[i] << 8, dbg_set);
		dev_info(dev->dev, "RAW debug_set 0x%08x port 0x%08x\n",
			 readl(dbg_set), readl(dbg_port));
	}

	dbg_set = dev->yuv_base + REG_CAMCTL_DBG_SET;
	dbg_port = dev->yuv_base + REG_CAMCTL_DBG_PORT;

	set = (readl(dbg_set) & 0xfff000) | 0x1;
	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		writel(set | debug_sel[i] << 8, dbg_set);
		dev_info(dev->dev, "YUV debug_set 0x%08x port 0x%08x\n",
			 readl(dbg_set), readl(dbg_port));
	}
}

void dump_topdebug_rdyreq_status(struct mtk_raw_device *dev)
{
	static const u32 debug_sel[] = {
		/* req group 1~7,10,11 */
		0x0, 0x4, 0x8, 0xc, 0x10, 0x14, 0x18, 0x24, 0x28,
	};
	void __iomem *dbg_rdy, *dbg_req;
	int i;

	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		dbg_req = dev->base + REG_CAMCTL_MOD_REQ_STATUS;
		dbg_rdy = dev->base + REG_CAMCTL_MOD_RDY_STATUS;
		dbg_req = dbg_req + debug_sel[i];
		dbg_rdy = dbg_rdy + debug_sel[i];
		dev_info(dev->dev, "RAW debug_req 0x%08x debug_rdy 0x%08x\n",
			 readl(dbg_req), readl(dbg_rdy));
	}

	for (i = 0; i < ARRAY_SIZE(debug_sel); i++) {
		dbg_req = dev->yuv_base + REG_CAMCTL2_MOD_REQ_STATUS;
		dbg_rdy = dev->yuv_base + REG_CAMCTL2_MOD_RDY_STATUS;
		dbg_req = dbg_req + debug_sel[i];
		dbg_rdy = dbg_rdy + debug_sel[i];
		dev_info(dev->dev, "YUV debug_req 0x%08x debug_rdy 0x%08x\n",
			 readl(dbg_req), readl(dbg_rdy));
	}
}

#define MAX_DEBUG_SIZE (32)
void mtk_cam_dump_dma_debug(struct mtk_raw_device *raw_dev,
			    void __iomem *dmatop_base,
			    const char *dma_name,
			    struct dma_debug_item *items, int n)
{
	struct device *dev = raw_dev->dev;
	void __iomem *dbg_sel = dmatop_base + 0x88;
	void __iomem *dbg_port = dmatop_base + 0x8c;
	int i = 0;
	unsigned int vals[MAX_DEBUG_SIZE];
	int crc_en;

	if (n >= MAX_DEBUG_SIZE) {
		dev_info(dev, "%s: should enlarge array size for n(%d)\n",
			__func__, n);
		return;
	}

	crc_en = readl(dbg_sel) & BIT(24);

	for (i = 0; i < n; i++) {
		int cur_sel, actual_sel;

		cur_sel = items[i].debug_sel;
		writel(crc_en | cur_sel, dbg_sel);

		actual_sel = readl(dbg_sel);
		if ((actual_sel ^ cur_sel) & 0xffffff)
			dev_info(dev, "failed to write dbg_sel %08x actual %08x\n",
				 cur_sel, actual_sel);
		if (actual_sel & 0xc0000000)
			dev_info(dev, "dbg_sel: %08x\n", actual_sel);

		vals[i] = readl(dbg_port);
	};

	dev_info(dev, "%s: %s\n", __func__, dma_name);
	for (i = 0; i < n; i++)
		dev_info(dev, "%08x: %08x [%s]\n",
			 crc_en | items[i].debug_sel, vals[i], items[i].msg);
}

void mtk_cam_dump_ufd_debug(struct mtk_raw_device *raw_dev,
			    const char *mod_name,
			    struct dma_debug_item *items, int n)
{
	struct device *dev = raw_dev->dev;
	void __iomem *dbg_sel =  raw_dev->base + REG_CAMCTL_DBG_SET;
	void __iomem *dbg_port = raw_dev->base + REG_CAMCTL_DBG_PORT;
	int i = 0;
	unsigned int vals[MAX_DEBUG_SIZE];

	if (n >= MAX_DEBUG_SIZE) {
		dev_info(dev, "%s: should enlarge array size for n(%d)\n",
			__func__, n);
		return;
	}

	for (i = 0; i < n; i++) {
		int cur_sel, actual_sel;

		cur_sel = items[i].debug_sel;
		writel(cur_sel, dbg_sel);

		actual_sel = readl(dbg_sel);
		if ((actual_sel ^ cur_sel) & 0xffffff)
			dev_info(dev, "failed to write dbg_sel %08x actual %08x\n",
				 cur_sel, actual_sel);

		vals[i] = readl(dbg_port);
	};

	dev_info(dev, "%s: %s\n", __func__, mod_name);
	for (i = 0; i < n; i++)
		dev_info(dev, "%08x: %08x [%s]\n",
			 items[i].debug_sel, vals[i], items[i].msg);
}
