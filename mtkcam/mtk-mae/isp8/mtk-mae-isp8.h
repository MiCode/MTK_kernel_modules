/* SPDX-License-Identifier: GPL-2.0 */
//
// Copyright (c) 2018 MediaTek Inc.

#ifndef __MTK_MAE_ISP8_H__
#define __MTK_MAE_ISP8_H__

#include "mtk-mae.h"

/* ============ should align userspace define (start) ================== */
const uint32_t fd_pattern_width[FD_PATTERN_NUM] = {640, 480, 240, 120};
const uint32_t fd_pattern_height[FD_PATTERN_NUM] = {480, 360, 180, 90};

#define MAE_BASE_ADDR_ALIGN 16

/* for isp8 */
#define V0_FD_640_480_COEF_SIZE     (11479 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_640_480_CONFIG_SIZE   (4086 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_640_480_COEF_PAT_OFFSET   0
#define V0_FD_640_480_CONFIG_PAT_OFFSET 0

#define V0_FD_480_360_COEF_SIZE     (11471 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_480_360_CONFIG_SIZE   (2862 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_480_360_COEF_PAT_OFFSET   \
		(V0_FD_640_480_COEF_PAT_OFFSET + V0_FD_640_480_COEF_SIZE)
#define V0_FD_480_360_CONFIG_PAT_OFFSET \
		(V0_FD_640_480_CONFIG_PAT_OFFSET + V0_FD_640_480_CONFIG_SIZE)

#define V0_FD_240_180_COEF_SIZE     (11471 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_240_180_CONFIG_SIZE   (2180 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_240_180_COEF_PAT_OFFSET   \
		(V0_FD_480_360_COEF_PAT_OFFSET + V0_FD_480_360_COEF_SIZE)
#define V0_FD_240_180_CONFIG_PAT_OFFSET \
		(V0_FD_480_360_CONFIG_PAT_OFFSET + V0_FD_480_360_CONFIG_SIZE)

#define V0_FD_120_90_COEF_SIZE      (11471 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_120_90_CONFIG_SIZE    (1834 * MAE_BASE_ADDR_ALIGN)
#define V0_FD_120_90_COEF_PAT_OFFSET   \
		(V0_FD_240_180_COEF_PAT_OFFSET + V0_FD_240_180_COEF_SIZE)
#define V0_FD_120_90_CONFIG_PAT_OFFSET \
		(V0_FD_240_180_CONFIG_PAT_OFFSET + V0_FD_240_180_CONFIG_SIZE)

#define V0_ATTR_128_128_COEF_SIZE   (15290 * MAE_BASE_ADDR_ALIGN)
#define V0_ATTR_128_128_CONFIG_SIZE (2000 * MAE_BASE_ADDR_ALIGN)

#define V1_FD_IPN_640_480_COEF_SIZE (10955 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE (7937 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_640_480_CONFIG_SIZE (3 * V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE)
#define V1_FD_IPN_640_480_COEF_PAT_OFFSET   0
#define V1_FD_IPN_640_480_CONFIG_PAT_OFFSET 0

#define V1_FD_IPN_480_360_COEF_SIZE (10947 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE (6676 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_480_360_CONFIG_SIZE (3 * V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE)
#define V1_FD_IPN_480_360_COEF_PAT_OFFSET \
		(V1_FD_IPN_640_480_COEF_PAT_OFFSET + V1_FD_IPN_640_480_COEF_SIZE)
#define V1_FD_IPN_480_360_CONFIG_PAT_OFFSET \
		(V1_FD_IPN_640_480_CONFIG_PAT_OFFSET + V1_FD_IPN_640_480_CONFIG_SIZE)

#define V1_FD_IPN_240_180_COEF_SIZE (10947 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE (5942 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_240_180_CONFIG_SIZE (3 * V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE)
#define V1_FD_IPN_240_180_COEF_PAT_OFFSET \
		(V1_FD_IPN_480_360_COEF_PAT_OFFSET + V1_FD_IPN_480_360_COEF_SIZE)
#define V1_FD_IPN_240_180_CONFIG_PAT_OFFSET \
		(V1_FD_IPN_480_360_CONFIG_PAT_OFFSET + V1_FD_IPN_480_360_CONFIG_SIZE)

#define V1_FD_IPN_120_90_COEF_SIZE (10939 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE (4827 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_120_90_CONFIG_SIZE (3 * V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE)
#define V1_FD_IPN_120_90_COEF_PAT_OFFSET \
		(V1_FD_IPN_240_180_COEF_PAT_OFFSET + V1_FD_IPN_240_180_COEF_SIZE)
#define V1_FD_IPN_120_90_CONFIG_PAT_OFFSET \
		(V1_FD_IPN_240_180_CONFIG_PAT_OFFSET + V1_FD_IPN_240_180_CONFIG_SIZE)

#define V1_FD_FPN_480_360_COEF_SIZE (59130 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_FPN_480_360_CONFIG_SIZE (8182 * MAE_BASE_ADDR_ALIGN)

#define V1_FLD_FAC_112_112_COEF_SIZE (25411 * MAE_BASE_ADDR_ALIGN)
#define V1_FLD_FAC_112_112_CONFIG_SIZE (4412 * MAE_BASE_ADDR_ALIGN)

/* for mt6899 */
#define V1_FD_IPN_640_480_COEF_SIZE_mt6899 (10974 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE_mt6899 (127296)
#define V1_FD_IPN_640_480_CONFIG_SIZE_mt6899 (3 * V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE_mt6899)
#define V1_FD_IPN_640_480_COEF_PAT_OFFSET_mt6899   0
#define V1_FD_IPN_640_480_CONFIG_PAT_OFFSET_mt6899 0

#define V1_FD_IPN_480_360_COEF_SIZE_mt6899 (10963 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE_mt6899 (114128)
#define V1_FD_IPN_480_360_CONFIG_SIZE_mt6899 (3 * V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE_mt6899)
#define V1_FD_IPN_480_360_COEF_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_640_480_COEF_PAT_OFFSET_mt6899 + V1_FD_IPN_640_480_COEF_SIZE_mt6899)
#define V1_FD_IPN_480_360_CONFIG_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_640_480_CONFIG_PAT_OFFSET_mt6899 + V1_FD_IPN_640_480_CONFIG_SIZE_mt6899)

#define V1_FD_IPN_240_180_COEF_SIZE_mt6899 (10955 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE_mt6899 (100928)
#define V1_FD_IPN_240_180_CONFIG_SIZE_mt6899 (3 * V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE_mt6899)
#define V1_FD_IPN_240_180_COEF_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_480_360_COEF_PAT_OFFSET_mt6899 + V1_FD_IPN_480_360_COEF_SIZE_mt6899)
#define V1_FD_IPN_240_180_CONFIG_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_480_360_CONFIG_PAT_OFFSET_mt6899 + V1_FD_IPN_480_360_CONFIG_SIZE_mt6899)

