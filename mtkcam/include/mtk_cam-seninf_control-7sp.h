/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __MTK_CAM_SENINF_CONTROL_7SP_H__
#define __MTK_CAM_SENINF_CONTROL_7SP_H__

#include <linux/videodev2.h>

#define MAX_MUX_DEBUG_RESULT 15

struct mux_debug_result {
	__u8 seninf_mux;
	__u32 seninf_mux_en;
	__u32 seninf_mux_src;
	__u32 seninf_mux_irq;
	__u8 cam_mux;
	__u32 cam_mux_en;
	__u32 cam_mux_src;
	__u32 cam_mux_irq;
	__u32 exp_size;
	__u32 rec_size;
	__u32 frame_mointor_err;
	__u32 vc_feature;
	__u32 vc;
	__u32 dt;
	__u32 v_valid;
	__u32 h_valid;
	__u32 v_blank;
	__u32 h_blank;
	__u64 mipi_pixel_rate;
	__u64 vb_in_us;
	__u64 hb_in_us;
	__u64 line_time_in_us;
};

struct mtk_seninf_debug_result {
	__u8 mux_result_cnt;
	__u8 csi_port;
	__u8 seninf;
	__u8 data_lanes;
	__u8 packet_status_err;
	__u32 csi_mac_irq_status;
	__u32 csi_irq_status;
	__u64 mipi_rate;
	bool is_cphy;
	struct mux_debug_result mux_result[MAX_MUX_DEBUG_RESULT];
};

/* GET */
#define VIDIOC_MTK_G_SENINF_DEBUG_RESULT \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 1, struct mtk_seninf_debug_result)

#define VIDIOC_MTK_G_TSREC_TIMESTAMP \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 2, struct mtk_tsrec_timestamp_by_sensor_id)

/* SET */
#define VIDIOC_MTK_S_UPDATE_ISP_EN \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 50, int)

#define VIDIOC_MTK_S_TEST_STREAM_RAW0_EN \
	_IOWR('M', BASE_VIDIOC_PRIVATE + 51, int)

#endif  // __MTK_CAM_SENINF_CONTROL_7SP_H__
