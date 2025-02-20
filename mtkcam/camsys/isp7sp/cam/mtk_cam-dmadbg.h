/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_DMADBG_H
#define __MTK_CAM_RAW_DMADBG_H

#include "mtk_cam-raw_debug.h"

static __maybe_unused struct dma_debug_item dbg_UFD_R2[] = {
	{0x00002101, "ufd_r2_status"},
	{0x00002201, "ufd_r2_ufdg_crop_cnt"},
	{0x00002301, "ufd_r2_ufdb_crop_cnt"},
	{0x00002401, "ufd_r2_kernel_pixel_number_eq"},
	{0x00002501, "ufd_r2_error"},
	{0x00002601, "ufd_r2_ufdg_kernel_line_cnt"},
	{0x00002701, "ufd_r2_ufdb_kernel_line_cnt"},
	{0x00002801, "ufd_r2_state_checksum"},
	{0x00002901, "ufd_r2_line_pix_cnt_tmp"},
	{0x00002A01, "ufd_r2_line_pix_cnt"},
	{0x00002B01, "ufd_r2_ufod_gr_crc"},
};

static __maybe_unused struct dma_debug_item dbg_UFD_R5[] = {
	{0x00002109, "ufd_r5_status"},
	{0x00002209, "ufd_r5_ufdg_crop_cnt"},
	{0x00002309, "ufd_r5_ufdb_crop_cnt"},
	{0x00002409, "ufd_r5_kernel_pixel_number_eq"},
	{0x00002509, "ufd_r5_error"},
	{0x00002609, "ufd_r5_ufdg_kernel_line_cnt"},
	{0x00002709, "ufd_r5_ufdb_kernel_line_cnt"},
	{0x00002809, "ufd_r5_state_checksum"},
	{0x00002909, "ufd_r5_line_pix_cnt_tmp"},
	{0x00002A09, "ufd_r5_line_pix_cnt"},
	{0x00002B09, "ufd_r5_ufod_gr_crc"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R2[] = {
	{0x00000006, "rawi_r2 32(hex) 0000"},
	{0x00000106, "rawi_r2 state_checksum"},
	{0x00000206, "rawi_r2 line_pix_cnt_tmp"},
	{0x00000306, "rawi_r2 line_pix_cnt"},
	{0x00000506, "rawi_r2 smi_debug_data (case 0)"},
	{0x00010606, "rawi_r2 aff(fifo)_debug_data (case 1)"},
	{0x00030606, "rawi_r2 aff(fifo)_debug_data (case 3)"},
	{0x01000046, "rawi_r2_smi_port / plane-0 / data-crc"},
	{0x01000047, "rawi_r2_smi_port / plane-0 / addr-crc"},
	{0x00000081, "rawi_r2_smi_port / smi_latency_mon output"},
	{0x000000A4, "rawi_r2_smi_port / plane-0 / { len-cnt, dle-cnt }"},
	{0x000004C0, "rawi_r2_smi_port / plane-0 / maddr_max record"},
	{0x000004C1, "rawi_r2_smi_port / plane-0 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R2_UFD[] = {
	{0x00000007, "rawi_r2 ufd 32(hex) 0000"},
	{0x00000107, "rawi_r2 ufd state_checksum"},
	{0x00000207, "rawi_r2 ufd line_pix_cnt_tmp"},
	{0x00000307, "rawi_r2 ufd line_pix_cnt"},
	{0x01000048, "rawi_r2_smi_port / plane-1 / data-crc"},
	{0x000000A5, "rawi_r2_smi_port / plane-1 / { len-cnt, dle-cnt }"},
	{0x000005C0, "rawi_r2_smi_port / plane-1 / maddr_max record"},
	{0x000005C1, "rawi_r2_smi_port / plane-1 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R3[] = {
	{0x00000009, "rawi_r3 32(hex) 0000"},
	{0x00000109, "rawi_r3 state_checksum"},
	{0x00000209, "rawi_r3 line_pix_cnt_tmp"},
	{0x00000309, "rawi_r3 line_pix_cnt"},
	{0x00000509, "rawi_r3 smi_debug_data (case 0)"},
	{0x00010609, "rawi_r3 aff(fifo)_debug_data (case 1)"},
	{0x00030609, "rawi_r3 aff(fifo)_debug_data (case 3)"},
	{0x01000049, "rawi_r3_smi_port / plane-0 / data-crc"},
	{0x0100004A, "rawi_r3_smi_port / plane-0 / addr-crc"},
	{0x00000082, "rawi_r3_smi_port / smi_latency_mon output"},
	{0x000000A6, "rawi_r3_smi_port / plane-0 / { len-cnt, dle-cnt }"},
	{0x000006C0, "rawi_r3_smi_port / plane-0 / maddr_max record"},
	{0x000006C1, "rawi_r3_smi_port / plane-0 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R3_UFD[] = {
	{0x0000000A, "rawi_r3 ufd 32(hex) 0000"},
	{0x0000010A, "rawi_r3 ufd state_checksum"},
	{0x0000020A, "rawi_r3 ufd line_pix_cnt_tmp"},
	{0x0000030A, "rawi_r3 ufd line_pix_cnt"},
	{0x0100004B, "rawi_r3_smi_port / plane-1 / data-crc"},
	{0x000000A7, "rawi_r3_smi_port / plane-1 / { len-cnt, dle-cnt }"},
	{0x000007C0, "rawi_r3_smi_port / plane-1 / maddr_max record"},
	{0x000007C1, "rawi_r3_smi_port / plane-1 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R5[] = {
	{0x0000000C, "rawi_r5 32(hex) 0000"},
	{0x0000010C, "rawi_r5 state_checksum"},
	{0x0000020C, "rawi_r5 line_pix_cnt_tmp"},
	{0x0000030C, "rawi_r5 line_pix_cnt"},
	{0x0000050C, "rawi_r5 smi_debug_data (case 0)"},
	{0x0001060C, "rawi_r5 aff(fifo)_debug_data (case 1)"},
	{0x0003060C, "rawi_r5 aff(fifo)_debug_data (case 3)"},
	{0x0100004C, "rawi_r5_smi_port / plane-0 / data-crc"},
	{0x0100004D, "rawi_r5_smi_port / plane-0 / addr-crc"},
	{0x00000083, "rawi_r5_smi_port / smi_latency_mon output"},
	{0x000000A8, "rawi_r5_smi_port / plane-0 / { len-cnt, dle-cnt }"},
	{0x000008C0, "rawi_r5_smi_port / plane-0 / maddr_max record"},
	{0x000008C1, "rawi_r5_smi_port / plane-0 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RAWI_R5_UFD[] = {
	{0x0000000D, "rawi_r5 ufd 32(hex) 0000"},
	{0x0000010D, "rawi_r5 ufd state_checksum"},
	{0x0000020D, "rawi_r5 ufd line_pix_cnt_tmp"},
	{0x0000030D, "rawi_r5 ufd line_pix_cnt"},
	{0x0100004E, "rawi_r5_smi_port / plane-1 / data-crc"},
	{0x000000A9, "rawi_r5_smi_port / plane-1 / { len-cnt, dle-cnt }"},
	{0x000009C0, "rawi_r5_smi_port / plane-1 / maddr_max record"},
	{0x000009C1, "rawi_r5_smi_port / plane-1 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_IMGO_R1[] = {
	{0x00000017, "imgo_r1 32(hex) 0000"},
	{0x00000117, "imgo_r1 state_checksum"},
	{0x00000217, "imgo_r1 line_pix_cnt_tmp"},
	{0x00000317, "imgo_r1 line_pix_cnt"},
	{0x00000817, "imgo_r1 smi_debug_data (case 0)"},
	{0x00010717, "imgo_r1 aff(fifo)_debug_data (case 1)"},
	{0x00030717, "imgo_r1 aff(fifo)_debug_data (case 3)"},

	{0x01000057, "imgo_r1_smi_port / plane-0 (i.e. imgo_r1) / data-crc"},

	{0x00000087, "imgo_r1_smi_port / smi_latency_mon output"},

	{0x000000AA, "imgo_r1_smi_port / plane-0 / {len-cnt, dle-cnt}"},
	{0x000000AB, "imgo_r1_smi_port / plane-0 / {load_com-cnt, bvalid-cnt}"},

	{0x000012C0, "imgo_r1_smi_port / plane-0 / maddr_max record"},
	{0x000012C1, "imgo_r1_smi_port / plane-0 / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_AAHO_R1[] = {
	{0x0000001D, "32(hex) 0000"},
	{0x0000011D, "state_checksum"},
	{0x0000021D, "line_pix_cnt_tmp"},
	{0x0000031D, "line_pix_cnt"},
	{0x0000041D, "important_status"},
	{0x0000051D, "cmd_data_cnt"},
	{0x0000061D, "cmd_cnt_for_bvalid_phase"},
	{0x0000071D, "input_h_cnt"},
	{0x0000081D, "input_v_cnt"},
	{0x0000091D, "xfer_y_cnt"},

	{0x0100005D, "aao_r1_smi_port_data_crc_p1"},
};

static __maybe_unused struct dma_debug_item dbg_PDO_R1[] = {
	{0x0000001B, "32(hex) 0000"},
	{0x0000011B, "state_checksum"},
	{0x0000021B, "line_pix_cnt_tmp"},
	{0x0000031B, "line_pix_cnt"},
	{0x0000041B, "important_status"},
	{0x0000051B, "cmd_data_cnt"},
	{0x0000061B, "cmd_cnt_for_bvalid_phase"},
	{0x0000071B, "input_h_cnt"},
	{0x0000081B, "input_v_cnt"},
	{0x0000091B, "xfer_y_cnt"},

	{0x0100005B, "ufeo_r1_smi_port_data_crc_p2"},
};

static __maybe_unused struct dma_debug_item dbg_PDI_R1[] = {
	{0x00000015, "pdi_r1 32(hex) 0000"},
	{0x00000115, "pdi_r1 state_checksum"},
	{0x00000215, "pdi_r1 line_pix_cnt_tmp"},
	{0x00000315, "pdi_r1 line_pix_cnt"},
	{0x00000415, "pdi_r1 important_status"},
	{0x00000515, "pdi_r1 cmd_data_cnt"},
	{0x00000615, "pdi_r1 tilex_byte_cnt"},
	{0x00000715, "pdi_r1 tiley_cnt"},
	{0x00000815, "pdi_r1 burst_line_cnt"},
};

// M4U_PORT CAM3_YUVO_R1 : yuvo_r1 + yuvbo_r1
static __maybe_unused struct dma_debug_item dbg_YUVO_R1[] = {
	{0x00000000, "yuvo_r1 32(hex) 0000"},
	{0x00000100, "yuvo_r1 state_checksum"},
	{0x00000200, "yuvo_r1 line_pix_cnt_tmp"},
	{0x00000300, "yuvo_r1 line_pix_cnt"},
	{0x00000800, "yuvo_r1 smi_debug_data (case 0)"},
	{0x00010700, "yuvo_r1 aff(fifo)_debug_data (case 1)"},
	{0x00030700, "yuvo_r1 aff(fifo)_debug_data (case 3)"},

	{0x00000001, "yuvbo_r1 32(hex) 0000"},
	{0x00000101, "yuvbo_r1 state_checksum"},
	{0x00000201, "yuvbo_r1 line_pix_cnt_tmp"},
	{0x00000301, "yuvbo_r1 line_pix_cnt"},
	{0x00000801, "yuvbo_r1 smi_debug_data (case 0)"},
	{0x00010701, "yuvbo_r1 aff(fifo)_debug_data (case 1)"},
	{0x00030701, "yuvbo_r1 aff(fifo)_debug_data (case 3)"},

	{0x01000040, "yuvo_r1_smi_port / plane-0 (i.e. yuvo_r1) / data-crc"},
	{0x01000041, "yuvo_r1_smi_port / plane-1 (i.e. yuvbo_r1) / data-crc"},

	{0x00000080, "yuvo_r1_smi_port / smi_latency_mon output"},

	{0x000000AD, "yuvo_r1_smi_port / plane-0 (i.e. yuvo_r1) / { len-cnt, dle-cnt }"},
	{0x000000AE, "yuvo_r1_smi_port / plane-0 (i.e. yuvo_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000AF, "yuvo_r1_smi_port / plane-1 (i.e. yuvbo_r1) / { len-cnt, dle-cnt }"},
	{0x000000B0, "yuvo_r1_smi_port / plane-1 (i.e. yuvbo_r1) / { load_com-cnt, bvalid-cnt }"},

	{0x000000C0, "yuvo_r1_smi_port / plane-0 - yuvo_r1_smi_port / plane-0 (i.e. yuvo_r1) / maddr_max record"},
	{0x000000C1, "yuvo_r1_smi_port / plane-0 - yuvo_r1_smi_port / plane-0 (i.e. yuvo_r1) / maddr_min record"},
	{0x000001C0, "yuvo_r1_smi_port / plane-1 - yuvo_r1_smi_port / plane-0 (i.e. yuvbo_r1) / maddr_max record"},
	{0x000001C1, "yuvo_r1_smi_port / plane-1 - yuvo_r1_smi_port / plane-0 (i.e. yuvbo_r1) / maddr_min record"},
};

// M4U_PORT CAM3_YUVO_R3 : yuvo_r3 + yuvbo_r3
static __maybe_unused struct dma_debug_item dbg_YUVO_R3[] = {
	{0x00000004, "yuvo_r3 32(hex) 0000"},
	{0x00000104, "yuvo_r3 state_checksum"},
	{0x00000204, "yuvo_r3 line_pix_cnt_tmp"},
	{0x00000304, "yuvo_r3 line_pix_cnt"},
	{0x00000804, "smi_debug_data (case 0)"},
	{0x00010704, "aff(fifo)_debug_data (case 1)"},
	{0x00030704, "aff(fifo)_debug_data (case 3)"},

	{0x00000005, "yuvbo_r3 32(hex) 0000"},
	{0x00000105, "yuvbo_r3 state_checksum"},
	{0x00000205, "yuvbo_r3 line_pix_cnt_tmp"},
	{0x00000305, "yuvbo_r3 line_pix_cnt"},
	{0x00000805, "yuvbo_r3 smi_debug_data (case 0)"},
	{0x00010705, "yuvbo_r3 aff(fifo)_debug_data (case 1)"},
	{0x00030705, "yuvbo_r3 aff(fifo)_debug_data (case 3)"},

	{0x01000042, "yuvo_r3_smi_port / plane-0 (i.e. yuvo_r3) / data-crc"},
	{0x01000043, "yuvo_r3_smi_port / plane-1 (i.e. yuvbo_r1) / data-crc"},

	{0x00000081, "yuvo_r3_smi_port / smi_latency_mon output"},

	{0x000000B1, "yuvo_r3_smi_port / plane-0 (i.e. yuvo_r3) / { len-cnt, dle-cnt }"},
	{0x000000B2, "yuvo_r3_smi_port / plane-0 (i.e. yuvo_r3) / { load_com-cnt, bvalid-cnt }"},
	{0x000000B3, "yuvo_r3_smi_port / plane-1 (i.e. yuvbo_r3) / { len-cnt, dle-cnt }"},
	{0x000000B4, "yuvo_r3_smi_port / plane-1 (i.e. yuvbo_r3) / { load_com-cnt, bvalid-cnt }"},

	{0x000002C0, "yuvo_r3_smi_port / plane-0 - yuvo_r3_smi_port / plane-0 (i.e. yuvo_r3) / maddr_max record"},
	{0x000002C1, "yuvo_r3_smi_port / plane-0 - yuvo_r3_smi_port / plane-0 (i.e. yuvo_r3) / maddr_min record"},
	{0x000003C0, "yuvo_r3_smi_port / plane-1 - yuvo_r3_smi_port / plane-0 (i.e. yuvbo_r3) / maddr_max record"},
	{0x000003C1, "yuvo_r3_smi_port / plane-1 - yuvo_r3_smi_port / plane-0 (i.e. yuvbo_r3) / maddr_min record"},
};

// M4U_PORT CAM3_YUVCO_R1 : yuvco_r1 + yuvdo_r1 + yuvco_r3 + yuvdo_r3
static __maybe_unused struct dma_debug_item dbg_YUVCO_R1[] = {
	{0x00000002, "yuvco_r1 32(hex) 0000"},
	{0x00000102, "yuvco_r1 state_checksum"},
	{0x00000202, "yuvco_r1 line_pix_cnt_tmp"},
	{0x00000302, "yuvco_r1 line_pix_cnt"},
	{0x00000802, "yuvco_r1 smi_debug_data (case 0)"},
	{0x00010702, "yuvco_r1 aff(fifo)_debug_data (case 1)"},
	{0x00030702, "yuvco_r1 aff(fifo)_debug_data (case 3)"},

	{0x00000003, "yuvdo_r1 32(hex) 0000"},
	{0x00000103, "yuvdo_r1 state_checksum"},
	{0x00000203, "yuvdo_r1 line_pix_cnt_tmp"},
	{0x00000303, "yuvdo_r1 line_pix_cnt"},
	{0x00000803, "yuvdo_r1 smi_debug_data (case 0)"},
	{0x00010703, "yuvdo_r1 aff(fifo)_debug_data (case 1)"},
	{0x00030703, "yuvdo_r1 aff(fifo)_debug_data (case 3)"},

	{0x01000044, "yuvco_r1_smi_port / plane-0 (i.e. yuvco_r1) / data-crc"},
	{0x01000045, "yuvco_r1_smi_port / plane-1 (i.e. yuvco_r1) / data-crc"},
	{0x01000046, "yuvco_r1_smi_port / plane-2 (i.e. yuvco_r1) / data-crc"},
	{0x01000047, "yuvco_r1_smi_port / plane-3 (i.e. yuvco_r1) / data-crc"},

	{0x00000082, "yuvco_r1_smi_port / smi_latency_mon output"},

	{0x000004C0, "yuvco_r1_smi_port / plane-0 - yuvco_r1_smi_port / plane-0 (i.e. yuvco_r1) / maddr_max record"},
	{0x000004C1, "yuvco_r1_smi_port / plane-0 - yuvco_r1_smi_port / plane-0 (i.e. yuvco_r1) / maddr_min record"},
	{0x000005C0, "yuvco_r1_smi_port / plane-1 - yuvco_r1_smi_port / plane-0 (i.e. yuvdo_r1) / maddr_max record"},
	{0x000005C1, "yuvco_r1_smi_port / plane-1 - yuvco_r1_smi_port / plane-0 (i.e. yuvdo_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_YUVCO_R3[] = {
	{0x00000006, "yuvco_r3 32(hex) 0000"},
	{0x00000106, "yuvco_r3 state_checksum"},
	{0x00000206, "yuvco_r3 line_pix_cnt_tmp"},
	{0x00000306, "yuvco_r3 line_pix_cnt"},
	{0x00000806, "yuvco_r3 smi_debug_data (case 0)"},
	{0x00010706, "yuvco_r3 aff(fifo)_debug_data (case 1)"},
	{0x00030706, "yuvco_r3 aff(fifo)_debug_data (case 3)"},

	{0x00000007, "yuvdo_r3 32(hex) 0000"},
	{0x00000107, "yuvdo_r3 state_checksum"},
	{0x00000207, "yuvdo_r3 line_pix_cnt_tmp"},
	{0x00000307, "yuvdo_r3 line_pix_cnt"},
	{0x00000807, "yuvdo_r3 smi_debug_data (case 0)"},
	{0x00010707, "yuvdo_r3 aff(fifo)_debug_data (case 1)"},
	{0x00030707, "yuvdo_r3 aff(fifo)_debug_data (case 3)"},

	{0x01000044, "yuvco_r1_smi_port / plane-0 (i.e. yuvco_r1) / data-crc"},
	{0x01000045, "yuvco_r1_smi_port / plane-1 (i.e. yuvco_r1) / data-crc"},
	{0x01000046, "yuvco_r1_smi_port / plane-2 (i.e. yuvco_r1) / data-crc"},
	{0x01000047, "yuvco_r1_smi_port / plane-3 (i.e. yuvco_r1) / data-crc"},

	{0x00000082, "yuvco_r1_smi_port / smi_latency_mon output"},

	{0x000006C0, "yuvco_r1_smi_port / plane-2 - yuvco_r1_smi_port / plane-0 (i.e. yuvco_r3) / maddr_max record"},
	{0x000006C1, "yuvco_r1_smi_port / plane-2 - yuvco_r1_smi_port / plane-0 (i.e. yuvco_r3) / maddr_max record"},
	{0x000007C0, "yuvco_r1_smi_port / plane-3 - yuvco_r1_smi_port / plane-0 (i.e. yuvdo_r3) / maddr_max record"},
	{0x000007C1, "yuvco_r1_smi_port / plane-3 - yuvco_r1_smi_port / plane-0 (i.e. yuvdo_r3) / maddr_min record"},
};

// M4U_PORT CAM3_YUVO_R2 : yuvo_r2 + yuvbo_r2 + yuvo_r4 + yuvbo_r4 + yuvo_r5 + yuvbo_r5
static __maybe_unused struct dma_debug_item dbg_YUVO_R2[] = {
	{0x00000008, "yuvo_r2 32(hex) 0000"},
	{0x00000108, "yuvo_r2 state_checksum"},
	{0x00000208, "yuvo_r2 line_pix_cnt_tmp"},
	{0x00000308, "yuvo_r2 line_pix_cnt"},
	{0x00000408, "yuvo_r2 important_status"},
	{0x00000508, "yuvo_r2 cmd_data_cnt"},
	{0x00000608, "yuvo_r2 cmd_cnt_for_bvalid_phase"},
	{0x00000708, "yuvo_r2 input_h_cnt"},
	{0x00000808, "yuvo_r2 input_v_cnt"},
	{0x00000908, "yuvo_r2 xfer_y_cnt"},

	{0x00000009, "yuvbo_r2 32(hex) 0000"},
	{0x00000109, "yuvbo_r2 state_checksum"},
	{0x00000209, "yuvbo_r2 line_pix_cnt_tmp"},
	{0x00000309, "yuvbo_r2 line_pix_cnt"},
	{0x00000409, "yuvbo_r2 important_status"},
	{0x00000509, "yuvbo_r2 cmd_data_cnt"},
	{0x00000609, "yuvbo_r2 cmd_cnt_for_bvalid_phase"},
	{0x00000709, "yuvbo_r2 input_h_cnt"},
	{0x00000809, "yuvbo_r2 input_v_cnt"},
	{0x00000909, "yuvbo_r2 xfer_y_cnt"},

	{0x01000048, "yuvo_r2_smi_port / plane-0 (i.e. yuvo_r2) / data-crc"},
	{0x01000049, "yuvo_r2_smi_port / plane-1 (i.e. yuvbo_r2) / data-crc"},

	{0x00000083, "yuvo_r2_smi_port / smi_latency_mon output"},

	{0x000008C0, "yuvo_r2_smi_port / plane-0 - yuvo_r2_smi_port / plane-1 (i.e. cqi_r3) / maddr_max record"},
	{0x000008C1, "yuvo_r2_smi_port / plane-0 - yuvo_r2_smi_port / plane-1 (i.e. cqi_r3) / maddr_min record"},
	{0x000009C0, "yuvo_r2_smi_port / plane-1 - yuvo_r2_smi_port / plane-1 (i.e. cqi_r4) / maddr_max record"},
	{0x000009C1, "yuvo_r2_smi_port / plane-1 - yuvo_r2_smi_port / plane-1 (i.e. cqi_r4) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_YUVO_R4[] = {
	{0x0000000A, "yuvo_r4 32(hex) 0000"},
	{0x0000010A, "yuvo_r4 state_checksum"},
	{0x0000020A, "yuvo_r4 line_pix_cnt_tmp"},
	{0x0000030A, "yuvo_r4 line_pix_cnt"},
	{0x0000040A, "yuvo_r4 important_status"},
	{0x0000050A, "yuvo_r4 cmd_data_cnt"},
	{0x0000060A, "yuvo_r4 cmd_cnt_for_bvalid_phase"},
	{0x0000070A, "yuvo_r4 input_h_cnt"},
	{0x0000080A, "yuvo_r4 input_v_cnt"},
	{0x0000090A, "yuvo_r4 xfer_y_cnt"},

	{0x0000000B, "yuvbo_r4 32(hex) 0000"},
	{0x0000010B, "yuvbo_r4 state_checksum"},
	{0x0000020B, "yuvbo_r4 line_pix_cnt_tmp"},
	{0x0000030B, "yuvbo_r4 line_pix_cnt"},
	{0x0000040B, "yuvbo_r4 important_status"},
	{0x0000050B, "yuvbo_r4 cmd_data_cnt"},
	{0x0000060B, "yuvbo_r4 cmd_cnt_for_bvalid_phase"},
	{0x0000070B, "yuvbo_r4 input_h_cnt"},
	{0x0000080B, "yuvbo_r4 input_v_cnt"},
	{0x0000090B, "yuvbo_r4 xfer_y_cnt"},

	{0x0100004A, "yuvo_r2_smi_port / plane-2 (i.e. yuvo_r4) / data-crc"},
	{0x0100004B, "yuvo_r2_smi_port / plane-3 (i.e. yuvbo_r4) / data-crc"},

	{0x00000083, "yuvo_r2_smi_port / smi_latency_mon output"},

	{0x00000AC0, "yuvo_r2_smi_port / plane-2 - yuvo_r2_smi_port / plane-0 (i.e. lsci_r1) / maddr_max record"},
	{0x00000AC1, "yuvo_r2_smi_port / plane-2 - yuvo_r2_smi_port / plane-0 (i.e. lsci_r1) / maddr_min record"},
	{0x00000BC0, "yuvo_r2_smi_port / plane-3 - yuvo_r2_smi_port / plane-0 (i.e. bpci_r1) / maddr_max record"},
	{0x00000BC1, "yuvo_r2_smi_port / plane-3 - yuvo_r2_smi_port / plane-0 (i.e. bpci_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_YUVO_R5[] = {
	{0x0000000C, "yuvo_r5 32(hex) 0000"},
	{0x0000010C, "yuvo_r5 state_checksum"},
	{0x0000020C, "yuvo_r5 line_pix_cnt_tmp"},
	{0x0000030C, "yuvo_r5 line_pix_cnt"},
	{0x0000040C, "yuvo_r5 important_status"},
	{0x0000050C, "yuvo_r5 cmd_data_cnt"},
	{0x0000060C, "yuvo_r5 cmd_cnt_for_bvalid_phase"},
	{0x0000070C, "yuvo_r5 input_h_cnt"},
	{0x0000080C, "yuvo_r5 input_v_cnt"},
	{0x0000090C, "yuvo_r5 xfer_y_cnt"},

	{0x0000000D, "yuvbo_r5 32(hex) 0000"},
	{0x0000010D, "yuvbo_r5 state_checksum"},
	{0x0000020D, "yuvbo_r5 line_pix_cnt_tmp"},
	{0x0000030D, "yuvbo_r5 line_pix_cnt"},
	{0x0000040D, "yuvbo_r5 important_status"},
	{0x0000050D, "yuvbo_r5 cmd_data_cnt"},
	{0x0000060D, "yuvbo_r5 cmd_cnt_for_bvalid_phase"},
	{0x0000070D, "yuvbo_r5 input_h_cnt"},
	{0x0000080D, "yuvbo_r5 input_v_cnt"},
	{0x0000090D, "yuvbo_r5 xfer_y_cnt"},

	{0x0100004C, "yuvo_r2_smi_port / plane-4 (i.e. yuvo_r5) / data-crc"},
	{0x0100004D, "yuvo_r2_smi_port / plane-5 (i.e. yuvbo_r5) / data-crc"},

	{0x00000083, "yuvo_r2_smi_port / smi_latency_mon output"},

	{0x00000CC0, "yuvo_r2_smi_port / plane-4 - yuvo_r2_smi_port / plane-1 (i.e. bpci_r2) / maddr_max record"},
	{0x00000CC1, "yuvo_r2_smi_port / plane-4 - yuvo_r2_smi_port / plane-1 (i.e. bpci_r2) / maddr_min record"},
	{0x00000DC0, "yuvo_r2_smi_port / plane-5 - yuvo_r2_smi_port / plane-2 (i.e. bpci_r3) / maddr_max record"},
	{0x00000DC1, "yuvo_r2_smi_port / plane-5 - yuvo_r2_smi_port / plane-2 (i.e. bpci_r3) / maddr_min record"},
};

// M4U_PORT CAM3_RZH1N2TO_R1 : rzh1n2to_r1 + rzh1n2tbo_r1 + rzh1n2to_r2 + rzh1n2to_r3 + rzh1n2tbo_r3
static __maybe_unused struct dma_debug_item dbg_RZH1N2TO_R1[] = {
	{0x0000000E, "rzh1n2to_r1 32(hex) 0000"},
	{0x0000010E, "rzh1n2to_r1 state_checksum"},
	{0x0000020E, "rzh1n2to_r1 line_pix_cnt_tmp"},
	{0x0000030E, "rzh1n2to_r1 line_pix_cnt"},
	{0x0000040E, "rzh1n2to_r1 important_status"},
	{0x0000050E, "rzh1n2to_r1 cmd_data_cnt"},
	{0x0000060E, "rzh1n2to_r1 cmd_cnt_for_bvalid_phase"},
	{0x0000070E, "rzh1n2to_r1 input_h_cnt"},
	{0x0000080E, "rzh1n2to_r1 input_v_cnt"},
	{0x0000090E, "rzh1n2to_r1 xfer_y_cnt"},

	{0x0000000F, "rzh1n2tbo_r1 32(hex) 0000"},
	{0x0000010F, "rzh1n2tbo_r1 state_checksum"},
	{0x0000020F, "rzh1n2tbo_r1 line_pix_cnt_tmp"},
	{0x0000030F, "rzh1n2tbo_r1 line_pix_cnt"},
	{0x0000040F, "rzh1n2tbo_r1 important_status"},
	{0x0000050F, "rzh1n2tbo_r1 cmd_data_cnt"},
	{0x0000060F, "rzh1n2tbo_r1 cmd_cnt_for_bvalid_phase"},
	{0x0000070F, "rzh1n2tbo_r1 input_h_cnt"},
	{0x0000080F, "rzh1n2tbo_r1 input_v_cnt"},
	{0x0000090F, "rzh1n2tbo_r1 xfer_y_cnt"},

	{0x00000084, "rzh1n2to_r1_smi_port / smi_latency_mon output"},

	{0x00000EC0, "rzh1n2to_r1_smi_port / plane-0 - rzh1n2to_r1_smi_port / plane-3 (i.e. rzh1n2to_r1) / maddr_max record"},
	{0x00000EC1, "rzh1n2to_r1_smi_port / plane-0 - rzh1n2to_r1_smi_port / plane-3 (i.e. rzh1n2to_r1) / maddr_min record"},
	{0x00000FC0, "rzh1n2to_r1_smi_port / plane-1 - rzh1n2to_r1_smi_port / plane-4 (i.e. rzh1n2tbo_r1) / maddr_max record"},
	{0x00000FC1, "rzh1n2to_r1_smi_port / plane-1 - rzh1n2to_r1_smi_port / plane-4 (i.e. rzh1n2tbo_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RZH1N2TO_R2[] = {
	{0x00000010, "rzh1n2to_r2 32(hex) 0000"},
	{0x00000110, "rzh1n2to_r2 state_checksum"},
	{0x00000210, "rzh1n2to_r2 line_pix_cnt_tmp"},
	{0x00000310, "rzh1n2to_r2 line_pix_cnt"},
	{0x00000410, "rzh1n2to_r2 important_status"},
	{0x00000510, "rzh1n2to_r2 cmd_data_cnt"},
	{0x00000610, "rzh1n2to_r2 cmd_cnt_for_bvalid_phase"},
	{0x00000710, "rzh1n2to_r2 input_h_cnt"},
	{0x00000810, "rzh1n2to_r2 input_v_cnt"},
	{0x00000910, "rzh1n2to_r2 xfer_y_cnt"},

	{0x01000050, "rzh1n2to_r1_smi_port / plane-2 (i.e. rzh1n2to_r2) / data-crc"},

	{0x00000084, "rzh1n2to_r1_smi_port / smi_latency_mon output"},

	{0x000010C0, "rzh1n2to_r1_smi_port / plane-2 - rzh1n2to_r1_smi_port / plane-0 (i.e. rzh1n2to_r2) / maddr_max record"},
	{0x000010C1, "rzh1n2to_r1_smi_port / plane-2 - rzh1n2to_r1_smi_port / plane-0 (i.e. rzh1n2to_r2) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_RZH1N2TO_R3[] = {
	{0x00000011, "rzh1n2to_r3 32(hex) 0000"},
	{0x00000111, "rzh1n2to_r3 state_checksum"},
	{0x00000211, "rzh1n2to_r3 line_pix_cnt_tmp"},
	{0x00000311, "rzh1n2to_r3 line_pix_cnt"},
	{0x00000411, "rzh1n2to_r3 important_status"},
	{0x00000511, "rzh1n2to_r3 cmd_data_cnt"},
	{0x00000611, "rzh1n2to_r3 cmd_cnt_for_bvalid_phase"},
	{0x00000711, "rzh1n2to_r3 input_h_cnt"},
	{0x00000811, "rzh1n2to_r3 input_v_cnt"},
	{0x00000911, "rzh1n2to_r3 xfer_y_cnt"},

	{0x00000012, "rzh1n2tbo_r3 32(hex) 0000"},
	{0x00000112, "rzh1n2tbo_r3 state_checksum"},
	{0x00000212, "rzh1n2tbo_r3 line_pix_cnt_tmp"},
	{0x00000312, "rzh1n2tbo_r3 line_pix_cnt"},
	{0x00000412, "rzh1n2tbo_r3 important_status"},
	{0x00000512, "rzh1n2tbo_r3 cmd_data_cnt"},
	{0x00000612, "rzh1n2tbo_r3 cmd_cnt_for_bvalid_phase"},
	{0x00000712, "rzh1n2tbo_r3 input_h_cnt"},
	{0x00000812, "rzh1n2tbo_r3 input_v_cnt"},
	{0x00000912, "rzh1n2tbo_r3 xfer_y_cnt"},

	{0x01000051, "rzh1n2to_r1_smi_port / plane-3 (i.e. rzh1n2to_r3) / data-crc"},
	{0x01000052, "rzh1n2to_r1_smi_port / plane-4 (i.e. rzh1n2tbo_r3) / data-crc"},

	{0x00000084, "rzh1n2to_r1_smi_port / smi_latency_mon output"},

	{0x000011C0, "rzh1n2to_r1_smi_port / plane-3 - rzh1n2to_r1_smi_port / plane-1 (i.e. rzh1n2to_r3) / maddr_max record"},
	{0x000011C1, "rzh1n2to_r1_smi_port / plane-3 - rzh1n2to_r1_smi_port / plane-1 (i.e. rzh1n2to_r3) / maddr_min record"},
	{0x000012C0, "rzh1n2to_r1_smi_port / plane-4 - rzh1n2to_r1_smi_port / plane-2 (i.e. rzh1n2tbo_r3) / maddr_max record"},
	{0x000012C1, "rzh1n2to_r1_smi_port / plane-4 - rzh1n2to_r1_smi_port / plane-2 (i.e. rzh1n2tbo_r3) / maddr_min record"},
};

// M4U_PORT CAM3_DRZS4NO_R1 : drzs4no_r1 + drzs4no_r2 + drzs4no_r3 + lmvo_r1 + actso_r1
static __maybe_unused struct dma_debug_item dbg_DRZS4NO_R1[] = {
	{0x00000013, "drzs4no_r1 32(hex) 0000"},
	{0x00000113, "drzs4no_r1 state_checksum"},
	{0x00000213, "drzs4no_r1 line_pix_cnt_tmp"},
	{0x00000313, "drzs4no_r1 line_pix_cnt"},
	{0x00000413, "drzs4no_r1 important_status"},
	{0x00000513, "drzs4no_r1 cmd_data_cnt"},
	{0x00000613, "drzs4no_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000713, "drzs4no_r1 input_h_cnt"},
	{0x00000813, "drzs4no_r1 input_v_cnt"},
	{0x00000913, "drzs4no_r1 xfer_y_cnt"},

	{0x01000053, "drzs4no_r1_smi_port / plane-0 (i.e. drzs4no_r1) / data-crc"},

	{0x00000085, "drzs4no_r1_smi_port / smi_latency_mon output"},

	{0x000013C0, "drzs4no_r1_smi_port / plane-0 - drzs4no_r1_smi_port / plane-0 (i.e. drzs4no_r1) / maddr_max record"},
	{0x000013C1, "drzs4no_r1_smi_port / plane-0 - drzs4no_r1_smi_port / plane-0 (i.e. drzs4no_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_DRZS4NO_R2[] = {
	{0x00000014, "drzs4no_r2 32(hex) 0000"},
	{0x00000114, "drzs4no_r2 state_checksum"},
	{0x00000214, "drzs4no_r2 line_pix_cnt_tmp"},
	{0x00000314, "drzs4no_r2 line_pix_cnt"},
	{0x00000414, "drzs4no_r2 important_status"},
	{0x00000514, "drzs4no_r2 cmd_data_cnt"},
	{0x00000614, "drzs4no_r2 cmd_cnt_for_bvalid_phase"},
	{0x00000714, "drzs4no_r2 input_h_cnt"},
	{0x00000814, "drzs4no_r2 input_v_cnt"},
	{0x00000914, "drzs4no_r2 xfer_y_cnt"},

	{0x01000054, "drzs4no_r1_smi_port / plane-1 (i.e. drzs4no_r2) / data-crc"},

	{0x00000085, "drzs4no_r1_smi_port / smi_latency_mon output"},

	{0x000014C0, "drzs4no_r1_smi_port / plane-1 - drzs4no_r1_smi_port / plane-0 (i.e. drzs4no_r2) / maddr_max record"},
	{0x000014C1, "drzs4no_r1_smi_port / plane-1 - drzs4no_r1_smi_port / plane-0 (i.e. drzs4no_r2) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_DRZS4NO_R3[] = {
	{0x00000015, "drzs4no_r3 32(hex) 0000"},
	{0x00000115, "drzs4no_r3 state_checksum"},
	{0x00000215, "drzs4no_r3 line_pix_cnt_tmp"},
	{0x00000315, "drzs4no_r3 line_pix_cnt"},
	{0x00000415, "drzs4no_r3 important_status"},
	{0x00000515, "drzs4no_r3 cmd_data_cnt"},
	{0x00000615, "drzs4no_r3 cmd_cnt_for_bvalid_phase"},
	{0x00000715, "drzs4no_r3 input_h_cnt"},
	{0x00000815, "drzs4no_r3 input_v_cnt"},
	{0x00000915, "drzs4no_r3 xfer_y_cnt"},

	{0x01000055, "drzs4no_r1_smi_port / plane-2 (i.e. drzs4no_r3) / data-crc"},

	{0x00000085, "drzs4no_r1_smi_port / smi_latency_mon output"},

	{0x000015C0, "drzs4no_r1_smi_port / plane-2 - drzs4no_r1_smi_port / plane-1 (i.e. drzs4no_r3) / maddr_max record"},
	{0x000015C1, "drzs4no_r1_smi_port / plane-2 - drzs4no_r1_smi_port / plane-1 (i.e. drzs4no_r3) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_LMVO_R1[] = {
	{0x00000016, "lmvo_r1 32(hex) 0000"},
	{0x00000116, "lmvo_r1 state_checksum"},
	{0x00000216, "lmvo_r1 line_pix_cnt_tmp"},
	{0x00000316, "lmvo_r1 line_pix_cnt"},
	{0x00000416, "lmvo_r1 important_status"},
	{0x00000516, "lmvo_r1 cmd_data_cnt"},
	{0x00000616, "lmvo_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000716, "lmvo_r1 input_h_cnt"},
	{0x00000816, "lmvo_r1 input_v_cnt"},
	{0x00000916, "lmvo_r1 xfer_y_cnt"},

	{0x01000056, "drzs4no_r1_smi_port / plane-3 (i.e. lmvo_r1) / data-crc"},

	{0x00000085, "drzs4no_r1_smi_port / smi_latency_mon output"},

	{0x000016C0, "drzs4no_r1_smi_port / plane-3 - drzs4no_r1_smi_port / plane-2 (i.e. lmvo_r1) / maddr_max record"},
	{0x000016C1, "drzs4no_r1_smi_port / plane-3 - drzs4no_r1_smi_port / plane-2 (i.e. lmvo_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_ACTSO_R1[] = {
	{0x00000017, "actso_r1 32(hex) 0000"},
	{0x00000117, "actso_r1 state_checksum"},
	{0x00000217, "actso_r1 line_pix_cnt_tmp"},
	{0x00000317, "actso_r1 line_pix_cnt"},
	{0x00000417, "actso_r1 important_status"},
	{0x00000517, "actso_r1 cmd_data_cnt"},
	{0x00000617, "actso_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000717, "actso_r1 input_h_cnt"},
	{0x00000817, "actso_r1 input_v_cnt"},
	{0x00000917, "actso_r1 xfer_y_cnt"},

	{0x01000057, "drzs4no_r1_smi_port / plane-4 (i.e. actso_r1) / data-crc"},

	{0x00000085, "drzs4no_r1_smi_port / smi_latency_mon output"},

	{0x000017C0, "drzs4no_r1_smi_port / plane-4 - drzs4no_r1_smi_port / plane-0 (i.e. actsoo_r1) / maddr_max record"},
	{0x000017C1, "drzs4no_r1_smi_port / plane-4 - drzs4no_r1_smi_port / plane-0 (i.e. actsoo_r1) / maddr_min record"},
};

// M4U_PORT CAM3_TNCSO_R1 : tncso_r1 + tncsbo_r1 + tncsho_r1 + tncsyo_r1
static __maybe_unused struct dma_debug_item dbg_TNCSO_R1[] = {
	{0x00000018, "tncso_r1 32(hex) 0000"},
	{0x00000118, "tncso_r1 state_checksum"},
	{0x00000218, "tncso_r1 line_pix_cnt_tmp"},
	{0x00000318, "tncso_r1 line_pix_cnt"},
	{0x00000418, "tncso_r1 important_status"},
	{0x00000518, "tncso_r1 cmd_data_cnt"},
	{0x00000618, "tncso_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000718, "tncso_r1 input_h_cnt"},
	{0x00000818, "tncso_r1 input_v_cnt"},
	{0x00000918, "tncso_r1 xfer_y_cnt"},

	{0x01000058, "tncso_r1_smi_port / plane-0 (i.e. tncso_r1) / data-crc"},

	{0x00000086, "tncso_r1_smi_port / smi_latency_mon output"},

	{0x000018C0, "tncso_r1_smi_port / plane-0 - tncso_r1_smi_port / plane-1 (i.e. tncso_r1) / maddr_max record"},
	{0x000018C1, "tncso_r1_smi_port / plane-0 - tncso_r1_smi_port / plane-1 (i.e. tncso_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_TNCSBO_R1[] = {
	{0x00000019, "tncsbo_r1 32(hex) 0000"},
	{0x00000119, "tncsbo_r1 state_checksum"},
	{0x00000219, "tncsbo_r1 line_pix_cnt_tmp"},
	{0x00000319, "tncsbo_r1 line_pix_cnt"},
	{0x00000419, "tncsbo_r1 important_status"},
	{0x00000519, "tncsbo_r1 cmd_data_cnt"},
	{0x00000619, "tncsbo_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000719, "tncsbo_r1 input_h_cnt"},
	{0x00000819, "tncsbo_r1 input_v_cnt"},
	{0x00000919, "tncsbo_r1 xfer_y_cnt"},

	{0x01000059, "tncso_r1_smi_port / plane-1 (i.e. tncsbo_r1) / data-crc"},

	{0x00000086, "tncso_r1_smi_port / smi_latency_mon output"},

	{0x000019C0, "tncso_r1_smi_port / plane-1 - tncso_r1_smi_port / plane-0 (i.e. tncsbo_r1) / maddr_max record"},
	{0x000019C1, "tncso_r1_smi_port / plane-1 - tncso_r1_smi_port / plane-0 (i.e. tncsbo_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_TNCSHO_R1[] = {
	{0x0000001A, "tncsho_r1 32(hex) 0000"},
	{0x0000011A, "tncsho_r1 state_checksum"},
	{0x0000021A, "tncsho_r1 line_pix_cnt_tmp"},
	{0x0000031A, "tncsho_r1 line_pix_cnt"},
	{0x0000041A, "tncsho_r1 important_status"},
	{0x0000051A, "tncsho_r1 cmd_data_cnt"},
	{0x0000061A, "tncsho_r1 cmd_cnt_for_bvalid_phase"},
	{0x0000071A, "tncsho_r1 input_h_cnt"},
	{0x0000081A, "tncsho_r1 input_v_cnt"},
	{0x0000091A, "tncsho_r1 xfer_y_cnt"},

	{0x0100005A, "tncso_r1_smi_port / plane-2 (i.e. tncsho_r1) / data-crc"},

	{0x00000086, "tncso_r1_smi_port / smi_latency_mon output"},

	{0x00001AC0, "tncso_r1_smi_port / plane-2 - tncso_r1_smi_port / plane-1 (i.e. tncsho_r1) / maddr_max record"},
	{0x00001AC1, "tncso_r1_smi_port / plane-2 - tncso_r1_smi_port / plane-1 (i.e. tncsho_r1) / maddr_max record"},
};

static __maybe_unused struct dma_debug_item dbg_TNCSYO_R1[] = {
	{0x0000001B, "tncsyo_r1 32(hex) 0000"},
	{0x0000011B, "tncsyo_r1 state_checksum"},
	{0x0000021B, "tncsyo_r1 line_pix_cnt_tmp"},
	{0x0000031B, "tncsyo_r1 line_pix_cnt"},
	{0x0000041B, "tncsyo_r1 important_status"},
	{0x0000051B, "tncsyo_r1 cmd_data_cnt"},
	{0x0000061B, "tncsyo_r1 cmd_cnt_for_bvalid_phase"},
	{0x0000071B, "tncsyo_r1 input_h_cnt"},
	{0x0000081B, "tncsyo_r1 input_v_cnt"},
	{0x0000091B, "tncsyo_r1 xfer_y_cnt"},

	{0x0100005B, "tncso_r1_smi_port / plane-3 (i.e. tncsyo_r1) / data-crc"},

	{0x00000086, "tncso_r1_smi_port / smi_latency_mon output"},

	{0x00001BC0, "tncso_r1_smi_port / plane-3 - tncso_r1_smi_port / plane-2 (i.e. tncsyo_r1) / maddr_max record"},
	{0x00001BC0, "tncso_r1_smi_port / plane-3 - tncso_r1_smi_port / plane-2 (i.e. tncsyo_r1) / maddr_min record"},
};

static __maybe_unused struct dma_debug_item dbg_ulc_cmd_cnt[] = {
	{0x00000511, "bpci_r1 cmd_data_cnt"},
	{0x00000512, "bpci_r2 cmd_data_cnt"},
	{0x00000513, "bpci_r3 cmd_data_cnt"},
	{0x00000516, "aai_r1 cmd_data_cnt"},
	{0x00000518, "rawi_r6 cmd_data_cnt"},
	{0x0000051F, "tsfso_r1 cmd_data_cnt"},
	{0x0000061F, "tsfso_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000520, "ltmso_r1 cmd_data_cnt"},
	{0x00000620, "ltmso_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000521, "tsfso_r2 cmd_data_cnt"},
	{0x00000621, "tsfso_r2 cmd_cnt_for_bvalid_phase"},
	{0x00000522, "flko_r1 cmd_data_cnt"},
	{0x00000622, "flko_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000523, "ufeo_r1 cmd_data_cnt"},
	{0x00000623, "ufeo_r1 cmd_cnt_for_bvalid_phase"},
	{0x00000528, "bino_r1 cmd_data_cnt"},
	{0x00000628, "bino_r1 cmd_cnt_for_bvalid_phase"},
};

static __maybe_unused struct dma_debug_item dbg_ori_cmd_cnt[] = {
	{0x000000A0, "rawi_r2_smi_port / plane-0 (i.e. rawi_r2) / { len-cnt, dle-cnt }"},
	{0x000000A1, "ufdi_r2_smi_port / plane-0 (i.e. ufdi_r2) / { len-cnt, dle-cnt }"},
	{0x000000A2, "rawi_r3_smi_port / plane-0 (i.e. rawi_r3) / { len-cnt, dle-cnt }"},
	{0x000000A3, "ufdi_r3_smi_port / plane-0 (i.e. ufdi_r3) / { len-cnt, dle-cnt }"},
	{0x000000A4, "rawi_r4_smi_port / plane-0 (i.e. rawi_r4) / { len-cnt, dle-cnt }"},
	{0x000000A5, "rawi_r5_smi_port / plane-0 (i.e. rawi_r5) / { len-cnt, dle-cnt }"},
	{0x000000A6, "cqi_r1_smi_port / plane-0 (i.e. cqi_r1) / { len-cnt, dle-cnt }"},
	{0x000000A7, "cqi_r1_smi_port / plane-1 (i.e. cqi_r3) / { len-cnt, dle-cnt }"},
	{0x000000A8, "cqi_r2_smi_port / plane-0 (i.e. cqi_r2) / { len-cnt, dle-cnt }"},
	{0x000000A9, "cqi_r2_smi_port / plane-1 (i.e. cqi_r4) / { len-cnt, dle-cnt }"},
	{0x000000AA, "lsci_r1_smi_port / plane-0 (i.e. lsci_r1) / { len-cnt, dle-cnt }"},
	{0x000000AB, "imgo_r1_smi_port / plane-0 (i.e. imgo_r1) / { len-cnt, dle-cnt }"},
	{0x000000AC, "imgo_r1_smi_port / plane-0 (i.e. imgo_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000AD, "fho_r1_smi_port / plane-0 (i.e. fho_r1) / { len-cnt, dle-cnt }"},
	{0x000000AE, "fho_r1_smi_port / plane-0 (i.e. fho_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000AF, "fho_r1_smi_port / plane-1 (i.e. aaho_r1) / { len-cnt, dle-cnt }"},
	{0x000000B0, "fho_r1_smi_port / plane-1 (i.e. aaho_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000B1, "fho_r1_smi_port / plane-2 (i.e. pdo_r1) / { len-cnt, dle-cnt }"},
	{0x000000B2, "fho_r1_smi_port / plane-2 (i.e. pdo_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000B3, "aao_r1_smi_port / plane-0 (i.e. aao_r1) / { len-cnt, dle-cnt }"},
	{0x000000B4, "aao_r1_smi_port / plane-0 (i.e. aao_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000B5, "aao_r1_smi_port / plane-1 (i.e. afo_r1) / { len-cnt, dle-cnt }"},
	{0x000000B6, "aao_r1_smi_port / plane-1 (i.e. afo_r1) / { load_com-cnt, bvalid-cnt }"},
	{0x000000B7, "ufdi_r5_smi_port / plane-0 (i.e. ufdi_r5) / { len-cnt, dle-cnt }"},
};
#endif /*__MTK_CAM_RAW_DMADBG_H*/
