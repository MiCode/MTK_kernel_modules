/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MTK_AOV_DATA_H
#define MTK_AOV_DATA_H

#include <linux/ioctl.h>

#include "mtk_cam-aov.h"

/*
 * For user space <-> kernel communication
 */
#define AOV_DEV_START             _IOW('H', 0, struct aov_user)
#define AOV_DEV_SENSOR_ON         _IOW('H', 1, struct sensor_notify)
#define AOV_DEV_SENSOR_OFF        _IOW('H', 2, struct sensor_notify)
#define AOV_DEV_DQEVENT           _IOR('H', 3, struct aov_dqevent)
#define AOV_DEV_STOP              _IO('H', 4)
#define AOV_DEV_QEA               _IO('H', 5)
#define AOV_DEV_PWR_UT            _IO('H', 6)
#define AOV_DEV_DISP_ON_UT        _IO('H', 7)
#define AOV_DEV_DISP_OFF_UT       _IO('H', 8)
#define AOV_DEV_TURN_ON_ULPOSC    _IO('H', 9)
#define AOV_DEV_TURN_OFF_ULPOSC   _IO('H', 10)

#if IS_ENABLED(CONFIG_COMPAT)
#define COMPAT_AOV_DEV_START        _IOW('H', 0, struct aov_user)
#define COMPAT_AOV_DEV_DQEVENT      _IOR('H', 1, struct aov_dqevent)
#define COMPAT_AOV_DEV_SENSOR_ON    _IOW('H', 2, struct sensor_notify)
#define COMPAT_AOV_DEV_SENSOR_OFF   _IOW('H', 3, struct sensor_notify)
#define COMPAT_AOV_DEV_STOP         _IO('H', 4)
#define COMPAT_AOV_DEV_QEA          _IO('H', 5)
#define COMPAT_AOV_DEV_PWR_UT       _IO('H', 6)
#define COMPAT_AOV_DEV_DISP_ON_UT   _IO('H', 7)
#define COMPAT_AOV_DEV_DISP_OFF_UT  _IO('H', 8)
#define COMPAT_AOV_DEV_TURN_ON_ULPOSC _IO('H', 9)
#define COMPAT_AOV_DEV_TURN_OFF_ULPOSC _IO('H', 10)
#endif

/*
 * For APMCU <-> SCP communication
 */
#define AOV_SCP_CMD_READY            (0)
#define AOV_SCP_CMD_START            (1)
#define AOV_SCP_CMD_PWR_ON           (2)
#define AOV_SCP_CMD_PWR_OFF          (3)
#define AOV_SCP_CMD_FRAME            (4)
#define AOV_SCP_CMD_NOTIFY           (5)
#define AOV_SCP_CMD_STOP             (6)
#define AOV_SCP_CMD_SMI_DUMP         (7)
#define AOV_SCP_CMD_SMI_DUMP_DONE    (8)
#define AOV_SCP_CMD_ON_UT            (9)
#define AOV_SCP_CMD_OFF_UT           (10)
#define AOV_SCP_CMD_QEA              (11)
#define AOV_SCP_CMD_PWR_UT           (12)
#define AOV_SCP_CMD_RESET_SENSOR     (13)
#define AOV_SCP_CMD_RESET_SENSOR_END (14)
#define AOV_SCP_CMD_TURN_ON_ULPOSC   (15)
#define AOV_SCP_CMD_TURN_OFF_ULPOSC  (16)
#define AOV_SCP_CMD_MAX              (17)
#define AOV_SCP_CMD_ACK              (0x8000)

#define AOV_DEBUG_MODE_DUMP       (1)  // General debug
#define AOV_DEBUG_MODE_NDD        (2)  // NDD debug mode

#define AOV_DISP_MODE_OFF         (0)
#define AOV_DiSP_MODE_ON          (1)

#define AOV_MAX_BASE_EVENT        (2)
#define AOV_MAX_NDD_EVENT         (1)

