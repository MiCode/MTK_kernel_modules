/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 MediaTek Inc. */

#ifndef __ADAPTOR_H__
#define __ADAPTOR_H__

#include <linux/i2c.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#include "adaptor-def.h"
#include "adaptor-subdrv.h"
#include "adaptor-tsrec-cb-ctrl.h"
#include "adaptor-sentest.h"
#include "imgsensor-user.h"
#include "mtk-i3c-i2c-wrap.h"

/* frame-sync */
#include "frame_sync.h"

#define to_ctx(__sd) container_of(__sd, struct adaptor_ctx, sd)

#define adaptor_logd(_ctx, format, args...) do { \
	if ((_ctx) && unlikely(*((_ctx)->sensor_debug_flag))) { \
		dev_info((_ctx)->dev, "[%s][%s][%s] " format, \
			(_ctx)->sd.name, \
			((_ctx)->subdrv) ? ((_ctx)->subdrv->name) : "null", __func__, ##args); \
	} \
} while (0)

#define adaptor_loge(_ctx, format, args...) do { \
	if (_ctx) { \
		dev_info((_ctx)->dev, "[%s][%s][%s] ERROR: " format, \
			(_ctx)->sd.name, \
			((_ctx)->subdrv) ? ((_ctx)->subdrv->name) : "null", __func__, ##args); \
	} \
} while (0)

#define adaptor_logm(_ctx, format, args...) do { \
	if ((_ctx) && unlikely(*((_ctx)->sensor_debug_flag)==2)) { \
		dev_info((_ctx)->dev, "[%s][%s][%s] " format, \
			(_ctx)->sd.name, \
			((_ctx)->subdrv) ? ((_ctx)->subdrv->name) : "null", __func__, ##args); \
	} \
} while (0)

#define adaptor_logi(_ctx, format, args...) do { \
	if (_ctx) { \
		dev_info((_ctx)->dev, "[%s][%s][%s] " format, \
			(_ctx)->sd.name, \
			((_ctx)->subdrv) ? ((_ctx)->subdrv->name) : "null", __func__, ##args); \
	} \
} while (0)

struct adaptor_ctx;
static unsigned int sensor_debug;
static unsigned int set_ctrl_unlock;
#define VC_MULTI_CAMERA 1
#define ALWAYS_ON_POWER 0
#ifdef IMGSENSOR_FUSION_TEST_WORKAROUND
extern unsigned int gSensor_num;
extern unsigned int is_multicam;
extern unsigned int is_imgsensor_fusion_test_workaround;
#endif

struct adaptor_ae_ctrl_dbg_info {
	/* timestamp info when get ae ctrl */
	u64 sys_ts_g_ae_ctrl;
	u64 sys_ts_update_sof_cnt_at_g_ae_ctrl;
};

enum IMGSENSOR_STATE {
	IMGSENSOR_STATE_POWER_OFF,
	IMGSENSOR_STATE_POWER_ON,
	IMGSENSOR_STATE_ERROR
};

union feature_para {
	u64 u64[4];
	u32 u32[8];
	u16 u16[16];
	u8 u8[32];
};

struct sensor_mode {
	u32 id;
	u32 llp;
	u32 fll;
	u32 width;
	u32 height;
	u32 mipi_pixel_rate;
	u32 cust_pixel_rate;
	u32 max_framerate;
	u64 pclk;
	u64 linetime_in_ns;
	u64 linetime_in_ns_readout;
	int fine_intg_line;
	struct mtk_csi_param csi_param;
	u8 esd_reset_by_user;
	u32 active_line_num;
	enum mtk_sensor_usage usage;
};

struct adaptor_hw_ops {
	int (*set)(struct adaptor_ctx *ctx, void *data, int val);
	int (*unset)(struct adaptor_ctx *ctx, void *data, int val);
	void *data;
};

struct adaptor_work {
	struct kthread_work work;
	struct kthread_delayed_work dwork;
	struct adaptor_ctx *ctx;
	struct mtk_hdr_ae ae_ctrl;
};


struct adaptor_ctx {
	struct mutex mutex;
	struct i2c_client *i2c_client;
	struct i3c_i2c_device ixc_client;
	struct device *dev;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_fwnode_endpoint ep;
	struct kthread_worker adaptor_worker;
	struct task_struct *adaptor_kworker_task;

	/* V4L2 Controls */
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *exposure_abs;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *pd_pixel_region;
	struct v4l2_ctrl *max_fps;
	struct v4l2_ctrl *test_pattern;

