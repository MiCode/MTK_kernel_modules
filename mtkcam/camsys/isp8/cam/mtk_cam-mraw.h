/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_MRAW_H
#define __MTK_CAM_MRAW_H

#include <linux/kfifo.h>
#include <linux/suspend.h>

#include "mtk_cam-plat.h"
#include "mtk_cam-engine.h"
#include "mtk_cam-dvfs_qos.h"

#define MRAW_WRITE_BITS(RegAddr, RegName, FieldName, FieldValue) do {\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName = FieldValue;\
	writel_relaxed(reg.Raw, RegAddr);\
} while (0)

#define MRAW_WRITE_REG(RegAddr, RegValue) ({\
	writel_relaxed(RegValue, RegAddr);\
})

#define MRAW_READ_BITS(RegAddr, RegName, FieldName) ({\
	union RegName reg;\
	\
	reg.Raw = readl_relaxed(RegAddr);\
	reg.Bits.FieldName;\
})

#define MRAW_READ_REG(RegAddr) ({\
	unsigned int var;\
	\
	var = readl_relaxed(RegAddr);\
	var;\
})

struct mtk_cam_ctx;
struct mtk_cam_request;

enum mraw_module_id {
	MRAW_START = 0,
	MRAW_0 = MRAW_START,
	MRAW_1 = 1,
	MRAW_2 = 2,
	MRAW_3 = 3,
	MRAW_END
};

enum mraw_db_load_src {
	MRAW_DB_SRC_SUB_P1_DONE = 0,
	MRAW_DB_SRC_SOF         = 1,
};

enum mraw_int_en1 {
	MRAW_INT_EN1_VS_INT_EN                   = (1L<<0),
	MRAW_INT_EN1_TG_INT1_EN                  = (1L<<1),
	MRAW_INT_EN1_TG_INT2_EN                  = (1L<<2),
	MRAW_INT_EN1_TG_INT3_EN                  = (1L<<3),
	MRAW_INT_EN1_TG_INT4_EN                  = (1L<<4),
	MRAW_INT_EN1_EXPDON_EN                   = (1L<<5),
	MRAW_INT_EN1_TG_ERR_EN                   = (1L<<6),
	MRAW_INT_EN1_TG_GBERR_EN                 = (1L<<7),
	MRAW_INT_EN1_TG_SOF_INT_EN               = (1L<<8),
	MRAW_INT_EN1_TG_SOF_WAIT_EN              = (1L<<9),
	MRAW_INT_EN1_TG_SOF_DROP_EN              = (1L<<10),
	MRAW_INT_EN1_VS_INT_ORG_EN               = (1L<<11),
	MRAW_INT_EN1_CQ_DB_LOAD_ERR_EN           = (1L<<12),
	MRAW_INT_EN1_CQ_MAX_START_DLY_ERR_INT_EN = (1L<<13),
	MRAW_INT_EN1_CQ_CODE_ERR_EN              = (1L<<14),
	MRAW_INT_EN1_CQ_VS_ERR_EN                = (1L<<15),
	MRAW_INT_EN1_CQ_TRIG_DLY_INT_EN          = (1L<<16),
	MRAW_INT_EN1_CQ_SUB_CODE_ERR_EN          = (1L<<17),
	MRAW_INT_EN1_CQ_SUB_VS_ERR_EN            = (1L<<18),
	MRAW_INT_EN1_PASS1_DONE_EN               = (1L<<19),
	MRAW_INT_EN1_SW_PASS1_DONE_EN            = (1L<<20),
	MRAW_INT_EN1_SUB_PASS1_DONE_EN           = (1L<<21),
	MRAW_INT_EN1_DMA_ERR_EN                  = (1L<<22),
	MRAW_INT_EN1_SW_ENQUE_ERR_EN             = (1L<<23),
	MRAW_INT_EN1_INT_WCLR_EN                 = (1L<<24),
};

enum mraw_int_en5 {
	MRAW_INT_EN5_IMGO_M1_ERR_EN              = (1L<<0),
	MRAW_INT_EN5_IMGBO_M1_ERR_EN             = (1L<<1),
	MRAW_INT_EN5_CPIO_M1_ERR_EN              = (1L<<2),
};

