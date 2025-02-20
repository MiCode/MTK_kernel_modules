/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __MTK_CAM_SENINF_H__
#define __MTK_CAM_SENINF_H__

#include <linux/kthread.h>
#include <linux/remoteproc.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>

#include "clk-fmeter.h"
#include "mtk_cam-seninf-def.h"
#include "imgsensor-user.h"
#include "mtk_cam-seninf-regs.h"
#include "mtk_cam-aov.h"
#include "mtk_cam-aov-data.h"
#include <linux/atomic.h>
#include <linux/kfifo.h>

/* def V4L2_MBUS_CSI2_IS_USER_DEFINED_DATA */
#define SENINF_VC_ROUTING

#define CSI_EFUSE_SET
//#define SENINF_UT_DUMP
#define ERR_DETECT_TEST

#define seninf_logi(_ctx, format, args...) do { \
	if ((_ctx) && (_ctx)->sensor_sd) { \
		dev_info((_ctx)->dev, "[%s][%s] " format, \
			(_ctx)->sensor_sd->name, __func__, ##args); \
	} \
} while (0)

#define seninf_logd(_ctx, format, args...) do { \
	if ((_ctx) && (_ctx)->sensor_sd && unlikely(*((_ctx)->core->seninf_dbg_log))) { \
		dev_info((_ctx)->dev, "[%s][%s] " format, \
			(_ctx)->sensor_sd->name, __func__, ##args); \
	} \
} while (0)

struct seninf_ctx;

/* aov sensor use */
#define AOV_SENINF_NUM 6
extern struct mtk_seninf_aov_param g_aov_param;
extern struct seninf_ctx *aov_ctx[AOV_SENINF_NUM];

struct seninf_struct_pair {
	u32 first;
	u32 second;
};

struct seninf_struct_map {
	const char * const key;
	u32 value;
};

struct seninf_mux {
	struct list_head list;
	int idx;
};

struct seninf_cam_mux {
	struct list_head list;
	int idx;
};

struct clk_fmeter_info {
	u8 fmeter_type;
	u32 fmeter_no;
};

#define DT_REMAP_MAX_CNT 4
struct seninf_vcinfo {
	struct seninf_vc vc[SENINF_VC_MAXCNT];
	int cnt;
};

#define SEQ_DT_MAX_CNT 4
struct seninf_glp_dt {
	u32 dt[SEQ_DT_MAX_CNT];
	int cnt;
};

struct seninf_dfs {
	struct device *dev;
	struct regulator *reg;
	unsigned long *freqs;
	unsigned long *volts;
	int cnt;
};

struct mtk_seninf_work {
	struct kthread_work work;
	struct kthread_delayed_work dwork;
	struct seninf_ctx *ctx;
	union work_data_t {
		unsigned int sof;
		void *data_ptr;
	} data;
};

struct seninf_struct_pair_u64 {
	u64 first;
	u64 second;
};

struct mtk_seninf_cdphy_dvfs_step {
	struct seninf_struct_pair cphy_data_rate;
	struct seninf_struct_pair dphy_data_rate;
	unsigned int csi_clk;
	struct seninf_struct_pair cdphy_voltage;
};

struct seninf_core {
	struct device *dev;
	int pm_domain_cnt;
	struct device **pm_domain_devs;
	struct clk *clk[CLK_MAXCNT];
	struct seninf_dfs dfs;
	struct list_head list;
	struct list_head list_mux;
	struct seninf_struct_pair mux_range[TYPE_MAX_NUM];
	struct seninf_mux mux[SENINF_MUX_NUM];
	struct seninf_struct_pair muxvr_range[TYPE_MAX_NUM];
#ifdef SENINF_DEBUG
	struct list_head list_cam_mux;
	struct seninf_struct_pair cammux_range[TYPE_MAX_NUM];
	struct seninf_cam_mux cam_mux[SENINF_CAM_MUX_NUM];
#endif
	struct mutex mutex;
	struct mutex cammux_page_ctrl_mutex;
	struct mutex seninf_top_mux_mutex;
	void __iomem *reg_if;
	void __iomem *reg_ana;
	int refcnt;

	/* CCU control flow */
	phandle rproc_ccu_phandle;
	struct rproc *rproc_ccu_handle;

	/* platform properties */
	int cphy_settle_delay_dt;
	int dphy_settle_delay_dt;
	int settle_delay_ck;
	int hs_trail_parameter;
	unsigned int force_glp_en; /* force enable generic long packet */
	bool is_porting_muxvr_range;

	spinlock_t spinlock_irq;

	struct kthread_worker seninf_worker;
	struct task_struct *seninf_kworker_task;

