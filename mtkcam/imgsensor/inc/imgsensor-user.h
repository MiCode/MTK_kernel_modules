/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 MediaTek Inc. */

#ifndef __IMGSENSOR_USER_H__
#define __IMGSENSOR_USER_H__

#include <linux/videodev2.h>

#include "kd_imgsensor_define_v4l2.h"
#include "mtk_camera-v4l2-controls-common.h"
#include "mtk_camera-videodev2.h"

#define DEFAULT_WIDTH 1600
#define DEFAULT_HEIGHT 1200

enum {
	PAD_SINK = 0,
	PAD_SRC_RAW0,
	PAD_SRC_RAW1,
	PAD_SRC_RAW2,
	PAD_SRC_RAW3,
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
	PAD_SRC_FLICKER,
	PAD_SRC_GENERAL0,
	PAD_SRC_META0,
	PAD_SRC_META1,
	PAD_MAXCNT,
	PAD_ERR = 0xffff,
};

enum sentest_ctrl_id {
	/* GET CTRL */
	SENTEST_G_CTRL_ID_MIN,
	SENTEST_G_SENSOR_PROFILE = SENTEST_G_CTRL_ID_MIN,
	SENTEST_G_TSREC_TIME_STAMP,
	SENTEST_G_CTRL_ID_MAX,

	/* SET CTRL */
	SENTEST_S_CTRL_ID_MIN,
	SENTEST_S_SENSOR_PROFILE_EN = SENTEST_S_CTRL_ID_MIN,
	SENTEST_S_SENSOR_LBMF_DO_DELAY_AE_EN,
	SENTEST_S_TSREC_TRAGET_FRAME_ID,
	SENTEST_S_CTRL_ID_MAX,
};
enum mtk_sensor_usage {
	MTK_SENSOR_USAGE_SINGLE,
	MTK_SENSOR_USAGE_COMB,
	MTK_SENSOR_USAGE_MUTI,
	MTK_SENSOR_USAGE_NONCOMB,
};

struct mtk_awb_gain {
	__u32 abs_gain_gr;
	__u32 abs_gain_r;
	__u32 abs_gain_b;
	__u32 abs_gain_gb;
};

struct mtk_shutter_gain_sync {
	__u64 shutter;
	__u32 gain;
};

struct mtk_dual_gain {
	__u32 le_gain;
	__u32 se_gain;
};

struct mtk_ihdr_shutter_gain {
	__u64 le_shutter;
	__u64 se_shutter;
	__u32 gain;
};

struct mtk_pixel_mode {
	__u32 pixel_mode;
	__u32 pad_id;
};


struct mtk_hdr_shutter {
	__u64 le_shutter;
	__u64 se_shutter;
};

struct mtk_shutter_frame_length {
	__u64 shutter;
	__u32 frame_length;
	__u32 auto_extend_en;
};

struct mtk_fps_by_scenario {
	__u32 scenario_id;
	__u32 fps;
};

struct mtk_pclk_by_scenario {
	__u32 scenario_id;
	__u64 pclk;
};

struct mtk_llp_fll_by_scenario {
	__u32 scenario_id;
	__u32 llp;
	__u32 fll;
};

struct mtk_gain_range_by_scenario {
	__u32 scenario_id;
	__u32 min_gain;
	__u32 max_gain;
};

struct mtk_min_shutter_by_scenario {
	__u32 scenario_id;
	__u32 min_shutter;
	__u32 shutter_step;
};

struct mtk_base_gain_iso_n_step {
	__u32 min_gain_iso;
	__u32 gain_step;
	__u32 gain_type;
};

struct mtk_crop_by_scenario {
	__u32 scenario_id;
	struct SENSOR_WINSIZE_INFO_STRUCT *p_winsize;
};

struct mtk_vcinfo_by_scenario {
	__u32 scenario_id;
	struct SENSOR_VC_INFO2_STRUCT *p_vcinfo;
};

struct mtk_pdaf_info_by_scenario {
	__u32 scenario_id;
	struct SET_PD_BLOCK_INFO_T *p_pd;
};

struct mtk_cap {
	__u32 scenario_id;
	__u32 cap;
};

struct mtk_binning_type {
	__u32 scenario_id;
	__u32 HDRMode;
	__u32 binning_type;
};

struct mtk_ana_gain_table {
	int size;
	__u32 *p_buf;
};

struct mtk_llp_fll {
	__u32 llp;
	__u32 fll;
};

