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
#define HW_PASS1_DON_ST			(1L << 1)
#define SW_PASS1_DON_ST			(1L << 2)

/* YUV siganl */
#define YUV_PASS1_DON_ST		(1L << 0)
#define YUV_DON_ST			(1L << 1)
#define YUV_DMA_ERR_ST			(1L << 2)

/* err status */
#define TG_OVRUN_ST			(1L << 6)
#define TG_GBERR_ST			(1L << 7)
//#define CQ_DB_LOAD_ERR_ST		(1L << 12)
#define CQ_MAIN_CODE_ERR_ST		(1L << 19)
#define CQ_MAIN_VS_ERR_ST		(1L << 20)
#define CQ_MAIN_TRIG_DLY_ST		(1L << 21)
#define SW_ENQUE_ERR_ST		(1L << 5)
#define DMA_ERR_ST			(1L << 4)

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
			 SOF_INT_ST)

#define INT17_ST_MASK_CAM ( |\
			 HW_PASS1_DON_ST |\
			 SW_PASS1_DON_ST)


/* IRQ Error Mask */
#define INT_ST_MASK_CAM_ERR (TG_OVRUN_ST	 |\
			     TG_GBERR_ST)

#define INT17_ST_MASK_CAM_ERR (DMA_ERR_ST)

#define INT21_ST_MASK_CAM_ERR (CQ_MAIN_CODE_ERR_ST	 |\
					 CQ_MAIN_VS_ERR_S)

#define ISP_SENINF_TOP_ASYNC_CG(regs)			(regs + 0x0008)
#define ISP_SENINF_TOP_OUTMUX_CG(regs)			(regs + 0x000C)
#define ISP_SENINF_TOP_CTL(regs)			(regs + 0x0018)

#define ISP_SENINF_ASYNC_CFG(regs)			(regs + 0x0000)

#define ISP_SENINF_TM_CORE0_CTL(regs)			(regs + 0x0010)
#define ISP_SENINF_TM_SIZE(regs)			(regs + 0x0018)
#define ISP_SENINF_TM_DUM(regs)				(regs + 0x001C)
#define ISP_SENINF_TM_CON0(regs)			(regs + 0x0024)
#define ISP_SENINF_TM_CON1(regs)			(regs + 0x0028)
#define ISP_SENINF_TM_CON2(regs)			(regs + 0x002C)

#define ISP_SENINF_TM_EXP1_CTRL(regs)			(regs + 0x0044)
#define ISP_SENINF_TM_EXP2_CTRL(regs)			(regs + 0x0048)

#define ISP_SENINF_OUTMUX_PIX_MODE(regs)		(regs + 0x0008)
#define ISP_SENINF_OUTMUX_SOURCE_CFG0(regs)		(regs + 0x000C)
#define ISP_SENINF_OUTMUX_SOURCE_CFG1(regs)		(regs + 0x0010)
#define ISP_SENINF_OUTMUX_SOURCE_CFG2(regs)		(regs + 0x0014)
#define ISP_SENINF_OUTMUX_SRC_SEL(regs)			(regs + 0x0018)
#define ISP_SENINF_OUTMUX_CFG_DONE(regs)		(regs + 0x001C)
#define ISP_SENINF_OUTMUX_CFG_RDY(regs)			(regs + 0x002C)
#define ISP_SENINF_OUTMUX_TAG_VCDT(regs, tag)		(regs + 0x0080 + (tag * 0x1c))

#define CAMSYS_MAIN_REG_HALT1_EN(regs)			(regs + 0x00C4)
#define CAMSYS_MAIN_REG_HALT2_EN(regs)			(regs + 0x00C8)
#define CAMSYS_MAIN_REG_HALT3_EN(regs)			(regs + 0x00CC)
#define CAMSYS_MAIN_REG_HALT4_EN(regs)			(regs + 0x00D0)
#define CAMSYS_MAIN_REG_HALT5_EN(regs)			(regs + 0x00D4)
#define CAMSYS_MAIN_REG_HALT6_EN(regs)			(regs + 0x00D8)

/* camsv_a/b */
#define HALT1_EN					0x6
#define HALT2_EN					0x6
/* mraw/pda */
#define HALT3_EN					0xA
#define HALT4_EN					0xA
/* raw_a/yuv_a */
#define HALT5_EN					0xBC00
#define HALT6_EN					0x3
/* raw_b/yuv_b */
#define HALT7_EN					0xBC00
#define HALT8_EN					0x3
/* raw_c/yuv_c */
#define HALT9_EN					0xBC00
#define HALT10_EN					0x3
/* camsv_c/d/e/f */
#define HALT13_EN					0xF0

