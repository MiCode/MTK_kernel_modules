/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 MediaTek Inc. */

#ifndef __ADAPTOR_SUBDRV_H__
#define __ADAPTOR_SUBDRV_H__

#include <media/v4l2-subdev.h>

//#include "kd_imgsensor_define_v4l2.h"
#include "imgsensor-user.h"
#include "adaptor-def.h"
#include "mtk-i3c-i2c-wrap.h"

#define IMGSENSOR_FUSION_TEST_WORKAROUND

#define MT6985_PHY_CTRL_VERSIONS "mt6985"
#define MT6897_PHY_CTRL_VERSIONS "mt6897"
#define MT6989_PHY_CTRL_VERSIONS "mt6989"
#define MT6878_PHY_CTRL_VERSIONS "mt6878"
#define MT6991_PHY_CTRL_VERSIONS "mt6991"

#define DEBUG_LOG(ctx, ...) do { \
	if (ctx->i2c_client) \
		imgsensor_info.sd = i2c_get_clientdata(ctx->i2c_client); \
	if (ctx->ixc_client.protocol) \
		imgsensor_info.sd = adaptor_ixc_get_clientdata(&ctx->ixc_client); \
	if (imgsensor_info.sd) \
		imgsensor_info.adaptor_ctx_ = to_ctx(imgsensor_info.sd); \
	if ((imgsensor_info.adaptor_ctx_) \
		&& unlikely(*((imgsensor_info.adaptor_ctx_)->sensor_debug_flag))) { \
		LOG_INF(__VA_ARGS__); \
	} \
} while (0)

/* def V4L2_MBUS_CSI2_IS_USER_DEFINED_DATA */
#define IMGSENSOR_VC_ROUTING

#define PARAM_DEFAULT 0
#define PARAM_UNDEFINED 0
#define GLP_DT_MAX_CNT 4
enum {
	I2C_DT_ADDR_16_DATA_8 = 0,
	I2C_DT_ADDR_16_DATA_16,
	I2C_DT_MAXCNT,
};

enum {
	HW_ID_AVDD = 0,
	HW_ID_DVDD,
	HW_ID_DOVDD,
	HW_ID_AFVDD,
	HW_ID_AFVDD1,
	HW_ID_AVDD1,
	HW_ID_AVDD2,
	HW_ID_AVDD3,
	HW_ID_AVDD4,
	HW_ID_BASE,
	HW_ID_PDN,
	HW_ID_RST,
	HW_ID_MCLK,
	HW_ID_MCLK_DRIVING_CURRENT,
	HW_ID_MIPI_SWITCH,
	HW_ID_DVDD1,
	HW_ID_DVDD2,
	HW_ID_OISVDD,
	HW_ID_OISEN,
	HW_ID_RST1,
	HW_ID_MCLK1,
	HW_ID_MCLK1_DRIVING_CURRENT,
	HW_ID_PONV,
	HW_ID_SCL,
	HW_ID_SDA,
	HW_ID_EINT,
	HW_ID_MAXCNT,
};

#define HW_ID_NAMES \
	"HW_ID_AVDD", \
	"HW_ID_DVDD", \
	"HW_ID_DOVDD", \
	"HW_ID_AFVDD", \
	"HW_ID_AFVDD1", \
	"HW_ID_AVDD1", \
	"HW_ID_AVDD2", \
	"HW_ID_AVDD3", \
	"HW_ID_AVDD4", \
	"HW_ID_BASE", \
	"HW_ID_PDN", \
	"HW_ID_RST", \
	"HW_ID_MCLK", \
	"HW_ID_MCLK_DRIVING_CURRENT", \
	"HW_ID_MIPI_SWITCH", \
	"HW_ID_DVDD1", \
	"HW_ID_DVDD2", \
	"HW_ID_OISVDD", \
	"HW_ID_OISEN", \
	"HW_ID_RST1", \
	"HW_ID_MCLK1", \
	"HW_ID_MCLK1_DRIVING_CURRENT", \
	"HW_ID_PONV", \
	"HW_ID_SCL", \
	"HW_ID_SDA", \
	"HW_ID_EINT", \

enum AOV_MODE_CTRL_OPS {
	AOV_MODE_CTRL_OPS_SENSING_CTRL = 0,
	AOV_MODE_CTRL_OPS_MONTION_DETECTION_CTRL,
	AOV_MODE_CTRL_OPS_SENSING_UT_ON_SCP,
	AOV_MODE_CTRL_OPS_SENSING_UT_ON_APMCU,
	AOV_MODE_CTRL_OPS_DPHY_GLOBAL_TIMING_CONTINUOUS_CLK,
	AOV_MODE_CTRL_OPS_DPHY_GLOBAL_TIMING_NON_CONTINUOUS_CLK,
	AOV_MODE_CTRL_OPS_MAX_NUM,
};