#define V1_FD_IPN_120_90_COEF_SIZE_mt6899 (10955 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE_mt6899 (99792)
#define V1_FD_IPN_120_90_CONFIG_SIZE_mt6899 (3 * V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE_mt6899)
#define V1_FD_IPN_120_90_COEF_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_240_180_COEF_PAT_OFFSET_mt6899 + V1_FD_IPN_240_180_COEF_SIZE_mt6899)
#define V1_FD_IPN_120_90_CONFIG_PAT_OFFSET_mt6899 \
		(V1_FD_IPN_240_180_CONFIG_PAT_OFFSET_mt6899 + V1_FD_IPN_240_180_CONFIG_SIZE_mt6899)

#define V1_FD_FPN_480_360_COEF_SIZE_mt6899 (59189 * MAE_BASE_ADDR_ALIGN)
#define V1_FD_FPN_480_360_CONFIG_SIZE_mt6899 (200976)

#define V1_FLD_FAC_112_112_COEF_SIZE_mt6899 (25435 * MAE_BASE_ADDR_ALIGN)
#define V1_FLD_FAC_112_112_CONFIG_SIZE_mt6899 (102016)

/* ============ should align userspace define (end) ================== */
struct config_info {
	const uint32_t size;
	const uint32_t rotate_offset;
	const uint32_t rotate_size;
};

struct coef_info {
	const uint32_t size;
};

struct mae_priv_data {
	/* v0 */
	const struct config_info fd_v0_config_info[FD_PATTERN_NUM];
	const struct coef_info fd_v0_coef_info[FD_PATTERN_NUM];
	const struct config_info attr_v0_config_info;
	const struct coef_info attr_v0_coef_info;
	const uint32_t v0_fd_coef_offset[FD_PATTERN_NUM];
	const uint32_t v0_fd_config_offset[FD_PATTERN_NUM];

	/* v1 */
	const struct config_info fd_v1_ipn_config_info[FD_PATTERN_NUM];
	const struct coef_info fd_v1_ipn_coef_info[FD_PATTERN_NUM];
	const struct config_info fac_v1_config_info;
	const struct coef_info fac_v1_coef_info;
	const struct config_info fd_v1_fpn_config_info;
	const struct coef_info fd_v1_fpn_coef_info;
	const uint32_t v1_fd_ipn_coef_offset[FD_PATTERN_NUM];
	const uint32_t v1_fd_ipn_config_offset[FD_PATTERN_NUM];
	const uint32_t v1_fd_ipn_config_single_size[FD_PATTERN_NUM];

	/* 1: cmdq_pkt_poll_timeout, 2: cmdq_pkt_poll_sleep*/
	int cmdq_polling_option;
};

#define FACE_NUM_REG_OFFSET              (0xC)

/* Reg address */
#define MAE_BASE                         (0x34310000)
#define FDVT_START                       (0x000)
#define FDVT_LEN                         (0x158)
#define FDVT_ENABLE                      (0x004)
#define FDVT_INT_EN                      (0x018)
#define FDVT_DMA_CTL                     (0x094)


// FLD V0 reg
#define FLD_IMG_BASE_ADDR                (0x400)
#define FLD_MS_BASE_ADDR                 (0x404)
#define FLD_FP_BASE_ADDR                 (0x408)
#define FLD_TR_BASE_ADDR                 (0x40C)
#define FLD_SH_BASE_ADDR                 (0x410)
#define FLD_CV_BASE_ADDR                 (0x414)
#define FLD_BS_BASE_ADDR                 (0x418)
#define FLD_PP_BASE_ADDR                 (0x41C)
#define FLD_FP_FORT_OFST                 (0x420)
#define FLD_TR_FORT_OFST                 (0x424)
#define FLD_SH_FORT_OFST                 (0x428)
#define FLD_CV_FORT_OFST                 (0x42C)

#define FLD_FACE_0_INFO_0           (0x430)
#define FLD_FACE_0_INFO_1           (0x434)
#define FLD_FACE_1_INFO_0           (0x438)
#define FLD_FACE_1_INFO_1           (0x43C)
#define FLD_FACE_2_INFO_0           (0x440)
#define FLD_FACE_2_INFO_1           (0x444)
#define FLD_FACE_3_INFO_0           (0x448)
#define FLD_FACE_3_INFO_1           (0x44C)
#define FLD_FACE_4_INFO_0           (0x450)
#define FLD_FACE_4_INFO_1           (0x454)
#define FLD_FACE_5_INFO_0           (0x458)
#define FLD_FACE_5_INFO_1           (0x45C)
#define FLD_FACE_6_INFO_0           (0x460)
#define FLD_FACE_6_INFO_1           (0x464)
#define FLD_FACE_7_INFO_0           (0x468)
#define FLD_FACE_7_INFO_1           (0x46C)
#define FLD_FACE_8_INFO_0           (0x470)
#define FLD_FACE_8_INFO_1           (0x474)
#define FLD_FACE_9_INFO_0           (0x478)
#define FLD_FACE_9_INFO_1           (0x47C)
#define FLD_FACE_10_INFO_0          (0x480)
#define FLD_FACE_10_INFO_1          (0x484)
#define FLD_FACE_11_INFO_0          (0x488)
#define FLD_FACE_11_INFO_1          (0x48C)
#define FLD_FACE_12_INFO_0          (0x490)
#define FLD_FACE_12_INFO_1          (0x494)
#define FLD_FACE_13_INFO_0          (0x498)
#define FLD_FACE_13_INFO_1          (0x49C)
#define FLD_FACE_14_INFO_0          (0x4A0)
#define FLD_FACE_14_INFO_1          (0x4A4)

#define FLD_NUM_CONFIG_0		(0x4A8)
#define FLD_FACE_NUM			(0x4AC)

#define FLD_PCA_MEAN_SCALE_0	(0x4B0)
#define FLD_PCA_MEAN_SCALE_1	(0x4B4)
#define FLD_PCA_MEAN_SCALE_2	(0x4B8)
#define FLD_PCA_MEAN_SCALE_3	(0x4BC)
#define FLD_PCA_MEAN_SCALE_4	(0x4C0)
#define FLD_PCA_MEAN_SCALE_5	(0x4C4)
#define FLD_PCA_MEAN_SCALE_6	(0x4C8)
#define FLD_PCA_VEC_0			(0x4CC)
#define FLD_PCA_VEC_1			(0x4D0)
#define FLD_PCA_VEC_2			(0x4D4)
#define FLD_PCA_VEC_3			(0x4D8)
#define FLD_PCA_VEC_4			(0x4DC)
#define FLD_PCA_VEC_5			(0x4E0)
#define FLD_PCA_VEC_6			(0x4E4)
#define FLD_CV_BIAS_FR_0		(0x4E8)
#define FLD_CV_BIAS_PF_0		(0x4EC)
#define FLD_CV_RANGE_FR_0		(0x4F0)
#define FLD_CV_RANGE_FR_1		(0x4F4)
#define FLD_CV_RANGE_PF_0		(0x4F8)
#define FLD_CV_RANGE_PF_1		(0x4FC)
#define FLD_PP_COEF				(0x500)
#define FLD_SRC_SIZE			(0x504)
#define FLD_CMDQ_SRC_SIZE		(0x504)
#define FLD_SRC_PITCH			(0x508)
#define FLD_BS_CONFIG0			(0x50C)
#define FLD_BS_CONFIG1			(0x510)
#define FLD_BS_CONFIG2			(0x514)