#define REG_HALT1_EN					0x00c4
#define REG_HALT2_EN					0x00c8
#define REG_HALT3_EN					0x00cc
#define REG_HALT4_EN					0x00cd
#define REG_HALT5_EN					0x00d4
#define REG_HALT6_EN					0x00d8
#define REG_HALT7_EN					0x00dc
#define REG_HALT8_EN					0x00e0
#define REG_HALT9_EN					0x00e4
#define REG_HALT10_EN					0x00e8
#define REG_HALT13_EN					0x00f4


#define CAM_REG_CTL_RAW_INT17_EN(regs)			(regs + 0x0900)
#define CAM_REG_CTL_RAW_INT17_STATUS(regs)		(regs + 0x0904)
#define CAM_REG_CTL_RAW_INT18_EN(regs)			(regs + 0x0910)
#define CAM_REG_CTL_RAW_INT18_STATUS(regs)		(regs + 0x0914)
#define CAM_REG_CTL_RAW_INT21_EN(regs)			(regs + 0x0940)
#define CAM_REG_CTL_RAW_INT21_STATUS(regs)		(regs + 0x0944)
#define CAM_REG_CTL_RAW_INT20_EN(regs)			(regs + 0x0930)
#define CAM_REG_CTL_RAW_INT20_STATUS(regs)		(regs + 0x0934)

#define CAM_REG_CTL_RAW_INT2_EN(regs)			(regs + 0x0810) //raw dmao done
#define CAM_REG_CTL_RAW_INT2_STATUS(regs)		(regs + 0x0814)
#define CAM_REG_CTL_RAW_INT3_EN(regs)			(regs + 0x0820) //raw dmai done
#define CAM_REG_CTL_RAW_INT3_STATUS(regs)		(regs + 0x0824)
#define CAM_REG_CTL_RAW_INT5_EN(regs)			(regs + 0x0840)
#define CAM_REG_CTL_RAW_INT5_STATUS(regs)		(regs + 0x0844)
//#define CAM_REG_CTL_RAW_INT7_EN(regs)			(regs +	0x0160)
//#define CAM_REG_CTL_RAW_INT7_STATUS(regs)		(regs + 0x0164)

#define CAM_REG_CTL_RAW_INT17_STATUSX(regs)		(regs + 0x0908)
#define CAM_REG_CTL_RAW_INT18_STATUSX(regs)		(regs + 0x0918)
#define CAM_REG_CTL_RAW_INT21_STATUSX(regs)		(regs + 0x0948)
#define CAM_REG_CTL_RAW_INT20_STATUSX(regs)		(regs + 0x0938)
#define CAM_REG_CTL_RAW_INT2_STATUSX(regs)		(regs + 0x0818)
#define CAM_REG_CTL_RAW_INT3_STATUSX(regs)		(regs + 0x0828)
#define CAM_REG_CTL_RAW_INT5_STATUSX(regs)		(regs + 0x0848)
#define CTL_CQ_THR0_DONE_ST				BIT(0)
#//define CAM_REG_CTL_RAW_INT7_STATUSX(regs)		(regs + 0x0168)
#define CTL_CQ_THRSUB_DONE_ST				BIT(4)


#define CAM_REG_CTL2_RAW_INT17_EN(regs)			(regs + 0x0900)
#define CAM_REG_CTL2_RAW_INT2_EN(regs)			(regs + 0x0810)
//#define CAM_REG_CTL2_RAW_INT4_EN(regs)			(regs + 0x0130)
#define CAM_REG_CTL2_RAW_INT5_EN(regs)			(regs + 0x0840)

#define CAM_REG_CTL2_RAW_INT17_STATUS(regs)		(regs + 0x0904)
#define CAM_REG_CTL2_RAW_INT2_STATUS(regs)		(regs + 0x0814)
//#define CAM_REG_CTL2_RAW_INT4_STATUS(regs)		(regs + 0x0134)
#define CAM_REG_CTL2_RAW_INT5_STATUS(regs)		(regs + 0x0844)

#define CAM_REG_CTL2_RAW_INT17_STATUSX(regs)		(regs + 0x0908)
#define CAM_REG_CTL2_RAW_INT2_STATUSX(regs)		    (regs + 0x0818)
//#define CAM_REG_CTL2_RAW_INT4_STATUSX(regs)		(regs + 0x0138)
#define CAM_REG_CTL2_RAW_INT5_STATUSX(regs)		    (regs + 0x0848)

