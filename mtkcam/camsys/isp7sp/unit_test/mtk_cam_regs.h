/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Bibby Hsieh <bibby.hsieh@mediatek.com>
 */

/* normal siganl */
#define VS_INT_ST			(1L << 0)
#define TG_INT1_ST			(1L << 1)
#define TG_INT2_ST			(1L << 2)
#define EXPDON_ST			(1L << 5)
#define SOF_INT_ST			(1L << 8)
#define HW_PASS1_DON_ST			(1L << 21)
#define SW_PASS1_DON_ST			(1L << 23)

/* YUV siganl */
#define YUV_PASS1_DON_ST		(1L << 0)
#define YUV_DON_ST			(1L << 1)
#define YUV_DMA_ERR_ST			(1L << 2)

/* err status */
#define TG_OVRUN_ST			(1L << 6)
#define TG_GBERR_ST			(1L << 7)
//#define CQ_DB_LOAD_ERR_ST		(1L << 12)
#define CQ_MAIN_CODE_ERR_ST		(1L << 16)
#define CQ_MAIN_VS_ERR_ST		(1L << 17)
#define CQ_MAIN_TRIG_DLY_ST		(1L << 18)
//#define LSCI_ERR_ST			(1L << 24)
#define DMA_ERR_ST			(1L << 25)

/* CAM DMA done status */
#define IMGO_DONE_ST			(1L << 0)
//#define CQI_R1_DONE_ST			(1L << 8)

#define DCIF_LAST_SOF_INT_ST		(1L << 5)
#define DCIF_LAST_CQ_START_INT_ST	(1L << 7)

enum topdebug_event {
	ALL_THE_TIME	= 1 << 0,
	TG_OVERRUN	= 1 << 1,
	CQ_MAIN_VS_ERR	= 1 << 2,
	CQ_SUB_VS_ERR	= 1 << 3,
	RAW_DMA_ERR	= 1 << 4,
	YUV_DMA_ERR	= 1 << 5,
};

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
			     CQ_MAIN_CODE_ERR_ST |\
			     CQ_MAIN_VS_ERR_ST	 |\
			     DMA_ERR_ST)

#define ISP_SENINF_CTRL(regs)				(regs + 0x0200)
#define ISP_SENINF_TSETMDL_CTRL(regs)		(regs + 0x0220)

#define ISP_SENINF_TOP_MUX_CTRL_0(regs)		(regs + 0x0010)
#define ISP_SENINF_TOP_MUX_CTRL_1(regs)		(regs + 0x0014)
#define ISP_SENINF_TOP_MUX_CTRL_2(regs)		(regs + 0x0018)
#define ISP_SENINF_TOP_MUX_CTRL_3(regs)		(regs + 0x001C)
#define ISP_SENINF_TOP_MUX_CTRL_4(regs)		(regs + 0x0020)
#define ISP_SENINF_TOP_MUX_CTRL_5(regs)		(regs + 0x0024)

#define ISP_SENINF_MUX_CTRL_0(regs)			(regs + 0x0D00)
#define ISP_SENINF_MUX_CTRL_1(regs)			(regs + 0x0D04)
#define ISP_SENINF_MUX_OPT(regs)			(regs + 0x0D08)
#define ISP_SENINF_MUX_VC_SEL0(regs)		(regs + 0x0E00)
#define ISP_SENINF_MUX_VC_SEL1(regs)		(regs + 0x0E04)
#define ISP_SENINF_MUX_VC_SEL2(regs)		(regs + 0x0E08)
#define ISP_SENINF_MUX_VC_SEL3(regs)		(regs + 0x0E0C)
#define ISP_SENINF_MUX_VC_SEL4(regs)		(regs + 0x0E10)
#define ISP_SENINF_MUX_VC_SEL5(regs)		(regs + 0x0E14)
#define ISP_SENINF_MUX_VC_SEL6(regs)		(regs + 0x0E18)
#define ISP_SENINF_MUX_VC_SEL7(regs)		(regs + 0x0E1C)