struct mtk_pdaf_data {
	int offset;
	int size;
	__u8 *p_buf;
};

struct mtk_pdfocus_area {
	__u32 pos;
	__u32 size;
};

struct mtk_mipi_pixel_rate {
	__u32 scenario_id;
	__u32 mipi_pixel_rate;
};

struct mtk_4cell_data {
	int type;
	int size;
	__u8 *p_buf;
};

struct mtk_hdr_atr {
	__u32 limit_gain;
	__u32 ltc_rate;
	__u32 post_gain;
};

struct mtk_hdr_exposure {
	union {
		struct {
			__u64 le_exposure;
			__u64 me_exposure;
			__u64 se_exposure;
			__u64 sse_exposure;
			__u64 ssse_exposure;
		};

		__u64 arr[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	};

};

struct mtk_hdr_gain {
	union {
		struct {
			__u32 le_gain;
			__u32 me_gain;
			__u32 se_gain;
			__u32 sse_gain;
			__u32 ssse_gain;
		};

		__u32 arr[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	};

};

struct mtk_hdr_ae {
	struct mtk_hdr_exposure exposure;
	struct mtk_hdr_gain gain;
	struct mtk_hdr_exposure w_exposure;
	struct mtk_hdr_gain w_gain;
	__u32 actions;
	__u32 subsample_tags;
	int req_id;
};

struct mtk_seamless_switch_param {
	struct mtk_hdr_ae ae_ctrl[2];
	__u32 frame_length[2];
	__u32 target_scenario_id;
};

/* struct mtk_regs
 * @size:
	the size of buffer in bytes.
 * @p_buf:
	addr, val, addr, val, ... in that order
	in unit of __u16
 */
struct mtk_regs {
	int size;
	__u16 *p_buf;
};

struct mtk_sensor_info {
	char name[64];
	__u32 id;
	__u32 dir;
	__u32 bitOrder;
	__u32 orientation;
	__u32 horizontalFov;
	__u32 verticalFov;
	__u32 dts_idx;
};

struct mtk_scenario_timing {
	__u32 llp;
	__u32 fll;
	__u32 width;
	__u32 height;
	__u32 mipi_pixel_rate;
	__u32 max_framerate;
	__u64 pclk;
	__u64 linetime_in_ns;
};

struct mtk_scenario_combo_info {
	__u32 scenario_id;
	struct mtk_scenario_timing *p_timing;
	struct SET_PD_BLOCK_INFO_T *p_pd;
	struct SENSOR_WINSIZE_INFO_STRUCT *p_winsize;
	struct SENSOR_VC_INFO2_STRUCT *p_vcinfo;
};

struct mtk_min_max_fps {
	__u32 min_fps;
	__u32 max_fps;
};

struct mtk_feature_info {
	__u32 scenario_id;
	struct ACDK_SENSOR_INFO_STRUCT *p_info;
	struct ACDK_SENSOR_CONFIG_STRUCT *p_config;
	struct ACDK_SENSOR_RESOLUTION_INFO_STRUCT *p_resolution;
};

struct mtk_lsc_tbl {
	int index;
	int size;
	__u16 *p_buf;
};

struct mtk_sensor_control {
	int scenario_id;
	struct ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT *p_window;
	struct ACDK_SENSOR_CONFIG_STRUCT *p_config;
};

struct mtk_stagger_info {
	__u32 scenario_id;
	__u32 count;
	int order[IMGSENSOR_STAGGER_EXPOSURE_CNT];
};

struct mtk_stagger_target_scenario {
	__u32 scenario_id;
	__u32 exposure_num;
	__u32 target_scenario_id;
};

struct mtk_seamless_target_scenarios {
	__u32 scenario_id;
	__u32 count;
	__u32 *target_scenario_ids;
};

struct mtk_stagger_max_exp_time {
	__u32 scenario_id;
	__u32 exposure;
	__u32 max_exp_time;
};

struct mtk_max_exp_line {
	__u32 scenario_id;
	__u32 exposure;
	__u32 max_exp_line;
};

struct mtk_exp_margin {
	__u32 scenario_id;
	__u32 margin;
};

struct mtk_sensor_value {
	__u32 scenario_id;
	__u32 value;
};

struct mtk_sensor_static_param {
	__u32 scenario_id;
	__u32 fps;
	__u32 vblank;
	__u32 hblank;
	__u32 pixelrate;
	__u32 cust_pixelrate;
	__u32 grab_w;
	__u32 grab_h;
};

enum mtk_cam_seninf_tsrec_exp_id {
	TSREC_EXP_ID_AUTO = 0,
	TSREC_EXP_ID_1,
	TSREC_EXP_ID_2,
	TSREC_EXP_ID_3,
};

enum mtk_mbus_frame_desc_dt_remap_type {
	MTK_MBUS_FRAME_DESC_REMAP_NONE = 0,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW12,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW16,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW20,
	MTK_MBUS_FRAME_DESC_REMAP_TO_RAW24,
};

enum mtk_frame_desc_parsing_type {
	MTK_EBD_PARSING_TYPE_MIPI_RAW_NA = 0,
	MTK_EBD_PARSING_TYPE_MIPI_RAW8,
	MTK_EBD_PARSING_TYPE_MIPI_RAW10,
	MTK_EBD_PARSING_TYPE_MIPI_RAW12,
	MTK_EBD_PARSING_TYPE_MIPI_RAW14,
};

enum mtk_frame_desc_fs_seq {
	MTK_FRAME_DESC_FS_SEQ_UNKNOWN = 0,
	MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
	MTK_FRAME_DESC_FS_SEQ_FIRST = MTK_FRAME_DESC_FS_SEQ_ONLY_ONE,
	MTK_FRAME_DESC_FS_SEQ_LAST,
};

struct mtk_mbus_frame_desc_entry_csi2 {
	u8 channel;
	u8 data_type;
	u8 enable;
	u8 dt_remap_to_type;
	u16 hsize;
	u16 vsize;
	u16 user_data_desc;
	u8 is_sensor_hw_pre_latch_exp;
	enum mtk_cam_seninf_tsrec_exp_id cust_assign_to_tsrec_exp_id;
	u16 valid_bit;
	u8 is_active_line;
	u8 ebd_parsing_type; // for ebd parser query how to parse content
	enum mtk_frame_desc_fs_seq fs_seq;
};

struct mtk_mbus_frame_desc_entry {
	//enum v4l2_mbus_frame_desc_flags flags;
	//u32 pixelcode;
	//u32 length;
	union {
		struct mtk_mbus_frame_desc_entry_csi2 csi2;
	} bus;
};

#define MTK_FRAME_DESC_ENTRY_MAX 16
enum mtk_mbus_frame_desc_type {
	MTK_MBUS_FRAME_DESC_TYPE_PLATFORM,
	MTK_MBUS_FRAME_DESC_TYPE_PARALLEL,
	MTK_MBUS_FRAME_DESC_TYPE_CCP2,
	MTK_MBUS_FRAME_DESC_TYPE_CSI2,
};

struct mtk_mbus_frame_desc {
	enum mtk_mbus_frame_desc_type type;
	struct mtk_mbus_frame_desc_entry entry[MTK_FRAME_DESC_ENTRY_MAX];
	unsigned short num_entries;
};

struct mtk_csi_param {
	__u32 dphy_trail;
	__u32 dphy_data_settle;
	__u32 dphy_clk_settle;
	__u32 cphy_settle;
	__u8 legacy_phy;
	__u8 not_fixed_trail_settle;
	__u32 dphy_csi2_resync_dmy_cycle;
	__u8 not_fixed_dphy_settle;
	__u8 dphy_init_deskew_support;
	__u8 cphy_lrte_support;
	__u8 clk_lane_no_initial_flow;
	__u8 initial_skew;
};

struct mtk_sensor_saturation_info {
	__u32 gain_ratio;
	__u32 OB_pedestal;
	__u32 saturation_level;
	/* The merged raw by the dcg sensor merging mode is merged from several bits of raws */
	__u32 adc_bit;
	/* The OB value before merging */
	__u32 ob_bm;
};

struct mtk_n_1_mode {
	__u32 n;
	__u8 en;
};

enum FS_SYNC_TYPE {
	FS_SYNC_TYPE_NONE = 0,

