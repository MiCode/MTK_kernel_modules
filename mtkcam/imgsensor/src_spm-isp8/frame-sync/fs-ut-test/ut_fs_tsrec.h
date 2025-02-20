/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __UT_FS_TSREC_H__
#define __UT_FS_TSREC_H__


/*******************************************************************************
 * for unit test (define/struct/enum) - TSREC
 ******************************************************************************/
#define SENINF_MAX_CNT               (12)


/*----------------------------------------------------------------------------*/
/* !!! sync from imgsensor-user.h !!! */
/*----------------------------------------------------------------------------*/
enum {
	PAD_SINK = 0,
	PAD_SRC_RAW0,
	PAD_SRC_RAW1,
	PAD_SRC_RAW2,
	PAD_SRC_RAW_W0,
	PAD_SRC_RAW_W1,
	PAD_SRC_RAW_W2,
	PAD_SRC_RAW_EXT0,
	PAD_SRC_PDAF0,
	PAD_SRC_PDAF1,
	PAD_SRC_PDAF2,
	PAD_SRC_PDAF3,
	PAD_SRC_PDAF4,
	PAD_SRC_PDAF5,
	PAD_SRC_PDAF6,
	PAD_SRC_HDR0,
	PAD_SRC_HDR1,
	PAD_SRC_HDR2,
	PAD_SRC_GENERAL0,
	PAD_MAXCNT,
	PAD_ERR = 0xffff,
};

enum mtk_cam_seninf_tsrec_exp_id {
	TSREC_EXP_ID_AUTO = 0,
	TSREC_EXP_ID_1,
	TSREC_EXP_ID_2,
	TSREC_EXP_ID_3,
};


/*----------------------------------------------------------------------------*/
/* !!! sync from linux/irqreturn.h for UT testing !!! */
/*----------------------------------------------------------------------------*/
enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};
// typedef enum irqreturn irqreturn_t; // check service will fail.
#define irqreturn_t enum irqreturn


/*----------------------------------------------------------------------------*/
/* !!! fake for linux/compiler_types.h for UT testing !!! */
/*----------------------------------------------------------------------------*/
struct device_node {
	const char *name;
	const char *full_name;
};

struct device {
	struct device_node *of_node;
};


/*----------------------------------------------------------------------------*/
/* !!! fake for linux/compiler_types.h for UT testing !!! */
/*----------------------------------------------------------------------------*/
// #define __iomem __attribute__((noderef, address_space(__iomem)))
#define __iomem


#define __u32 unsigned int
#define __u64 unsigned long long


/*----------------------------------------------------------------------------*/
/* !!! fake for seninf struct for UT testing !!! */
/*----------------------------------------------------------------------------*/
struct seninf_core {};
struct seninf_ctx {
	struct device *dev;
	int seninfIdx;
};


/*----------------------------------------------------------------------------*/
/* !!! sync from mtk_camera-v4l2-controls-common.h !!! */
/*----------------------------------------------------------------------------*/
#define TSREC_TS_REC_MAX_CNT (4)
#define TSREC_EXP_MAX_CNT    (3)

struct mtk_cam_seninf_tsrec_vsync_info {
	/* source info */
	__u32 tsrec_no;
	__u32 seninf_idx;

	__u64 irq_sys_time_ns; // ktime_get_boottime_ns()
	__u64 irq_tsrec_ts_us;
};


struct mtk_cam_seninf_tsrec_timestamp_exp {
	__u64 ts_us[TSREC_TS_REC_MAX_CNT];
};

struct mtk_cam_seninf_tsrec_timestamp_info {
	/* source info */
	__u32 tsrec_no;
	__u32 seninf_idx;

	/* basic info */
	__u32 tick_factor; // MHz

	/* interrupt pre-latch exp no */
	__u32 irq_pre_latch_exp_no;
	/* record when receive a interrupt (top-half) */
	__u64 irq_sys_time_ns; // ktime_get_boottime_ns()
	__u64 irq_tsrec_ts_us;

	/* current tick when query/send tsrec timestamp info */
	__u64 tsrec_curr_tick;
	struct mtk_cam_seninf_tsrec_timestamp_exp exp_recs[TSREC_EXP_MAX_CNT];
};


/**
 * TSREC - call back info
 *
 *         call back function prototype, see mtk_cam-seninf-tsrec.c
 */
enum tsrec_cb_cmd {
	/* user get tsrec information */
	TSREC_CB_CMD_READ_CURR_TS,
	TSREC_CB_CMD_READ_TS_INFO,
};

enum tsrec_cb_ctrl_error_type {
	TSREC_CB_CTRL_ERR_NONE = 0,
	TSREC_CB_CTRL_ERR_INVALID,
	TSREC_CB_CTRL_ERR_NOT_CONNECTED_TO_TSREC,
	TSREC_CB_CTRL_ERR_CB_FUNC_PTR_NULL,
	TSREC_CB_CTRL_ERR_CMD_NOT_FOUND,
	TSREC_CB_CTRL_ERR_CMD_ARG_PTR_NULL,
	TSREC_CB_CTRL_ERR_CMD_IN_SENINF_SUSPEND,
};

/* call back function prototype, see mtk_cam-seninf-tsrec.c */
typedef int (*tsrec_cb_handler_func_ptr)(const unsigned int seninf_idx,
	const unsigned int tsrec_no, const unsigned int cmd, void *arg,
	const char *caller);

struct mtk_cam_seninf_tsrec_cb_info {
	/* check this sensor is => 1: with TSREC; 0: NOT with TSREC */
	__u32 is_connected_to_tsrec;

	/* !!! below data are valid ONLY when "is_connected_to_tsrec != 0" !!! */
	__u32 seninf_idx;
	__u32 tsrec_no;
	tsrec_cb_handler_func_ptr tsrec_cb_handler;
};


/*******************************************************************************
 * for unit test (function) - TSREC
 ******************************************************************************/
unsigned int ut_fs_tsrec_g_tsrec_max_cnt(void);
unsigned int ut_fs_tsrec_g_seninf_max_cnt(void);
unsigned int ut_fs_tsrec_g_tsrec_irq_max_cnt(void);

void ut_fs_tsrec_write_reg(const unsigned int addr, const unsigned int val);
unsigned int ut_fs_tsrec_read_reg(const unsigned int addr);

unsigned int ut_fs_tsrec_chk_fifo_not_empty(void);
void ut_fs_tsrec_fifo_out(void *p_data);
unsigned int ut_fs_tsrec_fifo_in(const void *p_data);


/*******************************************************************************
 * for unit test (fake function of linux APIs)
 ******************************************************************************/
static inline int tsrec_utils_of_irq_count(struct device_node *dev)
{
	return ut_fs_tsrec_g_tsrec_irq_max_cnt();
}

static inline void disable_irq(unsigned int irq) {}
static inline void enable_irq(unsigned int irq) {}

unsigned long long ktime_get_boottime_ns(void);


/*******************************************************************************
 * for unit test (main function) - TSREC
 ******************************************************************************/
void ut_fs_tsrec(void);

#endif