static const unsigned int fld_face_info_idx_0[MAX_FLD_V0_FACE_NUM] = {
	FLD_FACE_0_INFO_0, FLD_FACE_1_INFO_0, FLD_FACE_2_INFO_0,
	FLD_FACE_3_INFO_0, FLD_FACE_4_INFO_0, FLD_FACE_5_INFO_0,
	FLD_FACE_6_INFO_0, FLD_FACE_7_INFO_0, FLD_FACE_8_INFO_0,
	FLD_FACE_9_INFO_0, FLD_FACE_10_INFO_0, FLD_FACE_11_INFO_0,
	FLD_FACE_12_INFO_0, FLD_FACE_13_INFO_0, FLD_FACE_14_INFO_0
};

static const unsigned int fld_face_info_idx_1[MAX_FLD_V0_FACE_NUM] = {
	FLD_FACE_0_INFO_1, FLD_FACE_1_INFO_1, FLD_FACE_2_INFO_1,
	FLD_FACE_3_INFO_1, FLD_FACE_4_INFO_1, FLD_FACE_5_INFO_1,
	FLD_FACE_6_INFO_1, FLD_FACE_7_INFO_1, FLD_FACE_8_INFO_1,
	FLD_FACE_9_INFO_1, FLD_FACE_10_INFO_1, FLD_FACE_11_INFO_1,
	FLD_FACE_12_INFO_1, FLD_FACE_13_INFO_1, FLD_FACE_14_INFO_1
};

// 0x1000 MAE_CTRL_CENTER
#define MAE_CTRL_CENTER_BASE             (0x1000)
#define MAE_CTRL_CENTER_LEN              (0x308)
#define MAE_TRIG_RST_CTRL                (MAE_CTRL_CENTER_BASE + 0x0000)
#define MAE_CLK_CTRL                     (MAE_CTRL_CENTER_BASE + 0x0004)
#define MAE_IRQ_DDREN_CMDQ_CTRL          (MAE_CTRL_CENTER_BASE + 0x0010)
#define MAE_IRQ_CTRL0                    (MAE_CTRL_CENTER_BASE + 0x0014)
#define MAE_IRQ_CTRL1                    (MAE_CTRL_CENTER_BASE + 0x0018)
#define MAE_SYS_SHADOW_CTRL              (MAE_CTRL_CENTER_BASE + 0x001C)
#define MAE_TRIG_SEL_4_6                 (MAE_CTRL_CENTER_BASE + 0x004C)

// 0x4200 MAE_RSZ0
#define MAE_MAISR_BASE                   (0x4200)
#define MAE_0170_MAISR                   (MAE_MAISR_BASE + 0x170)
#define MAE_0174_MAISR                   (MAE_MAISR_BASE + 0x174)
#define MAE_MAISR_LEN                     (0x0600)

// 0x4800 MAE_RSZ0
#define MAE_RSZ0_BASE                    (0x4800)
#define MAE_REG_0080_RSZ0                (MAE_RSZ0 + 0x080)
#define MAE_REG_0084_RSZ0                (MAE_RSZ0 + 0x084)
#define MAE_REG_01A8_RSZ0                (MAE_RSZ0 + 0x1A8)
#define MAE_RSZ0_LEN                     (0x01F4)

// 0x4C00 MAE_UDMA_W
#define MAE_UDMA_W_BASE                 (0x4C00)
#define MAE_REG_0054_UDMA_W             (MAE_UDMA_W_BASE + 0x0054)

// 0x5000 MAE_CMP
#define MAE_CMP_BASE                    (0x5000)
#define MAE_CMP_LEN                     (0x0190)

// 0x5200 MAE_UDMA_R
#define MAE_UDMA_R_BASE                 (0x5200)
#define MAE_REG_0054_UDMA_R             (MAE_UDMA_R_BASE + 0x0054)
#define MAE_REG_0100_UDMA_R             (MAE_UDMA_R_BASE + 0x0100)
#define MAE_REG_0120_UDMA_R             (MAE_UDMA_R_BASE + 0x0120)
#define MAE_REG_0124_UDMA_R             (MAE_UDMA_R_BASE + 0x0124)
#define MAE_REG_0128_UDMA_R             (MAE_UDMA_R_BASE + 0x0128)
#define MAE_REG_012C_UDMA_R             (MAE_UDMA_R_BASE + 0x012C)
#define MAE_REG_0130_UDMA_R             (MAE_UDMA_R_BASE + 0x0130)
#define MAE_REG_0180_UDMA_R             (MAE_UDMA_R_BASE + 0x0180)
#define MAE_REG_0184_UDMA_R             (MAE_UDMA_R_BASE + 0x0184)
#define MAE_REG_01A0_UDMA_R             (MAE_UDMA_R_BASE + 0x01A0)
#define MAE_REG_01A4_UDMA_R             (MAE_UDMA_R_BASE + 0x01A4)
#define MAE_REG_01C0_UDMA_R             (MAE_UDMA_R_BASE + 0x01C0)
#define MAE_REG_01C4_UDMA_R             (MAE_UDMA_R_BASE + 0x01C4)
#define MAE_REG_01C8_UDMA_R             (MAE_UDMA_R_BASE + 0x01C8)
#define MAE_REG_01D4_UDMA_R             (MAE_UDMA_R_BASE + 0x01D4)

// 0x5800 MAE_RDMA_5
#define MAE_RDMA_5_BASE                   (0x5800)
#define MAE_REG_0004_MAE_RDMA_5           (MAE_RDMA_5_BASE + 0x0004)
#define MAE_RDMA_5_LEN                    (0x1FC)

// 0X6000 RSZ1
#define RSZ1_BASE                         (0x6000)
#define RSZ1_BASE_LEN                     (0x1FC)
#define REG_0004_RSZ1                     (RSZ1_BASE + 0x0004)  // reg_ini_factor_ho_1_0
#define REG_0008_RSZ1                     (RSZ1_BASE + 0x0008)  // reg_ini_factor_ho_1_1
#define REG_000C_RSZ1                     (RSZ1_BASE + 0x000C)  // reg_ini_factor_ve_1_0
#define REG_0010_RSZ1                     (RSZ1_BASE + 0x0010)  // reg_ini_factor_ve_1_1
#define REG_001C_RSZ1                     (RSZ1_BASE + 0x001C)  // reg_scale_factor_ho_1_0
#define REG_0020_RSZ1                     (RSZ1_BASE + 0x0020)  // reg_scale_factor_ho_1_1
#define REG_0024_RSZ1                     (RSZ1_BASE + 0x0024)  // reg_scale_factor_ve_1_0[15:0]
#define REG_0028_RSZ1                     (RSZ1_BASE + 0x0028)  // reg_scale_factor_ve_1_1[7:0]
                                                                // reg_scale_ve_en_1[8:8]
                                                                // reg_v_shift_mode_en_1[9:9]
                                                                // reg_scale_factor_ve_msb_1[15:12]
