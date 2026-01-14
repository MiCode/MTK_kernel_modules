/* SPDX-License-Identifier: GPL-2.0 */
//
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_AIE_REG_7_H__
#define __MTK_AIE_REG_7_H__

/* AIE 3.0 register offset */
#define AIE_START_REG 0x000
#define AIE_ENABLE_REG 0x004
#define AIE_LOOP_REG 0x008
#define AIE_YUV2RGB_CON_BASE_ADR_REG 0x00c
#define AIE_RS_CON_BASE_ADR_REG 0x010
#define AIE_FD_CON_BASE_ADR_REG 0x014
#define AIE_INT_EN_REG 0x018
#define AIE_INT_REG 0x01c
#define FDVT_YUV2RGB_CON 0x020
#define FDVT_SRC_WD_HT 0x040
#define FDVT_DES_WD_HT 0x044
#define AIE_RESULT_0_REG 0x08c
#define AIE_RESULT_1_REG 0x090
#define AIE_DMA_CTL_REG 0x094
#define FDVT_DEBUG_INFO_0 0x10c
#define FDVT_DEBUG_INFO_1 0x110
#define FDVT_DEBUG_INFO_2 0x114

#define FDVT_YUV2RGB_CON_BASE_ADR_MSB    0x14C
#define FDVT_RS_CON_BASE_ADR_MSB         0x150
#define FDVT_FD_CON_BASE_ADR_MSB         0x154

#define FDVT_CTRL_REG      0x098
#define FDVT_IN_BASE_ADR_0 0x09c
#define FDVT_IN_BASE_ADR_1 0x0a0
#define FDVT_IN_BASE_ADR_2 0x0a4
#define FDVT_IN_BASE_ADR_3 0x0a8
#define FDVT_OUT_BASE_ADR_0 0x0ac
#define FDVT_OUT_BASE_ADR_1 0x0b0
#define FDVT_OUT_BASE_ADR_2 0x0b4
#define FDVT_OUT_BASE_ADR_3 0x0b8
#define FDVT_KERNEL_BASE_ADR_0 0x0bc
#define FDVT_KERNEL_BASE_ADR_1 0x0c0
#define DMA_DEBUG_SEL_REG 0x3f4

#define FDVT_DMA_RDMA_0_CHECK_SUM 0x240
#define FDVT_DMA_RDMA_1_CHECK_SUM 0x244
#define FDVT_DMA_RDMA_2_CHECK_SUM 0x248
#define FDVT_DMA_RDMA_3_CHECK_SUM 0x24c

#define FDVT_DMA_WDMA_0_CHECK_SUM 0x250
#define FDVT_DMA_WDMA_1_CHECK_SUM 0x254
#define FDVT_DMA_WDMA_2_CHECK_SUM 0x258
#define FDVT_DMA_WDMA_3_CHECK_SUM 0x25c

#define FDVT_DMA_DEBUG_SEL 0x278
#define FDVT_DMA_DEBUG_DATA_RDMA_0_0 0x280
#define FDVT_DMA_DEBUG_DATA_RDMA_0_1 0x284
#define FDVT_DMA_DEBUG_DATA_RDMA_0_2 0x288
#define FDVT_DMA_DEBUG_DATA_RDMA_0_3 0x28c

#define FDVT_DMA_DEBUG_DATA_RDMA_1_0 0x290
#define FDVT_DMA_DEBUG_DATA_RDMA_1_1 0x294
#define FDVT_DMA_DEBUG_DATA_RDMA_1_2 0x298
#define FDVT_DMA_DEBUG_DATA_RDMA_1_3 0x29c

#define FDVT_DMA_DEBUG_DATA_RDMA_2_0 0x2a0
#define FDVT_DMA_DEBUG_DATA_RDMA_2_1 0x2a4
#define FDVT_DMA_DEBUG_DATA_RDMA_2_2 0x2a8
#define FDVT_DMA_DEBUG_DATA_RDMA_2_3 0x2ac