#define AOV_MAX_USER_SIZE         (offsetof(struct aov_user, aaa_size))
#define AOV_MAX_SENIF_SIZE        (2 * 1024)
#define AOV_MAX_AAA_SIZE          (60 * 1024)
#define AOV_MAX_TUNING_SIZE       (2 * 1024)
#define AOV_MAX_AIE_SIZE          (162 * 1024)
#define AOV_MAX_FLD_SIZE          (3 * 1024 * 1024)	// 3MB
#define AOV_MAX_AIE_SIZE_V2       (280 * 1024)
#define AOV_MAX_FLD_SIZE_V2       (1 * 1024 * 1024)	// 1MB

#define AOV_MAX_YUVO1_OUTPUT      (737280 + 32)  // 640 x 480, nv12 12-bit
#define AOV_MAX_YUVO2_OUTPUT      (184320 + 32)  // 320 x 240, nv12 12-bit
#define AOV_MAX_AIE_OUTPUT        (32 * 1024)
#define AOV_MAX_FLD_OUTPUT        (2 * 1024)
#define AOV_MAX_APU_OUTPUT        (256 * 1024)
#define AOV_MAX_FR_RECORD         (5)
#define AOV_MAX_IMGO_OUTPUT       (921600 + 32)  // 640 x 480, bayer12
#define AOV_MAX_AAO_OUTPUT        (158 * 1024)
#define AOV_MAX_AAHO_OUTPUT       (1 * 1024)
#define AOV_MAX_META_OUTPUT       (6 * 1024)
#define AOV_MAX_AWB_OUTPUT        (1 * 1024)

#define AOV_MAX_SENSOR_COUNT      (64)

extern void mtk_aie_aov_memcpy(char *buffer);
extern void mtk_fld_aov_memcpy(char *buffer);

/**
 * Detection objects.
 * Detection modes.
 */
enum AovDetectionObject : uint32_t {
	/**
	 * No event will be returned.
	 */
	eOBJECT_NONE = 0x0,

	/**
	 * The simple face information.
	 * Face frame information.
	 */
	eOBJECT_FACE_SIMPLE = (0x1 << 0),

	/**
	 * The full face information.
	 * Face frame and facial features information.
	 */
	eOBJECT_FACE_FULL = (0x1 << 1),

	/**
	 * The gaze information.
	 */
	eOBJECT_GAZE = (0x1 << 2),

	/**
	 * The gesture information.
	 */
	eOBJECT_GESTURE = (0x1 << 3),

	/**
	 * The QR code scanner information.
	 */
	eOBJECT_QRCODE_SCANNER = (0x1 << 4),

	/**
	 * The face recognition information.
	 */
	eOBJECT_FACE_RECOGNITION = (0x1 << 5)
};

struct sensor_notify {
	uint32_t count;
	int32_t sensor[AOV_MAX_SENSOR_COUNT];
};

struct aov_dqevent {
	uint32_t session;
	uint32_t frame_id;
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t frame_mode;
	uint32_t detect_mode;

	// for object detection
	uint32_t aie_size;
	void *aie_output;

	uint32_t fld_size;
	void *fld_output;

	uint32_t apu_size;
	void *apu_output;

	// for general debug
	uint32_t yuvo1_width;
	uint32_t yuvo1_height;
	uint32_t yuvo1_format;
	uint32_t yuvo1_stride;
	void *yuvo1_output;

	uint32_t yuvo2_width;
	uint32_t yuvo2_height;
	uint32_t yuvo2_format;
	uint32_t yuvo2_stride;
	void *yuvo2_output;

	// for NDD debug mode
	uint32_t imgo_width;
	uint32_t imgo_height;
	uint32_t imgo_format;
	uint32_t imgo_order;
	uint32_t imgo_stride;
	void *imgo_output;

	uint32_t aao_size;
	void *aao_output;

	uint32_t aaho_size;
	void *aaho_output;

	uint32_t meta_size;
	void *meta_output;

	uint32_t awb_size;
	void *awb_output;
};

