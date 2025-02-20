/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_H
#define __MTK_CAM_RAW_H

#include <linux/kfifo.h>
#include "mtk_cam-engine.h"
#include "mtk_cam-dvfs_qos.h"

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

#define call_io_ops(raw, func, ...) \
({\
	typeof(raw) _raw = (raw);\
	typeof(_raw->io_ops) _io_ops = _raw->io_ops;\
	_io_ops && _io_ops->func ? _io_ops->func(_raw, ##__VA_ARGS__) : 0;\
})

struct mtk_raw_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	int irq;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *dmatop_base;
	void __iomem *dmatop_base_inner;
	void __iomem *qof_base;
	void __iomem *yuv_base;
	void __iomem *yuv_base_inner;
	void __iomem *rms_base;
	void __iomem *rms_base_inner;
	void __iomem *larb_vcsel;
	u64 base_reg_addr;
	u64 base_inner_reg_addr;
	unsigned int num_clks;
	struct clk **clks;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block pm_notifier;
#endif
	/* larb */
	unsigned int num_larbs;
	struct platform_device **larbs;
	struct mtk_camsys_qos qos;

	int		fifo_size;
	void		*msg_buffer;
	struct kfifo	msg_fifo;
	atomic_t	is_fifo_overflow;

	struct engine_callback *engine_cb;
	struct engine_fsm fsm;
	struct apply_cq_ref *cq_ref;

	int fps;
	int subsample_ratio;
	bool is_timeshared;
	bool is_slave;

	u64 sof_count;
	u64 vsync_count;

	/* for subsample, sensor-control */
	bool sub_sensor_ctrl_en;
	int set_sensor_idx;
	int cur_vsync_idx;

	atomic_t vf_en;

	/* error handling related */
	int tg_grab_err_handle_cnt;
	int dma_err_handle_cnt;
	int tg_overrun_handle_cnt;
	u64 apply_ts;
	bool log_en;
	int default_printk_cnt;
	/* preisp synchronized used */
	int tg_count;
	char str_debug_irq_data[128];

	/* QOF */
	const struct raw_io_ops *io_ops;
	bool trigger_cq_by_qof;
	int apmcu_voter_cnt;
	spinlock_t apmcu_voter_lock;
	spinlock_t qof_ctrl_lock;

	atomic_t time_share_used;/*identify first and last*/
	atomic_t time_share_on_process;/*identify busy*/

	/* ois compensation */
	bool lock_done_ctrl;
};

struct mtk_yuv_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *dmatop_base;
	void __iomem *dmatop_base_inner;
	void __iomem *larb_vcsel;
	unsigned int num_clks;
	struct clk **clks;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block pm_notifier;
#endif
	unsigned int num_larbs;
	struct platform_device **larbs;
	struct mtk_camsys_qos qos;
};

struct raw_io_ops {
	u32 (*readl)(struct mtk_raw_device *raw, void __iomem *base, u32 offset);
	u32 (*readl_relaxed)(struct mtk_raw_device *raw, void __iomem *base, u32 offset);
	void (*writel)(struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset);
	void (*writel_relaxed)(struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset);
};

extern struct raw_io_ops basic_io_ops;

u32 basic_readl(
	struct mtk_raw_device *raw, void __iomem *base, u32 offset);
u32 basic_readl_relaxed(
	struct mtk_raw_device *raw, void __iomem *base, u32 offset);
void basic_writel(
	struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset);
void basic_writel_relaxed(
	struct mtk_raw_device *raw, u32 val, void __iomem *base, u32 offset);

struct mtk_rms_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	void __iomem *base;
	void __iomem *base_inner;
	unsigned int num_clks;
	struct clk **clks;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block pm_notifier;
#endif
};


/* aa debug info */
struct mtk_ae_debug_data {
	u64 OBC_R1_Sum[4];
	u64 OBC_R2_Sum[4];
	u64 OBC_R3_Sum[4];
	u64 LTM_Sum[4];
	u64 AESTAT_Sum[8];
	u64 DGN_Sum[8];
	u64 CCM_Sum[4];
};