#define FDVT_DMA_DEBUG_DATA_RDMA_3_0 0x2b0
#define FDVT_DMA_DEBUG_DATA_RDMA_3_1 0x2b4
#define FDVT_DMA_DEBUG_DATA_RDMA_3_2 0x2b8
#define FDVT_DMA_DEBUG_DATA_RDMA_3_3 0x2bc

#define FDVT_DMA_DEBUG_DATA_WDMA_0_0 0x2c0
#define FDVT_DMA_DEBUG_DATA_WDMA_0_1 0x2c4
#define FDVT_DMA_DEBUG_DATA_WDMA_0_2 0x2c8
#define FDVT_DMA_DEBUG_DATA_WDMA_0_3 0x2cc

#define FDVT_DMA_DEBUG_DATA_WDMA_1_0 0x2d0
#define FDVT_DMA_DEBUG_DATA_WDMA_1_1 0x2d4
#define FDVT_DMA_DEBUG_DATA_WDMA_1_2 0x2d8
#define FDVT_DMA_DEBUG_DATA_WDMA_1_3 0x2dc

#define FDVT_DMA_DEBUG_DATA_WDMA_2_0 0x2e0
#define FDVT_DMA_DEBUG_DATA_WDMA_2_1 0x2e4
#define FDVT_DMA_DEBUG_DATA_WDMA_2_2 0x2e8
#define FDVT_DMA_DEBUG_DATA_WDMA_2_3 0x2ec

#define FDVT_DMA_DEBUG_DATA_WDMA_3_0 0x2f0
#define FDVT_DMA_DEBUG_DATA_WDMA_3_1 0x2f4
#define FDVT_DMA_DEBUG_DATA_WDMA_3_2 0x2f8
#define FDVT_DMA_DEBUG_DATA_WDMA_3_3 0x2fc
#define FDVT_DMA_ERR_STATUS 0x300

#define FDVT_WRA_0_CON3_REG 0x254
#define FDVT_WRA_1_CON3_REG 0x284

#define FDVT_RDA_0_CON3_REG 0x2b4
#define FDVT_RDA_1_CON3_REG 0x2e4

#define FDVT_WRB_0_CON3_REG 0x314
#define FDVT_WRB_1_CON3_REG 0x344

#define FDVT_RDB_0_CON3_REG 0x374
#define FDVT_RDB_1_CON3_REG 0x3a4

