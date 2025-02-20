/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_H__
#define __MTK_CAM_SENINF_TSREC_H__


#ifdef FS_UT
#include "ut_fs_tsrec.h"
#else

#include <linux/device.h>  /* for device structure */

#include "mtk_cam-seninf.h"
#include "imgsensor-user.h"

#endif // FS_UT


/* force disable TSREC software flow */
// #define FORCE_DISABLE_TSREC


/******************************************************************************
 * TSREC information structures/enums
 *****************************************************************************/
struct mtk_cam_seninf_tsrec_vc_dt_info {
	/* general info */
	unsigned int vc;
	unsigned int dt;
	unsigned int out_pad; // PAD_SRC_RAW0 ~ PAD_SRC_RAW2

	/* custom assign info */
	/* --- 0:AUTO, others: 1/2/3 (see imgsensor-user.h for details) */
	enum mtk_cam_seninf_tsrec_exp_id cust_assign_to_tsrec_exp_id;

	/* for sensor HW property */
	unsigned int is_sensor_hw_pre_latch_exp;
};


/******************************************************************************
 * TSREC basic/utilities functions
 *****************************************************************************/
unsigned int get_tsrec_timer_en_status(void);

unsigned int chk_exist_tsrec_hw(const char *caller,
	const unsigned int do_print_log);

unsigned int chk_tsrec_no_valid(const unsigned int tsrec_no,
	const char *caller);


/*
 * return:
 *     0 => tsrec timer is not enabled;
 *          (give caller tick & time in us to 0)
 *     1 => tsrec timer is enabled;
 *          (give caller tsrec current tick & time in us)
 */
int mtk_cam_seninf_g_tsrec_current_time(
	unsigned long long *p_tick, unsigned long long *p_time_us);


/******************************************************************************
 * TSREC-REGS notify/get status functions
 *****************************************************************************/
void notify_tsrec_update_top_cfg(const unsigned int val);

void notify_tsrec_update_timer_en_status(const unsigned int val);
void notify_tsrec_update_timer_cfg(const unsigned int val);
unsigned int notify_tsrec_get_timer_en_status(void);

void notify_tsrec_update_intr_en(const unsigned int val);
void notify_tsrec_update_intr_status(const unsigned int val);
void notify_tsrec_update_intr_en_2(const unsigned int val);
void notify_tsrec_update_intr_status_2(const unsigned int val);

void notify_tsrec_update_tsrec_n_clk_en_status(const unsigned int tsrec_no,
	const unsigned int val);
void notify_tsrec_update_tsrec_n_trig_src(const unsigned int tsrec_no,
	const unsigned int reg_val);
void notify_tsrec_update_tsrec_n_exp_vc_dt(const unsigned int tsrec_no,
	const unsigned int exp_n, const unsigned int reg_val);


/******************************************************************************
 * TSREC flow/logic CTRL functions
 *****************************************************************************/
/*
 * call this API when seninf runtime resume/suspend.
 */
void mtk_cam_seninf_tsrec_timer_enable(const unsigned int en);


/*
 * call these API when seninf get vcinfo.
 */
void mtk_cam_seninf_tsrec_reset_vc_dt_info(struct seninf_ctx *inf_ctx,
	const unsigned int seninf_idx);
void mtk_cam_seninf_tsrec_update_vc_dt_info(struct seninf_ctx *inf_ctx,
	const unsigned int seninf_idx,
	const struct mtk_cam_seninf_tsrec_vc_dt_info *vc_dt_info);


/*
 * call this API when set mux idle.
 */
void mtk_cam_seninf_tsrec_n_reset(const unsigned int seninf_idx);


/*
 * call this API when you really want to start setup tsrec
 *     to listen sensor vsync/hsync signal.
 */
void mtk_cam_seninf_tsrec_n_start(const unsigned int seninf_idx,
	const unsigned int tsrec_no);


void mtk_cam_seninf_tsrec_query_ts_records(const unsigned int tsrec_no);


/*
 * return: 0 without any error; others => has error.
 *
 * for user get timestamp information that in tsrec.
 * --- pack timestamp information into below structure.
 */
int mtk_cam_seninf_tsrec_get_timestamp_info(const unsigned int tsrec_no,
	struct mtk_cam_seninf_tsrec_timestamp_info *ts_info);


int tsrec_cb_handler(const unsigned int seninf_idx, const unsigned int tsrec_no,
	const unsigned int cmd, void *arg, const char *caller);


/******************************************************************************
 * TSREC debug/unit-test functions
 *****************************************************************************/
void mtk_cam_seninf_tsrec_dbg_dump_vc_dt_info(const unsigned int seninf_idx,
	const char *caller);

void mtk_cam_seninf_tsrec_dbg_dump_tsrec_n_regs_info(const char *caller);

void mtk_cam_seninf_tsrec_dbg_dump_ts_records(const unsigned int tsrec_no);


/******************************************************************************
 * TSREC init/sysfs/irq-init functions
 *****************************************************************************/
#ifndef FS_UT
/*
 * call this API when seninf core probe.
 */
void mtk_cam_seninf_tsrec_irq_init(struct seninf_core *core, const int irq);
#endif // !FS_UT


/*
 * call this API when seninf core probe.
 */
#ifndef FS_UT
void mtk_cam_seninf_tsrec_init(struct device *dev, void __iomem *p_seninf_base);
#else
void mtk_cam_seninf_tsrec_init(void);
#endif // !FS_UT


/*
 * call this API when seninf core remove.
 */
void mtk_cam_seninf_tsrec_uninit(void);


/******************************************************************************
 * TSREC ONLY for unit-test functions
 *****************************************************************************/
#ifdef FS_UT
void mtk_cam_seninf_tsrec_ut_dbg_sysfs_ctrl(const unsigned int cmd_val);
void mtk_cam_seninf_tsrec_ut_dbg_irq_seninf_tsrec(void);
void mtk_cam_seninf_tsrec_init_for_ut_test(const unsigned int flag,
	const unsigned int tsrec_hw_cnt, const unsigned int seninf_hw_cnt);
#endif // FS_UT


#endif
