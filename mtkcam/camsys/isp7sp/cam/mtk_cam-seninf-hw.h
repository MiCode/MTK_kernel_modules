/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2019 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_HW_H__
#define __MTK_CAM_SENINF_HW_H__

//#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
#include <aee.h>

#ifndef CONFIG_MTK_AEE_FEATURE
#define seninf_aee_print(string, args...) \
	pr_info("[SENINF] error:"string, ##args)
#else
#define seninf_aee_print(string, args...) do { \
		aee_kernel_exception_api(__FILE__, __LINE__, \
			DB_OPT_DEFAULT | DB_OPT_FTRACE, \
			DB_OPT_PROCESS_COREDUMP | DB_OPT_PROCMEM, \
			DB_OPT_NE_JBT_TRACES | DB_OPT_PID_SMAPS, \
			DB_OPT_DUMPSYS_PROCSTATS | DB_OPT_DUMP_DISPLAY, \
			"Seninf", "[SENINF] error:"string, ##args); \
		pr_info("[SENINF] error:"string, ##args);  \
	} while (0)
#endif

#define MAX_MUX_VCINFO_DEBUG 15
#define MAX_TS_SIZE 4

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

struct mtk_cam_seninf_mux_meter {
	u32 width;
	u32 height;
	u32 h_valid;
	u32 h_blank;
	u32 v_valid;
	u32 v_blank;
	s64 mipi_pixel_rate;
	s64 vb_in_us;
	s64 hb_in_us;
	s64 line_time_in_us;
};

struct mtk_cam_seninf_vcinfo_debug {
	u32 seninf_mux;
	u32 seninf_mux_en;
	u32 seninf_mux_src;
	u32 seninf_mux_irq;
	u32 cam_mux;
	u32 cam_mux_en;
	u32 cam_mux_src;
	u32 cam_mux_irq;
	u32 exp_size;
	u32 rec_size;
	u32 frame_mointor_err;
	u32 vc_feature;
	u32 vc;
	u32 dt;
};
struct mtk_cam_seninf_debug {
	__u8 mux_result_cnt;
	__u32 csi_mac_irq_status;
	__u32 csi_irq_status;
	__u32 packet_cnt_status;
	struct mtk_cam_seninf_mux_meter meter[MAX_MUX_VCINFO_DEBUG];
	struct mtk_cam_seninf_vcinfo_debug vcinfo_debug[MAX_MUX_VCINFO_DEBUG];
};

struct mtk_tsrec_timestamp_by_sensor_id {
	__u8 sensor_idx;
	__u64 ts_us[MAX_TS_SIZE];
};

extern int update_isp_clk(struct seninf_ctx *ctx);

struct mtk_cam_seninf_ops {
	int (*_init_iomem)(struct seninf_ctx *ctx,
			      void __iomem *if_base, void __iomem *ana_base);
	int (*_init_port)(struct seninf_ctx *ctx, int port);
	int (*_is_cammux_used)(struct seninf_ctx *ctx, int cam_mux);
	int (*_cammux)(struct seninf_ctx *ctx, int cam_mux);
	int (*_disable_cammux)(struct seninf_ctx *ctx, int cam_mux);
	int (*_disable_all_cammux)(struct seninf_ctx *ctx);
	int (*_set_top_mux_ctrl)(struct seninf_ctx *ctx,
						int mux_idx, int seninf_src);
	int (*_get_top_mux_ctrl)(struct seninf_ctx *ctx, int mux_idx);
	int (*_get_cammux_ctrl)(struct seninf_ctx *ctx, int cam_mux);
	u32 (*_get_cammux_res)(struct seninf_ctx *ctx, int cam_mux);
	int (*_set_cammux_vc)(struct seninf_ctx *ctx, int cam_mux,
					 int vc_sel, int dt_sel, int vc_en, int dt_en);
	int (*_set_cammux_tag)(struct seninf_ctx *ctx, int cam_mux,
					int vc_sel, int dt_sel, int tag, int first);
	int (*_set_cammux_src)(struct seninf_ctx *ctx, int src,
					  int target, int exp_hsize, int exp_vsize, int dt);
	int (*_set_vc)(struct seninf_ctx *ctx, int seninfIdx,
				  struct seninf_vcinfo *vcinfo, struct seninf_glp_dt *glpinfo);
	int (*_set_mux_ctrl)(struct seninf_ctx *ctx, int mux,
					int hsPol, int vsPol, int src_sel,
					int pixel_mode);
	int (*_set_mux_vc_split_all)(struct seninf_ctx *ctx, int mux, u8 cam_type);
	int (*_set_mux_crop)(struct seninf_ctx *ctx, int mux,
					int start_x, int end_x, int enable);
	int (*_is_mux_used)(struct seninf_ctx *ctx, int mux);
	int (*_mux)(struct seninf_ctx *ctx, int mux);
	int (*_disable_mux)(struct seninf_ctx *ctx, int mux);
	int (*_disable_all_mux)(struct seninf_ctx *ctx);
	int (*_set_cammux_chk_pixel_mode)(struct seninf_ctx *ctx,
							 int cam_mux, int pixelMode);
	int (*_set_test_model)(struct seninf_ctx *ctx,
			int mux, int cam_mux, int pixelMode, int filter, int con, int vc, int dt,
			int muxvr_ofs);
	int (*_set_csi_mipi)(struct seninf_ctx *ctx);
	int (*_poweroff)(struct seninf_ctx *ctx);
	int (*_reset)(struct seninf_ctx *ctx, int seninfIdx);
	int (*_set_idle)(struct seninf_ctx *ctx);
	int (*_get_mux_meter)(struct seninf_ctx *ctx, int mux,
					 struct mtk_cam_seninf_mux_meter *meter);
	ssize_t (*_show_status)(struct device *dev, struct device_attribute *attr, char *buf);
	int (*_switch_to_cammux_inner_page)(struct seninf_ctx *ctx, bool inner);
	int (*_set_cammux_next_ctrl)(struct seninf_ctx *ctx, int src, int target);
	int (*_update_mux_pixel_mode)(struct seninf_ctx *ctx, int mux, int pixel_mode);
	int (*_irq_handler)(int irq, void *data);
	int (*_thread_irq_handler)(int irq, void *data);
	void (*_init_irq_fifo)(struct seninf_core *core);
	void (*_uninit_irq_fifo)(struct seninf_core *core);
	int (*_set_sw_cfg_busy)(struct seninf_ctx *ctx, bool enable, int index);
	int (*_set_cam_mux_dyn_en)(struct seninf_ctx *ctx, bool enable, int cam_mux, int index);
	int (*_reset_cam_mux_dyn_en)(struct seninf_ctx *ctx, int index);
	int (*_enable_global_drop_irq)(struct seninf_ctx *ctx, bool enable, int index);
	int (*_enable_cam_mux_vsync_irq)(struct seninf_ctx *ctx, bool enable, int cam_mux);
	int (*_set_all_cam_mux_vsync_irq)(struct seninf_ctx *ctx, bool enable);
	int (*_debug)(struct seninf_ctx *ctx);
	int (*_get_debug_reg_result)(struct seninf_ctx *ctx, void *arg);
	int (*_get_tsrec_timestamp)(struct seninf_ctx *ctx, void *arg);
	int (*_eye_scan)(struct seninf_ctx *ctx, u32 key, int val, char *plog, int logbuf_size);
	int (*_set_reg)(struct seninf_ctx *ctx, u32 key, u64 val);
	int (*_set_phya_clock_src)(struct seninf_ctx *ctx, u64 val);
	ssize_t (*_show_err_status)(struct device *dev, struct device_attribute *attr, char *buf);
	int (*_enable_stream_err_detect)(struct seninf_ctx *ctx);
	int (*_debug_init_deskew_irq)(struct seninf_ctx *ctx);
	int (*_debug_init_deskew_begin_end_apply_code)(struct seninf_ctx *ctx);
	int (*_debug_current_status)(struct seninf_ctx *ctx);
	int (*_set_csi_afifo_pop)(struct seninf_ctx *ctx);
	int (*_get_csi_irq_status)(struct seninf_ctx *ctx);
	unsigned int seninf_num;
	unsigned int mux_num;
	unsigned int cam_mux_num;
	unsigned int pref_mux_num;
	const char *iomem_ver;

};

extern struct mtk_cam_seninf_ops mtk_csi_phy_3_0;
extern struct mtk_cam_seninf_ops mtk_csi_phy_3_1;
extern struct mtk_cam_seninf_ops *g_seninf_ops;

#endif