#define FDVT_BASE_HW                        0x15310000
#define FDVT_START_HW                      (FDVT_BASE_HW + 0x000)
#define FDVT_ENABLE_HW                     (FDVT_BASE_HW + 0x004)
#define FDVT_LOOP_HW                       (FDVT_BASE_HW + 0x008)
#define FDVT_YUV2RGB_CON_BASE_ADR_HW       (FDVT_BASE_HW + 0x00c)
#define FDVT_RS_CON_BASE_ADR_HW            (FDVT_BASE_HW + 0x010)
#define FDVT_FD_CON_BASE_ADR_HW            (FDVT_BASE_HW + 0x014)
#define FDVT_INT_EN_HW                     (FDVT_BASE_HW + 0x018)
#define FDVT_INT_HW                        (FDVT_BASE_HW + 0x01c)
#define FDVT_YUV2RGB_CON_HW                (FDVT_BASE_HW + 0x020)
#define FDVT_RS_CON_HW                     (FDVT_BASE_HW + 0x024)
#define FDVT_RS_FDRZ_CON0_HW               (FDVT_BASE_HW + 0x028)
#define FDVT_RS_FDRZ_CON1_HW               (FDVT_BASE_HW + 0x02c)
#define FDVT_RS_SRZ_CON0_HW                (FDVT_BASE_HW + 0x030)
#define FDVT_RS_SRZ_CON1_HW                (FDVT_BASE_HW + 0x034)
#define FDVT_RS_SRZ_CON2_HW                (FDVT_BASE_HW + 0x038)
#define FDVT_RS_SRZ_CON3_HW                (FDVT_BASE_HW + 0x03c)
#define FDVT_SRC_WD_HT_HW                  (FDVT_BASE_HW + 0x040)
#define FDVT_DES_WD_HT_HW                  (FDVT_BASE_HW + 0x044)
#define FDVT_CONV_WD_HT_HW                 (FDVT_BASE_HW + 0x048)
#define FDVT_KERNEL_HW                     (FDVT_BASE_HW + 0x04c)
#define FDVT_FD_PACK_MODE_HW               (FDVT_BASE_HW + 0x050)
#define FDVT_CONV0_HW                      (FDVT_BASE_HW + 0x054)
#define FDVT_CONV1_HW                      (FDVT_BASE_HW + 0x058)
#define FDVT_CONV2_HW                      (FDVT_BASE_HW + 0x05c)
#define FDVT_RPN_HW                        (FDVT_BASE_HW + 0x060)
#define FDVT_RPN_IMAGE_COORD_HW            (FDVT_BASE_HW + 0x064)
#define FDVT_FD_ANCHOR_0_HW                (FDVT_BASE_HW + 0x068)
#define FDVT_FD_ANCHOR_1_HW                (FDVT_BASE_HW + 0x06c)
#define FDVT_FD_ANCHOR_2_HW                (FDVT_BASE_HW + 0x070)
#define FDVT_FD_ANCHOR_3_HW                (FDVT_BASE_HW + 0x074)
#define FDVT_FD_ANCHOR_4_HW                (FDVT_BASE_HW + 0x078)
#define FDVT_ANCHOR_SHIFT_MODE_0_HW        (FDVT_BASE_HW + 0x07c)
#define FDVT_ANCHOR_SHIFT_MODE_1_HW        (FDVT_BASE_HW + 0x080)
#define FDVT_LANDMARK_SHIFT_MODE_0_HW      (FDVT_BASE_HW + 0x084)
#define FDVT_LANDMARK_SHIFT_MODE_1_HW      (FDVT_BASE_HW + 0x088)
#define FDVT_RESULT_0_HW                   (FDVT_BASE_HW + 0x08c)
#define FDVT_RESULT_1_HW                   (FDVT_BASE_HW + 0x090)
#define FDVT_DMA_CTL_HW                    (FDVT_BASE_HW + 0x094)
#define FDVT_CTRL_HW                       (FDVT_BASE_HW + 0x098)
#define FDVT_IN_BASE_ADR_0_HW              (FDVT_BASE_HW + 0x09c)
#define FDVT_IN_BASE_ADR_1_HW              (FDVT_BASE_HW + 0x0a0)
#define FDVT_IN_BASE_ADR_2_HW              (FDVT_BASE_HW + 0x0a4)
#define FDVT_IN_BASE_ADR_3_HW              (FDVT_BASE_HW + 0x0a8)
#define FDVT_OUT_BASE_ADR_0_HW             (FDVT_BASE_HW + 0x0ac)
#define FDVT_OUT_BASE_ADR_1_HW             (FDVT_BASE_HW + 0x0b0)
#define FDVT_OUT_BASE_ADR_2_HW             (FDVT_BASE_HW + 0x0b4)
#define FDVT_OUT_BASE_ADR_3_HW             (FDVT_BASE_HW + 0x0b8)
#define FDVT_KERNEL_BASE_ADR_0_HW          (FDVT_BASE_HW + 0x0bc)
#define FDVT_KERNEL_BASE_ADR_1_HW          (FDVT_BASE_HW + 0x0c0)
#define FDVT_IN_SIZE_0_HW                  (FDVT_BASE_HW + 0x0c4)
#define FDVT_IN_STRIDE_0_HW                (FDVT_BASE_HW + 0x0c8)
#define FDVT_IN_SIZE_1_HW                  (FDVT_BASE_HW + 0x0cc)
#define FDVT_IN_STRIDE_1_HW                (FDVT_BASE_HW + 0x0d0)
#define FDVT_IN_SIZE_2_HW                  (FDVT_BASE_HW + 0x0d4)
#define FDVT_IN_STRIDE_2_HW                (FDVT_BASE_HW + 0x0d8)
#define FDVT_IN_SIZE_3_HW                  (FDVT_BASE_HW + 0x0dc)
#define FDVT_IN_STRIDE_3_HW                (FDVT_BASE_HW + 0x0e0)
#define FDVT_OUT_SIZE_0_HW                 (FDVT_BASE_HW + 0x0e4)
#define FDVT_OUT_STRIDE_0_HW               (FDVT_BASE_HW + 0x0e8)
#define FDVT_OUT_SIZE_1_HW                 (FDVT_BASE_HW + 0x0ec)
#define FDVT_OUT_STRIDE_1_HW               (FDVT_BASE_HW + 0x0f0)
#define FDVT_OUT_SIZE_2_HW                 (FDVT_BASE_HW + 0x0f4)
#define FDVT_OUT_STRIDE_2_HW               (FDVT_BASE_HW + 0x0f8)
#define FDVT_OUT_SIZE_3_HW                 (FDVT_BASE_HW + 0x0fc)
#define FDVT_OUT_STRIDE_3_HW               (FDVT_BASE_HW + 0x100)
#define FDVT_KERNEL_SIZE_HW                (FDVT_BASE_HW + 0x104)
#define FDVT_KERNEL_STRIDE_HW              (FDVT_BASE_HW + 0x108)
#define FDVT_DEBUG_INFO_0_HW               (FDVT_BASE_HW + 0x10c)
#define FDVT_DEBUG_INFO_1_HW               (FDVT_BASE_HW + 0x110)
#define FDVT_DEBUG_INFO_2_HW               (FDVT_BASE_HW + 0x114)
#define FDVT_SPARE_CELL_HW                 (FDVT_BASE_HW + 0x118)
#define FDVT_VERSION_HW                    (FDVT_BASE_HW + 0x11c)
#define FDVT_PADDING_CON0_HW               (FDVT_BASE_HW + 0x120)
#define FDVT_PADDING_CON1_HW               (FDVT_BASE_HW + 0x124)
#define FDVT_SECURE_REGISTER               (FDVT_BASE_HW + 0x13C)
#define DMA_SOFT_RSTSTAT_HW                (FDVT_BASE_HW + 0x200)
#define TDRI_BASE_ADDR_HW                  (FDVT_BASE_HW + 0x204)
#define TDRI_OFST_ADDR_HW                  (FDVT_BASE_HW + 0x208)
#define TDRI_XSIZE_HW                      (FDVT_BASE_HW + 0x20c)
#define VERTICAL_FLIP_EN_HW                (FDVT_BASE_HW + 0x210)
#define DMA_SOFT_RESET_HW                  (FDVT_BASE_HW + 0x214)
#define LAST_ULTRA_EN_HW                   (FDVT_BASE_HW + 0x218)
#define SPECIAL_FUN_EN_HW                  (FDVT_BASE_HW + 0x21c)
#define FDVT_WRA_0_BASE_ADDR_HW            (FDVT_BASE_HW + 0x230)
#define FDVT_WRA_0_OFST_ADDR_HW            (FDVT_BASE_HW + 0x238)