#define ISP_SENINF_TM_CTL(regs)				(regs + 0x0F08)
#define ISP_SENINF_TM_SIZE(regs)			(regs + 0x0F0C)
#define ISP_SENINF_TM_CLK(regs)				(regs + 0x0F10)
#define ISP_SENINF_TM_DUM(regs)				(regs + 0x0F18)

#define ISP_SENINF_CAM_MUX_PCSR_CTRL(regs)			(regs + 0x17000)
#define ISP_SENINF_CAM_MUX_PCSR_OPT(regs)			(regs + 0x17004)
#define ISP_SENINF_CAM_MUX_PCSR_VC_SEL(regs)	(regs + 0x17020)
#define ISP_SENINF_CAM_MUX_PCSR_DT_SEL(regs)	(regs + 0x17024)

#define ISP_SENINF_CAM_MUX_GCSR_CTRL(regs)			(regs + 0x17F00)

#define CAMSYS_MAIN_REG_HALT1_EN(regs)			(regs + 0x00C4)
#define CAMSYS_MAIN_REG_HALT2_EN(regs)			(regs + 0x00C8)
#define CAMSYS_MAIN_REG_HALT3_EN(regs)			(regs + 0x00CC)
#define CAMSYS_MAIN_REG_HALT4_EN(regs)			(regs + 0x00D0)
#define CAMSYS_MAIN_REG_HALT5_EN(regs)			(regs + 0x00D4)
#define CAMSYS_MAIN_REG_HALT6_EN(regs)			(regs + 0x00D8)

/* camsv_a/b */
#define HALT1_EN					0x2
#define HALT2_EN					0x2
/* raw_a/yuv_a */
#define HALT5_EN					0xF860
#define HALT6_EN					0x6F
/* raw_b/yuv_b */
#define HALT7_EN					0xF860
#define HALT8_EN					0x6F
/* raw_c/yuv_c */
#define HALT9_EN					0xF860
#define HALT10_EN					0x6F
/* camsv_c/d/e/f */
#define HALT13_EN					0xF0
#define REG_HALT1_EN					0x00c4
#define REG_HALT2_EN					0x00c8
#define REG_HALT5_EN					0x00d4
#define REG_HALT6_EN					0x00d8
#define REG_HALT7_EN					0x00dc
#define REG_HALT8_EN					0x00e0
#define REG_HALT9_EN					0x00e4
#define REG_HALT10_EN					0x00e8
#define REG_HALT13_EN					0x00f4


#define CAM_REG_CTL_RAW_INT_EN(regs)			(regs + 0x0100)
#define CAM_REG_CTL_RAW_INT_STATUS(regs)		(regs + 0x0104)
#define CAM_REG_CTL_RAW_INT2_EN(regs)			(regs + 0x0110)
#define CAM_REG_CTL_RAW_INT2_STATUS(regs)		(regs + 0x0114)
#define CAM_REG_CTL_RAW_INT3_EN(regs)			(regs + 0x0120)
#define CAM_REG_CTL_RAW_INT3_STATUS(regs)		(regs + 0x0124)
#define CAM_REG_CTL_RAW_INT4_EN(regs)			(regs + 0x0130)
#define CAM_REG_CTL_RAW_INT4_STATUS(regs)		(regs + 0x0134)
#define CAM_REG_CTL_RAW_INT5_EN(regs)			(regs + 0x0140)
#define CAM_REG_CTL_RAW_INT5_STATUS(regs)		(regs + 0x0144)
#define CAM_REG_CTL_RAW_INT6_EN(regs)			(regs + 0x0150)
#define CAM_REG_CTL_RAW_INT6_STATUS(regs)		(regs + 0x0154)
#define CAM_REG_CTL_RAW_INT7_EN(regs)			(regs +	0x0160)
#define CAM_REG_CTL_RAW_INT7_STATUS(regs)		(regs + 0x0164)