#define REG_002C_RSZ1                     (RSZ1_BASE + 0x002C)  //
#define REG_0034_RSZ1                     (RSZ1_BASE + 0x0034)  //
#define REG_005C_RSZ1                     (RSZ1_BASE + 0x005C)  //
#define REG_0060_RSZ1                     (RSZ1_BASE + 0x0060)  //
#define REG_0064_RSZ1                     (RSZ1_BASE + 0x0064)  //
#define REG_0068_RSZ1                     (RSZ1_BASE + 0x0068)  //
#define REG_0080_RSZ1                     (RSZ1_BASE + 0x0080)  //
#define REG_0084_RSZ1                     (RSZ1_BASE + 0x0084)  //
#define REG_00A0_RSZ1                     (RSZ1_BASE + 0x00A0)  // reg_h_size[13:0]
#define REG_00A4_RSZ1                     (RSZ1_BASE + 0x00A4)  // reg_v_size[13:0]
#define REG_00A8_RSZ1                     (RSZ1_BASE + 0x00A8)  //
#define REG_00AC_RSZ1                     (RSZ1_BASE + 0x00AC)  //
#define REG_00C0_RSZ1                     (RSZ1_BASE + 0x00C0)  // reg_pre_crop_h_crop_en_1[0]
                                                                // reg_pre_crop_v_crop_en_1[1]
#define REG_00C4_RSZ1                     (RSZ1_BASE + 0x00C4)  //
#define REG_00C8_RSZ1                     (RSZ1_BASE + 0x00C8)  // reg_pre_crop_h_st_1
#define REG_00CC_RSZ1                     (RSZ1_BASE + 0x00CC)  // reg_pre_crop_h_length_1
#define REG_00D0_RSZ1                     (RSZ1_BASE + 0x00D0)  // reg_pre_crop_hfde_size_1
#define REG_00D4_RSZ1                     (RSZ1_BASE + 0x00D4)  // reg_pre_crop_v_st_1
#define REG_00D8_RSZ1                     (RSZ1_BASE + 0x00D8)  // reg_pre_crop_v_length_1
#define REG_0104_RSZ1                     (RSZ1_BASE + 0x0104)  // reg_post_ins_hv_insert_en[0]
#define REG_0108_RSZ1                     (RSZ1_BASE + 0x0108)  //
#define REG_010C_RSZ1                     (RSZ1_BASE + 0x010C)  // reg_post_ins_blk_hpre[13:0]
#define REG_0110_RSZ1                     (RSZ1_BASE + 0x0110)  // reg_post_ins_h_length[13:0]
#define REG_0114_RSZ1                     (RSZ1_BASE + 0x0114)  // reg_post_ins_hfde_size[13:0]
#define REG_0118_RSZ1                     (RSZ1_BASE + 0x0118)  // reg_post_ins_blk_vpre[13:0]
#define REG_011C_RSZ1                     (RSZ1_BASE + 0x011C)  // reg_post_ins_v_length[13:0]
#define REG_0120_RSZ1                     (RSZ1_BASE + 0x0120)  // reg_post_ins_vfde_size[13:0]
#define REG_0180_RSZ1                     (RSZ1_BASE + 0x0180)  //
#define REG_0184_RSZ1                     (RSZ1_BASE + 0x0184)  // reg_rsz_en[0], reg_1p_path_en[1]
                                                                // reg_postproc_en[2], reg_ins_path[3]

#define REG_01A0_RSZ1                     (RSZ1_BASE + 0x01A0)
#define REG_01A4_RSZ1                     (RSZ1_BASE + 0x01A4)
#define REG_01A8_RSZ1                     (RSZ1_BASE + 0x01A8)
#define REG_01AC_RSZ1                     (RSZ1_BASE + 0x01AC)
#define REG_01B0_RSZ1                     (RSZ1_BASE + 0x01B0)
#define REG_01B4_RSZ1                     (RSZ1_BASE + 0x01B4)
#define REG_01B8_RSZ1                     (RSZ1_BASE + 0x01B8)
#define REG_01BC_RSZ1                     (RSZ1_BASE + 0x01BC)
#define REG_01C0_RSZ1                     (RSZ1_BASE + 0x01C0)
#define REG_01C4_RSZ1                     (RSZ1_BASE + 0x01C4)
#define REG_01C8_RSZ1                     (RSZ1_BASE + 0x01C8)
#define REG_01CC_RSZ1                     (RSZ1_BASE + 0x01CC)
#define REG_01D0_RSZ1                     (RSZ1_BASE + 0x01D0)
#define REG_01D4_RSZ1                     (RSZ1_BASE + 0x01D4)
#define REG_01D8_RSZ1                     (RSZ1_BASE + 0x01D8)
#define REG_01DC_RSZ1                     (RSZ1_BASE + 0x01DC)




// 0X6200 RSZ2
#define RSZ2_BASE                         (0x6200)
#define RSZ2_BASE_LEN                     (0x1FC)
#define REG_0004_RSZ2                     (RSZ2_BASE + 0x0004)  // reg_ini_factor_ho_1_0
#define REG_0008_RSZ2                     (RSZ2_BASE + 0x0008)  // reg_ini_factor_ho_1_1
#define REG_000C_RSZ2                     (RSZ2_BASE + 0x000C)  // reg_ini_factor_ve_1_0
#define REG_0010_RSZ2                     (RSZ2_BASE + 0x0010)  // reg_ini_factor_ve_1_1
#define REG_001C_RSZ2                     (RSZ2_BASE + 0x001C)  // reg_scale_factor_ho_1_0
#define REG_0020_RSZ2                     (RSZ2_BASE + 0x0020)  // reg_scale_factor_ho_1_1
#define REG_0024_RSZ2                     (RSZ2_BASE + 0x0024)  // reg_scale_factor_ve_1_0[15:0]
#define REG_0028_RSZ2                     (RSZ2_BASE + 0x0028)  // reg_scale_factor_ve_1_1[7:0]
                                                                // reg_scale_ve_en_1[8:8]
                                                                // reg_v_shift_mode_en_1[9:9]
                                                                // reg_scale_factor_ve_msb_1[15:12]
                                                                //