#define FDVT_WRB_0_YSIZE_HW                (FDVT_BASE_HW + 0x304)
#define FDVT_WRB_0_STRIDE_HW               (FDVT_BASE_HW + 0x308)
#define FDVT_WRB_0_CON_HW                  (FDVT_BASE_HW + 0x30c)
#define FDVT_WRB_0_CON2_HW                 (FDVT_BASE_HW + 0x310)
#define FDVT_WRB_0_CON3_HW                 (FDVT_BASE_HW + 0x314)
#define FDVT_WRB_0_CROP_HW                 (FDVT_BASE_HW + 0x318)
#define FDVT_WRB_1_BASE_ADDR_HW            (FDVT_BASE_HW + 0x320)
#define FDVT_WRB_1_OFST_ADDR_HW            (FDVT_BASE_HW + 0x328)
#define FDVT_WRB_1_XSIZE_HW                (FDVT_BASE_HW + 0x330)
#define FDVT_WRB_1_YSIZE_HW                (FDVT_BASE_HW + 0x334)
#define FDVT_WRB_1_STRIDE_HW               (FDVT_BASE_HW + 0x338)
#define FDVT_WRB_1_CON_HW                  (FDVT_BASE_HW + 0x33c)
#define FDVT_WRB_1_CON2_HW                 (FDVT_BASE_HW + 0x340)
#define FDVT_WRB_1_CON3_HW                 (FDVT_BASE_HW + 0x344)
#define FDVT_WRB_1_CROP_HW                 (FDVT_BASE_HW + 0x348)
#define FDVT_RDB_0_BASE_ADDR_HW            (FDVT_BASE_HW + 0x350)
#define FDVT_RDB_0_OFST_ADDR_HW            (FDVT_BASE_HW + 0x358)
#define FDVT_RDB_0_XSIZE_HW                (FDVT_BASE_HW + 0x360)
#define FDVT_RDB_0_YSIZE_HW                (FDVT_BASE_HW + 0x364)
#define FDVT_RDB_0_STRIDE_HW               (FDVT_BASE_HW + 0x368)
#define FDVT_RDB_0_CON_HW                  (FDVT_BASE_HW + 0x36c)
#define FDVT_RDB_0_CON2_HW                 (FDVT_BASE_HW + 0x370)
#define FDVT_RDB_0_CON3_HW                 (FDVT_BASE_HW + 0x374)
#define FDVT_RDB_1_BASE_ADDR_HW            (FDVT_BASE_HW + 0x380)
#define FDVT_RDB_1_OFST_ADDR_HW            (FDVT_BASE_HW + 0x388)
#define FDVT_RDB_1_XSIZE_HW                (FDVT_BASE_HW + 0x390)
#define FDVT_RDB_1_YSIZE_HW                (FDVT_BASE_HW + 0x394)
#define FDVT_RDB_1_STRIDE_HW               (FDVT_BASE_HW + 0x398)
#define FDVT_RDB_1_CON_HW                  (FDVT_BASE_HW + 0x39c)
#define FDVT_RDB_1_CON2_HW                 (FDVT_BASE_HW + 0x3a0)
#define FDVT_RDB_1_CON3_HW                 (FDVT_BASE_HW + 0x3a4)
#define DMA_ERR_CTRL_HW                    (FDVT_BASE_HW + 0x3b0)
#define FDVT_WRA_0_ERR_STAT_HW             (FDVT_BASE_HW + 0x3b4)
#define FDVT_WRA_1_ERR_STAT_HW             (FDVT_BASE_HW + 0x3b8)
#define FDVT_WRB_0_ERR_STAT_HW             (FDVT_BASE_HW + 0x3bc)
#define FDVT_WRB_1_ERR_STAT_HW             (FDVT_BASE_HW + 0x3c0)
#define FDVT_RDA_0_ERR_STAT_HW             (FDVT_BASE_HW + 0x3c4)
#define FDVT_RDA_1_ERR_STAT_HW             (FDVT_BASE_HW + 0x3c8)
#define FDVT_RDB_0_ERR_STAT_HW             (FDVT_BASE_HW + 0x3cc)
#define FDVT_RDB_1_ERR_STAT_HW             (FDVT_BASE_HW + 0x3d0)
#define DMA_DEBUG_ADDR_HW                  (FDVT_BASE_HW + 0x3e0)
#define DMA_RSV1_HW                        (FDVT_BASE_HW + 0x3e4)
#define DMA_RSV2_HW                        (FDVT_BASE_HW + 0x3e8)
#define DMA_RSV3_HW                        (FDVT_BASE_HW + 0x3ec)
#define DMA_RSV4_HW                        (FDVT_BASE_HW + 0x3f0)
#define DMA_DEBUG_SEL_HW                   (FDVT_BASE_HW + 0x3f4)
#define DMA_BW_SELF_TEST_HW                (FDVT_BASE_HW + 0x3f8)

