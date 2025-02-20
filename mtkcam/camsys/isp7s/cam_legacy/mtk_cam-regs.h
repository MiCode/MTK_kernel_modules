/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CAM_REGS_H
#define _CAM_REGS_H

/* Raw Part */

/* normal siganl */
#define VS_INT_ST						BIT(0)
#define TG_INT1_ST						BIT(1)
#define TG_INT2_ST						BIT(2)
#define EXPDON_ST						BIT(5)
#define SOF_INT_ST						BIT(8)
#define TG_VS_INT_ORG_ST				BIT(11)
#define HW_PASS1_DON_ST					BIT(21)
#define SW_PASS1_DON_ST					BIT(23)

#define DCIF_LAST_SOF_INT_ST			BIT(5)
#define DCIF_FIRST_SOF_INT_ST			BIT(4)
#define P1_DONE_OVER_SOF_INT_ST			BIT(12)

#define P1_SKIP_FRAME_DC_STAG_INT_ST				BIT(13)
#define P1_SKIP_FRAME_1ST_PASS_RGBW_VHDR_INT_ST		BIT(16)
#define P1_SKIP_FRAME_2ND_PASS_RGBW_VHDR_INT_ST		BIT(17)
#define P1_SKIP_FRAME_1ST_PASS_TWO_SENSOR_INT_ST	BIT(18)
#define P1_SKIP_FRAME_2ND_PASS_TWO_SENSOR_INT_ST	BIT(19)
#define P1_SKIP_FRAME_ADL_INT_ST					BIT(28)

#define DCIF_SKIP_MASK (P1_SKIP_FRAME_DC_STAG_INT_ST	 |\
						P1_SKIP_FRAME_1ST_PASS_RGBW_VHDR_INT_ST |\
						P1_SKIP_FRAME_2ND_PASS_RGBW_VHDR_INT_ST |\
						P1_SKIP_FRAME_1ST_PASS_TWO_SENSOR_INT_ST |\
						P1_SKIP_FRAME_2ND_PASS_TWO_SENSOR_INT_ST |\
						P1_SKIP_FRAME_ADL_INT_ST)

/* err status */
#define TG_OVRUN_ST						BIT(6)
#define TG_GBERR_ST						BIT(7)
#define SOF_DROP_ST						BIT(10)

#define CQ_MAX_START_DLY_ERR_INT_ST			BIT(14)
#define CQ_MAX_START_DLY_SMALL_INT_ST			BIT(15)
#define CQ_MAIN_CODE_ERR_ST				BIT(16)
#define CQ_MAIN_VS_ERR_ST				BIT(17)
#define CQ_MAIN_TRIG_DLY_ST				BIT(18)
#define CQ_SUB_CODE_ERR_ST				BIT(19)
#define CQ_SUB_VS_ERR_ST				BIT(20)
#define DMA_ERR_ST						BIT(25)

/* CAM DMA done status */
#define AFO_DONE_ST						BIT(8)

/* RAW input trigger ctrl*/
#define RAWI_R2_TRIG					BIT(0)
#define RAWI_R3_TRIG					BIT(1)
#define RAWI_R5_TRIG					BIT(2)

/* IRQ signal mask */
#define INT_ST_MASK_CAM (VS_INT_ST	 |\
						TG_INT1_ST	 |\
						TG_INT2_ST	 |\
						EXPDON_ST       |\
						HW_PASS1_DON_ST |\
						SOF_INT_ST      |\
						SW_PASS1_DON_ST)

/* IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR (TG_OVRUN_ST	 |\
							TG_GBERR_ST	 |\
							SOF_DROP_ST |\
							CQ_MAIN_CODE_ERR_ST |\
							CQ_MAIN_VS_ERR_ST	 |\
							CQ_MAIN_TRIG_DLY_ST |\
							CQ_SUB_CODE_ERR_ST |\
							CQ_SUB_VS_ERR_ST	 |\
							DMA_ERR_ST)

/* IRQ Debug Mask */
#define INT_ST_MASK_CAM_DBG (SOF_DROP_ST |\
							CQ_MAIN_CODE_ERR_ST |\
							CQ_MAIN_VS_ERR_ST	 |\
							CQ_MAIN_TRIG_DLY_ST |\
							CQ_SUB_CODE_ERR_ST |\
							CQ_SUB_VS_ERR_ST)