#define CAM_REG_CTL_RAW_MOD5_DCM_DIS			0x0D10
#define CAM_REG_CTL_RAW_MOD6_DCM_DIS			0x0D14

#define CAM_REG_CTL_WFBC_EN				    0x0128
#define CAM_REG_CTL_WFBC_INC				0x0138

#define CAM_REG_CQ_THR0_CTL(regs)			    (regs + 0x1010)
#define CAM_REG_CQ_THR0_BASEADDR(regs)			(regs + 0x1014)
#define CAM_REG_CQ_THR0_DESC_SIZE(regs)			(regs + 0x101C)

#define CAM_REG_TG_SEN_MODE(regs)			(regs + 0x1200)
#define TG_CMOS_RDY_SEL					BIT(14)
#define CAM_REG_TG_SEN_GRAB_PXL(regs)			(regs + 0x1208)
#define CAM_REG_TG_SEN_GRAB_LIN(regs)			(regs + 0x120C)
#define CAM_REG_TG_PATH_CFG(regs)			(regs + 0x1210)
#define TG_TG_FULL_SEL					BIT(15)
#define CAM_REG_TG_FRMSIZE_ST(regs)			(regs + 0x1238)
#define CAM_REG_TG_VSEOL_SUB_CTL(regs)			(regs + 0x1260)
#define CAM_REG_TG_FRMSIZE_ST_R(regs)			(regs + 0x126C)

#define CAM_REG_CQI_R1A_CON0(regs)			(regs + 0x0220)
#define CAM_REG_CQI_R1A_CON1(regs)			(regs + 0x0224)
#define CAM_REG_CQI_R1A_CON2(regs)			(regs + 0x0228)
#define CAM_REG_CQI_R1A_CON3(regs)			(regs + 0x022C)
#define CAM_REG_CQI_R1A_CON4(regs)			(regs + 0x0230)

#define CAM_REG_CQI_R2A_CON0(regs)			(regs + 0x0290)
#define CAM_REG_CQI_R2A_CON1(regs)			(regs + 0x0294)
#define CAM_REG_CQI_R2A_CON2(regs)			(regs + 0x0298)
#define CAM_REG_CQI_R2A_CON3(regs)			(regs + 0x029C)
#define CAM_REG_CQI_R2A_CON4(regs)			(regs + 0x02A0)

#define CAM_REG_CQI_R3A_CON0(regs)			(regs + 0x0300)
#define CAM_REG_CQI_R3A_CON1(regs)			(regs + 0x0304)
#define CAM_REG_CQI_R3A_CON2(regs)			(regs + 0x0308)
#define CAM_REG_CQI_R3A_CON3(regs)			(regs + 0x030C)
#define CAM_REG_CQI_R3A_CON4(regs)			(regs + 0x0310)

#define CAM_REG_CQI_R4A_CON0(regs)			(regs + 0x0370)
#define CAM_REG_CQI_R4A_CON1(regs)			(regs + 0x0374)
#define CAM_REG_CQI_R4A_CON2(regs)			(regs + 0x0378)
#define CAM_REG_CQI_R4A_CON3(regs)			(regs + 0x037C)
#define CAM_REG_CQI_R4A_CON4(regs)			(regs + 0x0380)

#define CAM_REG_IMGO_CON0(regs)				(regs + 0x1020)
#define CAM_REG_IMGO_CON1(regs)				(regs + 0x1024)
#define CAM_REG_IMGO_CON2(regs)				(regs + 0x1028)
#define CAM_REG_IMGO_CON3(regs)				(regs + 0x102C)
#define CAM_REG_IMGO_CON4(regs)				(regs + 0x1030)

#define REG_CQI_R1A_CON0					0x0220
#define REG_CQI_R1A_CON1					0x0224
#define REG_CQI_R1A_CON2					0x0228
#define REG_CQI_R1A_CON3					0x022c
#define REG_CQI_R1A_CON4					0x0230

#define REG_CAMRAWDMATOP_LOW_LATENCY_LINE_CNT_IMGO_R1		0x4090
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R1		0x4090
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_YUVO_R3		0x4098
#define REG_CAMYUVDMATOP_LOW_LATENCY_LINE_CNT_DRZS4NO_R1	0x40A0