/* AIE 3.0 FLD register offset */
#define FLD_IMG_BASE_ADDR		0x400
#define FLD_MS_BASE_ADDR		0x404
#define FLD_FP_BASE_ADDR		0x408
#define FLD_TR_BASE_ADDR		0x40C
#define FLD_SH_BASE_ADDR		0x410
#define FLD_CV_BASE_ADDR		0x414
#define FLD_BS_BASE_ADDR		0x418
#define FLD_PP_BASE_ADDR		0x41C
#define FLD_FP_FORT_OFST		0x420
#define FLD_TR_FORT_OFST		0x424
#define FLD_SH_FORT_OFST		0x428
#define FLD_CV_FORT_OFST		0x42C

#define FLD_FACE_0_INFO_0		0x430
#define FLD_FACE_0_INFO_1		0x434
#define FLD_FACE_1_INFO_0		0x438
#define FLD_FACE_1_INFO_1		0x43C
#define FLD_FACE_2_INFO_0		0x440
#define FLD_FACE_2_INFO_1		0x444
#define FLD_FACE_3_INFO_0		0x448
#define FLD_FACE_3_INFO_1		0x44C
#define FLD_FACE_4_INFO_0		0x450
#define FLD_FACE_4_INFO_1		0x454
#define FLD_FACE_5_INFO_0		0x458
#define FLD_FACE_5_INFO_1		0x45C
#define FLD_FACE_6_INFO_0		0x460
#define FLD_FACE_6_INFO_1		0x464
#define FLD_FACE_7_INFO_0		0x468
#define FLD_FACE_7_INFO_1		0x46C
#define FLD_FACE_8_INFO_0		0x470
#define FLD_FACE_8_INFO_1		0x474
#define FLD_FACE_9_INFO_0		0x478
#define FLD_FACE_9_INFO_1		0x47C
#define FLD_FACE_10_INFO_0		0x480
#define FLD_FACE_10_INFO_1		0x484
#define FLD_FACE_11_INFO_0		0x488
#define FLD_FACE_11_INFO_1		0x48C
#define FLD_FACE_12_INFO_0		0x490
#define FLD_FACE_12_INFO_1		0x494
#define FLD_FACE_13_INFO_0		0x498
#define FLD_FACE_13_INFO_1		0x49C
#define FLD_FACE_14_INFO_0		0x4A0
#define FLD_FACE_14_INFO_1		0x4A4

