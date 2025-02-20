/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __MTK_CAM_SENINF_H__
#define __MTK_CAM_SENINF_H__

#include <linux/kthread.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>

#include "clk-fmeter.h"
#include "mtk_cam-seninf-def.h"
#include "mtk_cam-seninf-rproc-ctrl.h"
#include "imgsensor-user.h"
#include "mtk_cam-seninf-regs.h"
#include "mtk_cam-aov.h"
#include "mtk_cam-aov-data-isp8.h"
#include <linux/atomic.h>
#include <linux/kfifo.h>

/* def V4L2_MBUS_CSI2_IS_USER_DEFINED_DATA */
#define SENINF_VC_ROUTING

#define CSI_EFUSE_SET
//#define SENINF_UT_DUMP
#define ERR_DETECT_TEST
#undef DOUBLE_PIXEL_EN

#define seninf_logi(_ctx, format, args...) do { \
	if ((_ctx)) { \
		dev_info((_ctx)->dev, "[%s][%s] " format, \
			((_ctx)->sensor_sd) ? (_ctx)->sensor_sd->name : "none", \
			 __func__, ##args); \
	} else { \
		pr_info("[no_seninf_ctx][%s] " format, \
			 __func__, ##args); \
	} \
} while (0)

#define seninf_logd(_ctx, format, args...) do { \
	if ((_ctx) && unlikely(*((_ctx)->core->seninf_dbg_log))) { \
		dev_info((_ctx)->dev, "[%s][%s] " format, \
			((_ctx)->sensor_sd) ? (_ctx)->sensor_sd->name : "none", \
			__func__, ##args); \
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

struct seninf_outmux {
	struct list_head list;
	int idx;
	enum CAM_TYPE_ENUM cam_type;
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

struct csi_reg_base {
	void __iomem *reg_csi_base[SENINF_CSI_REG_BASE_NUN];
	enum SENINF_ASYNC_ENUM seninf_async_idx;
};

#define MAX_OUTMUX_TAG_NUM 8

struct outmux_tag_cfg {
	bool enable;
	u8 filt_vc;
	u8 filt_dt;
	u32 exp_hsize;
	u32 exp_vsize;
};

struct outmux_cfg {
	struct list_head list;
	u8 outmux_idx;
	u8 src_mipi;
	u8 src_sen;
	u8 pix_mode;
	struct outmux_tag_cfg tag_cfg[MAX_OUTMUX_TAG_NUM];
};

struct mtk_cam_seninf_bit_error {
	u32 bit_err_ctrl;
	u32 bit_err_cnt;
	u64 min_bit;
	u64 max_bit;
	u32 min_cycle_msb;
	u32 min_cycle_lsb;
	u32 max_cycle_msb;
	u32 max_cycle_lsb;
	u32 seninf_clk_mhz;
	u32 bit_rate_mhz;
};

struct mtk_cam_seninf_spacer_detector {
	u32 spacer;
	u32 vc;
	u32 dt;
	u32 valid_cnt;
	u32 num_hs1;
	u32 num_hs2;
	u32 wc;
	u32 trio;
};

struct seninf_core {
	struct device *dev;
	int pm_domain_cnt;
	struct device **pm_domain_devs;
	struct clk *clk[CLK_MAXCNT];
	struct seninf_dfs dfs;
	struct list_head list;
#ifdef SENINF_DEBUG
	struct list_head list_outmux;
	struct seninf_outmux outmux[SENINF_OUTMUX_NUM];
#endif
	struct mutex mutex;
	struct mutex seninf_top_rg_mutex;
	struct mutex cammux_page_ctrl_mutex;
	struct mutex seninf_top_mux_mutex;
	void __iomem *reg_seninf_top;
	void __iomem *reg_seninf_async;
	void __iomem *reg_seninf_tm;
	void __iomem *reg_seninf_outmux[SENINF_OUTMUX_NUM];
	void __iomem *reg_seninf_outmux_inner[SENINF_OUTMUX_NUM];

	struct csi_reg_base reg_csi_base[CSI_PORT_PHYSICAL_MAX_NUM];
	//void __iomem *reg_if;
	//void __iomem *reg_ana;
	int refcnt;

	/* CCU rproc ctrl */
	struct seninf_rproc_ccu_ctrl ccu_rproc_ctrl;

	/* platform properties */
	int cphy_settle_delay_dt;
	int dphy_settle_delay_dt;
	int settle_delay_ck;
	int hs_trail_parameter;
	unsigned int force_glp_en; /* force enable generic long packet */
	bool is_porting_muxvr_range;

	spinlock_t spinlock_irq;
	spinlock_t spinlock_aov;

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

struct mtk_cam_sentest_watchdog {
	struct timer_list timer;
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
	unsigned int tsrec_idx;
	u64 SecInfo_addr;
	int seninfAsyncIdx;
	int seninfSelSensor;
	int pad2cam[PAD_MAXCNT][MAX_DEST_NUM];
	int pad_tag_id[PAD_MAXCNT][MAX_DEST_NUM];
	/* bit error rate */
	struct mtk_cam_seninf_bit_error ber;
	/* cphy lrte spacer detector */
	struct mtk_cam_seninf_spacer_detector lrte_sd;

	/* fake sensor */
	struct mtk_fake_sensor_info fake_sensor_info;

	/* remote sensor */
	struct v4l2_subdev *sensor_sd;
	int sensor_pad_idx;
	int current_sensor_id;

	/* provided by sensor */
	struct seninf_vcinfo vcinfo;
	enum mtk_sensor_usage sensor_usage;

	/* store vc info of current mode  */
	struct seninf_vcinfo cur_vcinfo;

	u16 vc_group[VC_CH_GROUP_MAX_NUM];
	int fsync_vsync_src_pad; // e.g., raw, 3A-meta(general-embedded)
	int fps_n;
	int fps_d;
	u32 csi_clk;
	u32 hblank_pct;
	u32 vblank_pct;

	/* ref vs */
	int cur_first_vs;
	int cur_last_vs;

	/* */
	u64 cfg_done_max_delay; /* in us */

	/* dfs */
	int isp_freq;

	void __iomem *reg_ana_csi_rx[CSI_PORT_MAX_NUM];
	void __iomem *reg_ana_dphy_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_ana_cphy_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_csirx_mac_csi[CSI_PORT_MAX_NUM];
	void __iomem *reg_csirx_mac_top[CSI_PORT_MAX_NUM];
	void __iomem *reg_if_top;
	void __iomem *reg_if_async;
	void __iomem *reg_if_outmux[SENINF_OUTMUX_NUM];
	void __iomem *reg_if_outmux_inner[SENINF_OUTMUX_NUM];
	void __iomem *reg_if_tg[SENINF_ASYNC_NUM];

	/* resources */
	struct list_head list_outmux;
	//struct seninf_mux *mux_by[VC_CH_GROUP_MAX_NUM][TYPE_MAX_NUM];
	bool outmux_disable_list[SENINF_OUTMUX_NUM];
	bool outmux_disable_list_for_v2[SENINF_OUTMUX_NUM];

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
	unsigned int set_abort_flag;

	/* for sentest use */
	bool sentest_adjust_isp_en;
	bool sentest_seamless_ut_en;
	bool sentest_seamless_is_set_camtg_done;
	bool sentest_mipi_measure_en;
	u64 sentest_irq_counter;
	u64 sentest_seamless_irq_ref;
	enum SENTEST_SEAMLESS_STATUS sentest_seamless_ut_status;
	struct mtk_cam_sentest_watchdog sentest_watchdog;
	struct mtk_seamless_switch_param sentest_seamless_cfg;
	struct kthread_worker sentest_worker;
	struct task_struct *sentest_kworker_task;

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
	u32 debug_cur_mac_csi2_size_chk_ctrl5;
	u32 debug_cur_mac_csi2_size_chk_rcv0;
	u32 debug_cur_mac_csi2_size_chk_rcv1;
	u32 debug_cur_mac_csi2_size_chk_rcv2;
	u32 debug_cur_mac_csi2_size_chk_rcv3;
	u32 debug_cur_mac_csi2_size_chk_rcv4;
	u32 debug_cur_mac_csi2_size_chk_rcv5;
	u32 debug_cur_mac_csi2_size_chk_exp0;
	u32 debug_cur_mac_csi2_size_chk_exp1;
	u32 debug_cur_mac_csi2_size_chk_exp2;
	u32 debug_cur_mac_csi2_size_chk_exp3;
	u32 debug_cur_mac_csi2_size_chk_exp4;
	u32 debug_cur_mac_csi2_size_chk_exp5;
	u32 debug_cur_mac_csi2_size_chk_err0;
	u32 debug_cur_mac_csi2_size_chk_err1;
	u32 debug_cur_mac_csi2_size_chk_err2;
	u32 debug_cur_mac_csi2_size_chk_err3;
	u32 debug_cur_mac_csi2_size_chk_err4;
	u32 debug_cur_mac_csi2_size_chk_err5;
	u32 debug_cur_mac_csi2_size_irq_en0;
	u32 debug_cur_mac_csi2_size_irq_en1;
	u32 debug_cur_mac_csi2_size_irq_en2;
	u32 debug_cur_mac_csi2_size_irq_en3;
	u32 debug_cur_mac_csi2_size_irq_en4;
	u32 debug_cur_mac_csi2_size_irq_en5;
	u32 debug_cur_mac_csi2_size_irq0;
	u32 debug_cur_mac_csi2_size_irq1;
	u32 debug_cur_mac_csi2_size_irq2;
	u32 debug_cur_mac_csi2_size_irq3;
	u32 debug_cur_mac_csi2_size_irq4;
	u32 debug_cur_mac_csi2_size_irq5;
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

enum seninf_tag_order_type {
	EXPOSURE_FIRST,
	EXPOSURE_MIDDLE,
	EXPOSURE_LAST,
};

#endif
