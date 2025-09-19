/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_REGS_H__
#define __MTK_CAM_SENINF_TSREC_REGS_H__


#ifndef FS_UT
#include <linux/io.h>
#else
#include "ut_fs_tsrec.h"
#endif // !FS_UT


/******************************************************************************
 * TSREC registers CTRL/operation function
 *****************************************************************************/
/*
 * TSREC_CFG --- config clk enable for TSREC.
 */
void mtk_cam_seninf_s_tsrec_top_cfg_clk_en_bit(const unsigned int tsrec_n,
	const unsigned int clk_en);


/*
 * TSREC_TIMER_CFG -- config clk enable/clr for timer.
 */
void mtk_cam_seninf_s_tsrec_timer_cfg(const unsigned int en);


/*
 * TSREC_TIMER_CFG -- config clk latchh for timer.
 */
unsigned long long mtk_cam_seninf_tsrec_latch_time(void);


void mtk_cam_seninf_s_tsrec_intr_wclr_en(const unsigned int wclr_en);


void mtk_cam_seninf_tsrec_s_device_irq_sel(const unsigned int irq_id,
	const unsigned int val);


/*---------------------------------------------------------------------------*/
/*
 * TSREC_n_CFG --- tsrec n cnt clear.
 *     @clr_exp_cnt_n:
 *         negative => clear all exp cnt.
 */
void mtk_cam_seninf_s_tsrec_n_cfg(const unsigned int tsrec_n,
	const int clr_exp_cnt_n);


/*
 * TSREC_n_INT_EN -- interrupt enable.
 */
unsigned int mtk_cam_seninf_g_tsrec_n_intr_en(const unsigned int tsrec_n);
void mtk_cam_seninf_s_tsrec_n_intr_en(const unsigned int tsrec_n,
	const unsigned int exp0, const unsigned int exp1, const unsigned int exp2,
	const unsigned int trig_src, const unsigned int en);


/*
 * TSREC_n_INT_STATUS -- interrupt status.
 */
unsigned int mtk_cam_seninf_g_tsrec_n_intr_status(const unsigned int tsrec_n);
void mtk_cam_seninf_clr_tsrec_n_intr_status(const unsigned int tsrec_n,
	const unsigned int mask);


/*
 * TSREC_n_SW_RST --- tsrec n sw rst.
 */
void mtk_cam_seninf_s_tsrec_sw_rst(const unsigned int tsrec_n,
	const unsigned int en);


/*
 * TSREC_n_TS_CNT --- tsrec n timestamp cnt (0~3).
 */
unsigned int mtk_cam_seninf_g_tsrec_ts_cnt(const unsigned int tsrec_n);
unsigned int mtk_cam_seninf_g_tsrec_ts_cnt_by_exp_n(const unsigned int reg_val,
	const unsigned int exp_n);


/*
 * TSREC_n_TRIG_SRC --- tsrec n hsync or vsync config.
 *     @exp_n: negative => for all exp.
 *     @type: 0 => vsync; 1 => hsync.
 */
void mtk_cam_seninf_s_tsrec_trig_src(const unsigned int tsrec_n,
	const int exp_n, const unsigned int src_type);


/*
 * TSREC_N_EXPn_VC_DT --- tsrec n expN vc/dt select.
 */
void mtk_cam_seninf_s_tsrec_exp_vc_dt(const unsigned int tsrec_n,
	const unsigned int exp_n,
	const unsigned int vsync_vc,
	const unsigned int hsync_vc, const unsigned int hsync_dt);

void mtk_cam_seninf_tsrec_rst_tsrec_exp_vc_dt(const unsigned int tsrec_n,
	const unsigned int exp_n);


/*
 * TSREC_N_EXPn_CNTs --- tsrec n expN cntS value. (each exp clk cnt <=> TS)
 *     @cnt: timetamp record cnt. (4 records. (0~3))
 */
unsigned long long mtk_cam_seninf_g_tsrec_exp_cnt(const unsigned int tsrec_n,
	const unsigned int exp_n, const unsigned int cnt);


/******************************************************************************
 * TSREC registers init function
 *****************************************************************************/
void mtk_cam_seninf_tsrec_regs_iomem_uninit(void);
void mtk_cam_seninf_tsrec_regs_iomem_init(void __iomem *p_seninf_base,
	const unsigned int tsrec_hw_cnt);


#endif