/* FLD CMDQ FACE INFO */
#define FLD_CMDQ_FACE_0_INFO_0		(FDVT_BASE_HW + 0x430)
#define FLD_CMDQ_FACE_0_INFO_1		(FDVT_BASE_HW + 0x434)
#define FLD_CMDQ_FACE_1_INFO_0		(FDVT_BASE_HW + 0x438)
#define FLD_CMDQ_FACE_1_INFO_1		(FDVT_BASE_HW + 0x43C)
#define FLD_CMDQ_FACE_2_INFO_0		(FDVT_BASE_HW + 0x440)
#define FLD_CMDQ_FACE_2_INFO_1		(FDVT_BASE_HW + 0x444)
#define FLD_CMDQ_FACE_3_INFO_0		(FDVT_BASE_HW + 0x448)
#define FLD_CMDQ_FACE_3_INFO_1		(FDVT_BASE_HW + 0x44C)
#define FLD_CMDQ_FACE_4_INFO_0		(FDVT_BASE_HW + 0x450)
#define FLD_CMDQ_FACE_4_INFO_1		(FDVT_BASE_HW + 0x454)
#define FLD_CMDQ_FACE_5_INFO_0		(FDVT_BASE_HW + 0x458)
#define FLD_CMDQ_FACE_5_INFO_1		(FDVT_BASE_HW + 0x45C)
#define FLD_CMDQ_FACE_6_INFO_0		(FDVT_BASE_HW + 0x460)
#define FLD_CMDQ_FACE_6_INFO_1		(FDVT_BASE_HW + 0x464)
#define FLD_CMDQ_FACE_7_INFO_0		(FDVT_BASE_HW + 0x468)
#define FLD_CMDQ_FACE_7_INFO_1		(FDVT_BASE_HW + 0x46C)
#define FLD_CMDQ_FACE_8_INFO_0		(FDVT_BASE_HW + 0x470)
#define FLD_CMDQ_FACE_8_INFO_1		(FDVT_BASE_HW + 0x474)
#define FLD_CMDQ_FACE_9_INFO_0		(FDVT_BASE_HW + 0x478)
#define FLD_CMDQ_FACE_9_INFO_1		(FDVT_BASE_HW + 0x47C)
#define FLD_CMDQ_FACE_10_INFO_0		(FDVT_BASE_HW + 0x480)
#define FLD_CMDQ_FACE_10_INFO_1		(FDVT_BASE_HW + 0x484)
#define FLD_CMDQ_FACE_11_INFO_0		(FDVT_BASE_HW + 0x488)
#define FLD_CMDQ_FACE_11_INFO_1		(FDVT_BASE_HW + 0x48C)
#define FLD_CMDQ_FACE_12_INFO_0		(FDVT_BASE_HW + 0x490)
#define FLD_CMDQ_FACE_12_INFO_1		(FDVT_BASE_HW + 0x494)
#define FLD_CMDQ_FACE_13_INFO_0		(FDVT_BASE_HW + 0x498)
#define FLD_CMDQ_FACE_13_INFO_1		(FDVT_BASE_HW + 0x49C)
#define FLD_CMDQ_FACE_14_INFO_0		(FDVT_BASE_HW + 0x4A0)
#define FLD_CMDQ_FACE_14_INFO_1		(FDVT_BASE_HW + 0x4A4)