#define REG_002C_RSZ2                     (RSZ2_BASE + 0x002C)  //
#define REG_0034_RSZ2                     (RSZ2_BASE + 0x0034)  //
#define REG_005C_RSZ2                     (RSZ2_BASE + 0x005C)  //
#define REG_0060_RSZ2                     (RSZ2_BASE + 0x0060)  //
#define REG_0064_RSZ2                     (RSZ2_BASE + 0x0064)  //
#define REG_0068_RSZ2                     (RSZ2_BASE + 0x0068)  //
#define REG_0080_RSZ2                     (RSZ2_BASE + 0x0080)  //
#define REG_0084_RSZ2                     (RSZ2_BASE + 0x0084)  //
#define REG_00A0_RSZ2                     (RSZ2_BASE + 0x00A0)  //
#define REG_00A4_RSZ2                     (RSZ2_BASE + 0x00A4)  //
#define REG_00A8_RSZ2                     (RSZ2_BASE + 0x00A8)  //
#define REG_00AC_RSZ2                     (RSZ2_BASE + 0x00AC)  //
#define REG_00C0_RSZ2                     (RSZ2_BASE + 0x00C0)  //
#define REG_00C4_RSZ2                     (RSZ2_BASE + 0x00C4)  //
#define REG_00C8_RSZ2                     (RSZ2_BASE + 0x00C8)  //
#define REG_00CC_RSZ2                     (RSZ2_BASE + 0x00CC)  //
#define REG_00D0_RSZ2                     (RSZ2_BASE + 0x00D0)  //
#define REG_00D4_RSZ2                     (RSZ2_BASE + 0x00D4)  //
#define REG_00D8_RSZ2                     (RSZ2_BASE + 0x00D8)  //
#define REG_0104_RSZ2                     (RSZ2_BASE + 0x0104)  //
#define REG_0108_RSZ2                     (RSZ2_BASE + 0x0108)  //
#define REG_010C_RSZ2                     (RSZ2_BASE + 0x010C)  //
#define REG_0110_RSZ2                     (RSZ2_BASE + 0x0110)  //
#define REG_0114_RSZ2                     (RSZ2_BASE + 0x0114)  //
#define REG_0118_RSZ2                     (RSZ2_BASE + 0x0118)  //
#define REG_011C_RSZ2                     (RSZ2_BASE + 0x011C)  //
#define REG_0120_RSZ2                     (RSZ2_BASE + 0x0120)  //
#define REG_0180_RSZ2                     (RSZ2_BASE + 0x0180)  //
#define REG_0184_RSZ2                     (RSZ2_BASE + 0x0184)  //
#define REG_01A8_RSZ2                     (RSZ2_BASE + 0x01A8)  //  reg_dbf_off


// 0X6400 RSZ3
#define RSZ3_BASE                         (0x6400)
#define RSZ3_BASE_LEN                     (0x1FC)
#define REG_0004_RSZ3                     (RSZ3_BASE + 0x0004)  // reg_ini_factor_ho_1_0
#define REG_0008_RSZ3                     (RSZ3_BASE + 0x0008)  // reg_ini_factor_ho_1_1
#define REG_000C_RSZ3                     (RSZ3_BASE + 0x000C)  // reg_ini_factor_ve_1_0
#define REG_0010_RSZ3                     (RSZ3_BASE + 0x0010)  // reg_ini_factor_ve_1_1
#define REG_001C_RSZ3                     (RSZ3_BASE + 0x001C)  // reg_scale_factor_ho_1_0
#define REG_0020_RSZ3                     (RSZ3_BASE + 0x0020)  // reg_scale_factor_ho_1_1
#define REG_0024_RSZ3                     (RSZ3_BASE + 0x0024)  // reg_scale_factor_ve_1_0[15:0]
#define REG_0028_RSZ3                     (RSZ3_BASE + 0x0028)  // reg_scale_factor_ve_1_1[7:0]
                                                                // reg_scale_ve_en_1[8:8]
                                                                // reg_v_shift_mode_en_1[9:9]
                                                                // reg_scale_factor_ve_msb_1[15:12]
                                                                //
#define REG_002C_RSZ3                     (RSZ3_BASE + 0x002C)  //
#define REG_0034_RSZ3                     (RSZ3_BASE + 0x0034)  //
#define REG_005C_RSZ3                     (RSZ3_BASE + 0x005C)  //
#define REG_0060_RSZ3                     (RSZ3_BASE + 0x0060)  //
#define REG_0064_RSZ3                     (RSZ3_BASE + 0x0064)  //
#define REG_0068_RSZ3                     (RSZ3_BASE + 0x0068)  //
#define REG_0080_RSZ3                     (RSZ3_BASE + 0x0080)  //
#define REG_0084_RSZ3                     (RSZ3_BASE + 0x0084)  //
#define REG_00A0_RSZ3                     (RSZ3_BASE + 0x00A0)  //
#define REG_00A4_RSZ3                     (RSZ3_BASE + 0x00A4)  //
#define REG_00A8_RSZ3                     (RSZ3_BASE + 0x00A8)  //
#define REG_00AC_RSZ3                     (RSZ3_BASE + 0x00AC)  //
#define REG_00C0_RSZ3                     (RSZ3_BASE + 0x00C0)  //
#define REG_00C4_RSZ3                     (RSZ3_BASE + 0x00C4)  //
#define REG_00C8_RSZ3                     (RSZ3_BASE + 0x00C8)  //
#define REG_00CC_RSZ3                     (RSZ3_BASE + 0x00CC)  //
#define REG_00D0_RSZ3                     (RSZ3_BASE + 0x00D0)  //
#define REG_00D4_RSZ3                     (RSZ3_BASE + 0x00D4)  //
#define REG_00D8_RSZ3                     (RSZ3_BASE + 0x00D8)  //
#define REG_0104_RSZ3                     (RSZ3_BASE + 0x0104)  //
#define REG_0108_RSZ3                     (RSZ3_BASE + 0x0108)  //
#define REG_010C_RSZ3                     (RSZ3_BASE + 0x010C)  //
#define REG_0110_RSZ3                     (RSZ3_BASE + 0x0110)  //
#define REG_0114_RSZ3                     (RSZ3_BASE + 0x0114)  //
#define REG_0118_RSZ3                     (RSZ3_BASE + 0x0118)  //
#define REG_011C_RSZ3                     (RSZ3_BASE + 0x011C)  //
#define REG_0120_RSZ3                     (RSZ3_BASE + 0x0120)  //
#define REG_0180_RSZ3                     (RSZ3_BASE + 0x0180)  //
#define REG_0184_RSZ3                     (RSZ3_BASE + 0x0184)  //
#define REG_01A8_RSZ3                     (RSZ3_BASE + 0x01A8)  //  reg_dbf_off


// 0X6A04 MAISR_APB_BASE
#define MAE_MAISR_APB_BASE                (0x6A00)
#define MAE_COEF_ROTATE                   (MAE_MAISR_APB_BASE + 0x0004)

// 0x7000 MMFD_POST
#define MMFD_POST_BASE                    (0x7000)
#define MMFD_POST_LEN                     (0x0114)
#define MAE_REG_POST_DBF_OFF              (MMFD_POST_BASE + 0x0004)
#define MAE_REG_X_OFFSET_0                (MMFD_POST_BASE + 0x0010)
#define MAE_REG_X_OFFSET_1                (MMFD_POST_BASE + 0x0014)
#define MAE_REG_X_OFFSET_2                (MMFD_POST_BASE + 0x0018)
#define MAE_REG_Y_OFFSET_0                (MMFD_POST_BASE + 0x001C)
#define MAE_REG_Y_OFFSET_1                (MMFD_POST_BASE + 0x0020)
#define MAE_REG_Y_OFFSET_2                (MMFD_POST_BASE + 0x0024)