#define CAM_REG_CTL_RAW_INT_STATUSX(regs)		(regs + 0x0108)
#define CAM_REG_CTL_RAW_INT2_STATUSX(regs)		(regs + 0x0118)
#define CAM_REG_CTL_RAW_INT3_STATUSX(regs)		(regs + 0x0128)
#define CAM_REG_CTL_RAW_INT4_STATUSX(regs)		(regs + 0x0138)
#define CAM_REG_CTL_RAW_INT5_STATUSX(regs)		(regs + 0x0148)
#define CAM_REG_CTL_RAW_INT6_STATUSX(regs)		(regs + 0x0158)
#define CTL_CQ_THR0_DONE_ST				BIT(0)
#define CAM_REG_CTL_RAW_INT7_STATUSX(regs)		(regs + 0x0168)
#define CTL_CQ_THRSUB_DONE_ST				BIT(4)


#define CAM_REG_CTL2_RAW_INT_EN(regs)			(regs + 0x0100)
#define CAM_REG_CTL2_RAW_INT2_EN(regs)			(regs + 0x0110)
#define CAM_REG_CTL2_RAW_INT4_EN(regs)			(regs + 0x0130)
#define CAM_REG_CTL2_RAW_INT5_EN(regs)			(regs + 0x0140)

#define CAM_REG_CTL2_RAW_INT_STATUS(regs)		(regs + 0x0104)
#define CAM_REG_CTL2_RAW_INT2_STATUS(regs)		(regs + 0x0114)
#define CAM_REG_CTL2_RAW_INT4_STATUS(regs)		(regs + 0x0134)
#define CAM_REG_CTL2_RAW_INT5_STATUS(regs)		(regs + 0x0144)

#define CAM_REG_CTL2_RAW_INT_STATUSX(regs)		(regs + 0x0108)
#define CAM_REG_CTL2_RAW_INT2_STATUSX(regs)		(regs + 0x0118)
#define CAM_REG_CTL2_RAW_INT4_STATUSX(regs)		(regs + 0x0138)
#define CAM_REG_CTL2_RAW_INT5_STATUSX(regs)		(regs + 0x0148)

#define CAM_REG_CTL_RAW_MOD5_DCM_DIS			0x0310
#define CAM_REG_CTL_RAW_MOD6_DCM_DIS			0x0314

#define CAM_REG_CTL_WFBC_EN				0x03B0
#define CAM_REG_CTL_WFBC_INC				0x03B4

#define CAM_REG_CQ_THR0_CTL(regs)			    (regs + 0x0410)
#define CAM_REG_CQ_THR0_BASEADDR(regs)			(regs + 0x0414)
#define CAM_REG_CQ_THR0_DESC_SIZE(regs)			(regs + 0x041C)

#define CAM_REG_TG_SEN_MODE(regs)			(regs + 0x0700)
#define TG_CMOS_RDY_SEL					BIT(14)
#define CAM_REG_TG_SEN_GRAB_PXL(regs)			(regs + 0x0708)
#define CAM_REG_TG_SEN_GRAB_LIN(regs)			(regs + 0x070C)
#define CAM_REG_TG_PATH_CFG(regs)			(regs + 0x0710)
#define TG_TG_FULL_SEL					BIT(15)
#define CAM_REG_TG_FRMSIZE_ST(regs)			(regs + 0x0738)
#define CAM_REG_TG_VSEOL_SUB_CTL(regs)			(regs + 0x0760)
#define CAM_REG_TG_FRMSIZE_ST_R(regs)			(regs + 0x076C)

#define CAM_REG_CQI_R1A_CON0(regs)			(regs + 0x4120)
#define CAM_REG_CQI_R1A_CON1(regs)			(regs + 0x4124)
#define CAM_REG_CQI_R1A_CON2(regs)			(regs + 0x4128)
#define CAM_REG_CQI_R1A_CON3(regs)			(regs + 0x412C)
#define CAM_REG_CQI_R1A_CON4(regs)			(regs + 0x4130)