	unsigned int data_not_enough_detection_cnt;
	unsigned int err_lane_resync_detection_cnt;
	unsigned int crc_err_detection_cnt;
	unsigned int ecc_err_double_detection_cnt;
	unsigned int ecc_err_corrected_detection_cnt;
	/* seninf_mux fifo overrun irq */
	unsigned int fifo_overrun_detection_cnt;
	/* cam_mux h/v size irq */
	unsigned int size_err_detection_cnt;
#ifdef ERR_DETECT_TEST
	/* enable csi err detect flag */
	unsigned int err_detect_test_flag;
#endif
	/* mipi error detection count */
	unsigned int detection_cnt;
	/* enable csi irq flag */
	unsigned int csi_irq_en_flag;
	/* enable vsync irq flag */
	unsigned int vsync_irq_en_flag;
	unsigned int vsync_irq_detect_csi_irq_error_flag;
	/* flags for continuous detection */
	unsigned int err_detect_init_flag;
	unsigned int err_detect_termination_flag;

	/* aov sensor use */
	int pwr_refcnt_for_aov;
	int aov_sensor_id;
	int current_sensor_id;

	/* debug flag for vsync */
	u32 *seninf_vsync_debug_flag;

	/* debug log flag */
	u32 *seninf_dbg_log;

	/* debug flag for aov csi clk */
	enum mtk_cam_seninf_csi_clk_for_param aov_csi_clk_switch_flag;
	u32 aov_ut_debug_for_get_csi_param;
	/* abnormal deinit flag */
	u32 aov_abnormal_deinit_flag;
	u32 aov_abnormal_deinit_usr_fd_kill_flag;
	/* abnormal init flag */
	u32 aov_abnormal_init_flag;

	struct clk_fmeter_info fmeter[CLK_FMETER_MAX];

	/* dvfs vcore power */
	struct regulator *dvfsrc_vcore_power;
	struct mtk_seninf_cdphy_dvfs_step cdphy_dvfs_step[CDPHY_DVFS_STEP_MAX_NUM];
};

struct seninf_ctx {
	struct v4l2_subdev subdev;
	struct v4l2_async_notifier notifier;
	struct device *dev;
	struct v4l2_ctrl_handler ctrl_handler;
	struct media_pad pads[PAD_MAXCNT];
	struct v4l2_subdev_format fmt[PAD_MAXCNT];
	struct seninf_core *core;
	struct list_head list;

	int port;
	int portNum;
	int portA;
	int portB;
	int num_data_lanes;
	s64 mipi_pixel_rate;
	s64 buffered_pixel_rate;
	s64 customized_pixel_rate;

	unsigned int is_4d1c:1;
	unsigned int is_cphy:1;
	unsigned int is_test_model:4;
	unsigned int is_aov_test_model;
	unsigned int is_aov_real_sensor;
#ifdef SENINF_DEBUG
	unsigned int is_test_streamon:1;
#endif
#ifdef CSI_EFUSE_SET
	unsigned int m_csi_efuse;
#endif
	unsigned int is_secure:1;
	u64 SecInfo_addr;
	int seninfIdx;
	int pad2cam[PAD_MAXCNT][MAX_DEST_NUM];
	int pad_tag_id[PAD_MAXCNT][MAX_DEST_NUM];

	/* remote sensor */
	struct v4l2_subdev *sensor_sd;
	int sensor_pad_idx;

	/* provided by sensor */
	struct seninf_vcinfo vcinfo;

	/* store vc info of current mode  */
	struct seninf_vcinfo cur_vcinfo;

	u16 vc_group[VC_CH_GROUP_MAX_NUM];
	int fsync_vsync_src_pad; // e.g., raw, 3A-meta(general-embedded)
	int fps_n;
	int fps_d;

	/* dfs */
	int isp_freq;

	void __iomem *reg_ana_csi_rx[CSI_PORT_MAX_NUM];
	void __iomem *reg_ana_dphy_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_ana_cphy_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_mipi_csi_top_ctrl[MIPI_CSI_TOP_CTRL_NUM];
	void __iomem *reg_csirx_mac_csi[CSI_PORT_MAX_NUM];
	void __iomem *reg_csirx_mac_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_if_top;
	void __iomem *reg_if_ctrl[SENINF_NUM];
	void __iomem *reg_if_cam_mux;
	void __iomem *reg_if_cam_mux_gcsr;
	void __iomem *reg_if_cam_mux_pcsr[SENINF_CAM_MUX_NUM];
	void __iomem *reg_if_tg[SENINF_NUM];
	void __iomem *reg_if_csi2[SENINF_NUM];
	void __iomem *reg_if_mux[SENINF_MUX_NUM];