/* camsys */
#define REG_CG_CON				0x0000
#define REG_CG_SET				0x0004
#define REG_CG_CLR				0x0008
#define REG_SW_RST				0x00a0

#define REG_HALT1_EN					0x00c4
#define REG_HALT2_EN					0x00c8
#define REG_HALT3_EN					0x00cc
#define REG_HALT4_EN					0x00d0
#define REG_HALT5_EN					0x00d4
#define REG_HALT6_EN					0x00d8
#define REG_HALT7_EN					0x00dc
#define REG_HALT8_EN					0x00e0
#define REG_HALT9_EN					0x00e4
#define REG_HALT10_EN					0x00e8
#define REG_HALT11_EN					0x00ec
#define REG_HALT12_EN					0x00f0
#define REG_HALT13_EN					0x00f4

#define REG_FLASH					0x018c
#define REG_ULTRA_HALT1_EN				0x00f8
#define REG_ULTRA_HALT2_EN				0x00fc
#define REG_ULTRA_HALT3_EN				0x0100
#define REG_ULTRA_HALT4_EN				0x0104
#define REG_ULTRA_HALT5_EN				0x0108
#define REG_ULTRA_HALT6_EN				0x010c
#define REG_ULTRA_HALT7_EN				0x0110
#define REG_ULTRA_HALT8_EN				0x0114
#define REG_ULTRA_HALT9_EN				0x0118
#define REG_ULTRA_HALT10_EN				0x011c
#define REG_ULTRA_HALT11_EN				0x0120
#define REG_ULTRA_HALT12_EN				0x0124
#define REG_ULTRA_HALT13_EN				0x0128

#define REG_PREULTRA_HALT1_EN			0x012c
#define REG_PREULTRA_HALT2_EN			0x0130
#define REG_PREULTRA_HALT3_EN			0x0134
#define REG_PREULTRA_HALT4_EN			0x0138
#define REG_PREULTRA_HALT5_EN			0x013c
#define REG_PREULTRA_HALT6_EN			0x0140
#define REG_PREULTRA_HALT7_EN			0x0144
#define REG_PREULTRA_HALT8_EN			0x0148
#define REG_PREULTRA_HALT9_EN			0x014c
#define REG_PREULTRA_HALT10_EN			0x0150
#define REG_PREULTRA_HALT11_EN			0x0154
#define REG_PREULTRA_HALT12_EN			0x0158
#define REG_PREULTRA_HALT13_EN			0x015c

#define REG_CAMSYS_AXI_MUX			0x0334

/* camsv_a/b */
#define HALT1_EN		0x3
#define HALT2_EN		0x3
/* raw_a/yuv_a */
#define HALT5_EN		0xF90
#define HALT6_EN		0x6F
/* raw_b/yuv_b */
#define HALT7_EN		0xF90
#define HALT8_EN		0x6F
/* raw_c/yuv_c */
#define HALT9_EN		0xF90
#define HALT10_EN		0x6F
/* camsv_c/d/e/f */
#define HALT13_EN		0xFF

/* Status check */
#define REG_MOD_EN						0x0000
#define REG_MOD2_EN						0x0004


/* DMA Enable Register, DMA_EN */
#define	CQI_R1_EN			BIT(6)
#define	CQI_R2_EN			BIT(7)
#define	CQI_R3_EN			BIT(8)
#define	CQI_R4_EN			BIT(9)
#define CQI_ALL_EN (CQI_R1_EN | CQI_R2_EN |\
		    CQI_R3_EN | CQI_R4_EN)

#define REG_CTL_MOD5_EN					0x0010
#define REG_CTL_MOD6_EN					0x0014
/* RAW input trigger*/
#define REG_CTL_RAWI_TRIG				0x00C0