#define MAE_REG_MMFD_O_SCALE_0            (MMFD_POST_BASE + 0x0028)
#define MAE_REG_MMFD_O_SCALE_1            (MMFD_POST_BASE + 0x002C)
#define MAE_REG_MMFD_O_SCALE_2            (MMFD_POST_BASE + 0x0030)

#define MAE_REG_H_SIZE0                   (MMFD_POST_BASE + 0x0080)
#define MAE_REG_H_SIZE1                   (MMFD_POST_BASE + 0x0084)
#define MAE_REG_H_SIZE2                   (MMFD_POST_BASE + 0x0088)
#define MAE_REG_V_SIZE0                   (MMFD_POST_BASE + 0x008C)
#define MAE_REG_V_SIZE1                   (MMFD_POST_BASE + 0x0090)
#define MAE_REG_V_SIZE2                   (MMFD_POST_BASE + 0x0094)

#define MAE_REG_H_MIN0                    (MMFD_POST_BASE + 0x0098)
#define MAE_REG_H_MIN1                    (MMFD_POST_BASE + 0x009C)
#define MAE_REG_H_MIN2                    (MMFD_POST_BASE + 0x00A0)
#define MAE_REG_V_MIN0                    (MMFD_POST_BASE + 0x00A4)
#define MAE_REG_V_MIN1                    (MMFD_POST_BASE + 0x00A8)
#define MAE_REG_V_MIN2                    (MMFD_POST_BASE + 0x00AC)

#define MAE_REG_SCORE_TH0                 (MMFD_POST_BASE + 0x00B0)
#define MAE_REG_SCORE_TH1                 (MMFD_POST_BASE + 0x00B4)
#define MAE_REG_SCORE_TH2                 (MMFD_POST_BASE + 0x00B8)

#define MAE_REG_FACE_NUM0                 (MMFD_POST_BASE + 0x00C0)
#define MAE_REG_FACE_NUM1                 (MMFD_POST_BASE + 0x00C4)
#define MAE_REG_FACE_NUM2                 (MMFD_POST_BASE + 0x00C8)

#define MAE_REG_FACE1_NUM0                (MMFD_POST_BASE + 0x00CC)
#define MAE_REG_FACE1_NUM1                (MMFD_POST_BASE + 0x00D0)
#define MAE_REG_FACE1_NUM2                (MMFD_POST_BASE + 0x00D4)

#define MAE_REG_FACE2_NUM0                (MMFD_POST_BASE + 0x00D8)
#define MAE_REG_FACE2_NUM1                (MMFD_POST_BASE + 0x00DC)
#define MAE_REG_FACE2_NUM2                (MMFD_POST_BASE + 0x00E0)

#define MAE_REG_FACE3_NUM0                (MMFD_POST_BASE + 0x00E4)
#define MAE_REG_FACE3_NUM1                (MMFD_POST_BASE + 0x00E8)
#define MAE_REG_FACE3_NUM2                (MMFD_POST_BASE + 0x00EC)

#define MAE_REG_FACE4_NUM0                (MMFD_POST_BASE + 0x00F0)
#define MAE_REG_FACE4_NUM1                (MMFD_POST_BASE + 0x00F4)
#define MAE_REG_FACE4_NUM2                (MMFD_POST_BASE + 0x00F8)


#define MAE_REG_H_MAX0                    (MMFD_POST_BASE + 0x0100)
#define MAE_REG_H_MAX1                    (MMFD_POST_BASE + 0x0104)
#define MAE_REG_H_MAX2                    (MMFD_POST_BASE + 0x0108)
#define MAE_REG_V_MAX0                    (MMFD_POST_BASE + 0x010C)
#define MAE_REG_V_MAX1                    (MMFD_POST_BASE + 0x0110)
#define MAE_REG_V_MAX2                    (MMFD_POST_BASE + 0x0114)

// 0x7400 MAE_DRV
#define MAE_DRV_W_BASE                    (0x7400)
#define MAE_DRV_W_LEN                     (0x0164)
#define MAE_REG_EXTRN_BASE0_00_0_W        (MAE_DRV_W_BASE + 0x0000)
#define MAE_REG_EXTRN_BASE0_00_1_W        (MAE_DRV_W_BASE + 0x0004)
#define MAE_REG_EXTRN_BASE0_01_0_W        (MAE_DRV_W_BASE + 0x0008)
#define MAE_REG_EXTRN_BASE0_01_1_W        (MAE_DRV_W_BASE + 0x000C)
#define MAE_REG_EXTRN_BASE0_02_0_W        (MAE_DRV_W_BASE + 0x0010)
#define MAE_REG_EXTRN_BASE0_02_1_W        (MAE_DRV_W_BASE + 0x0014)
#define MAE_REG_EXTRN_BASE0_03_0_W        (MAE_DRV_W_BASE + 0x0018)
#define MAE_REG_EXTRN_BASE0_03_1_W        (MAE_DRV_W_BASE + 0x001C)
#define MAE_REG_EXTRN_BASE0_04_0_W        (MAE_DRV_W_BASE + 0x0020)
#define MAE_REG_EXTRN_BASE0_04_1_W        (MAE_DRV_W_BASE + 0x0024)
#define MAE_REG_EXTRN_BASE0_05_0_W        (MAE_DRV_W_BASE + 0x0028)
#define MAE_REG_EXTRN_BASE0_05_1_W        (MAE_DRV_W_BASE + 0x002C)
#define MAE_REG_EXTRN_BASE0_06_0_W        (MAE_DRV_W_BASE + 0x0030)
#define MAE_REG_EXTRN_BASE0_06_1_W        (MAE_DRV_W_BASE + 0x0034)
#define MAE_REG_EXTRN_BASE0_07_0_W        (MAE_DRV_W_BASE + 0x0038)
#define MAE_REG_EXTRN_BASE0_07_1_W        (MAE_DRV_W_BASE + 0x003C)
#define MAE_REG_EXTRN_BASE0_08_0_W        (MAE_DRV_W_BASE + 0x0040)
#define MAE_REG_EXTRN_BASE0_08_1_W        (MAE_DRV_W_BASE + 0x0044)
#define MAE_REG_EXTRN_BASE0_09_0_W        (MAE_DRV_W_BASE + 0x0048)
#define MAE_REG_EXTRN_BASE0_09_1_W        (MAE_DRV_W_BASE + 0x004C)
#define MAE_REG_EXTRN_BASE0_10_0_W        (MAE_DRV_W_BASE + 0x0050)
#define MAE_REG_EXTRN_BASE0_10_1_W        (MAE_DRV_W_BASE + 0x0054)
#define MAE_REG_EXTRN_BASE0_11_0_W        (MAE_DRV_W_BASE + 0x0058)
#define MAE_REG_EXTRN_BASE0_11_1_W        (MAE_DRV_W_BASE + 0x005C)
#define MAE_REG_EXTRN_BASE0_12_0_W        (MAE_DRV_W_BASE + 0x0060)
#define MAE_REG_EXTRN_BASE0_12_1_W        (MAE_DRV_W_BASE + 0x0064)
#define MAE_REG_EXTRN_BASE0_13_0_W        (MAE_DRV_W_BASE + 0x0068)
#define MAE_REG_EXTRN_BASE0_13_1_W        (MAE_DRV_W_BASE + 0x006C)
#define MAE_REG_EXTRN_BASE0_14_0_W        (MAE_DRV_W_BASE + 0x0070)
#define MAE_REG_EXTRN_BASE0_14_1_W        (MAE_DRV_W_BASE + 0x0074)