enum GENERI_LONG_PACKET_DT {
	GLP_DT_0X10 = 0x10,
	GLP_DT_0X11,
	GLP_DT_0X12,
	GLP_EBD_DT = GLP_DT_0X12,
	GLP_DT_0X13,
	GLP_DT_0X14,
	GLP_DT_0X15,
	GLP_DT_0X16,
	GLP_DT_0X17,
};

struct subdrv_pw_seq_entry {
	int id;
	int val;
	int delay;
};

struct u32_min_max {
	u32 min;
	u32 max;
};

struct u64_min_max {
	u64 min;
	u64 max;
};

struct eeprom_info_struct {
	u32 header_id;
	u32 addr_header_id;
	u8 i2c_write_id;

	u8 qsc_support;
	u16 qsc_size;
	u16 addr_qsc;
	u16 sensor_reg_addr_qsc;
	u8 *qsc_table;

	u8 pdc_support;
	u16 pdc_size;
	u16 addr_pdc;
	u16 sensor_reg_addr_pdc;
	u8 *pdc_table;

	u8 lrc_support;
	u16 lrc_size;
	u16 addr_lrc;
	u16 sensor_reg_addr_lrc;
	u8 *lrc_table;

	u8 xtalk_support; /* [1]sw-remo; [0]hw-remo; 0, not support */
	u16 xtalk_size;
	u16 addr_xtalk;
	u16 sensor_reg_addr_xtalk;
	u8 *xtalk_table;

	/* preloaded data */
	u8 *preload_qsc_table;
	u8 *preload_pdc_table;
	u8 *preload_lrc_table;
	u8 *preload_xtalk_table;
};

struct dcg_info_struct {
	enum IMGSENSOR_DCG_MODE dcg_mode;
	enum IMGSENSOR_DCG_GAIN_MODE dcg_gain_mode;
	enum IMGSENSOR_DCG_GAIN_BASE dcg_gain_base;
	u32 dcg_gain_ratio_min;
	u32 dcg_gain_ratio_max;
	u32 dcg_gain_ratio_step;
	u32 *dcg_gain_table;
	u32 dcg_gain_table_size;
};


#define MAX_EBD_PIXEL_OFFSET_NUM 2
struct ebd_loc {
	u16 loc_line;
	u16 loc_pix[MAX_EBD_PIXEL_OFFSET_NUM];
};