#define CAM_REG_CQI_R2A_CON0(regs)			(regs + 0x4190)
#define CAM_REG_CQI_R2A_CON1(regs)			(regs + 0x4194)
#define CAM_REG_CQI_R2A_CON2(regs)			(regs + 0x4198)
#define CAM_REG_CQI_R2A_CON3(regs)			(regs + 0x419C)
#define CAM_REG_CQI_R2A_CON4(regs)			(regs + 0x41A0)

#define CAM_REG_CQI_R3A_CON0(regs)			(regs + 0x4200)
#define CAM_REG_CQI_R3A_CON1(regs)			(regs + 0x4204)
#define CAM_REG_CQI_R3A_CON2(regs)			(regs + 0x4208)
#define CAM_REG_CQI_R3A_CON3(regs)			(regs + 0x420C)
#define CAM_REG_CQI_R3A_CON4(regs)			(regs + 0x4210)

#define CAM_REG_CQI_R4A_CON0(regs)			(regs + 0x4270)
#define CAM_REG_CQI_R4A_CON1(regs)			(regs + 0x4274)
#define CAM_REG_CQI_R4A_CON2(regs)			(regs + 0x4278)
#define CAM_REG_CQI_R4A_CON3(regs)			(regs + 0x427C)
#define CAM_REG_CQI_R4A_CON4(regs)			(regs + 0x4280)

#define CAM_REG_IMGO_CON0(regs)				(regs + 0x4780)
#define CAM_REG_IMGO_CON1(regs)				(regs + 0x4784)
#define CAM_REG_IMGO_CON2(regs)				(regs + 0x4788)
#define CAM_REG_IMGO_CON3(regs)				(regs + 0x478C)
#define CAM_REG_IMGO_CON4(regs)				(regs + 0x4790)

#define REG_CQI_R1A_CON0					0x4120
#define REG_CQI_R1A_CON1					0x4124
#define REG_CQI_R1A_CON2					0x4128
#define REG_CQI_R1A_CON3					0x412C
#define REG_CQI_R1A_CON4					0x4130

#define REG_CAMRAWDMATOP_LOW_LATENCY_LINE_CNT_IMGO_R1		0x4090
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R1		0x4090
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R3		0x4098
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_DRZS4NO_R1	0x40A0