	/* custom v4l2 ctrls */
	struct v4l2_ctrl *anti_flicker;
	struct v4l2_ctrl *analogue_gain;
	struct v4l2_ctrl *awb_gain;
	struct v4l2_ctrl *shutter_gain_sync;
	struct v4l2_ctrl *ihdr_shutter_gain;
	struct v4l2_ctrl *dual_gain;
	struct v4l2_ctrl *hdr_shutter;
	struct v4l2_ctrl *shutter_frame_length;
	struct v4l2_ctrl *pdfocus_area;
	struct v4l2_ctrl *hdr_atr;
	struct v4l2_ctrl *hdr_tri_shutter;
	struct v4l2_ctrl *hdr_tri_gain;
	struct v4l2_ctrl *hdr_ae_ctrl;

	/* custom v4l2 ctrls - frame sync - */
	struct v4l2_ctrl *frame_sync;
	struct v4l2_ctrl *fsync_async_master;
	struct v4l2_ctrl *fsync_map_id;
	struct v4l2_ctrl *fsync_listen_target;

	/* hw handles */
	struct clk *clk[CLK_MAXCNT];
	struct regulator *regulator[REGULATOR_MAXCNT];
	struct pinctrl *pinctrl;
	struct pinctrl_state *state[STATE_MAXCNT];
	struct adaptor_hw_ops hw_ops[HW_ID_MAXCNT];
	unsigned long long pmic_on_tick;
	unsigned long long first_sensor_power_on_tick;
	unsigned long long pmic_delayus;

	/* seninf info */
	int seninf_idx;

	/* sensor */
	struct subdrv_entry *subdrv;
	struct subdrv_ctx subctx;
	struct sensor_mode mode[MODE_MAXCNT];
	struct sensor_mode *cur_mode;
	struct sensor_mode *try_format_mode;
	enum IMGSENSOR_STATE sensor_state;
	int mode_cnt;
	MSDK_SENSOR_INFO_STRUCT sensor_info;
	MSDK_SENSOR_CONFIG_STRUCT sensor_cfg;
	int fmt_code[SENSOR_SCENARIO_ID_MAX];
	int idx; /* requireed by frame-sync modules */
	int dts_idx; /* dts idx (before reindex) */
	int forbid_idx; /* idx forbidden to open in sametime */
	struct mtk_hdr_ae ae_memento;

	u32 seamless_scenarios[SENSOR_SCENARIO_ID_MAX];

	/* sensor property */
	u32 location;
	u32 rotation;

	/* sentest */
	struct mtk_cam_sentest_cfg_info sentest_cfg_info;

	/* frame-sync */
	struct FrameSync *fsync_mgr;
	unsigned int fsync_out_fl;
	unsigned int fsync_out_fl_arr[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	int needs_fsync_assign_fl;

	/* tsrec */
	struct adaptor_tsrec_cb_ctrl tsrec_cb_ctrl;
	struct mtk_cam_seninf_tsrec_vsync_info vsync_info;
	struct mtk_cam_seninf_tsrec_timestamp_info ts_info;
	/* tsrec & fsync mgr */
	spinlock_t fsync_pre_latch_ts_info_update_lock;
	struct mtk_cam_seninf_tsrec_timestamp_info fsync_pre_latch_ts_info;

	/* flags */
	unsigned int is_streaming:1;
	unsigned int is_sensor_inited:1;
	unsigned int is_sensor_scenario_inited:1;
	unsigned int is_sensor_reset_stream_off:1;
	unsigned int is_i2c_bus_scp:1;

	int open_refcnt;
	int power_refcnt;
	int mclk_refcnt;

	struct subdrv_pw_seq_entry *ctx_pw_seq;

	/* debug var */
	struct adaptor_ae_ctrl_dbg_info ae_ctrl_dbg_info;
	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;

	unsigned long long sys_ts_update_sof_cnt;
	unsigned int *sensor_debug_flag;
	unsigned int *p_set_ctrl_unlock_flag;
	unsigned int sof_cnt;
	int req_id; /* from mtk hdr ae ctrl */
	u64 shutter_for_timeout;
	u64 framelength_for_timeout;
	u64 last_framelength;
	struct wakeup_source *sensor_ws;
	unsigned int aov_pm_ops_flag;	/* flag for aov pm ops */
	unsigned int aov_mclk_ulposc_flag;	/* flag for aov switch mclk to ulposc */
	u32 cust_aov_csi_clk;
	const char *phy_ctrl_ver;

	/* embedded data dump */
	struct mutex ebd_lock;
	struct mtk_ebd_dump_record latest_ebd;
#if ALWAYS_ON_POWER
	unsigned int always_on_flag;
	struct notifier_block notifier_blk;
#endif
};

#endif