#define REG_CTL_MISC					0x0060
#define CTL_DB_EN						BIT(4)
#define CTL_DB_LOAD_FORCE				BIT(5)

#define REG_CTL_SW_CTL					0x00C4

#define REG_CTL_START					0x00B0

#define REG_CTL_RAW_INT_EN				0x00F0
#define REG_CTL_RAW_INT_STAT			0x00F4
#define REG_CTL_RAW_INT_STATX			0x00F8
#define REG_CTL_RAW_INT2_EN				0x00FC
#define REG_CTL_RAW_INT2_STAT			0x0100
#define REG_CTL_RAW_INT3_STAT			0x0110
#define REG_CTL_RAW_INT4_STAT			0x0120
#define REG_CTL_RAW_INT5_STAT			0x0130
#define REG_CTL_RAW_INT6_EN				0x0138
#define REG_CTL_RAW_INT6_STAT			0x0140
#define REG_CTL_RAW_INT7_EN				0x0148
#define REG_CTL_RAW_INT7_STAT			0x0150

#define REG_CTL_RAW_MOD_DCM_DIS			0x0300
#define REG_CTL_RAW_MOD2_DCM_DIS		0x0304
#define REG_CTL_RAW_MOD3_DCM_DIS		0x0308
#define REG_CTL_RAW_MOD5_DCM_DIS		0x0310
#define REG_CTL_RAW_MOD6_DCM_DIS		0x0314

#define REG_CTL_DBG_SET					0x00E0
#define REG_CTL_DBG_PORT				0x00E4
#define REG_CTL_DBG_SET2				0x00E8

#define REG_DMA_DBG_SEL					0x4070
#define REG_DMA_DBG_PORT				0x4074

#define REG_CTL_RAW_MOD_REQ_STAT		0x0340
#define REG_CTL_RAW_MOD2_REQ_STAT		0x0344
#define REG_CTL_RAW_MOD3_REQ_STAT		0x0348
#define REG_CTL_RAW_MOD4_REQ_STAT		0x034c /* CAMCTL2 */
#define REG_CTL_RAW_MOD5_REQ_STAT		0x0350
#define REG_CTL_RAW_MOD6_REQ_STAT		0x0354

#define REG_CTL_RAW_MOD_RDY_STAT		0x0360
#define REG_CTL_RAW_MOD2_RDY_STAT		0x0364
#define REG_CTL_RAW_MOD3_RDY_STAT		0x0368
#define REG_CTL_RAW_MOD4_RDY_STAT		0x036c /* CAMCTL2 */
#define REG_CTL_RAW_MOD5_RDY_STAT		0x0370
#define REG_CTL_RAW_MOD6_RDY_STAT		0x0374

#define REG_CQ_EN						0x0400
#define REG_SCQ_START_PERIOD			0x0408
#define REG_CQ_THR0_CTL					0x0410
#define REG_CQ_SUB_CQ_EN			    0x06B0
#define REG_CQ_SUB_THR0_CTL				0x06C0

#define REG_CTL_SW_PASS1_DONE			0x00c8
#define SW_DONE_SAMPLE_EN				BIT(8)
#define REG_CTL_SW_SUB_CTL					0x00cc

#define REG_CQ_THR0_BASEADDR				0x0414
#define REG_CQ_THR0_BASEADDR_MSB			0x0418
#define REG_CQ_THR0_DESC_SIZE				0x041C
#define REG_CAMCQ_SCQ_TRIG_TIME				0x040C
#define REG_CQ_SUB_THR0_BASEADDR_2			0x06CC
#define REG_CQ_SUB_THR0_BASEADDR_MSB_2		0x06D0
#define REG_CQ_SUB_THR0_DESC_SIZE_2			0x06D8
#define REG_CQ_SUB_THR0_BASEADDR_1			0x06C4
#define REG_CQ_SUB_THR0_BASEADDR_MSB_1		0x06C8
#define REG_CQ_SUB_THR0_DESC_SIZE_1			0x06D4