#define FLD_NUM_CONFIG_0		0x4A8
#define FLD_FACE_NUM			0x4AC
#define FLD_CMDQ_FACE_NUM		(FDVT_BASE_HW + 0x4AC)

#define FLD_PCA_MEAN_SCALE_0	0x4B0
#define FLD_PCA_MEAN_SCALE_1	0x4B4
#define FLD_PCA_MEAN_SCALE_2	0x4B8
#define FLD_PCA_MEAN_SCALE_3	0x4BC
#define FLD_PCA_MEAN_SCALE_4	0x4C0
#define FLD_PCA_MEAN_SCALE_5	0x4C4
#define FLD_PCA_MEAN_SCALE_6	0x4C8
#define FLD_PCA_VEC_0			0x4CC
#define FLD_PCA_VEC_1			0x4D0
#define FLD_PCA_VEC_2			0x4D4
#define FLD_PCA_VEC_3			0x4D8
#define FLD_PCA_VEC_4			0x4DC
#define FLD_PCA_VEC_5			0x4E0
#define FLD_PCA_VEC_6			0x4E4
#define FLD_CV_BIAS_FR_0		0x4E8
#define FLD_CV_BIAS_PF_0		0x4EC
#define FLD_CV_RANGE_FR_0		0x4F0
#define FLD_CV_RANGE_FR_1		0x4F4
#define FLD_CV_RANGE_PF_0		0x4F8
#define FLD_CV_RANGE_PF_1		0x4FC
#define FLD_PP_COEF				0x500
#define FLD_SRC_SIZE			0x504
#define FLD_CMDQ_SRC_SIZE		(FDVT_BASE_HW + 0x504)
#define FLD_SRC_PITCH			0x508
#define FLD_CMDQ_SRC_PITCH		(FDVT_BASE_HW + 0x508)
#define FLD_BS_CONFIG0			0x50C
#define FLD_BS_CONFIG1			0x510
#define FLD_BS_CONFIG2			0x514
#define FLD_FLD_BUSY			0x518
#define FLD_FLD_DONE			0x51C
#define FLD_FLD_SECURE			0x520
#define FLD_FLD_SECURE_W		0x524
#define FLD_FLD_DBG_SEL			0x528
#define FLD_FLD_STM_DBG_DATA0	0x52C
#define FLD_FLD_STM_DBG_DATA1	0x530
#define FLD_FLD_STM_DBG_DATA2	0x534
#define FLD_FLD_STM_DBG_DATA3	0x538
#define FLD_FLD_SH_DBG_DATA0	0x53C
#define FLD_FLD_SH_DBG_DATA1	0x540
#define FLD_FLD_SH_DBG_DATA2	0x544
#define FLD_FLD_SH_DBG_DATA3	0x548
#define FLD_FLD_SH_DBG_DATA4	0x54C
#define FLD_FLD_SH_DBG_DATA5	0x550
#define FLD_FLD_SH_DBG_DATA6	0x554
#define FLD_FLD_SH_DBG_DATA7	0x558

