/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_H
#define __MTK_CAM_RAW_H

#include <linux/kfifo.h>
#include "mtk_cam-engine.h"
#include "mtk_cam-dvfs_qos.h"

struct mtk_cam_request_stream_data;

#define SCQ_DEADLINE_MS  15 // ~1/2 frame length
#define SCQ_DEADLINE_MS_STAGGER  30 // ~1/2 frame length
#define SCQ_DEFAULT_CLK_RATE 208 // default 208MHz
#define USINGSCQ 1

enum raw_module_id {
	RAW_A = 0,
	RAW_B = 1,
	RAW_C = 2,
	RAW_NUM = 3,
};

struct mtk_cam_dev;
struct mtk_cam_ctx;

struct mtk_raw_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	int irq;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *yuv_base;
	void __iomem *yuv_base_inner;
	unsigned int num_clks;
	struct clk **clks;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block pm_notifier;
#endif
	/* larb */
	struct platform_device *larb_pdev;
	struct mtk_camsys_qos qos;

	int		fifo_size;
	void		*msg_buffer;
	struct kfifo	msg_fifo;
	atomic_t	is_fifo_overflow;

	struct engine_callback *engine_cb;
	struct engine_fsm fsm;
	struct apply_cq_ref *cq_ref;

	bool enable_hsf;
	int fps;
	int subsample_ratio;

	bool is_slave;

	u64 sof_count;
	u64 vsync_count;

	/* for subsample, sensor-control */
	bool sub_sensor_ctrl_en;
	int set_sensor_idx;
	int cur_vsync_idx;

	u8 time_shared_busy;
	u8 time_shared_busy_ctx_id;
	atomic_t vf_en;

	/* error handling related */
	int tg_overrun_handle_cnt;
	int tg_grab_err_handle_cnt;

	int default_printk_cnt;
};

struct mtk_yuv_device {
	struct device *dev;
	unsigned int id;
	int irq;
	void __iomem *base;
	void __iomem *base_inner;
	unsigned int num_clks;
	struct clk **clks;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block pm_notifier;
#endif
	struct platform_device *larb_pdev;
	struct mtk_camsys_qos qos;
};

/* aa debug info */
struct mtk_ae_debug_data {
	u64 OBC_R1_Sum[4];
	u64 OBC_R2_Sum[4];
	u64 OBC_R3_Sum[4];
	u64 AA_Sum[4];
	u64 LTM_Sum[4];
};

/* CQ setting */
void initialize(struct mtk_raw_device *dev, int is_slave, int is_dc,
		struct engine_callback *cb);
void subsample_enable(struct mtk_raw_device *dev, int ratio);
void stagger_enable(struct mtk_raw_device *dev);
void stagger_disable(struct mtk_raw_device *dev);
void update_scq_start_period(struct mtk_raw_device *dev, int scq_ms);
void apply_cq(struct mtk_raw_device *dev,
	      struct apply_cq_ref *ref,
	      dma_addr_t cq_addr,
	      unsigned int cq_size, unsigned int cq_offset,
	      unsigned int sub_cq_size, unsigned int sub_cq_offset);
/* db */
void dbload_force(struct mtk_raw_device *dev);
void toggle_db(struct mtk_raw_device *dev);
void enable_tg_db(struct mtk_raw_device *dev, int en);


/* trigger */
void stream_on(struct mtk_raw_device *dev, int on);
void immediate_stream_off(struct mtk_raw_device *dev);
void trigger_rawi_r2(struct mtk_raw_device *dev);
void trigger_rawi_r5(struct mtk_raw_device *dev);
void trigger_adl(struct mtk_raw_device *dev);
void raw_dump_debug_status(struct mtk_raw_device *dev);

/* reset */
void reset(struct mtk_raw_device *dev);

/* iommu debug */
int mtk_raw_translation_fault_cb(int port, dma_addr_t mva, void *data);
int mtk_yuv_translation_fault_cb(int port, dma_addr_t mva, void *data);

/* aa debug info */
void fill_aa_info(struct mtk_raw_device *raw_dev,
				  struct mtk_ae_debug_data *ae_info);

extern struct platform_driver mtk_cam_raw_driver;
extern struct platform_driver mtk_cam_yuv_driver;

static inline u32 dmaaddr_lsb(dma_addr_t addr)
{
	return addr & (BIT_MASK(32) - 1UL);
}

static inline u32 dmaaddr_msb(dma_addr_t addr)
{
	return addr >> 32;
}
static inline int is_subsample_en(struct mtk_raw_device *dev)
{
	return dev->sub_sensor_ctrl_en;
}

int raw_to_tg_idx(int raw_id);

#endif /*__MTK_CAM_RAW_H*/