enum mraw_wfbc {
	MRAW_WFBC_IMGO                           = (1L<<0),
	MRAW_WFBC_IMGBO                          = (1L<<1),
	MRAW_WFBC_CPIO                           = (1L<<2),
};

enum mqe_mode {
	UL_MODE = 0,
	UR_MODE,
	DL_MODE,
	DR_MODE,
	PD_L_MODE,
	PD_R_MODE,
	PD_M_MODE,
	PD_B01_MODE,
	PD_B02_MODE,
};

enum mbn_dir {
	MBN_POW_VERTICAL = 0,
	MBN_POW_HORIZONTAL,
	MBN_POW_SPARSE_CONCATENATION,
	MBN_POW_SPARSE_INTERLEVING,
};

enum cpi_dir {
	CPI_POW_VERTICAL = 0,
	CPI_POW_HORIZONTAL,
	CPI_POW_SPARSE_CONCATENATION,
	CPI_POW_SPARSE_INTERLEVING,
};

struct mtk_mraw_device {
	struct device *dev;
	struct mtk_cam_device *cam;
	unsigned int id;
	int irq;
	void __iomem *base;
	void __iomem *base_inner;
	void __iomem *top;
	unsigned int num_clks;
	struct clk **clks;
	struct mtk_mraw_pipeline *pipeline;
	unsigned int cammux_id;

	int fifo_size;
	void *msg_buffer;
	struct kfifo msg_fifo;
	atomic_t is_fifo_overflow;

	struct engine_fsm fsm;
	struct apply_cq_ref *cq_ref;

	unsigned int sof_count;
	unsigned int frame_wait_to_process;
#ifdef CONFIG_PM_SLEEP
	struct notifier_block notifier_blk;
#endif
	atomic_t is_vf_on;
	atomic_t is_sw_clr;

	/* mmqos */
	struct mtk_camsys_qos qos;

	/* for BWR */
	int mraw_avg_applied_bw_w;
	int mraw_peak_applied_bw_w;

	unsigned int mraw_error_count;
};

void mraw_reset(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_reset_msgfifo(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_dev_config(struct mtk_mraw_device *mraw_dev, unsigned int sub_ratio,
	int frm_time_us);
void mtk_cam_mraw_update_start_period(struct mtk_mraw_device *mraw_dev, int scq_ms);
int mtk_cam_mraw_dev_stream_on(struct mtk_mraw_device *mraw_dev, bool on);
int mtk_cam_mraw_top_config(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_dma_config(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_fbc_config(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_top_enable(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_fbc_enable(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_tg_disable(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_top_disable(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_fbc_disable(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_vf_on(struct mtk_mraw_device *mraw_dev, bool on);
int mtk_cam_mraw_is_vf_on(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_toggle_tg_db(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_toggle_db(struct mtk_mraw_device *mraw_dev);
int mtk_cam_mraw_trigger_wfbc_inc(struct mtk_mraw_device *mraw_dev);
void apply_mraw_cq(struct mtk_mraw_device *mraw_dev,
	      dma_addr_t cq_addr, unsigned int cq_size, unsigned int cq_offset,
	      int initial);
void mtk_cam_mraw_copy_user_input_param(struct mtk_cam_device *cam,	void *vaddr,
	struct mtk_mraw_pipeline *mraw_pipe);
int mtk_cam_mraw_cal_cfg_info(struct mtk_cam_device *cam,
	unsigned int pipe_id, struct mtkcam_ipi_mraw_frame_param *mraw_param,
	unsigned int imgo_fmt);
void mtk_cam_mraw_get_mqe_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
void mtk_cam_mraw_get_mbn_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
void mtk_cam_mraw_get_cpi_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
void mtk_cam_mraw_get_dbg_size(struct mtk_cam_device *cam, unsigned int pipe_id,
	unsigned int *width, unsigned int *height);
int mtk_mraw_translation_fault_callback(int port, dma_addr_t mva, void *data);
void mtk_cam_mraw_debug_dump(struct mtk_mraw_device *mraw_dev);
int mtk_mraw_runtime_suspend(struct device *dev);
int mtk_mraw_runtime_resume(struct device *dev);
extern struct platform_driver mtk_cam_mraw_driver;

#endif