/* error status */
#define REG_RAWI_R2_ERR_STAT				0x42F4
#define REG_UFDI_R2_ERR_STAT				0x4364
#define REG_RAWI_R3_ERR_STAT				0x42F4 //Ponsot x
#define REG_RAWI_R5_ERR_STAT				0x44B4
#define REG_UFDI_R5_ERR_STAT				0x4524
#define REG_CQI_R1_ERR_STAT				0x4134
#define REG_CQI_R2_ERR_STAT				0x41A4
#define REG_CQI_R3_ERR_STAT				0x4214
#define REG_CQI_R4_ERR_STAT				0x4284
#define REG_LSCI_R1_ERR_STAT				0x4694
#define REG_BPCI_R1_ERR_STAT				0x4594
#define REG_BPCI_R2_ERR_STAT				0x45D4
#define REG_BPCI_R3_ERR_STAT				0x46F4 //Ponsot x
#define REG_PDI_R1_ERR_STAT				0x46D4
#define REG_AAI_R1_ERR_STAT				0x4714
#define REG_CACI_R1_ERR_STAT				0x4754
#define REG_RAWI2_R6_ERR_STAT				0x4834 //Ponsot x
#define REG_IMGO_R1_ERR_STAT				0x4794
#define REG_FHO_R1_ERR_STAT				0x4844
#define REG_AAHO_R1_ERR_STAT				0x49F4
#define REG_PDO_R1_ERR_STAT				0x4974
#define REG_AAO_R1_ERR_STAT				0x49B4
#define REG_AFO_R1_ERR_STAT				0x4A34
#define REG_TSFSO_R1_ERR_STAT				0x4A74
#define REG_LTMSO_R1_ERR_STAT				0x4AF4
#define REG_LTMSHO_R1_ERR_STAT				0x4B34
#define REG_FLKO_R1_ERR_STAT				0x4934
#define REG_UFEO_R1_ERR_STAT				0x48F4
#define REG_TSFSO_R2_ERR_STAT				0x4E14 //Ponsot x
/* error status, yuv base */
#define REG_YUVO_R1_ERR_STAT				0x4234
#define REG_YUVBO_R1_ERR_STAT				0x42E4
#define REG_YUVCO_R1_ERR_STAT				0x4394
#define REG_YUVDO_R1_ERR_STAT				0x4444
#define REG_YUVO_R3_ERR_STAT				0x44F4
#define REG_YUVBO_R3_ERR_STAT				0x45A4
#define REG_YUVCO_R3_ERR_STAT				0x4654
#define REG_YUVDO_R3_ERR_STAT				0x4704
#define REG_YUVO_R2_ERR_STAT				0x47B4
#define REG_YUVBO_R2_ERR_STAT				0x47F4
#define REG_YUVO_R4_ERR_STAT				0x4834
#define REG_YUVBO_R4_ERR_STAT				0x4874
#define REG_RZH1N2TO_R1_ERR_STAT			0x4B34
#define REG_RZH1N2TBO_R1_ERR_STAT			0x4934
#define REG_RZH1N2TO_R2_ERR_STAT			0x49F4
#define REG_RZH1N2TO_R3_ERR_STAT			0x4AF4
#define REG_RZH1N2TBO_R3_ERR_STAT			0x4974
#define REG_DRZS4NO_R1_ERR_STAT				0x4A34
#define REG_DRZS4NO_R2_ERR_STAT				0x4A34 //Ponsot x
#define REG_DRZS4NO_R3_ERR_STAT				0x4AB4
#define REG_ACTSO_R1_ERR_STAT				0x4AF4 //Ponsot x
#define REG_TNCSYO_R1_ERR_STAT				0x4BF4 //Ponsot x
#define REG_YUVO_R5_ERR_STAT				0x48B4
#define REG_YUVBO_R5_ERR_STAT				0x48F4
#define REG_TCYSO_R1_ERR_STAT				0x49B4
#define REG_DRZH2NO_R8_ERR_STAT				0x4A74
#define REG_DRZB2NO_R1_ERR_STAT				0x4B74
#define REG_DRZB2NBO_R1_ERR_STAT			0x4BB4
#define REG_DRZB2NCO_R1_ERR_STAT			0x4BF4

#define REG_CTL_DBG_SET					0x03F0
#define REG_CTL_DBG_PORT				0x03F4
#define REG_CTL_DBG_SET2				0x03F8

#define REG_DMA_DBG_SEL					0x4070
#define REG_DMA_DBG_PORT				0x4074

#define CAMCTL_INT_TRIG        0x380
#define CAMCTL_INT_EN  0x100

/*  related */
#define REG_CQ_EN							0x0400
#define REG_SCQ_START_PERIOD				0x0408

#define REG_CQ_THR0_CTL						0x0410
#define REG_CQ_THR0_BASEADDR				0x0414
#define REG_CQ_THR0_BASEADDR_MSB			0x0418
#define REG_CQ_THR0_DESC_SIZE				0x041C
#define REG_CQ_SUB_CQ_EN			        0x0430
#define REG_CQ_SUB_THR0_CTL					0x0440
#define REG_CQ_SUB_THR0_BASEADDR_2			0x044C
#define REG_CQ_SUB_THR0_BASEADDR_MSB_2		0x0450
#define REG_CQ_SUB_THR0_DESC_SIZE_2			0x0458
#define REG_CQ_SUB_THR0_BASEADDR_1			0x0444
#define REG_CQ_SUB_THR0_BASEADDR_MSB_1		0x0448
#define REG_CQ_SUB_THR0_DESC_SIZE_1			0x0454