	/* below tags, chose one (default use VSYNC, mutually exclusive) */
	FS_SYNC_TYPE_VSYNC = 1 << 1,
	FS_SYNC_TYPE_READOUT_CENTER = 1 << 2,

	/* below tags, chose one (default use LE, mutually exclusive) */
	FS_SYNC_TYPE_LE = 1 << 3,
	FS_SYNC_TYPE_SE = 1 << 4,

	/* SA - Async mode */
	FS_SYNC_TYPE_ASYNC_MODE = 1 << 8,

	/* Optional flags */
	FS_SYNC_TYPE_AUTO_CLR_ASYNC_BIT = 1 << 12,

	/* Custom input parameter */
	FS_SYNC_TYPE_CUST_INPUT_PARA_BIT = 1 << 20,
};

struct mtk_fs_frame_length_info {
	/* for stable case, sensor min frame length */
	__u32 target_min_fl_us;

	/* sensor current frame length value */
	__u32 out_fl_us;
};

struct mtk_test_pattern_data {
	__u32 Channel_R;
	__u32 Channel_Gr;
	__u32 Channel_Gb;
	__u32 Channel_B;
};

struct mtk_fine_integ_line {
	__u32 scenario_id;
	int fine_integ_line;
};

struct mtk_sensor_mode_info {
	__u32 scenario_id;
	__u32 mode_exposure_num;
	__u32 active_line_num;
	__u64 avg_linetime_in_ns;
};

struct mtk_sensor_mode_config_info {
	__u32 current_scenario_id;
	__u32 count;
	struct mtk_sensor_mode_info seamless_scenario_infos[SENSOR_SCENARIO_ID_MAX];
};

struct mtk_DCG_gain_ratio_table {
	__u32 scenario_id;
	int size;
	__u32 *p_buf;
};

struct mtk_DCG_gain_ratio_range_by_scenario {
	__u32 scenario_id;
	__u32 min_gain_ratio;
	__u32 max_gain_ratio;
};

struct mtk_DCG_type_by_scenario {
	__u32 scenario_id;
	__u32 dcg_mode;
	__u32 dcg_gain_mode;
};

struct mtk_sensor_profile {
	__u64 i2c_init_period;
	__u16 i2c_init_table_len;
	__u64 i2c_cfg_period;
	__u16 i2c_cfg_table_len;
	__u64 hw_power_on_period;
};


struct mtk_adaptor_sentest_ctrl {
	enum sentest_ctrl_id ctrl_id;
	void *param_ptr;
};

struct mtk_gain_range {
	__u32 min;
	__u32 max;
};

struct mtk_shutter_range {
	__u64 min;
	__u64 max;
};

struct mtk_multi_exp_gain_range {
	struct mtk_gain_range exposure[5];
};

struct mtk_multi_exp_shutter_range {
	struct mtk_shutter_range exposure[5];
};

struct mtk_multi_exp_gain_range_by_scenario {
	__u32 scenario_id;
	__u32 exp_cnt;
	struct mtk_multi_exp_gain_range *multi_exp_gain_range;
};

struct mtk_multi_exp_shutter_range_by_scenario {
	__u32 scenario_id;
	__u32 exp_cnt;
	struct mtk_multi_exp_shutter_range *multi_exp_shutter_range;
};

struct mtk_sensor_ebd_info_by_scenario {
	__u32 input_scenario_id;
	__u16 exp_hsize;
	__u16 exp_vsize;
	__u8 dt_remap_to_type;
	__u8 data_type;
	__u8 ebd_parsing_type;
};

struct mtk_recv_sensor_ebd_line {
	__u32 req_id;
	char *req_fd_desc;
	__u32 mbus_code;
	__u32 stride;
	__u32 buf_sz;
	char *buf;
	__u8 ebd_parsing_type;
};

struct mtk_ebd_dump {
	__u32 frm_cnt;
	__u32 cit[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	__u32 again[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	__u32 dgain[IMGSENSOR_STAGGER_EXPOSURE_CNT];
	__u32 cit_shift;
	__u32 dol;
	__u32 fll;
	__u32 temperature;
};

struct mtk_ebd_dump_record {
	__u64 recv_ts;
	__u32 req_no;
	char req_fd_desc[50];
	struct mtk_ebd_dump record;
};

struct mtk_sensor_vc_info_by_scenario {
	__u32 scenario_id;
	struct mtk_mbus_frame_desc fd;
};

struct mtk_exp_line_by_scenario {
	__u32 scenario_id;
	__u32 fps;	// 10-base, ex: 30fps, set .fps = 300
	__u64 exp_line;
};

struct mtk_1sof_vsync_ts_info {
	__u64 vsync_ts_ns; // latest ts from streaming sensor
	__u32 fps; // 30FPS: 300
	__u32 target_timing_us;
};

struct mtk_fsync_hw_mcss_init_info {
	__u32 enable_mcss;
	__u32 is_mcss_master; // master or slave
};

struct mtk_fsync_hw_mcss_mask_frm_info {
	__u32 mask_frm_num;
	__u32 is_critical; // write I2C immediately
};


struct mtk_fake_sensor_info {
	__u32 is_fake_sensor;
	__u32 fps; /* 300 -> 30 FPS */
	enum IMGSENSOR_HDR_MODE_ENUM hdr_mode;
};


/* GET */

#define VIDIOC_MTK_G_DEF_FPS_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 1, struct mtk_fps_by_scenario)

#define VIDIOC_MTK_G_PCLK_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 2, struct mtk_pclk_by_scenario)

#define VIDIOC_MTK_G_LLP_FLL_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 3, struct mtk_llp_fll_by_scenario)

#define VIDIOC_MTK_G_GAIN_RANGE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 4, struct mtk_gain_range_by_scenario)

#define VIDIOC_MTK_G_MIN_SHUTTER_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 5, struct mtk_min_shutter_by_scenario)

#define VIDIOC_MTK_G_CROP_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 6, struct mtk_crop_by_scenario)

#define VIDIOC_MTK_G_VCINFO_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 7, struct mtk_vcinfo_by_scenario)

#define VIDIOC_MTK_G_SCENARIO_COMBO_INFO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 8, struct mtk_scenario_combo_info)

#define VIDIOC_MTK_G_BINNING_TYPE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 10, struct mtk_binning_type)

#define VIDIOC_MTK_G_PDAF_INFO_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 11, struct mtk_pdaf_info_by_scenario)

#define VIDIOC_MTK_G_LLP_FLL \
	_IOR('M', BASE_VIDIOC_PRIVATE + 12, struct mtk_llp_fll)

#define VIDIOC_MTK_G_PCLK \
	_IOR('M', BASE_VIDIOC_PRIVATE + 13, unsigned long long)

#define VIDIOC_MTK_G_PDAF_DATA \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 14, struct mtk_pdaf_data)

#define VIDIOC_MTK_G_PDAF_CAP \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 15, struct mtk_cap)

#define VIDIOC_MTK_G_PDAF_REGS \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 16, struct mtk_regs)

#define VIDIOC_MTK_G_MIPI_PIXEL_RATE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 17, struct mtk_mipi_pixel_rate)

#define VIDIOC_MTK_G_HDR_CAP \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 18, struct mtk_cap)

#define VIDIOC_MTK_G_TEST_PATTERN_CHECKSUM \
	_IOR('M', BASE_VIDIOC_PRIVATE + 20, __u32)

#define VIDIOC_MTK_G_BASE_GAIN_ISO_N_STEP \
	_IOR('M', BASE_VIDIOC_PRIVATE + 21, struct mtk_base_gain_iso_n_step)

#define VIDIOC_MTK_G_OFFSET_TO_START_OF_EXPOSURE \
	_IOR('M', BASE_VIDIOC_PRIVATE + 22, unsigned int)

#define VIDIOC_MTK_G_ANA_GAIN_TABLE_SIZE \
	_IOR('M', BASE_VIDIOC_PRIVATE + 23, struct mtk_ana_gain_table)

#define VIDIOC_MTK_G_ANA_GAIN_TABLE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 24, struct mtk_ana_gain_table)

#define VIDIOC_MTK_G_DELAY_INFO \
	_IOR('M', BASE_VIDIOC_PRIVATE + 25, struct SENSOR_DELAY_INFO_STRUCT)

#define VIDIOC_MTK_G_FEATURE_INFO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 26, struct mtk_feature_info)

#define VIDIOC_MTK_G_4CELL_DATA \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 27, struct mtk_4cell_data)

#define VIDIOC_MTK_G_AE_FRAME_MODE_FOR_LE \
	_IOR('M', BASE_VIDIOC_PRIVATE + 28, struct IMGSENSOR_AE_FRM_MODE)

#define VIDIOC_MTK_G_AE_EFFECTIVE_FRAME_FOR_LE \
	_IOR('M', BASE_VIDIOC_PRIVATE + 29, unsigned int)

#define VIDIOC_MTK_G_SENSOR_INFO \
	_IOR('M', BASE_VIDIOC_PRIVATE + 30, struct mtk_sensor_info)

#define VIDIOC_MTK_G_EXPOSURE_MARGIN_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 31, struct mtk_exp_margin)

#define VIDIOC_MTK_G_SEAMLESS_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 32, struct mtk_seamless_target_scenarios)

#define VIDIOC_MTK_G_CUSTOM_READOUT_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 33, struct mtk_sensor_value)

#define VIDIOC_MTK_G_STAGGER_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 34, struct mtk_stagger_target_scenario)

#define VIDIOC_MTK_G_MAX_EXPOSURE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 35, struct mtk_stagger_max_exp_time)

#define VIDIOC_MTK_G_PRELOAD_EEPROM_DATA \
	_IOR('M', BASE_VIDIOC_PRIVATE + 36, unsigned int)

#define VIDIOC_MTK_G_OUTPUT_FORMAT_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 37, struct mtk_sensor_value)

#define VIDIOC_MTK_G_FINE_INTEG_LINE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 38, struct mtk_fine_integ_line)

#define VIDIOC_MTK_G_MAX_EXPOSURE_LINE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 39, struct mtk_max_exp_line)

#define VIDIOC_MTK_G_RGBW_OUTPUT_MODE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 40, struct mtk_cap)

#define VIDIOC_MTK_G_DIG_GAIN_RANGE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 41, struct mtk_gain_range_by_scenario)

#define VIDIOC_MTK_G_DIG_GAIN_STEP \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 42, __u32)

#define VIDIOC_MTK_G_FS_FRAME_LENGTH_INFO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 43, struct mtk_fs_frame_length_info)

#define VIDIOC_MTK_G_DCG_GAIN_RATIO_TABLE_SIZE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 44, struct mtk_DCG_gain_ratio_table)

#define VIDIOC_MTK_G_DCG_GAIN_RATIO_TABLE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 45, struct mtk_DCG_gain_ratio_table)

#define VIDIOC_MTK_G_DCG_GAIN_RATIO_RANGE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 46, struct mtk_DCG_gain_ratio_range_by_scenario)

#define VIDIOC_MTK_G_DCG_TYPE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 47, struct mtk_DCG_type_by_scenario)

#define VIDIOC_MTK_G_SENSOR_PROFILE \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 48, struct mtk_sensor_profile)

#define VIDIOC_MTK_G_MULTI_EXP_GAIN_RANGE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 49, struct mtk_multi_exp_gain_range_by_scenario)

#define VIDIOC_MTK_G_MULTI_EXP_SHUTTER_RANGE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 50, struct mtk_multi_exp_shutter_range_by_scenario)

#define VIDIOC_MTK_G_EXP_LINE_BY_SCENARIO \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 51, struct mtk_exp_line_by_scenario)
/* SET */

#define VIDIOC_MTK_S_VIDEO_FRAMERATE \
	_IOW('M', BASE_VIDIOC_PRIVATE + 101, __u32)

#define VIDIOC_MTK_S_MAX_FPS_BY_SCENARIO \
	_IOW('M', BASE_VIDIOC_PRIVATE + 102, struct mtk_fps_by_scenario)

#define VIDIOC_MTK_S_FRAMERATE \
	_IOW('M', BASE_VIDIOC_PRIVATE + 103, __u32)

#define VIDIOC_MTK_S_HDR \
	_IOW('M', BASE_VIDIOC_PRIVATE + 104, int)

#define VIDIOC_MTK_S_PDAF_REGS \
	_IOW('M', BASE_VIDIOC_PRIVATE + 105, struct mtk_regs)

#define VIDIOC_MTK_S_PDAF \
	_IOW('M', BASE_VIDIOC_PRIVATE + 106, int)

#define VIDIOC_MTK_S_MIN_MAX_FPS \
	_IOW('M', BASE_VIDIOC_PRIVATE + 107, struct mtk_min_max_fps)

#define VIDIOC_MTK_S_LSC_TBL \
	_IOW('M', BASE_VIDIOC_PRIVATE + 108, struct mtk_lsc_tbl)

#define VIDIOC_MTK_S_CONTROL \
	_IOW('M', BASE_VIDIOC_PRIVATE + 109, struct mtk_sensor_control)

#define VIDIOC_MTK_S_TG \
	_IOW('M', BASE_VIDIOC_PRIVATE + 110, int)

#define VIDIOC_MTK_S_SENSOR_PROFILE_EN \
	_IOW('M', BASE_VIDIOC_PRIVATE + 111, int)

#define VIDIOC_MTK_S_SENTEST_LBMF_DELAY_DO_AE_EN \
	_IOW('M', BASE_VIDIOC_PRIVATE + 112, int)

#define VIDIOC_MTK_S_SENSOR_SENTEST_CTRL \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 113, struct mtk_adaptor_sentest_ctrl)

#endif