struct fr_info_t {
	uint32_t count;
	uint32_t offset[AOV_MAX_FR_RECORD];
};

struct base_event {
	uint32_t event_id;

	uint32_t session;
	uint32_t frame_id;
	uint32_t frame_width;
	uint32_t frame_height;
	uint32_t frame_mode;
	uint32_t detect_mode;

	// for object detection
	uint32_t aie_size;
	uint8_t aie_output[AOV_MAX_AIE_OUTPUT];

	uint32_t fld_size;
	uint8_t fld_output[AOV_MAX_FLD_OUTPUT];

	uint32_t apu_size;
	uint8_t apu_output[AOV_MAX_APU_OUTPUT];

	struct fr_info_t fr_info;

	// for general debug
	uint32_t yuvo1_width;
	uint32_t yuvo1_height;
	uint32_t yuvo1_format;
	uint32_t yuvo1_stride;
	uint8_t yuvo1_output[AOV_MAX_YUVO1_OUTPUT];

	uint32_t yuvo2_width;
	uint32_t yuvo2_height;
	uint32_t yuvo2_format;
	uint32_t yuvo2_stride;
	uint8_t yuvo2_output[AOV_MAX_YUVO2_OUTPUT];
} __aligned(4);

struct ndd_event {
	struct base_event base;

	// for NDD debug mode
	uint32_t imgo_width;
	uint32_t imgo_height;
	uint32_t imgo_format;
	uint32_t imgo_order;
	uint32_t imgo_stride;
	uint8_t imgo_output[AOV_MAX_IMGO_OUTPUT];

	uint32_t aao_size;
	uint8_t aao_output[AOV_MAX_AAO_OUTPUT];

	uint32_t aaho_size;
	uint8_t aaho_output[AOV_MAX_AAHO_OUTPUT];

	uint32_t meta_size;
	uint8_t meta_output[AOV_MAX_META_OUTPUT];

	uint32_t awb_size;
	uint8_t awb_output[AOV_MAX_AWB_OUTPUT];
} __aligned(4);

enum aov_log_id {
	AOV_LOG_ID_BASE,
	AOV_LOG_ID_RED,
	AOV_LOG_ID_AOV,
	AOV_LOG_ID_TLSF,
	AOV_LOG_ID_2A,
	AOV_LOG_ID_TUNING,
	AOV_LOG_ID_SENSOR,
	AOV_LOG_ID_SENIF,
	AOV_LOG_ID_UISP,
	AOV_LOG_ID_AIE,
	AOV_LOG_ID_APU,
	AOV_LOG_ID_MAX
};

struct aov_user {
	uint32_t session;
	uint32_t sensor_id;
	uint32_t sensor_scene;
	int32_t  sensor_orient;
	uint32_t sensor_face;
	uint32_t sensor_type;
	uint32_t sensor_bit;
	uint32_t sensor_ae;
	uint32_t format_order;
	uint32_t main_width;
	uint32_t main_height;
	uint32_t main_format;
	uint32_t sub_width;
	uint32_t sub_height;
	uint32_t sub_format;
	uint32_t frame_rate;
	uint32_t frame_mode;
	uint32_t power_mode;
	uint32_t debug_mode;
	uint32_t debug_level[AOV_LOG_ID_MAX];
	uint32_t trace_perf;
	uint32_t disable_fusion;
	uint32_t debug_drv_clk;
	uint32_t debug_drv_spm;
	uint32_t debug_drv_time;
	uint32_t debug_drv_bypass;
	uint32_t reserved[5];

	uint32_t aaa_size;
	void *aaa_info;

	uint32_t tuning_size;
	void *tuning_info;

	uint32_t pipe_id;
};

struct senif_start {
	uint8_t data[AOV_MAX_SENIF_SIZE];
} __aligned(8);

struct aaa_start {
	uint8_t data[AOV_MAX_AAA_SIZE];
} __aligned(8);

struct tuning {
	uint8_t data[AOV_MAX_TUNING_SIZE];
} __aligned(8);