#define CQ_DB_EN					BIT(4)
#define CQ_DB_LOAD_MODE					BIT(8)
#define CQ_RESET					BIT(16)
#define CTL_CQ_THR0_START				BIT(0)
#define CQ_THR0_MODE_IMMEDIATE				BIT(4)
#define CQ_THR0_MODE_CONTINUOUS				BIT(5)
#define CQ_THR0_DONE_SEL				BIT(8)
#define SCQ_EN						BIT(20)
#define SCQ_SUBSAMPLE_EN                                BIT(21)
#define SCQ_SUB_RESET					BIT(16)

#define CQ_THR0_EN						BIT(0)
#define CQ_CQI_R1_EN					BIT(15)
#define CQ_CQI_R2_EN					BIT(16)
#define CAMCQ_SCQ_EN					BIT(20)
#define PASS1_DONE_SEL					BIT(16)

/* camsys */
#define REG_CAMSYS_CG_SET				0x0004
#define REG_CAMSYS_CG_CLR				0x0008
#define REG_CTL_START					0x00B0

/* camctl */
#define CAM_REG_CTL_MISC				0x0060
#define REG_CTL_RAWI_TRIG				0x00C0
#define REG_CTL_SW_CTL					0x00C4
#define REG_CTL_SW_PASS1_DONE				0x00C8
#define REG_CTL_SW_SUB_CTL				0x00CC

/* TG */
#define REG_TG_SEN_MODE					0x0700
#define TG_SEN_MODE_CMOS_EN				BIT(0)

#define REG_TG_VF_CON					0x0704
#define TG_VFDATA_EN					BIT(0)
#define REG_TG_INTER_ST					0x073C
#define TG_CAM_CS_MASK					0x3f00
#define TG_IDLE_ST					BIT(8)

/* DBG */
#define CAM_REG_CQ_SUB_THR0_BASEADDR_1(regs)			(regs + 0x0444)
#define CAM_REG_CQ_SUB_THR0_BASEADDR_2(regs)			(regs + 0x044C)
#define CAM_REG_SUB_THR0_DESC_SIZE_1(regs)			(regs + 0x0454)
#define CAM_REG_SUB_THR0_DESC_SIZE_2(regs)			(regs + 0x0458)
#define CAM_REG_IMGO_ORIWDMA_BASE_ADDR(regs)			(regs + 0x4760)
#define CAM_REG_AAO_ULCWDMA_BASE_ADDR(regs)			(regs + 0x4980)
#define CAM_REG_YUVO_R1_ORIWDMA_BASE_ADDR(regs)			(regs + 0x4200)
#define CAM_REG_YUVO_R3_ORIWDMA_BASE_ADDR(regs)			(regs + 0x44C0)
#define CAM_REG_SMI_PORT_PSUEDO_MODE_EN(regs)			(regs + 0x4080)

#define CAM_REG_FBC_IMGO_R1_CTL2(regs)			        (regs + 0x3D04)
#define CAM_REG_FBC_YUVO_R1_CTL2(regs)			        (regs + 0x3C04)
#define CAM_REG_FBC_YUVO_R3_CTL2(regs)			        (regs + 0x3C24)

#define CAM_REG_CQ_EN(regs)				        (regs + 0x0400)
#define CAM_REG_SUB_CQ_EN(regs)				        (regs + 0x0430)

#define CAM_REG_SW_PASS1_DONE(regs)			        (regs + 0x00C8)
#define CAM_REG_SW_SUB_CTL(regs)			        (regs + 0x00CC)

#define CAM_REG_FBC_AAO_R1_CTL1(regs)			        (regs + 0x3D20)
#define CAM_REG_FBC_AAHO_R1_CTL1(regs)			        (regs + 0x3D10)
#define CAM_REG_FBC_AAO_R1_CTL2(regs)			        (regs + 0x3D24)
#define CAM_REG_FBC_AAHO_R1_CTL2(regs)			        (regs + 0x3D14)