#define SCQ_STAGGER_MODE					BIT(12)
#define SCQ_SUBSAMPLE_EN					BIT(21)
#define CQ_DB_EN							BIT(4)
#define CQ_DROP_FRAME_EN					BIT(1)

#define CQ_THR0_MODE_IMMEDIATE				BIT(4)
#define CQ_THR0_MODE_CONTINUOUS				BIT(5)
#define CQ_THR0_EN							0x1

#define REG_TG_SEN_MODE						0x0700
#define TG_CMOS_RDY_SEL						BIT(14)
#define TG_SEN_MODE_CMOS_EN					BIT(0)
#define TG_VFDATA_EN						BIT(0)
#define TG_TG_FULL_SEL						BIT(15)
#define TG_STAGGER_EN						BIT(22)

#define REG_TG_VF_CON						0x0704
#define REG_TG_SEN_GRAB_PXL					0x0708
#define REG_TG_SEN_GRAB_LIN					0x070C
#define REG_TG_PATH_CFG						0x0710
#define TG_DB_LOAD_DIS						BIT(8)

#define REG_TG_INTER_ST						0x073C
/* use this MASK to extract TG_CAM_CS from TG_INTER_ST */
#define TG_CAM_CS_MASK						0x3f00
#define TG_IDLE_ST							BIT(8)


#define REG_TG_FRMSIZE_ST					0x0738
#define REG_TG_DCIF_CTL						0x075C
#define TG_DCIF_EN							BIT(16)

#define REG_TG_FRMSIZE_ST_R					0x076C
#define REG_TG_TIME_STAMP					0x0778
#define REG_TG_TIME_STAMP_CNT				0x077C

/* tg flash */
#define	REG_TG_XENON_FLASH_CTL				0x0780
#define REG_TG_XENON_FLASH_OFFSET			0x0784
#define REG_TG_XENON_FLASH_HIGH_WIDTH			0x0788
#define REG_TG_XENON_FLASH_LOW_WIDTH			0x078C
#define	REG_TG_IR_FLASH_CTL				0x0798
#define REG_TG_IR_FLASH_OFFSET				0x079C
#define REG_TG_IR_FLASH_HIGH_WIDTH			0x07A0
#define REG_TG_IR_FLASH_LOW_WIDTH			0x07A4

#define REG_TG_VSEOL_SUB_CTL				0x0760

/* for raw & yuv's dma top base */
#define CAMDMATOP_BASE						0x4000

#define REG_DMA_SOFT_RST_STAT               0x4068
#define REG_DMA_SOFT_RST_STAT2              0x406C
#define REG_DMA_DBG_CHASING_STATUS          0x4098
#define REG_DMA_DBG_CHASING_STATUS2         0x409c

#define RAWI_R2_SMI_REQ_ST		BIT(0)
#define RAWI_R3_SMI_REQ_ST		BIT(16)
#define RAWI_R5_SMI_REQ_ST		BIT(16)

#define CAMCTL_RAWI_R2_EN		BIT(0)
#define CAMCTL_RAWI_R3_EN		BIT(2)
#define CAMCTL_RAWI_R5_EN		BIT(4)
#define RST_STAT_RAWI_R2		BIT(4)
#define RST_STAT_RAWI_R3		BIT(6)
#define RST_STAT_RAWI_R5		BIT(8)

#define RAW_RST_STAT_CHECK		(0x3fffffff ^ \
					(RST_STAT_RAWI_R2 |\
					 RST_STAT_RAWI_R3 |\
					 RST_STAT_RAWI_R5))
#define RAW_RST_STAT2_CHECK		0x7
#define YUV_RST_STAT_CHECK		0x1efffff


/* use spare register FH_SPARE_5 */
#define REG_FRAME_SEQ_NUM					0x4874

#define REG_CAMCTL_FBC_SEL				0x00A0
#define REG_CAMCTL_FBC_RCNT_INC				0x00A4