struct aie_start {
	uint8_t data[AOV_MAX_AIE_SIZE];
} __aligned(8);

struct fld_start {
	uint8_t data[AOV_MAX_FLD_SIZE];
} __aligned(8);

struct aov_start {
	// user parameter
	uint32_t session;
	uint32_t sensor_id;
	uint32_t sensor_scene;
	int32_t  sensor_orient;
	uint32_t sensor_face;
	uint32_t sensor_type;
	uint32_t sensor_bit;
	uint32_t sensor_ae;
	uint32_t format_order;
	uint32_t main_width;
	uint32_t main_height;
	uint32_t main_format;
	uint32_t sub_width;
	uint32_t sub_height;
	uint32_t sub_format;
	uint32_t frame_rate;
	uint32_t frame_mode;
	uint32_t power_mode;
	uint32_t debug_mode;
	uint32_t debug_level[AOV_LOG_ID_MAX];
	uint32_t trace_perf;
	uint32_t disable_fusion;
	uint32_t debug_drv_clk;
	uint32_t debug_drv_spm;
	uint32_t debug_drv_time;
	uint32_t debug_drv_bypass;
	uint32_t reserved[5];

	// display on/off
	uint32_t disp_mode;

	// aie available
	uint32_t aie_avail;

	// seninf/sensor
	struct senif_start senif_info;

	// aaa info
	struct aaa_start aaa_info;

	// tuning data
	struct tuning tuning_info;

	///aie info
	struct aie_start aie_info;

	///fld info
	struct fld_start fld_info;

	// aov event
	union {
		struct base_event base_event[AOV_MAX_BASE_EVENT];
		struct ndd_event ndd_event[AOV_MAX_NDD_EVENT];
	};
};

struct aie_start_v2 {
	uint8_t data[AOV_MAX_AIE_SIZE_V2];
} __aligned(8);

struct fld_start_v2 {
	uint8_t data[AOV_MAX_FLD_SIZE_V2];
} __aligned(8);

struct aov_start_v2 {
	// user parameter
	uint32_t session;
	uint32_t sensor_id;
	uint32_t sensor_scene;
	int32_t  sensor_orient;
	uint32_t sensor_face;
	uint32_t sensor_type;
	uint32_t sensor_bit;
	uint32_t sensor_ae;
	uint32_t format_order;
	uint32_t main_width;
	uint32_t main_height;
	uint32_t main_format;
	uint32_t sub_width;
	uint32_t sub_height;
	uint32_t sub_format;
	uint32_t frame_rate;
	uint32_t frame_mode;
	uint32_t power_mode;
	uint32_t debug_mode;
	uint32_t debug_level[AOV_LOG_ID_MAX];
	uint32_t trace_perf;
	uint32_t disable_fusion;
	uint32_t debug_drv_clk;
	uint32_t debug_drv_spm;
	uint32_t debug_drv_time;
	uint32_t debug_drv_bypass;
	uint32_t reserved[5];

	// display on/off
	uint32_t disp_mode;

	// aie available
	uint32_t aie_avail;

	// seninf/sensor
	struct senif_start senif_info;

	// aaa info
	struct aaa_start aaa_info;

	// tuning data
	struct tuning tuning_info;

	///aie info
	struct aie_start_v2 aie_info;

	///fld info
	struct fld_start_v2 fld_info;

	// aov event
	union {
		struct base_event base_event[AOV_MAX_BASE_EVENT];
		struct ndd_event ndd_event[AOV_MAX_NDD_EVENT];
	};
};

#define AOV_NOTIFY_AIE_AVAIL    (1)
#define AOV_NOTIFY_EVT_AVAIL    (2)

struct aov_notify {
	uint32_t notify;
	uint32_t status;
};

struct packet {
	uint16_t session;
	uint16_t sequence;
	uint16_t command;
	uint16_t auth;
	uint32_t buffer;
	uint32_t length;
} __packed;

#endif  // MTK_AOV_DATA_H