struct ebd_info_struct {
	struct ebd_loc frm_cnt_loc;
	struct ebd_loc coarse_integ_loc[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct ebd_loc ana_gain_loc[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct ebd_loc dig_gain_loc[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct ebd_loc coarse_integ_shift_loc;
	struct ebd_loc dol_loc;
	struct ebd_loc framelength_loc;
	struct ebd_loc temperature_loc;
};

struct subdrv_mode_struct {
	u16 *mode_setting_table;
	u32 mode_setting_len;
	u16 *mode_setting_table_for_md;
	u32 mode_setting_len_for_md;
	u32 seamless_switch_group;
	u16 *seamless_switch_mode_setting_table;
	u32 seamless_switch_mode_setting_len;
	enum IMGSENSOR_HDR_MODE_ENUM hdr_mode;
	u32 raw_cnt;
	u32 exp_cnt;

	u64 pclk;
	u32 linelength;
	u32 framelength;
	u16 max_framerate;
	u32 mipi_pixel_rate;
	u32 readout_length;
	u8 read_margin;
	u32 framelength_step;
	u32 coarse_integ_step;
	u32 min_exposure_line;
	struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info;

	enum IMGSENSOR_RGBW_OUTPUT_MODE rgbw_output_mode;

	/* aov param by mode */
	u8 aov_mode;
	u8 rosc_mode;
	u8 s_dummy_support;
	enum IMGSENSOR_AE_CONTROL_SUPPORT ae_ctrl_support;

	u8 pdaf_cap;
	struct SET_PD_BLOCK_INFO_T *imgsensor_pd_info;
	u32 ae_binning_ratio;
	int fine_integ_line;
	u8 delay_frame;
	struct mtk_csi_param csi_param;
	struct mtk_mbus_frame_desc_entry *frame_desc;
	u32 num_entries;

	/* assign value only if fixed value by mode */
	u8 sensor_output_dataformat;
	enum ACDK_SENSOR_OUTPUT_FORMAT_CELL_TYPE sensor_output_dataformat_cell_type;
	u32 ana_gain_min;
	u32 ana_gain_max;
	u32 dig_gain_min;
	u32 dig_gain_max;
	u32 dig_gain_step;
	struct u32_min_max multi_exposure_ana_gain_range[IMGSENSOR_EXPOSURE_CNT];
	struct u64_min_max multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_CNT];

	bool dpc_enabled; /* defect pixel correction */
	bool pdc_enabled; /* pd correction */
	struct mtk_sensor_saturation_info *saturation_info;
	struct dcg_info_struct dcg_info;
	u32 exposure_order_in_lbmf;
	u32 mode_type_in_lbmf;
	u32 sw_fl_delay;
};

#define REG_ADDR_MAXCNT 3
struct reg_ {
	u16 addr[REG_ADDR_MAXCNT];
};

struct subdrv_static_ctx {
	u32 sensor_id;
	struct reg_ reg_addr_sensor_id;
	u8 i2c_addr_table[5]; /* must end with 0xFF */
	u32 i2c_burst_write_support;
	u32 i2c_transfer_data_type;
	struct eeprom_info_struct *eeprom_info;
	u32 eeprom_num;
	u16 resolution[2];
	u8 mirror;

	u8 mclk; /* mclk freqency, suggest 24 or 26 for 24Mhz or 26Mhz */
	u8 isp_driving_current; /* mclk driving current */
	u8 sensor_interface_type;
	u8 mipi_sensor_type; /* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2, default is NCSI2 */
	u8 mipi_lane_num;
	u32 ob_pedestal;

	u8 sensor_output_dataformat;
	u32 ana_gain_def; /* ana_gain = 4x, ISO400 = 0dB */
	u32 ana_gain_min;
	u32 ana_gain_max;
	u32 ana_gain_type;
	u32 ana_gain_step;
	u32 *ana_gain_table;
	u32 ana_gain_table_size;
	u32 tuning_iso_base; /* ana_gain = 1x, support min ISO100 */
	u32 exposure_def;
	u32 exposure_min;
	u32 exposure_max;
	u32 exposure_step;
	u8 exposure_margin;
	u32 dig_gain_min;
	u32 dig_gain_max;
	u32 dig_gain_step;
	struct mtk_sensor_saturation_info *saturation_info;

	u32 frame_length_max;
	u8 ae_effective_frame;
	u8 frame_time_delay_frame; /* EX: sony => 3 ; non-sony => 2 */
	u32 start_exposure_offset;
#ifdef IMGSENSOR_FUSION_TEST_WORKAROUND
	u32 start_exposure_offset_custom;
#endif
	enum IMGSENSOR_PDAF_SUPPORT_TYPE_ENUM pdaf_type;
	enum IMGSENSOR_HDR_SUPPORT_TYPE_ENUM hdr_type;
	enum IMGSENSOR_RGBW_SUPPORT_TYPE_ENUM rgbw_support;
	u8 seamless_switch_support;
	enum SENSOR_SEAMLESS_SWITCH_TYPE seamless_switch_type;
	unsigned int seamless_switch_hw_re_init_time_ns;
	u8 seamless_switch_prsh_hw_fixed_value;
	u32 seamless_switch_prsh_length_lc;
	struct reg_ reg_addr_prsh_length_lines;
	u16 reg_addr_prsh_mode;

	u8 temperature_support;

	int (*g_temp)(void *arg);
	u16 (*g_gain2reg)(u32 arg);
	void (*g_cali)(void *arg);
	void (*s_gph)(void *arg, u8 en);
	void (*s_cali)(void *arg);
	int (*s_streaming_control)(void *arg, bool enable);
	void (*s_data_rate_global_timing_phy_ctrl)(void *arg);
	int (*s_pwr_seq_reset_view_to_sensing)(void *arg);

	u16 reg_addr_stream;
	u16 reg_addr_mirror_flip;
	struct reg_ reg_addr_exposure[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct reg_ reg_addr_exposure_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	u16 long_exposure_support;
	u16 reg_addr_exposure_lshift;
	struct reg_ reg_addr_ana_gain[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct reg_ reg_addr_ana_gain_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct reg_ reg_addr_dig_gain[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct reg_ reg_addr_dig_gain_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	struct reg_ reg_addr_frame_length;
	struct reg_ reg_addr_frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	u16 reg_addr_temp_en;
	u16 reg_addr_temp_read;
	u16 reg_addr_auto_extend;
	u16 reg_addr_frame_count;
	u16 reg_addr_fast_mode;
	u16 reg_addr_dcg_ratio;
	u16 reg_addr_fast_mode_in_lbmf;
	u16 reg_addr_stream_in_lbmf;

	u16 *init_setting_table;
	u32 init_setting_len;
	struct subdrv_mode_struct *mode;
	u32 sensor_mode_num;
	struct subdrv_feature_control *list;
	u32 list_len;
	u8 chk_s_off_sta;
	u8 chk_s_off_end;
	u8 cam_type;
	u32 checksum_value;

	u8 aov_sensor_support;
	u32 aov_csi_clk;	/* aov switch csi clk param */
	unsigned int sensor_mode_ops;
	bool sensor_debug_sensing_ut_on_scp;
	bool sensor_debug_dphy_global_timing_continuous_clk;
	u16 reg_addr_aov_mode_mirror_flip;
	u8 init_in_open;
	u8 streaming_ctrl_imp;

	/* custom stream control delay timing for hw limitation */
	u64 custom_stream_ctrl_delay;

	/* embedded data line parsing */
	struct ebd_info_struct ebd_info;

	/* record glp data type */
	u32 glp_dt[GLP_DT_MAX_CNT];
};

#define HDR_CAP_IHDR 0x1
#define HDR_CAP_MVHDR 0x2
#define HDR_CAP_ZHDR 0x4
#define HDR_CAP_3HDR 0x8
#define HDR_CAP_ATR 0x10

#define PDAF_CAP_PIXEL_DATA_IN_RAW 0x1
#define PDAF_CAP_PIXEL_DATA_IN_VC 0x2
#define PDAF_CAP_DIFF_DATA_IN_VC 0x4
#define PDAF_CAP_PDFOCUS_AREA 0x10

#define SUBDRV_I2C_BUF_SIZE 256
struct subdrv_ctx {
	struct i2c_client *i2c_client;
	struct i3c_i2c_device ixc_client;
	u8 i2c_write_id;
	u16 _i2c_data[SUBDRV_I2C_BUF_SIZE];
	u16 _size_to_write;
	u32 eeprom_index;
	u8 eeprom_i2c_write_id;

	u32 is_hflip:1;
	u32 is_vflip:1;
	u32 hdr_cap;
	u32 pdaf_cap;
	int max_frame_length;
	int ana_gain_min;
	int ana_gain_max;
	int ana_gain_step;
	int ana_gain_def;
	int exposure_min;
	int exposure_max;
	int exposure_step;
	int exposure_def;
	int le_shutter_def;
	int me_shutter_def;
	int se_shutter_def;
	int le_gain_def;
	int me_gain_def;
	int se_gain_def;

	enum SENSOR_SCENARIO_ID_ENUM current_scenario_id;
	bool is_streaming;
	u32 sof_cnt;
	u32 ref_sof_cnt;
	u32 sof_no;

	u8 mirror; /* mirrorflip information */
	u8 sensor_mode; /* record IMGSENSOR_MODE enum value */
	u32 exposure[IMGSENSOR_STAGGER_EXPOSURE_CNT]; /* current exposure */
	u32 ana_gain[IMGSENSOR_STAGGER_EXPOSURE_CNT]; /* current ana_gain */
	u32 dig_gain[IMGSENSOR_STAGGER_EXPOSURE_CNT]; /* current dig_gain */
	u32 shutter; /* current shutter */
	u32 gain; /* current gain */
	u64 pclk; /* current pclk */
	u32 frame_length_rg; /* current framelength in RG */
	u32 frame_length_in_lut_rg[IMGSENSOR_STAGGER_EXPOSURE_CNT]; /* current lbmf framelength in RG */
	u32 frame_length; /* current framelength */
	u32 frame_length_in_lut[IMGSENSOR_STAGGER_EXPOSURE_CNT]; /* current lbmf framelength */
	u32 frame_length_next; /* next framelength for sw delay fl */
	u32 line_length; /* current linelength */
	u32 min_frame_length; /* current framelength limitation */
	u8 margin; /* current (mode's) exp margin */
	u8 frame_time_delay_frame; /* EX: sony => 3 ; non-sony => 2 */
	u16 dummy_pixel; /* current dummypixel */
	u16 dummy_line; /* current dummline */
	u16 current_fps; /* current max fps */
	u32 readout_length; /* current readoutlength */
	u8 read_margin; /* current read margin */
	int autoflicker_en; /* record autoflicker enable or disable */
	u8 test_pattern; /* record test pattern mode or not */
	u8 ihdr_mode; /* ihdr enable or disable */
	u8 pdaf_mode; /* pdaf enable or disable */
	u8 hdr_mode; /* HDR mode : 0: disable HDR, 1:IHDR, 2:HDR, 9:ZHDR */
	struct IMGSENSOR_AE_FRM_MODE ae_frm_mode;
	u8 current_ae_effective_frame;
	bool extend_frame_length_en;
	bool is_seamless;
	bool fast_mode_on;
	bool ae_ctrl_gph_en;
	u16 l_shift;

	u32 is_read_preload_eeprom;
	u32 is_read_four_cell;

	struct pinctrl *pinctrl;
	struct pinctrl_state *state[STATE_MAXCNT];
	struct regulator *regulator[REGULATOR_MAXCNT];
	struct clk *clk[CLK_MAXCNT];

	struct subdrv_static_ctx s_ctx;

	u8 aov_sensor_support;
	/* aov switch csi clk param */
	u32 aov_csi_clk;
	const char *aov_phy_ctrl_ver;
	unsigned int sensor_mode_ops;
	bool sensor_debug_sensing_ut_on_scp;
	bool sensor_debug_dphy_global_timing_continuous_clk;

	bool *power_on_profile_en;
	struct mtk_sensor_profile sensor_pw_on_profile;

	/* for custom stream control delay timing */
	u64 stream_ctrl_start_time;
	u64 stream_ctrl_end_time;
};

struct subdrv_feature_control {
	MSDK_SENSOR_FEATURE_ENUM feature_id;
	int (*func)(struct subdrv_ctx *ctx, u8 *para, u32 *len);
};

struct subdrv_ops {
	int (*get_id)(struct subdrv_ctx *ctx, u32 *id);
	int (*init_ctx)(struct subdrv_ctx *ctx,
			struct i2c_client *i2c_client, u8 i2c_write_id);
	int (*open)(struct subdrv_ctx *ctx);
	int (*get_info)(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
			MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	int (*get_resolution)(struct subdrv_ctx *ctx,
			MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	int (*feature_control)(struct subdrv_ctx *ctx,
			MSDK_SENSOR_FEATURE_ENUM FeatureId,
			u8 *pFeaturePara,
			u32 *pFeatureParaLen);
	int (*control)(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM ScenarioId,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
			MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
	int (*close)(struct subdrv_ctx *ctx);
	int (*get_frame_desc)(struct subdrv_ctx *ctx,
			int scenario_id,
			struct mtk_mbus_frame_desc *fd);
	int (*get_temp)(struct subdrv_ctx *ctx, int *temp);
	int (*vsync_notify)(struct subdrv_ctx *ctx, unsigned int sof_cnt);
	int (*update_sof_cnt)(struct subdrv_ctx *ctx, unsigned int sof_cnt);
	int (*get_csi_param)(struct subdrv_ctx *ctx,
		enum SENSOR_SCENARIO_ID_ENUM scenario_id,
		struct mtk_csi_param *csi_param);
	int (*get_sensor_usage)(struct subdrv_ctx *ctx, enum mtk_sensor_usage *usage);
	int (*power_on)(struct subdrv_ctx *ctx, void *data);
	int (*power_off)(struct subdrv_ctx *ctx, void *data);
	int (*parse_ebd_line)(struct subdrv_ctx *ctx,
		struct mtk_recv_sensor_ebd_line *data,
		struct mtk_ebd_dump *obj);
	int (*set_ctrl_locker)(struct subdrv_ctx *ctx, u32 cid, bool *is_lock);
	int (*pre_open)(struct subdrv_ctx *ctx);
};

struct subdrv_entry {
	const char *name;
	unsigned int id;
	const struct subdrv_pw_seq_entry *pw_seq;
	const struct subdrv_ops *ops;
	int pw_seq_cnt;
};

#define subdrv_call(ctx, o, args...) \
({ \
	struct adaptor_ctx *__ctx = (ctx); \
	int __ret; \
	if (!__ctx || !__ctx->subdrv || !__ctx->subdrv->ops) \
		__ret = -ENODEV; \
	else if (!__ctx->subdrv->ops->o) \
		__ret = -ENOIOCTLCMD; \
	else \
		__ret = __ctx->subdrv->ops->o(&ctx->subctx, ##args); \
	__ret; \
})

#define subdrv_i2c_rd_u8(subctx, reg) \
({ \
	u8 __val = 0xff; \
	adaptor_i2c_rd_u8(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, &__val); \
	__val; \
})

#define subdrv_i2c_rd_u16(subctx, reg) \
({ \
	u16 __val = 0xffff; \
	adaptor_i2c_rd_u16(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, &__val); \
	__val; \
})

#define subdrv_i2c_wr_u8(subctx, reg, val) \
	adaptor_i2c_wr_u8(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, val)

#define subdrv_i2c_wr_u16(subctx, reg, val) \
	adaptor_i2c_wr_u16(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, val)

#define subdrv_i2c_wr_p8(subctx, reg, p_vals, n_vals) \
	adaptor_i2c_wr_p8(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_i2c_wr_p16(subctx, reg, p_vals, n_vals) \
	adaptor_i2c_wr_p16(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_i2c_wr_seq_p8(subctx, reg, p_vals, n_vals) \
	adaptor_i2c_wr_seq_p8(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_i2c_wr_regs_u8(subctx, list, len) \
	adaptor_i2c_wr_regs_u8(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, list, len)

#define subdrv_i2c_wr_regs_u16(subctx, list, len) \
	adaptor_i2c_wr_regs_u16(subctx->i2c_client, \
		subctx->i2c_write_id >> 1, list, len)
/* for i2c/i3c data transfer */
#define subdrv_ixc_rd_u8(subctx, reg) \
({ \
	u8 __val = 0xff; \
	adaptor_ixc_rd_u8(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, &__val); \
	__val; \
})

#define subdrv_ixc_rd_u16(subctx, reg) \
({ \
	u16 __val = 0xffff; \
	adaptor_ixc_rd_u16(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, &__val); \
	__val; \
})

#define subdrv_ixc_wr_u8(subctx, reg, val) \
	adaptor_ixc_wr_u8(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, val)

#define subdrv_ixc_wr_u16(subctx, reg, val) \
	adaptor_ixc_wr_u16(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, val)

#define subdrv_ixc_wr_p8(subctx, reg, p_vals, n_vals) \
	adaptor_ixc_wr_p8(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_ixc_wr_p16(subctx, reg, p_vals, n_vals) \
	adaptor_ixc_wr_p16(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_ixc_wr_seq_p8(subctx, reg, p_vals, n_vals) \
	adaptor_ixc_wr_seq_p8(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, reg, p_vals, n_vals)

#define subdrv_ixc_wr_regs_u8(subctx, list, len) \
	adaptor_ixc_wr_regs_u8(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, list, len)

#define subdrv_ixc_wr_regs_u16(subctx, list, len) \
	adaptor_ixc_wr_regs_u16(&subctx->ixc_client, \
		subctx->i2c_write_id >> 1, list, len)

#define FINE_INTEG_CONVERT(_shutter, _fine_integ) \
( \
	!(_fine_integ) ? \
	(_shutter) : \
	((((long long)(_shutter) - (_fine_integ)) > 0) ? (((_shutter) - (_fine_integ)) / 1000) : 0) \
)

#define CALC_LINE_TIME_IN_NS(pclk, linelength) \
({ \
	unsigned int val = 0; \
	do { \
		if (((pclk) / 1000) == 0) { \
			val = 0; \
			break; \
		} \
		val = \
			((unsigned long long)(linelength)*1000000+(((pclk)/1000)-1)) \
			/((pclk)/1000); \
	} while (0); \
	val; \
})

#define CONVERT_2_TOTAL_TIME(lineTimeInNs, lc) \
({ \
	unsigned int val = 0; \
	do { \
		if ((lineTimeInNs) == 0) { \
			val = 0; \
			break; \
		} \
		val = \
			(unsigned int)((unsigned long long)(lc)*(lineTimeInNs)/1000); \
	} while (0); \
	val; \
})

#define CONVERT_2_TOTAL_TIME_V2(pclk, linelength, lc) \
({ \
	unsigned int val = 0; \
	unsigned int lineTimeInNs = 0; \
	lineTimeInNs = CALC_LINE_TIME_IN_NS((pclk), (linelength)); \
	do { \
		if ((lineTimeInNs) == 0) { \
			val = 0; \
			break; \
		} \
		val = \
			(unsigned int)((unsigned long long)(lc)*(lineTimeInNs)/1000); \
	} while (0); \
	val; \
})

#endif