#define CAMCTL_IMGO_R1_RCNT_INC				BIT(0)
#define CAMCTL_CQ_THR0_DONE_ST				BIT(0)
#define CAMCTL_CQ_THRSUB_DONE_ST			BIT(4)
#define CTL_CQ_THR0_START					BIT(0)

/* AE debug info */
/* CAMSYS_RAW 0x1a03 */
#define REG_OBC_R1A                    0x1380
#define OFFSET_OBC_R1_R_SUM_L          (REG_OBC_R1A + 0x4)
#define OFFSET_OBC_R1_R_SUM_H          (REG_OBC_R1A + 0x8)
#define OFFSET_OBC_R1_B_SUM_L          (REG_OBC_R1A + 0xC)
#define OFFSET_OBC_R1_B_SUM_H          (REG_OBC_R1A + 0x10)
#define OFFSET_OBC_R1_GR_SUM_L         (REG_OBC_R1A + 0x14)
#define OFFSET_OBC_R1_GR_SUM_H         (REG_OBC_R1A + 0x18)
#define OFFSET_OBC_R1_GB_SUM_L         (REG_OBC_R1A + 0x1C)
#define OFFSET_OBC_R1_GB_SUM_H         (REG_OBC_R1A + 0x20)
#define OFFSET_OBC_R1_ACT_WIN_X        (REG_OBC_R1A + 0x64)
#define OFFSET_OBC_R1_ACT_WIN_Y        (REG_OBC_R1A + 0x68)

#define REG_OBC_R2A                    0x1840
#define OFFSET_OBC_R2_R_SUM_L          (REG_OBC_R2A + 0x4)
#define OFFSET_OBC_R2_R_SUM_H          (REG_OBC_R2A + 0x8)
#define OFFSET_OBC_R2_B_SUM_L          (REG_OBC_R2A + 0xC)
#define OFFSET_OBC_R2_B_SUM_H          (REG_OBC_R2A + 0x10)
#define OFFSET_OBC_R2_GR_SUM_L         (REG_OBC_R2A + 0x14)
#define OFFSET_OBC_R2_GR_SUM_H         (REG_OBC_R2A + 0x18)
#define OFFSET_OBC_R2_GB_SUM_L         (REG_OBC_R2A + 0x1C)
#define OFFSET_OBC_R2_GB_SUM_H         (REG_OBC_R2A + 0x20)
#define OFFSET_OBC_R2_ACT_WIN_X        (REG_OBC_R2A + 0x64)
#define OFFSET_OBC_R2_ACT_WIN_Y        (REG_OBC_R2A + 0x68)

#define REG_OBC_R3A                    0x1D00
#define OFFSET_OBC_R3_R_SUM_L          (REG_OBC_R3A + 0x4)
#define OFFSET_OBC_R3_R_SUM_H          (REG_OBC_R3A + 0x8)
#define OFFSET_OBC_R3_B_SUM_L          (REG_OBC_R3A + 0xC)
#define OFFSET_OBC_R3_B_SUM_H          (REG_OBC_R3A + 0x10)
#define OFFSET_OBC_R3_GR_SUM_L         (REG_OBC_R3A + 0x14)
#define OFFSET_OBC_R3_GR_SUM_H         (REG_OBC_R3A + 0x18)
#define OFFSET_OBC_R3_GB_SUM_L         (REG_OBC_R3A + 0x1C)
#define OFFSET_OBC_R3_GB_SUM_H         (REG_OBC_R3A + 0x20)
#define OFFSET_OBC_R3_ACT_WIN_X        (REG_OBC_R3A + 0x64)
#define OFFSET_OBC_R3_ACT_WIN_Y        (REG_OBC_R3A + 0x68)

#define REG_LTM_AE_DEBUG_B_MSB         0x23f0
#define REG_LTM_AE_DEBUG_B_LSB         0x23f4
#define REG_LTM_AE_DEBUG_GB_MSB        0x23f8
#define REG_LTM_AE_DEBUG_GB_LSB        0x23fc
#define REG_LTM_AE_DEBUG_GR_MSB        0x2400
#define REG_LTM_AE_DEBUG_GR_LSB        0x2404
#define REG_LTM_AE_DEBUG_R_MSB         0x2408
#define REG_LTM_AE_DEBUG_R_LSB         0x240c
#define REG_LTMS_ACT_WIN_X             0x2410
#define REG_LTMS_ACT_WIN_Y             0x2414