static const unsigned int fld_face_info_idx_0[FLD_MAX_INPUT] = {
	FLD_FACE_0_INFO_0, FLD_FACE_1_INFO_0, FLD_FACE_2_INFO_0,
	FLD_FACE_3_INFO_0, FLD_FACE_4_INFO_0, FLD_FACE_5_INFO_0,
	FLD_FACE_6_INFO_0, FLD_FACE_7_INFO_0, FLD_FACE_8_INFO_0,
	FLD_FACE_9_INFO_0, FLD_FACE_10_INFO_0, FLD_FACE_11_INFO_0,
	FLD_FACE_12_INFO_0, FLD_FACE_13_INFO_0, FLD_FACE_14_INFO_0
};

static const unsigned int fld_face_info_idx_1[FLD_MAX_INPUT] = {
	FLD_FACE_0_INFO_1, FLD_FACE_1_INFO_1, FLD_FACE_2_INFO_1,
	FLD_FACE_3_INFO_1, FLD_FACE_4_INFO_1, FLD_FACE_5_INFO_1,
	FLD_FACE_6_INFO_1, FLD_FACE_7_INFO_1, FLD_FACE_8_INFO_1,
	FLD_FACE_9_INFO_1, FLD_FACE_10_INFO_1, FLD_FACE_11_INFO_1,
	FLD_FACE_12_INFO_1, FLD_FACE_13_INFO_1, FLD_FACE_14_INFO_1
};

static const unsigned int fld_face_info_cmdq_idx_0[FLD_MAX_INPUT] = {
	FLD_CMDQ_FACE_0_INFO_0, FLD_CMDQ_FACE_1_INFO_0, FLD_CMDQ_FACE_2_INFO_0,
	FLD_CMDQ_FACE_3_INFO_0, FLD_CMDQ_FACE_4_INFO_0, FLD_CMDQ_FACE_5_INFO_0,
	FLD_CMDQ_FACE_6_INFO_0, FLD_CMDQ_FACE_7_INFO_0, FLD_CMDQ_FACE_8_INFO_0,
	FLD_CMDQ_FACE_9_INFO_0, FLD_CMDQ_FACE_10_INFO_0, FLD_CMDQ_FACE_11_INFO_0,
	FLD_CMDQ_FACE_12_INFO_0, FLD_CMDQ_FACE_13_INFO_0, FLD_CMDQ_FACE_14_INFO_0
};

static const unsigned int fld_face_info_cmdq_idx_1[FLD_MAX_INPUT] = {
	FLD_CMDQ_FACE_0_INFO_1, FLD_CMDQ_FACE_1_INFO_1, FLD_CMDQ_FACE_2_INFO_1,
	FLD_CMDQ_FACE_3_INFO_1, FLD_CMDQ_FACE_4_INFO_1, FLD_CMDQ_FACE_5_INFO_1,
	FLD_CMDQ_FACE_6_INFO_1, FLD_CMDQ_FACE_7_INFO_1, FLD_CMDQ_FACE_8_INFO_1,
	FLD_CMDQ_FACE_9_INFO_1, FLD_CMDQ_FACE_10_INFO_1, FLD_CMDQ_FACE_11_INFO_1,
	FLD_CMDQ_FACE_12_INFO_1, FLD_CMDQ_FACE_13_INFO_1, FLD_CMDQ_FACE_14_INFO_1
};

#endif /*__MTK_AIE_REG_7_H__*/