/* error status */
#define REG_RAWI_R2_ERR_STAT				0x05B8
#define REG_UFDI_R2_ERR_STAT				0x0628
#define REG_RAWI_R3_ERR_STAT				0x42F4 //Ponsot x
#define REG_RAWI_R5_ERR_STAT				0x44B4
#define REG_UFDI_R5_ERR_STAT				0x4524
#define REG_CQI_R1_ERR_STAT				0x0238
#define REG_CQI_R2_ERR_STAT				0x02A8
#define REG_CQI_R3_ERR_STAT				0x0318
#define REG_CQI_R4_ERR_STAT				0x0388
#define REG_LSCI_R1_ERR_STAT				0x0A78
#define REG_BPCI_R1_ERR_STAT				0x0938
#define REG_BPCI_R2_ERR_STAT				0x45D4
#define REG_BPCI_R3_ERR_STAT				0x46F4 //Ponsot x
#define REG_PDI_R1_ERR_STAT				0x0AF8
#define REG_AAI_R1_ERR_STAT				0x4714
#define REG_CACI_R1_ERR_STAT				0x08F8
#define REG_RAWI2_R6_ERR_STAT				0x4834 //Ponsot x
#define REG_IMGO_R1_ERR_STAT				0x1038
#define REG_FHO_R1_ERR_STAT				0x13A8
#define REG_AAHO_R1_ERR_STAT				0x1538
#define REG_PDO_R1_ERR_STAT				0x1498
#define REG_AAO_R1_ERR_STAT				0x14E8
#define REG_AFO_R1_ERR_STAT				0x1628
#define REG_TSFSO_R1_ERR_STAT				0x4A74
#define REG_LTMSO_R1_ERR_STAT				0x4AF4
#define REG_LTMSHO_R1_ERR_STAT				0x4B34
#define REG_FLKO_R1_ERR_STAT				0x1448
#define REG_UFEO_R1_ERR_STAT				0x10E8
#define REG_TSFSO_R2_ERR_STAT				0x1678 //Ponsot x
/* error status, yuv base */
#define REG_YUVO_R1_ERR_STAT				0x1038
#define REG_YUVBO_R1_ERR_STAT				0x10E8
#define REG_YUVCO_R1_ERR_STAT				0x1198
#define REG_YUVDO_R1_ERR_STAT				0x1248
#define REG_YUVO_R3_ERR_STAT				0x12F8
#define REG_YUVBO_R3_ERR_STAT				0x13A8
#define REG_YUVCO_R3_ERR_STAT				0x4654
#define REG_YUVDO_R3_ERR_STAT				0x4704
#define REG_YUVO_R2_ERR_STAT				0x15B8
#define REG_YUVBO_R2_ERR_STAT				0x1608
#define REG_YUVO_R4_ERR_STAT				0x1658
#define REG_YUVBO_R4_ERR_STAT				0x16A8
#define REG_RZH1N2TO_R1_ERR_STAT			0x1718
#define REG_RZH1N2TBO_R1_ERR_STAT			0x4934
#define REG_RZH1N2TO_R2_ERR_STAT			0x49F4
#define REG_RZH1N2TO_R3_ERR_STAT			0x4AF4
#define REG_RZH1N2TBO_R3_ERR_STAT			0x4974
#define REG_DRZS4NO_R1_ERR_STAT				0x18D4
#define REG_DRZS4NO_R2_ERR_STAT				0x4A34 //Ponsot x
#define REG_DRZS4NO_R3_ERR_STAT				0x4AB4
#define REG_ACTSO_R1_ERR_STAT				0x4AF4 //Ponsot x
#define REG_TNCSYO_R1_ERR_STAT				0x4BF4 //Ponsot x
#define REG_YUVO_R5_ERR_STAT				0x16F8
#define REG_YUVBO_R5_ERR_STAT				0x1748
#define REG_TCYSO_R1_ERR_STAT				0x1798
#define REG_DRZH2NO_R8_ERR_STAT				0x4A74
#define REG_DRZB2NO_R1_ERR_STAT				0x1768
#define REG_DRZB2NBO_R1_ERR_STAT			0x17B8
#define REG_DRZB2NCO_R1_ERR_STAT			0x1808

#define REG_CTL_DBG_SET					0x0F00
#define REG_CTL_DBG_SET2				0x0F04
#define REG_CTL_DBG_SET3				0x0F08
#define REG_CTL_DBG_PORT				0x0F0C

#define REG_DMA_DBG_SEL					0x0088
#define REG_DMA_DBG_PORT				0x008C

