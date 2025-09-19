/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2019 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_HW_H__
#define __MTK_CAM_SENINF_HW_H__

//#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
#include <aee.h>

#include "mtk_cam-seninf_control-8.h"

#define SENINF_AEE_GENERAL "Seninf"
#define SENINF_AEE_OUTMUX "Seninf: outmux error"
#define SENINF_AEE_FS_SEQ "Seninf: sensor fs sequence"
#define SENINF_AEE_FRMERR "Seninf: frame error"

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define seninf_aee_print(title, string, args...) do { \
		aee_kernel_exception_api(__FILE__, __LINE__, \
			DB_OPT_DEFAULT | DB_OPT_FTRACE, \
			title, "["title"]: "string, ##args); \
		pr_info("["title"]: "string, ##args);  \
	} while (0)
#else
#define seninf_aee_print(title, string, args...) \
	pr_info("["title"]: "string, ##args)
#endif

#define MAX_MUX_VCINFO_DEBUG 15
#define MAX_TS_SIZE 4

/* isp8 used. suspose isp8s no need, use grp rdy instead */
 #define SEAMLESS_OUTMUX_V3

/*ULPS-mode support*/
#undef CDPHY_ULPS_MODE_SUPPORT
//#define CDPHY_ULPS_MODE_SUPPORT

/*init deskew define*/
#define INIT_DESKEW_SUPPORT
//#define INIT_DESKEW_UT
//#define INIT_DESKEW_DEBUG

enum CSIRX_LANE_NUM {
	CSIRX_LANE_A0 = 0,
	CSIRX_LANE_A1,
	CSIRX_LANE_A2,
	CSIRX_LANE_B0,
	CSIRX_LANE_B1,
	CSIRX_LANE_B2,
	CSIRX_LANE_MAX_NUM
};


enum SET_REG_KEYS {
	REG_KEY_MIN = 0,
	REG_KEY_SETTLE_CK = REG_KEY_MIN,
	REG_KEY_SETTLE_DT,
	REG_KEY_HS_TRAIL_EN,
	REG_KEY_HS_TRAIL_PARAM,
	REG_KEY_CSI_IRQ_STAT,
	REG_KEY_CSI_RESYNC_CYCLE,
	REG_KEY_MUX_IRQ_STAT,
	REG_KEY_CAMMUX_IRQ_STAT,
	REG_KEY_CAMMUX_VSYNC_IRQ_EN,
	REG_KEY_CAMMUX_VSYNC_IRQ_EN_H,
	REG_KEY_CSI_IRQ_EN,
	REG_KEY_AOV_CSI_CLK_SWITCH,
	REG_KEY_MIPI_ERROR_DETECT_EN,
	REG_KEY_MAX_NUM
};

#define SET_REG_KEYS_NAMES \
	"RG_SETTLE_CK", \
	"RG_SETTLE_DT", \
	"RG_HS_TRAIL_EN", \
	"RG_HS_TRAIL_PARAM", \
	"RG_CSI_IRQ_STAT", \
	"RG_CSI_RESYNC_CYCLE", \
	"RG_MUX_IRQ_STAT", \
	"RG_CAMMUX_IRQ_STAT", \
	"RG_VSYNC_IRQ_EN", \
	"RG_VSYNC_IRQ_EN_H", \
	"RG_CSI_IRQ_EN", \
	"RG_AOV_CSI_CLK_SWITCH", \
	"RG_MIPI_ERROR_DETECT_EN", \

enum EYE_SCAN_KEYS {
	EYE_SCAN_KEYS_EQ_DG0_EN,
	EYE_SCAN_KEYS_EQ_SR0,
	EYE_SCAN_KEYS_EQ_DG1_EN,
	EYE_SCAN_KEYS_EQ_SR1,
	EYE_SCAN_KEYS_EQ_BW,
	EYE_SCAN_KEYS_CDR_DELAY,
	EYE_SCAN_KEYS_GET_CRC_STATUS,
	EYE_SCAN_KEYS_CDR_DELAY_DPHY_EN,
	EYE_SCAN_KEYS_FLUSH_CRC_STATUS,
	EYE_SCAN_KEYS_EQ_OFFSET,
	EYE_SCAN_KEYS_GET_EQ_DG0_EN,
	EYE_SCAN_KEYS_GET_EQ_SR0,
	EYE_SCAN_KEYS_GET_EQ_DG1_EN,
	EYE_SCAN_KEYS_GET_EQ_SR1,
	EYE_SCAN_KEYS_GET_EQ_BW,
	EYE_SCAN_KEYS_GET_CDR_DELAY,
	EYE_SCAN_KEYS_GET_EQ_OFFSET,
	EYE_SCAN_MAX_NUM
};

#define EYE_SCAN_KEYS_NAMES \
	"EQ_DG0_EN", \
	"EQ_SR0", \
	"EQ_DG1_EN", \
	"EQ_SR1", \
	"EQ_BW", \
	"CDR_DELAY", \
	"GET_CRC_STATUS", \
	"CDR_DELAY_DPHY_EN", \
	"FLUSH_CRC_STATUS",\
	"EQ_OFFSET",\
	"GET_EQ_DG0_EN", \
	"GET_EQ_SR0", \
	"GET_EQ_DG1_EN", \
	"GET_EQ_SR1", \
	"GET_EQ_BW", \
	"GET_CDR_DELAY", \
	"GET_EQ_OFFSET",\

struct mtk_cam_seninf_vcinfo_debug {
	u32 vc_feature;
	u32 tag_id;
	u32 vc;
	u32 dt;
	u32 exp_size_h;
	u32 exp_size_v;
	u32 rec_size_h;
	u32 rec_size_v;
	u32 outmux_id;
	u32 done_irq_status;
	u32 oversize_irq_status; /* not used */
	u32 incomplete_frame_status; /* not used */
	u32 ref_vsync_irq_status;
};

struct mtk_cam_seninf_mux_meter{};
struct mtk_cam_seninf_debug {
	u8 valid_result_cnt;
	u32 seninfAsyncIdx;
	u32 seninf_async_irq;
	u32 seninfAsync;
	u32 csi_mac_irq_status;
	u32 packet_cnt_status;
	struct mtk_cam_seninf_vcinfo_debug vcinfo_debug[MAX_MUX_VCINFO_DEBUG];
};

struct mtk_tsrec_timestamp_by_sensor_id {
	__u8 sensor_idx;
	__u64 ts_us[MAX_TS_SIZE];
};

enum MTK_CAM_OUTMUX_CFG_MODE {
	MTK_CAM_OUTMUX_CFG_MODE_NORMAL_CFG = 0,
	MTK_CAM_OUTMUX_CFG_MODE_EXP_NC = 1,
	MTK_CAM_OUTMUX_CFG_MODE_SAT = 2,
};

extern int update_isp_clk(struct seninf_ctx *ctx);

#define MAX_SENINF_DEV_GRP_CNT 4
struct mtk_cam_seninf_dev {
	unsigned int count;
	unsigned int val[MAX_SENINF_DEV_GRP_CNT];
};

struct mtk_cam_seninf_async_split {
	unsigned int async_idx;
	unsigned int is_split;
};

struct mtk_cam_seninf_ops {
	int (*_init_iomem)(struct seninf_ctx *ctx,
			      void __iomem *if_top_base, void __iomem *if_async_base,
			      void __iomem *if_tm_base, void __iomem *if_outmux[],
			      void __iomem *if_outmux_inner[],
			      void __iomem *csi_top_0, void __iomem *csi_top_1,
				  struct csi_reg_base *csi_base);
	int (*_init_port)(struct seninf_ctx *ctx, int port, struct csi_reg_base *csi_base);
	int (*_disable_outmux)(struct seninf_ctx *ctx, int outmux, bool immed);
	int (*_get_outmux_irq_st)(struct seninf_ctx *ctx, int outmux, bool clear);
	int (*_get_outmux_sel)(struct seninf_ctx *ctx, int outmux, int *asyncIdx, int *sensorSel);
	u32 (*_get_outmux_res)(struct seninf_ctx *ctx, int outmux, int tag);
	int (*_set_vc)(struct seninf_ctx *ctx, int seninfIdx,
				  struct seninf_vcinfo *vcinfo, struct seninf_glp_dt *glpinfo);
	int (*_set_outmux_cg)(struct seninf_ctx *ctx, int outmux, int en);
	int (*_is_outmux_used)(struct seninf_ctx *ctx, int outmux);
	int (*_disable_all_outmux)(struct seninf_ctx *ctx);
	int (*_wait_outmux_cfg_done)(struct seninf_ctx *ctx, u8 outmux_idx);
	int (*_config_outmux)(struct seninf_ctx *ctx, u8 outmux_idx, u8 src_mipi, u8 src_sen,
			u8 cfg_mode, struct outmux_tag_cfg *tag_cfg);
	int (*_apply_outmux_for_v2)(struct seninf_ctx *ctx, u8 outmux_idx,
			u8 cfg_mode, struct outmux_tag_cfg *tag_cfg, bool is_sensor_delay);
	int (*_set_outmux_ref_vsync_inner)(struct seninf_ctx *ctx, u8 outmux_idx);
	bool (*_chk_sensor_delay_with_wait)(struct seninf_ctx *ctx, u8 outmux_idx, bool *sensor_delay);
	int (*_set_outmux_ref_vsync)(struct seninf_ctx *ctx, u8 outmux_idx);
	int (*_set_outmux_cfg_done)(struct seninf_ctx *ctx, u8 outmux_idx);
	int (*_set_outmux_pixel_mode)(struct seninf_ctx *ctx,
							 int outmux, int pixelMode);
	int (*_set_outmux_grp_en)(struct seninf_ctx *ctx, u8 outmux_idx, bool grp_en);
	int (*_set_outmux_cfg_rdy)(struct seninf_ctx *ctx, u8 outmux_idx, bool cfg_rdy);
	int (*_set_test_model)(struct seninf_ctx *ctx, int intf);
	int (*_set_test_model_fake_sensor)(struct seninf_ctx *ctx, int intf);
	int (*_get_async_irq_st)(struct seninf_ctx *ctx, int async, bool clear);
	int (*_set_csi_mipi)(struct seninf_ctx *ctx);
	int (*_poweroff)(struct seninf_ctx *ctx);
	int (*_reset)(struct seninf_ctx *ctx, int seninfIdx);
	int (*_set_idle)(struct seninf_ctx *ctx);
	int (*_get_mux_meter)(struct seninf_ctx *ctx, int mux,
					 struct mtk_cam_seninf_mux_meter *meter);
	ssize_t (*_show_status)(struct device *dev, struct device_attribute *attr, char *buf);
	ssize_t (*_show_outmux_status)(struct device *dev, struct device_attribute *attr, char *buf,
				   unsigned int *outmuxs, unsigned int cnt);
	int (*_irq_handler)(int irq, void *data);
	int (*_thread_irq_handler)(int irq, void *data);
	void (*_init_irq_fifo)(struct seninf_core *core);
	void (*_uninit_irq_fifo)(struct seninf_core *core);
	int (*_enable_cam_mux_vsync_irq)(struct seninf_ctx *ctx, bool enable, int cam_mux);
	int (*_set_all_cam_mux_vsync_irq)(struct seninf_ctx *ctx, bool enable);
	int (*_debug)(struct seninf_ctx *ctx);
	int (*get_seninf_debug_core_dump)(struct seninf_ctx *ctx,
										  struct mtk_cam_seninf_debug *arg);
	int (*_get_tsrec_timestamp)(struct seninf_ctx *ctx, void *arg);
	int (*_eye_scan)(struct seninf_ctx *ctx, u32 key, int val, char *plog, int logbuf_size);
	int (*_set_reg)(struct seninf_ctx *ctx, u32 key, u64 val);
	int (*_set_phya_clock_src)(struct seninf_ctx *ctx);
	ssize_t (*_show_err_status)(struct device *dev, struct device_attribute *attr, char *buf);
	int (*_enable_stream_err_detect)(struct seninf_ctx *ctx);
	int (*_debug_init_deskew_irq)(struct seninf_ctx *ctx);
	int (*_debug_init_deskew_begin_end_apply_code)(struct seninf_ctx *ctx);
	int (*_debug_current_status)(struct seninf_ctx *ctx);
	int (*_set_csi_afifo_pop)(struct seninf_ctx *ctx);
	int (*_get_csi_irq_status)(struct seninf_ctx *ctx);
	int (*_common_reg_setup)(struct seninf_ctx *ctx);
	int (*_get_device_sel_setting)(struct device *dev, struct mtk_cam_seninf_dev *dev_setting);
	void (*_seninf_dump_mipi_err)(struct seninf_core *core, struct mtk_cam_seninf_vsync_info *vsync_info);
	int (*_show_mac_chk_status)(struct seninf_ctx *ctx, int is_clear);
	int (*_get_csi_HV_HB_meter)(struct seninf_ctx *ctx, struct mtk_cam_seninf_meter_info *info,
										const int valid_measure_req);
	unsigned int async_num;
	unsigned int outmux_num;
	const char *iomem_ver;

};

extern struct mtk_cam_seninf_ops mtk_csi_phy_3_0;
extern struct mtk_cam_seninf_ops *g_seninf_ops;

#endif