#define MAE_REG_INTRN_BASE_0_W            (MAE_DRV_W_BASE + 0x0078)
#define MAE_REG_INTRN_BASE_1_W            (MAE_DRV_W_BASE + 0x007C)

#define MAE_REG_WDMA_DBF_OFF              (MAE_DRV_W_BASE + 0x0080)

#define MAE_REG_EXTRN_LN_OFFSET_00_W      (MAE_DRV_W_BASE + 0x0100)
#define MAE_REG_EXTRN_LN_OFFSET_01_W      (MAE_DRV_W_BASE + 0x0104)
#define MAE_REG_EXTRN_LN_OFFSET_02_W      (MAE_DRV_W_BASE + 0x0108)
#define MAE_REG_EXTRN_LN_OFFSET_03_W      (MAE_DRV_W_BASE + 0x010C)
#define MAE_REG_EXTRN_LN_OFFSET_04_W      (MAE_DRV_W_BASE + 0x0110)
#define MAE_REG_EXTRN_LN_OFFSET_05_W      (MAE_DRV_W_BASE + 0x0114)
#define MAE_REG_EXTRN_LN_OFFSET_06_W      (MAE_DRV_W_BASE + 0x0118)
#define MAE_REG_EXTRN_LN_OFFSET_07_W      (MAE_DRV_W_BASE + 0x011C)
#define MAE_REG_EXTRN_LN_OFFSET_08_W      (MAE_DRV_W_BASE + 0x0120)
#define MAE_REG_EXTRN_LN_OFFSET_09_W      (MAE_DRV_W_BASE + 0x0124)
#define MAE_REG_EXTRN_LN_OFFSET_10_W      (MAE_DRV_W_BASE + 0x0128)
#define MAE_REG_EXTRN_LN_OFFSET_11_W      (MAE_DRV_W_BASE + 0x012C)
#define MAE_REG_EXTRN_LN_OFFSET_12_W      (MAE_DRV_W_BASE + 0x0130)
#define MAE_REG_EXTRN_LN_OFFSET_13_W      (MAE_DRV_W_BASE + 0x0134)
#define MAE_REG_EXTRN_LN_OFFSET_14_W      (MAE_DRV_W_BASE + 0x0138)

// 0x7600 MAE_DRV_R
#define MAE_DRV_R_BASE                    (0x7600)
#define MAE_DRV_R_LEN                     (0x01FC)
#define MAE_REG_EXTRN_BASE0_00_0_R        (MAE_DRV_R_BASE + 0x0000)
#define MAE_REG_EXTRN_BASE0_00_1_R        (MAE_DRV_R_BASE + 0x0004)
#define MAE_REG_EXTRN_BASE0_01_0_R        (MAE_DRV_R_BASE + 0x0008)
#define MAE_REG_EXTRN_BASE0_01_1_R        (MAE_DRV_R_BASE + 0x000C)
#define MAE_REG_EXTRN_BASE0_02_0_R        (MAE_DRV_R_BASE + 0x0010)
#define MAE_REG_EXTRN_BASE0_02_1_R        (MAE_DRV_R_BASE + 0x0014)

#define MAE_REG_INTRN_BASE_0_R            (MAE_DRV_R_BASE + 0x0078)
#define MAE_REG_INTRN_BASE_1_R            (MAE_DRV_R_BASE + 0x007C)

#define MAE_REG_EXTRN_BASE1_00_0_R        (MAE_DRV_R_BASE + 0x0080)
#define MAE_REG_EXTRN_BASE1_00_1_R        (MAE_DRV_R_BASE + 0x0084)
#define MAE_REG_EXTRN_BASE1_01_0_R        (MAE_DRV_R_BASE + 0x0088)
#define MAE_REG_EXTRN_BASE1_01_1_R        (MAE_DRV_R_BASE + 0x008C)
#define MAE_REG_EXTRN_BASE1_02_0_R        (MAE_DRV_R_BASE + 0x0090)
#define MAE_REG_EXTRN_BASE1_02_1_R        (MAE_DRV_R_BASE + 0x0094)

#define MAE_REG_EXTRN_LN_OFFSET_00_R      (MAE_DRV_R_BASE + 0x0100)
#define MAE_REG_EXTRN_LN_OFFSET_01_R      (MAE_DRV_R_BASE + 0x0104)
#define MAE_REG_EXTRN_LN_OFFSET_02_R      (MAE_DRV_R_BASE + 0x0108)

#define MAE_REG_EXTRN_MEM_CONFIG          (MAE_DRV_R_BASE + 0x0140)

#define MAE_REG_OUTER_SRC_HSIZE_00        (MAE_DRV_R_BASE + 0x0148)
#define MAE_REG_OUTER_SRC_VSIZE_00        (MAE_DRV_R_BASE + 0x014C)
#define MAE_REG_OUTER_SRC_HSIZE_01        (MAE_DRV_R_BASE + 0x0150)
#define MAE_REG_OUTER_SRC_VSIZE_01        (MAE_DRV_R_BASE + 0x0154)
#define MAE_REG_OUTER_SRC_HSIZE_02        (MAE_DRV_R_BASE + 0x0158)
#define MAE_REG_OUTER_SRC_VSIZE_02        (MAE_DRV_R_BASE + 0x015C)

#define MAE_REG_0178_MAE_DRV_R            (MAE_DRV_R_BASE + 0x0178)
#define MAE_REG_017C_MAE_DRV_R            (MAE_DRV_R_BASE + 0x017C)

#define MAE_REG_OUTER_CONFIG_BASE_00_0    (MAE_DRV_R_BASE + 0x0180)
#define MAE_REG_OUTER_CONFIG_BASE_00_1    (MAE_DRV_R_BASE + 0x0184)
#define MAE_REG_OUTER_CONFIG_BASE_01_0    (MAE_DRV_R_BASE + 0x0188)
#define MAE_REG_OUTER_CONFIG_BASE_01_1    (MAE_DRV_R_BASE + 0x018C)
#define MAE_REG_OUTER_CONFIG_BASE_02_0    (MAE_DRV_R_BASE + 0x0190)
#define MAE_REG_OUTER_CONFIG_BASE_02_1    (MAE_DRV_R_BASE + 0x0194)