#define CAMCTL_INT17_TRIG        0x90C
#define CAMCTL_INT17_EN  0x900

/*  related */
#define REG_CQ_EN							0x1000
#define REG_SCQ_START_PERIOD				0x1008

#define REG_CQ_THR0_CTL						0x1010
#define REG_CQ_THR0_BASEADDR				0x1014
#define REG_CQ_THR0_BASEADDR_MSB			0x1018
#define REG_CQ_THR0_DESC_SIZE				0x101C
#define REG_CQ_SUB_CQ_EN			        0x1030
#define REG_CQ_SUB_THR0_CTL					0x1040
#define REG_CQ_SUB_THR0_BASEADDR_2			0x104C
#define REG_CQ_SUB_THR0_BASEADDR_MSB_2		0x1050
#define REG_CQ_SUB_THR0_DESC_SIZE_2			0x1058
#define REG_CQ_SUB_THR0_BASEADDR_1			0x1044
#define REG_CQ_SUB_THR0_BASEADDR_MSB_1		0x1048
#define REG_CQ_SUB_THR0_DESC_SIZE_1			0x1054
#define REG_CTL_START						0x0408

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
//#define CQ_CQI_R1_EN					BIT(15)
//#define CQ_CQI_R2_EN					BIT(16)
#define CAMCQ_SCQ_EN					BIT(20)
//#define PASS1_DONE_SEL					BIT(16)

/* camsys */
#define REG_CAMSYS_CG_SET				0x0004
#define REG_CAMSYS_CG_CLR				0x0008


/* camctl */
#define REG_CTL_DB_LOAD_CTL1            0x0414
#define CAM_REG_CTL_MISC				0x01E0
#define REG_CTL_RAWI_TRIG				0x00C4
#define REG_CTL_SW_CTL					0x0430
#define REG_CTL_SW_PASS1_DONE			0x0100
#define REG_CTL_SW_SUB_CTL				0x0104

/* TG */
#define REG_TG_SEN_MODE					0x1200
#define TG_SEN_MODE_CMOS_EN				BIT(0)

#define REG_TG_VF_CON					0x1204
#define TG_VFDATA_EN					BIT(0)
#define REG_TG_INTER_ST					0x123C
#define TG_CAM_CS_MASK					0x3f00
#define TG_IDLE_ST					BIT(8)

/* DBG */
#define CAM_REG_CQ_SUB_THR0_BASEADDR_1(regs)			(regs + 0x1044)
#define CAM_REG_CQ_SUB_THR0_BASEADDR_2(regs)			(regs + 0x104C)
#define CAM_REG_SUB_THR0_DESC_SIZE_1(regs)			(regs + 0x1054)
#define CAM_REG_SUB_THR0_DESC_SIZE_2(regs)			(regs + 0x1058)
#define CAM_REG_IMGO_ORIWDMA_BASE_ADDR(regs)			(regs + 0x1000)
#define CAM_REG_AAO_ULCWDMA_BASE_ADDR(regs)			(regs + 0x14B0)
#define CAM_REG_YUVO_R1_ORIWDMA_BASE_ADDR(regs)			(regs + 0x1000)
#define CAM_REG_YUVO_R3_ORIWDMA_BASE_ADDR(regs)			(regs + 0x12C0)
#define CAM_REG_SMI_PORT_PSUEDO_MODE_EN(regs)			(regs + 0x0098)

//#define CAM_REG_FBC_IMGO_R1_CTL2(regs)			        (regs + 0x3D04)
//#define CAM_REG_FBC_YUVO_R1_CTL2(regs)			        (regs + 0x3C04)
//#define CAM_REG_FBC_YUVO_R3_CTL2(regs)			        (regs + 0x3C24)

#define CAM_REG_CQ_EN(regs)				        (regs + 0x1000)
#define CAM_REG_SUB_CQ_EN(regs)				        (regs + 0x1030)

#define CAM_REG_SW_PASS1_DONE(regs)			        (regs + 0x0100)
#define CAM_REG_SW_SUB_CTL(regs)			        (regs + 0x0104)

//#define CAM_REG_FBC_AAO_R1_CTL1(regs)			        (regs + 0x3D20)
//#define CAM_REG_FBC_AAHO_R1_CTL1(regs)			        (regs + 0x3D10)
//#define CAM_REG_FBC_AAO_R1_CTL2(regs)			        (regs + 0x3D24)
//#define CAM_REG_FBC_AAHO_R1_CTL2(regs)			        (regs + 0x3D14)