#define REG_AA_R_SUM_L                 0x2a68
#define REG_AA_R_SUM_H                 0x2a6c
#define REG_AA_B_SUM_L                 0x2a70
#define REG_AA_B_SUM_H                 0x2a74
#define REG_AA_GR_SUM_L                0x2a78
#define REG_AA_GR_SUM_H                0x2a7c
#define REG_AA_GB_SUM_L                0x2a80
#define REG_AA_GB_SUM_H                0x2a84
#define REG_AA_ACT_WIN_X               0x2a88
#define REG_AA_ACT_WIN_Y               0x2a8c

#define DMA_OFFSET_CON0        0x020
#define DMA_OFFSET_CON1        0x024
#define DMA_OFFSET_CON2        0x028
#define DMA_OFFSET_CON3        0x02c
#define DMA_OFFSET_CON4        0x030
#define DMA_OFFSET_ERR_STAT    0x034

#define DMA_OFFSET_SPECIAL_DCIF    0x03c
#define DC_CAMSV_STAGER_EN         BIT(16)


#define FBC_R1A_BASE               0x3d00
#define FBC_R2A_BASE               0x37c0
#define REG_FBC_CTL1(base, idx)    (base + idx * 8)
#define REG_FBC_CTL2(base, idx)    (base + idx * 8 + 4)
#define WCNT_BIT_MASK				0xFF00
#define CNT_BIT_MASK				0xFF0000
#define TG_FULLSEL_BIT_MASK			0x8000
/* ORIDMA */
/* CAMSYS_RAW 0x1a03 */
#define REG_IMGO_R1_BASE       0x4760
#define REG_IMGO_R1_BASE_MSB   0x4764
#define REG_FHO_R1_BASE        0x4810
#define REG_FHO_R1_SPARE_5	   0x4874
#define REG_FHO_R1_SPARE_6	   0x4878
#define REG_AAHO_R1_BASE       0x49c0
#define REG_PDO_R1_BASE        0x4940
#define REG_AAO_R1_BASE        0x4980
#define REG_AFO_R1_BASE        0x4a00
/* CAMSYS_YUV 0x1a05 */
#define REG_YUVO_R1_BASE       0x4200
#define REG_YUVO_R1_BASE_MSB   0x4204
#define REG_YUVBO_R1_BASE      0x42b0
#define REG_YUVBO_R1_BASE_MSB  0x42b4
#define REG_YUVCO_R1_BASE      0x4360
#define REG_YUVCO_R1_BASE_MSB  0x4364
#define REG_YUVDO_R1_BASE      0x4410
#define REG_YUVDO_R1_BASE_MSB  0x4414
#define REG_YUVO_R3_BASE       0x44c0
#define REG_YUVO_R3_BASE_MSB   0x44c4
#define REG_YUVBO_R3_BASE      0x4570
#define REG_YUVBO_R3_BASE_MSB  0x4574
#define REG_YUVCO_R3_BASE      0x4620
#define REG_YUVCO_R3_BASE_MSB  0x4624
#define REG_YUVDO_R3_BASE      0x46D0
#define REG_YUVDO_R3_BASE_MSB  0x46D4

/* ULCDMA */
/* CAMSYS_RAW 0x1a03 */
#define REG_LTMSO_R1_BASE      0x4ac0
#define REG_LTMSHO_R1_BASE     0x4b00
#define REG_TSFSO_R1_BASE      0x4a40
#define REG_TSFSO_R2_BASE      0x4a80
#define REG_FLKO_R1_BASE       0x4900
#define REG_UFEO_R1_BASE       0x48c0