	/* resources */
	struct list_head list_mux;
	struct list_head list_cam_mux;
	struct seninf_mux *mux_by[VC_CH_GROUP_MAX_NUM][TYPE_MAX_NUM];

	/* flags */
	unsigned int csi_streaming:1;
	unsigned int streaming:1;

	int seninf_dphy_settle_delay_dt;
	int cphy_settle_delay_dt;
	int dphy_settle_delay_dt;
	int settle_delay_ck;
	int hs_trail_parameter;
	/*sensor mode customized csi params*/
	struct mtk_csi_param csi_param;

	int open_refcnt;
	struct mutex mutex;

	/* csi irq */
	unsigned int data_not_enough_cnt;
	unsigned int err_lane_resync_cnt;
	unsigned int crc_err_cnt;
	unsigned int ecc_err_double_cnt;
	unsigned int ecc_err_corrected_cnt;
	/* seninf_mux fifo overrun irq */
	unsigned int fifo_overrun_cnt;
	/* cam_mux h/v size irq */
	unsigned int size_err_cnt;
	/* error flag */
	unsigned int data_not_enough_flag;
	unsigned int err_lane_resync_flag;
	unsigned int crc_err_flag;
	unsigned int ecc_err_double_flag;
	unsigned int ecc_err_corrected_flag;
	unsigned int fifo_overrun_flag;
	unsigned int size_err_flag;
	unsigned int dbg_timeout;
	unsigned int dbg_last_dump_req;
	unsigned int power_status_flag;
	unsigned int esd_status_flag;

	/* for sentest use */
	bool allow_adjust_isp_en;
	bool single_raw_streaming_en;

	/* cammux switch debug element */
	struct mtk_cam_seninf_mux_param *dbg_chmux_param;
#ifdef ERR_DETECT_TEST
	unsigned int test_cnt;
#endif
	/* record pid */
	struct pid *pid;
	/* vcore_step/clk/clk_src index */
	u32 vcore_step_index;
	u32 clk_index;
	u32 clk_src_index;
	/* debug_current_status */
	u64 debug_cur_sys_time_in_ns;
	u32 debug_cur_dphy_irq;
	u32 debug_cur_cphy_irq;
	u32 debug_cur_mac_irq;
	u32 debug_cur_seninf_irq;
	u32 debug_cur_temp;
	u32 debug_cur_mac_csi2_size_chk_ctrl0;
	u32 debug_cur_mac_csi2_size_chk_ctrl1;
	u32 debug_cur_mac_csi2_size_chk_ctrl2;
	u32 debug_cur_mac_csi2_size_chk_ctrl3;
	u32 debug_cur_mac_csi2_size_chk_ctrl4;
	u32 debug_cur_mac_csi2_size_chk_rcv0;
	u32 debug_cur_mac_csi2_size_chk_rcv1;
	u32 debug_cur_mac_csi2_size_chk_rcv2;
	u32 debug_cur_mac_csi2_size_chk_rcv3;
	u32 debug_cur_mac_csi2_size_chk_rcv4;
};

struct mtk_cam_seninf_irq_event_st {
	/* for linux kfifo */
	void *msg_buffer;
	unsigned int fifo_size;
	atomic_t is_fifo_overflow;
	struct kfifo msg_fifo;
};

#define USED_CAMMUX_MAX_MUM 10
#define USED_CSI_MAX_MUM 4
struct mtk_cam_seninf_vsync_info {
	unsigned int vsync_irq_st;
	unsigned int vsync_irq_st_h;
	unsigned int ctx_port[USED_CSI_MAX_MUM];
	unsigned int csi_irq_st[USED_CSI_MAX_MUM];
	unsigned int csi_packet_cnt_st[USED_CSI_MAX_MUM];
	unsigned int used_csi_port_num;
	unsigned int used_cammux_num;
	unsigned int used_cammux[USED_CAMMUX_MAX_MUM];
	unsigned int seninf_mux_irq_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_chk_res_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_irq_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_tag_vc_sel_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_tag_dt_sel_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_ctrl_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_chk_ctrl_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_chk_err_res_st[USED_CAMMUX_MAX_MUM];
	unsigned int cammux_opt_st[USED_CAMMUX_MAX_MUM];
	u64 time_mono;
};

struct mtk_cam_seninf_vsync_work {
	struct kthread_work work;
	struct seninf_core *core;
	struct mtk_cam_seninf_vsync_info vsync_info;
};

enum VSYNC_DETECT_LOG_LEVEL {
	DISABLE_VSYNC_DETECT = 0,
	ENABLE_VSYNC_DETECT_PER_FRAME_INFO = 1,
	ENABLE_VSYNC_DETECT_ONLY_CSI_IRQ_STATUS_ERROR_INFO = 2,
};
#endif