/* CQ setting */
void initialize(struct mtk_raw_device *dev, struct engine_callback *cb,
			    int is_slave, int is_srt, int frm_time_us);
void init_camsys_settings(struct mtk_raw_device *dev, bool is_srt, int frm_time_us);
void subsample_enable(struct mtk_raw_device *dev, int ratio);
void stagger_enable(struct mtk_raw_device *dev);
void stagger_disable(struct mtk_raw_device *dev);
void update_scq_start_period(struct mtk_raw_device *dev, int scq_ms, int frame_ms);
void apply_cq(struct mtk_raw_device *dev,
	      dma_addr_t cq_addr,
	      unsigned int cq_size, unsigned int cq_offset,
	      unsigned int sub_cq_size, unsigned int sub_cq_offset);
/* db */
void dbload_force(struct mtk_raw_device *dev);
void toggle_db(struct mtk_raw_device *dev);
void enable_tg_db(struct mtk_raw_device *dev, int en);

/* fbc */
void rwfbc_inc_setup(struct mtk_raw_device *dev);
void set_sig_sel_master(struct mtk_raw_device *dev);
void set_sig_sel_slave(struct mtk_raw_device *dev);
void set_dcif_en_slave(struct mtk_raw_device *dev);
void check_master_raw_vf_en(struct mtk_raw_device *dev);


/* trigger */
void stream_on(struct mtk_raw_device *dev, int on, bool reset_at_off);
void immediate_stream_off(struct mtk_raw_device *dev);
void trigger_rawi_r2(struct mtk_raw_device *dev);
void trigger_rawi_r5(struct mtk_raw_device *dev);
void trigger_adl(struct mtk_raw_device *dev);

/* m2m only */
void m2m_update_sof_state(struct mtk_raw_device *dev);

struct cmdq_pkt;
void write_pkt_trigger_apu_dc(struct mtk_raw_device *dev, struct cmdq_pkt *pkt);
void write_pkt_trigger_apu_frame_mode(struct mtk_raw_device *dev,
				      struct cmdq_pkt *pkt);

int raw_dump_debug_status(struct mtk_raw_device *dev, int dma_debug_dump);

/* reset */
void reset(struct mtk_raw_device *dev);
void adlrd_reset(struct mtk_cam_device *dev);
void clear_reg(struct mtk_raw_device *dev);

/* workaround */
void ae_disable(struct mtk_raw_device *dev);
/* ois compensation */
void lock_done_ctrl_enable(struct mtk_raw_device *dev, int on);

/* iommu debug */
int mtk_raw_translation_fault_cb(int port, dma_addr_t mva, void *data);
int mtk_yuv_translation_fault_cb(int port, dma_addr_t mva, void *data);
int mtk_raw_runtime_suspend(struct device *dev);
int mtk_raw_runtime_resume(struct device *dev);
int mtk_yuv_runtime_suspend(struct device *dev);
int mtk_yuv_runtime_resume(struct device *dev);
int mtk_rms_runtime_suspend(struct device *dev);
int mtk_rms_runtime_resume(struct device *dev);


/* aa debug info */
void fill_aa_info(struct mtk_raw_device *raw_dev,
				  struct mtk_ae_debug_data *ae_info);

extern struct platform_driver mtk_cam_raw_driver;
extern struct platform_driver mtk_cam_yuv_driver;
extern struct platform_driver mtk_cam_rms_driver;

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

#define CG_RAW 0
#define CG_YUV 1
#define CG_RMS 2

int cg_dump_and_test(struct device *dev, int type, bool test);

void diable_rms_module(struct mtk_raw_device *dev);
void diable_rms_pcrp(struct mtk_raw_device *raw);

int mtk_cam_raw_reset_msgfifo(struct mtk_raw_device *dev);

#endif /*__MTK_CAM_RAW_H*/