/* CAMSYS_YUV 0x1a05 */
#define REG_YUVO_R2_BASE       0x4780
#define REG_YUVO_R2_BASE_MSB   0x4784
#define REG_YUVBO_R2_BASE      0x47c0
#define REG_YUVBO_R2_BASE_MSB  0x47c4
#define REG_YUVO_R4_BASE       0x4800
#define REG_YUVO_R4_BASE_MSB   0x4804
#define REG_YUVBO_R4_BASE      0x4840
#define REG_YUVBO_R4_BASE_MSB  0x4844
#define REG_YUVO_R5_BASE       0x4880
#define REG_YUVO_R5_BASE_MSB   0x4884
#define REG_YUVBO_R5_BASE      0x48c0
#define REG_YUVBO_R5_BASE_MSB  0x48c4
#define REG_RZH1N2TO_R1_BASE   0x4b40
#define REG_RZH1N2TO_R1_BASE_MSB    0x4b44
#define REG_RZH1N2TBO_R1_BASE  0x4900
#define REG_RZH1N2TBO_R1_BASE_MSB   0x4904
#define REG_RZH1N2TO_R2_BASE   0x49c0
#define REG_RZH1N2TO_R2_BASE_MSB    0x49c4
#define REG_RZH1N2TO_R3_BASE   0x4b00
#define REG_RZH1N2TO_R3_BASE_MSB    0x4b04
#define REG_RZH1N2TBO_R3_BASE  0x4940
#define REG_RZH1N2TBO_R3_BASE_MSB   0x4944
#define REG_TCYSO_R1_BASE      0x4980
#define REG_TCYSO_R1_BASE_MSB       0x4984
#define REG_DRZS4NO_R1_BASE    0x4a00
#define REG_DRZS4NO_R1_BASE_MSB     0x4a04
#define REG_DRZS4NO_R3_BASE    0x4ac0
#define REG_DRZS4NO_R3_BASE_MSB     0x4ac4
#define REG_DRZH2NO_R8_BASE    0x4a40
#define REG_DRZH2NO_R8_BASE_MSB     0x4a44
#define REG_RGBWI_R1_BASE      0x4b80
#define REG_RGBWI_R1_BASE_MSB       0x4b84

/* CAMSYS_RAW 0x1a03 */
#define REG_RAWI_R2_BASE       0x42c0
#define REG_RAWI_R2_BASE_MSB   0x4204
#define REG_UFDI_R2_BASE       0x4330
#define REG_UFDI_R2_BASE_MSB   0x4334
#define REG_RAWI_R3_BASE       0x43a0
#define REG_RAWI_R3_BASE_MSB   0x43a4
#define REG_UFDI_R3_BASE       0x4410
#define REG_UFDI_R3_BASE_MSB   0x4414
#define REG_RAWI_R5_BASE       0x4480
#define REG_RAWI_R5_BASE_MSB   0x4484
#define REG_UFDI_R5_BASE       0x44f0
#define REG_UFDI_R5_BASE_MSB   0x44f4
#define REG_CQI_R1_BASE        0x4100
#define REG_CQI_R1_BASE_MSB    0x4104
#define REG_CQI_R2_BASE        0x4170
#define REG_CQI_R2_BASE_MSB    0x4174
#define REG_CQI_R3_BASE        0x41e0
#define REG_CQI_R3_BASE_MSB    0x41e4
#define REG_CQI_R4_BASE        0x4250
#define REG_CQI_R4_BASE_MSB    0x4254
#define REG_LSCI_R1_BASE       0x4660
#define REG_BPCI_R1_BASE       0x4560
#define REG_BPCI_R2_BASE       0x45a0
#define REG_BPCI_R3_BASE       0x45e0
#define REG_PDI_R1_BASE        0x46a0
#define REG_AAI_R1_BASE        0x46e0
#define REG_CACI_R1_BASE       0x4720
#define REG_DRZB2NO_R1_BASE    0x4b40
#define REG_DRZB2NBO_R1_BASE   0x4b80
#define REG_DRZB2NCO_R1_BASE   0x4bc0

#endif	/* _CAM_REGS_H */