#define MAE_REG_OUTER_CONFIG_OFFSET_00_0  (MAE_DRV_R_BASE + 0x0198)
#define MAE_REG_OUTER_CONFIG_OFFSET_00_1  (MAE_DRV_R_BASE + 0x019C)
#define MAE_REG_OUTER_CONFIG_OFFSET_01_0  (MAE_DRV_R_BASE + 0x01A0)
#define MAE_REG_OUTER_CONFIG_OFFSET_01_1  (MAE_DRV_R_BASE + 0x01A4)
#define MAE_REG_OUTER_CONFIG_OFFSET_02_0  (MAE_DRV_R_BASE + 0x01A8)
#define MAE_REG_OUTER_CONFIG_OFFSET_02_1  (MAE_DRV_R_BASE + 0x01AC)

#define MAE_REG_OUTER_CONFIG_SIZE_00      (MAE_DRV_R_BASE + 0x01B0)
#define MAE_REG_OUTER_CONFIG_SIZE_01      (MAE_DRV_R_BASE + 0x01B4)
#define MAE_REG_OUTER_CONFIG_SIZE_02      (MAE_DRV_R_BASE + 0x01B8)

#define MAE_REG_OUTER_COEF_BASE_00_0      (MAE_DRV_R_BASE + 0x01C0)
#define MAE_REG_OUTER_COEF_BASE_00_1      (MAE_DRV_R_BASE + 0x01C4)
#define MAE_REG_OUTER_COEF_BASE_01_0      (MAE_DRV_R_BASE + 0x01C8)
#define MAE_REG_OUTER_COEF_BASE_01_1      (MAE_DRV_R_BASE + 0x01CC)
#define MAE_REG_OUTER_COEF_BASE_02_0      (MAE_DRV_R_BASE + 0x01D0)
#define MAE_REG_OUTER_COEF_BASE_02_1      (MAE_DRV_R_BASE + 0x01D4)

#define MAE_REG_OUTER_COEF_OFFSET_00_0    (MAE_DRV_R_BASE + 0x01D8)
#define MAE_REG_OUTER_COEF_OFFSET_00_1    (MAE_DRV_R_BASE + 0x01DC)
#define MAE_REG_OUTER_COEF_OFFSET_01_0    (MAE_DRV_R_BASE + 0x01E0)
#define MAE_REG_OUTER_COEF_OFFSET_01_1    (MAE_DRV_R_BASE + 0x01E4)
#define MAE_REG_OUTER_COEF_OFFSET_02_0    (MAE_DRV_R_BASE + 0x01E8)
#define MAE_REG_OUTER_COEF_OFFSET_02_1    (MAE_DRV_R_BASE + 0x01EC)

#define MAE_REG_OUTER_COEF_SIZE_00        (MAE_DRV_R_BASE + 0x01F0)
#define MAE_REG_OUTER_COEF_SIZE_01        (MAE_DRV_R_BASE + 0x01F4)
#define MAE_REG_OUTER_COEF_SIZE_02        (MAE_DRV_R_BASE + 0x01F8)

#define MAE_REG_RESERVE                   (MAE_DRV_R_BASE + 0x01FC)

// fld model size
#define fdvt_fld_blink_weight_forest14_size 6416
#define fdvt_fld_fp_forest00_om45_size 5344
#define fdvt_fld_fp_forest01_om45_size 5344
#define fdvt_fld_fp_forest02_om45_size 5344
#define fdvt_fld_fp_forest03_om45_size 5344
#define fdvt_fld_fp_forest04_om45_size 5344
#define fdvt_fld_fp_forest05_om45_size 5344
#define fdvt_fld_fp_forest06_om45_size 5344
#define fdvt_fld_fp_forest07_om45_size 5344
#define fdvt_fld_fp_forest08_om45_size 5344
#define fdvt_fld_fp_forest09_om45_size 5344
#define fdvt_fld_fp_forest10_om45_size 5344
#define fdvt_fld_fp_forest11_om45_size 5344
#define fdvt_fld_fp_forest12_om45_size 5344
#define fdvt_fld_fp_forest13_om45_size 5344
#define fdvt_fld_fp_forest14_om45_size 5344
#define fdvt_fld_leafnode_forest00_size 307200
#define fdvt_fld_leafnode_forest01_size 307200
#define fdvt_fld_leafnode_forest02_size 307200
#define fdvt_fld_leafnode_forest03_size 307200
#define fdvt_fld_leafnode_forest04_size 307200
#define fdvt_fld_leafnode_forest05_size 307200
#define fdvt_fld_leafnode_forest06_size 307200
#define fdvt_fld_leafnode_forest07_size 307200
#define fdvt_fld_leafnode_forest08_size 307200
#define fdvt_fld_leafnode_forest09_size 307200
#define fdvt_fld_leafnode_forest10_size 307200
#define fdvt_fld_leafnode_forest11_size 307200
#define fdvt_fld_leafnode_forest12_size 307200
#define fdvt_fld_leafnode_forest13_size 307200
#define fdvt_fld_leafnode_forest14_size 307200
#define fdvt_fld_tree_forest00_cv_weight_size 1072
#define fdvt_fld_tree_forest00_init_shape_size 192
#define fdvt_fld_tree_forest00_tree_node_size 16000
#define fdvt_fld_tree_forest01_cv_weight_size 1072
#define fdvt_fld_tree_forest01_tree_node_size 16000
#define fdvt_fld_tree_forest02_cv_weight_size 1072
#define fdvt_fld_tree_forest02_tree_node_size 16000
#define fdvt_fld_tree_forest03_cv_weight_size 1072
#define fdvt_fld_tree_forest03_tree_node_size 16000
#define fdvt_fld_tree_forest04_cv_weight_size 1072
#define fdvt_fld_tree_forest04_tree_node_size 16000
#define fdvt_fld_tree_forest05_cv_weight_size 1072
#define fdvt_fld_tree_forest05_tree_node_size 16000
#define fdvt_fld_tree_forest06_cv_weight_size 1072
#define fdvt_fld_tree_forest06_tree_node_size 16000
#define fdvt_fld_tree_forest07_cv_weight_size 1072
#define fdvt_fld_tree_forest07_tree_node_size 16000
#define fdvt_fld_tree_forest08_cv_weight_size 1072
#define fdvt_fld_tree_forest08_tree_node_size 16000
#define fdvt_fld_tree_forest09_cv_weight_size 1072
#define fdvt_fld_tree_forest09_tree_node_size 16000
#define fdvt_fld_tree_forest10_cv_weight_size 1072
#define fdvt_fld_tree_forest10_tree_node_size 16000
#define fdvt_fld_tree_forest11_cv_weight_size 1072
#define fdvt_fld_tree_forest11_tree_node_size 16000
#define fdvt_fld_tree_forest12_cv_weight_size 1072
#define fdvt_fld_tree_forest12_tree_node_size 16000
#define fdvt_fld_tree_forest13_cv_weight_size 1072
#define fdvt_fld_tree_forest13_tree_node_size 16000
#define fdvt_fld_tree_forest14_cv_weight_size 1072
#define fdvt_fld_tree_forest14_tree_node_size 16000

#endif /* __MTK_MAE_ISP8_H__ */
