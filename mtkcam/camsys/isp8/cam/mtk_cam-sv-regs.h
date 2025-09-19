/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CAMSV_REGS_H
#define _CAMSV_REGS_H

/* cam main */
#define REG_CAM_MAIN_SW_RST_1					0x0058
#define REG_CAM_MAIN_LARB13_VC_SEL				0x0590
#define REG_CAM_MAIN_LARB14_VC_SEL				0x0594
#define REG_CAM_MAIN_LARB29_VC_SEL				0x059C

/* central */
#define REG_CAMSVCENTRAL_DMA_EN_IMG				0x0040

#define REG_CAMSVCENTRAL_CAMSV_SPARE0			0x0060

#define REG_CAMSVCENTRAL_DCM_DIS				0x007C

#define REG_CAMSVCENTRAL_DCM_DIS_STATUS			0x0080

#define REG_CAMSVCENTRAL_SEN_MODE				0x0140
#define CAMSVCENTRAL_CMOS_EN					BIT(0)
union CAMSVCENTRAL_SEN_MODE {
	struct {
		unsigned int CMOS_EN					:	1;
		unsigned int SAT_SWITCH_GROUP			:	1;
		unsigned int SAT_DROP_EN				:	1;
		unsigned int rsv_3						:	1;
		unsigned int SENINF_PIX_MODE			:	1;
		unsigned int rsv_4						:	3;
		unsigned int MSTREAM_EN					:	1;
		unsigned int rsv_9						:	2;
		unsigned int TG_MODE_OFF				:	1;
		unsigned int PXL_CNT_RST_SRC			:	1;
		unsigned int FLUSH_ALL_SRC_DIS			:	1;
		unsigned int CMOS_RDY_SEL				:	1;
		unsigned int FIFO_FULL_SEL				:	1;
		unsigned int CAM_SUB_EN					:	1;
		unsigned int VS_SUB_EN					:	1;
		unsigned int SOF_SUB_EN					:	1;
		unsigned int DOWN_SAMPLE_EN				:	1;
		unsigned int rsv_20						:	4;
		unsigned int VSYNC_INT_POL				:	1;
		unsigned int rsv_25						:	1;
		unsigned int RGBW_EN					:	1;
		unsigned int LINE_OK_CHECK				:	1;
		unsigned int rsv_28						:	4;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_VF_CON					0x0148
#define CAMSVCENTRAL_VFDATA_EN					BIT(0)
union CAMSVCENTRAL_VF_CON {
	struct {
		unsigned int VFDATA_EN					:  1;
		unsigned int SINGLE_MODE				:  1;
		unsigned int rsv_2						:  2;
		unsigned int FR_CON						:  3;
		unsigned int rsv_7						:  1;
		unsigned int SP_DELAY					:  3;
		unsigned int rsv_11						:  1;
		unsigned int SPDELAY_MODE				:  1;
		unsigned int VFDATA_EN_MUX_0_SEL		:  1;
		unsigned int VFDATA_EN_MUX_1_SEL		:  1;
		unsigned int rsv_15						: 17;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_PATH_CFG				0x014C
union CAMSVCENTRAL_PATH_CFG {
	struct {
		unsigned int rsv_0						:  4;
		unsigned int JPGINF_EN					:  1;
		unsigned int rsv_5						:  2;
		unsigned int JPG_LINEND_EN				:  1;
		unsigned int SYNC_VF_EN_DB_LOAD_DIS		:  1;
		unsigned int rsv_9						: 10;
		unsigned int EXP_ESC					:  1;
		unsigned int rsv_20						: 12;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_DDR_CFG				0x0150
union CAMSVCENTRAL_DDR_CFG {
	struct {
		unsigned int DDR_TIMER_EN				:  1;
		unsigned int DDR_OR_CQ_EN				:  1;
		unsigned int DDR_MODE_SEL				:  2;
		unsigned int DDR_SET					:  1;
		unsigned int DDR_CLEAR					:  1;
		unsigned int rsv_6						: 10;
		unsigned int DDR_SW_R					:  1;
		unsigned int rsv_17						: 15;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_DDR_THRESHOLD			0x0154

#define REG_CAMSVCENTRAL_BW_QOS_CFG				0x0158
union CAMSVCENTRAL_BW_QOS_CFG {
	struct {
		unsigned int BW_QOS_TIMER_EN			:  1;
		unsigned int BW_QOS_OR_CQ_EN			:  1;
		unsigned int BW_QOS_MODE_SEL			:  2;
		unsigned int BW_QOS_SET					:  1;
		unsigned int BW_QOS_CLEAR				:  1;
		unsigned int rsv_6						: 10;
		unsigned int BW_QOS_SW_R				:  1;
		unsigned int rsv_17						: 15;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_BW_QOS_THRESHOLD		0x015c

#define REG_CAMSVCENTRAL_TAG_R_SEL				0x0174

#define REG_CAMSVCENTRAL_FRMSIZE_ST_R			0x0180
#define REG_CAMSVCENTRAL_FRMSIZE_ST				0x0188

#define REG_CAMSVCENTRAL_TIME_STAMP_CTL			0x018C
#define REG_CAMSVCENTRAL_TIME_STAMP_INC_CNT		0x01B0

#define REG_CAMSVCENTRAL_MODULE_DB				0x01B4
union CAMSVCENTRAL_MODULE_DB {
	struct {
		unsigned int rsv_0						: 26;
		unsigned int CAM_DB_LOAD_FORCE			:  1;
		unsigned int rsv_27						:  3;
		unsigned int CAM_DB_EN					:  1;
		unsigned int CAM_DB_LOCK				:  1;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_GROUP_TAG0				0x01B8
#define CAMSVCENTRAL_GROUP_TAG_SHIFT			0x4

#define REG_CAMSVCENTRAL_LAST_TAG				0x01C8
#define REG_CAMSVCENTRAL_FIRST_TAG				0x01CC

#define REG_CAMSVCENTRAL_SW_CTL					0x01D0

#define REG_CAMSVCENTRAL_DCIF_SET				0x01D4
#define REG_CAMSVCENTRAL_DCIF_SEL				0x01D8

#define REG_CAMSVCENTRAL_DONE_STATUS_EN			0x0344
union CAMSVCENTRAL_DONE_STATUS_EN {
	struct {
		unsigned int GP_PASS1_DONE_0_ST_EN		:  1;
		unsigned int GP_PASS1_DONE_1_ST_EN		:  1;
		unsigned int GP_PASS1_DONE_2_ST_EN		:  1;
		unsigned int GP_PASS1_DONE_3_ST_EN		:  1;
		unsigned int rsv_4						:  8;
		unsigned int EVS_DONE_ST_EN				:  1;
		unsigned int EVS_RING_LINE_END_EN		:  1;
		unsigned int EVS_COLLECT_LINE_DONE_EN	:  1;
		unsigned int EVS_FH_DONE_EN				:  1;
		unsigned int SW_PASS1_DONE_0_ST_EN		:  1;
		unsigned int SW_PASS1_DONE_1_ST_EN		:  1;
		unsigned int SW_PASS1_DONE_2_ST_EN		:  1;
		unsigned int SW_PASS1_DONE_3_ST_EN		:  1;
		unsigned int rsv_20						: 12;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_DONE_STATUS			0x0348
#define CAMSVCENTRAL_GP_PASS1_DONE_0_ST			BIT(0)
#define CAMSVCENTRAL_GP_PASS1_DONE_1_ST			BIT(1)
#define CAMSVCENTRAL_GP_PASS1_DONE_2_ST			BIT(2)
#define CAMSVCENTRAL_GP_PASS1_DONE_3_ST			BIT(3)
#define CAMSVCENTRAL_SW_GP_PASS1_DONE_0_ST		BIT(16)
#define CAMSVCENTRAL_SW_GP_PASS1_DONE_1_ST		BIT(17)
#define CAMSVCENTRAL_SW_GP_PASS1_DONE_2_ST		BIT(18)
#define CAMSVCENTRAL_SW_GP_PASS1_DONE_3_ST		BIT(19)

#define REG_CAMSVCENTRAL_ERR_STATUS_EN			0x034C

#define REG_CAMSVCENTRAL_ERR_STATUS				0x0350
#define CAMSVCENTRAL_DMA_SRAM_FULL_ST			BIT(0)
#define CAMSVCENTRAL_CQ_BUSY_DROP_ST			BIT(1)
#define CAMSVCENTRAL_GRAB_ERR_TAG1				BIT(8)
#define CAMSVCENTRAL_OVRUN_ERR_TAG1				BIT(9)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG1			BIT(10)
#define CAMSVCENTRAL_GRAB_ERR_TAG2				BIT(11)
#define CAMSVCENTRAL_OVRUN_ERR_TAG2				BIT(12)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG2			BIT(13)
#define CAMSVCENTRAL_GRAB_ERR_TAG3				BIT(14)
#define CAMSVCENTRAL_OVRUN_ERR_TAG3				BIT(15)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG3			BIT(16)
#define CAMSVCENTRAL_GRAB_ERR_TAG4				BIT(17)
#define CAMSVCENTRAL_OVRUN_ERR_TAG4				BIT(18)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG4			BIT(19)
#define CAMSVCENTRAL_GRAB_ERR_TAG5				BIT(20)
#define CAMSVCENTRAL_OVRUN_ERR_TAG5				BIT(21)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG5			BIT(22)
#define CAMSVCENTRAL_GRAB_ERR_TAG6				BIT(23)
#define CAMSVCENTRAL_OVRUN_ERR_TAG6				BIT(24)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG6			BIT(25)
#define CAMSVCENTRAL_GRAB_ERR_TAG7				BIT(26)
#define CAMSVCENTRAL_OVRUN_ERR_TAG7				BIT(27)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG7			BIT(28)
#define CAMSVCENTRAL_GRAB_ERR_TAG8				BIT(29)
#define CAMSVCENTRAL_OVRUN_ERR_TAG8				BIT(30)
#define CAMSVCENTRAL_DB_LOAD_ERR_TAG8			BIT(31)

#define REG_CAMSVCENTRAL_SOF_STATUS_EN			0x035C

#define REG_CAMSVCENTRAL_SOF_STATUS				0x0360
#define CAMSVCENTRAL_SOF_BIT_START				0x2
#define CAMSVCENTRAL_SOF_BIT_OFFSET				0x4

#define REG_CAMSVCENTRAL_COMMON_STATUS_EN		0x0364
#define REG_CAMSVCENTRAL_COMMON_STATUS			0x0368
#define CAMSVCENTRAL_DBG_INT_BIT_START			0x8
#define CAMSVCENTRAL_DBG_INT_BIT_OFFSET			0x2

#define REG_CAMSVCENTRAL_CHANNEL_STATUS_EN		0x036C
#define REG_CAMSVCENTRAL_CHANNEL_STATUS			0x0370
#define CAMSVCENTRAL_SW_ENQ_ERR_BIT_START		0x0
#define CAMSVCENTRAL_SW_ENQ_ERR_BIT_OFFSET		0x4

#define REG_CAMSVCENTRAL_FBC0_TAG1				0x0540
#define CAMSVCENTRAL_FBC0_TAG_SHIFT				0x40
union CAMSVCENTRAL_FBC0_TAG1 {
	struct {
		unsigned int rsv_0						:  7;
		unsigned int RCNT_INC_TAG1				:  1;
		unsigned int FBC_RESET_TAG1				:  1;
		unsigned int rsv_9						:  3;
		unsigned int FBC_LOCK_EN_TAG1			:  1;
		unsigned int rsv_13						:  1;
		unsigned int FBC_SUB_EN_TAG1			:  1;
		unsigned int FBC_EN_TAG1				:  1;
		unsigned int FBC_VALID_NUM_TAG1			:  8;
		unsigned int FBC_SUB_RATIO_TAG1			:  8;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_FBC1_TAG1				0x0544
#define CAMSVCENTRAL_FBC1_TAG_SHIFT				0x40
union CAMSVCENTRAL_FBC1_TAG1 {
	/* 0x1A100544 */
	struct {
		unsigned int RCNT_TAG1					:  8;
		unsigned int WCNT_TAG1					:  8;
		unsigned int FBC_CNT_TAG1				:  4;
		unsigned int rsv_20						:  5;
		unsigned int DROP_CNT_TAG1				:  7;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCENTRAL_GRAB_PXL_TAG1			0x0548
#define CAMSVCENTRAL_GRAB_PXL_TAG_SHIFT			0x40

#define REG_CAMSVCENTRAL_GRAB_LIN_TAG1			0x054C
#define CAMSVCENTRAL_GRAB_LIN_TAG_SHIFT			0x40

#define REG_CAMSVCENTRAL_VF_ST_TAG1				0x0550
#define CAMSVCENTRAL_VF_ST_TAG_SHIFT			0x40

#define REG_CAMSVCENTRAL_FORMAT_TAG1			0x0554
#define CAMSVCENTRAL_FORMAT_TAG_SHIFT			0x40

#define REG_CAMSVCENTRAL_CONFIG_TAG1			0x055C
#define CAMSVCENTRAL_CONFIG_TAG_SHIFT			0x40

#define REG_CAMSVCENTRAL_INT_EXP0_TAG1			0x0560
#define REG_CAMSVCENTRAL_INT_EXP1_TAG1			0x0564
#define CAMSVCENTRAL_INT_EXP0_OFFSET			0x40
#define CAMSVCENTRAL_INT_EXP1_OFFSET			0x40

#define REG_CAMSVCENTRAL_FH_SPARE_TAG_1			0x057C
#define CAMSVCENTRAL_FH_SPARE_SHIFT				0x40

/* dma */
#define REG_CAMSVDMATOP_CON1_IMG				0x0000
#define REG_CAMSVDMATOP_CON2_IMG				0x0004
#define REG_CAMSVDMATOP_CON3_IMG				0x0008
#define REG_CAMSVDMATOP_CON4_IMG				0x000C

#define REG_CAMSVDMATOP_CON1_LEN				0x0010
#define REG_CAMSVDMATOP_CON2_LEN				0x0014
#define REG_CAMSVDMATOP_CON3_LEN				0x0018
#define REG_CAMSVDMATOP_CON4_LEN				0x001C

#define REG_CAMSVDMATOP_SW_RST_CTL              0x0020

#define REG_CAMSVDMATOP_AXSLC_CMD				0x002C

#define REG_CAMSVDMATOP_DMA_DEBUG_SEL			0x0090
#define REG_CAMSVDMATOP_DMA_DEBUG_PORT			0x0094

#define REG_CAMSVDMATOP_CON1_IMG2				0x00E0
#define REG_CAMSVDMATOP_CON2_IMG2				0x00E4
#define REG_CAMSVDMATOP_CON3_IMG2				0x00E8
#define REG_CAMSVDMATOP_CON4_IMG2				0x00EC

#define REG_CAMSVDMATOP_CON1_LEN2				0x00F0
#define REG_CAMSVDMATOP_CON2_LEN2				0x00F4
#define REG_CAMSVDMATOP_CON3_LEN2				0x00F8
#define REG_CAMSVDMATOP_CON4_LEN2				0x00FC

#define REG_CAMSVDMATOP_WDMA_BASE_ADDR_IMG1		0x0200
#define CAMSVDMATOP_WDMA_BASE_ADDR_IMG_SHIFT	0x40

#define REG_CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG1	0x0204
#define CAMSVDMATOP_WDMA_BASE_ADDR_MSB_IMG_SHIFT	0x40

#define REG_CAMSVDMATOP_WDMA_BASIC_IMG1			0x0210
#define CAMSVDMATOP_WDMA_BASIC_IMG_SHIFT		0x40

union CAMSVDMATOP_WDMA_BASIC_IMG1 {
	struct {
		unsigned int MAX_BURST_LEN_IMG1			:  5;
		unsigned int rsv_5						:  3;
		unsigned int WDMA_RSV_IMG1				:  8;
		unsigned int STRIDE_IMG1				: 16;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVDMATOP_WDMA_SPECIAL_EN_IMG1	0x0218
#define CAMSVDMATOP_WDMA_SPECIAL_IMG_SHIFT		0x40

#define REG_CAMSVDMATOP_WDMA_AXSLC_SIZE_IMG1	0x0234
#define CAMSVDMATOP_WDMA__AXSLC_IMG_SHIFT		0x40

/* stg */
#define REG_CAMSVSTG1_EN_CTRL					0x0C00
#define REG_CAMSVSTG1_INIT_MODE_CTRL			0x0C04
#define REG_CAMSVSTG1_SW_CTRL					0x0C10
#define REG_CAMSVSTG1_NONE_SAME_PG_SEND_EN_CTRL	0x0C20
#define REG_CAMSVSTG1_LEADING_CNT_SRC			0x0C2C
#define REG_CAMSVSTG2_EN_CTRL					0x0CC0
#define REG_CAMSVSTG2_INIT_MODE_CTRL			0x0CC4
#define REG_CAMSVSTG2_SW_CTRL					0x0CD0
#define REG_CAMSVSTG2_NONE_SAME_PG_SEND_EN_CTRL	0x0CE0
#define REG_CAMSVSTG2_LEADING_CNT_SRC			0x0CEC

/* cq */
#define REG_CAMSVCQTOP_DEBUG					0x0
#define REG_CAMSVCQTOP_DEBUG_SET				0x0004

union CAMSVCQTOP_DEBUG_SET {
	struct {
		unsigned int CAMSVCQTOP_DEBUG_TOP_SEL		:  8;
		unsigned int CAMSVCQTOP_DEBUG_MOD_SEL		:  8;
		unsigned int CAMSVCQTOP_DEBUG_SEL			:  8;
		unsigned int rsv_24							:  8;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCQTOP_DCM_DIS					0x0008
#define REG_CAMSVCQTOP_SW_RST_CTL				0x0010
#define REG_CAMSVCQTOP_THR_START				0x0014

#define REG_CAMSVCQTOP_INT_0_EN					0x0018
union CAMSVCQTOP_INT_0_EN {
	struct {
		unsigned int CAMSVCQTOP_CSR_SCQ_SUB_THR_DONE_INT_EN			:  1;
		unsigned int CAMSVCQTOP_CSR_SCQ_MAX_START_DLY_ERR_INT_EN	:  1;
		unsigned int CAMSVCQTOP_CSR_SCQ_SUB_CODE_ERR_INT_EN			:  1;
		unsigned int CAMSVCQTOP_CSR_SCQ_SUB_VB_ERR_INT_EN			:  1;
		unsigned int CAMSVCQTOP_CSR_SCQ_TRIG_DLY_INT_EN				:  1;
		unsigned int CAMSVCQTOP_CSR_DMA_ERR_INT_EN		:  1;
		unsigned int CAMSVCQTOP_CSR_CQI_E1_DONE_INT_EN				:  1;
		unsigned int CAMSVCQTOP_CSR_CQI_E2_DONE_INT_EN				:  1;
		unsigned int CAMSVCQTOP_CSR_SCQ_MAX_START_DLY_SMALL_INT_EN	:  1;
		unsigned int CAMSVCQTOP_CSR_CQI_E1_OTF_UNDERFLOW_INT_EN		:  1;
		unsigned int CAMSVCQTOP_CSR_CQI_E2_OTF_UNDERFLOW_INT_EN		:  1;
		unsigned int rsv_11								: 20;
		unsigned int CAMSVCQTOP_CSR_INT_0_WCLR_EN		:  1;
	} Bits;
	unsigned int Raw;
};

#define CAMSVCQTOP_SCQ_SUB_THR_DONE				BIT(0)
#define CAMSVCQTOP_SCQ_SUB_CODE_ERR				BIT(2)
#define CAMSVCQTOP_SCQ_SUB_VB_ERR				BIT(3)
#define CAMSVCQTOP_DMA_ERR						BIT(5)
#define CAMSVCQTOP_CQI_E1_OTF_UNDERFLOW			BIT(9)
#define CAMSVCQTOP_CQI_E2_OTF_UNDERFLOW			BIT(10)

#define REG_CAMSVCQTOP_INT_0_STATUS				0x001C
#define CAMSVCQTOP_SCQ_SUB_THR_DONE				BIT(0)

#define REG_CAMSVCQ_CQ_EN						0x0100
union CAMSVCQ_CQ_EN {
	struct {
		unsigned int CAMSVCQ_CQ_APB_2T			:  1;
		unsigned int CAMSVCQ_CQ_DROP_FRAME_EN	:  1;
		unsigned int CAMSVCQ_CQ_SOF_SEL			:  1;
		unsigned int rsv_3						:  1;
		unsigned int CAMSVCQ_CQ_DB_EN			:  1;
		unsigned int rsv_5						:  3;
		unsigned int CAMSVCQ_CQ_DB_LOAD_MODE	:  1;
		unsigned int rsv_9						:  3;
		unsigned int CAMSVCQ_SCQ_STAGGER_MODE	:  1;
		unsigned int CAMSVCQ_SCQ_INVLD_CLR_SEL	:  1;
		unsigned int CAMSVCQ_SCQ_CAMSV_ENQ_FAIL_TRIG_EN	:  1;
		unsigned int CAMSVCQ_SCQ_LAST_FRAME_TRIG_EN		:  1;
		unsigned int CAMSVCQ_CQ_RESET			:  1;
		unsigned int CAMSVCQ_SCQ_INVLD_CLR_CHK	:  1;
		unsigned int CAMSVCQ_VHDR_LAST_FRAME_TRIG_EN	:  1;
		unsigned int rsv_19						:  2;
		unsigned int CAMSVCQ_SCQ_SUBSAMPLE_EN	:  1;
		unsigned int CAMSVCQ_CQ_DB_LOAD_SEL		:  1;
		unsigned int rsv_23						:  5;
		unsigned int CAMSVCQ_CQ_DBG_SEL			:  1;
		unsigned int CAMSVCQ_CQ_DBG_MAIN_SUB_SEL		:  1;
		unsigned int rsv_30						:  2;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCQ_SCQ_START_PERIOD			0x0108

#define REG_CAMSVCQ_SCQ_MISC					0x0120
#define REG_CAMSVCQ_CQ_SUB_EN					0x0130
union CAMSVCQ_CQ_SUB_EN {
	struct {
		unsigned int CAMSVCQ_CQ_SUB_APB_2T		:  1;
		unsigned int CAMSVCQ_CQ_SUB_DROP_FRAME_EN		:  1;
		unsigned int CAMSVCQ_CQ_SUB_SOF_SEL		:  1;
		unsigned int rsv_3						:  1;
		unsigned int CAMSVCQ_CQ_SUB_DB_EN		:  1;
		unsigned int rsv_5						:  3;
		unsigned int CAMSVCQ_CQ_SUB_DB_LOAD_MODE		:  1;
		unsigned int rsv_9						:  3;
		unsigned int CAMSVCQ_SCQ_SUB_STAGGER_MODE		:  1;
		unsigned int rsv_13						:  3;
		unsigned int CAMSVCQ_CQ_SUB_RESET		:  1;
		unsigned int rsv_17						: 11;
		unsigned int CAMSVCQ_CQ_SUB_DBG_SEL		:  1;
		unsigned int rsv_29						:  3;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCQ_CQ_SUB_THR0_CTL				0x0140
union CAMSVCQ_CQ_SUB_THR0_CTL {
	struct {
		unsigned int CAMSVCQ_CQ_SUB_THR0_EN		:  1;
		unsigned int rsv_1						:  3;
		unsigned int CAMSVCQ_CQ_SUB_THR0_MODE	:  2;
		unsigned int rsv_6						:  2;
		unsigned int CAMSVCQ_CQ_SUB_THR0_DONE_SEL		:  1;
		unsigned int rsv_9						: 23;
	} Bits;
	unsigned int Raw;
};

#define REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2		0x014C
#define REG_CAMSVCQ_CQ_SUB_THR0_BASEADDR_2_MSB	0x0150
#define REG_CAMSVCQ_CQ_SUB_THR0_DESC_SIZE_2		0x0158
#define REG_CAMSVCQ_SCQ_SUB_MISC				0x0160
#define REG_CAMSVCQI_E1_ORIRDMA_CON0			0x0620
#define REG_CAMSVCQI_E1_ORIRDMA_CON1			0x0624
#define REG_CAMSVCQI_E1_ORIRDMA_CON2			0x0628
#define REG_CAMSVCQI_E1_ORIRDMA_CON3			0x062C
#define REG_CAMSVCQI_E1_ORIRDMA_CON4			0x0630

#define REG_CAMSVCQI_E2_ORIRDMA_CON0			0x06A0
#define REG_CAMSVCQI_E2_ORIRDMA_CON1			0x06A4
#define REG_CAMSVCQI_E2_ORIRDMA_CON2			0x06A8
#define REG_CAMSVCQI_E2_ORIRDMA_CON3			0x06AC
#define REG_CAMSVCQI_E2_ORIRDMA_CON4			0x06B0

#define REG_CAMSVCQDMATOP_DMA_DBG_SEL			0x470
#define REG_CAMSVCQDMATOP_DMA_DBG_PORT			0x474

/*debug use*/
#define REG_MRAW_FHG_SPARE3						0x1188

/* error mask */
#define ERR_ST_MASK_TAG1_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG1 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG1 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG1)
#define ERR_ST_MASK_TAG2_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG2 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG2 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG2)
#define ERR_ST_MASK_TAG3_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG3 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG3 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG3)
#define ERR_ST_MASK_TAG4_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG4 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG4 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG4)
#define ERR_ST_MASK_TAG5_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG5 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG5 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG5)
#define ERR_ST_MASK_TAG6_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG6 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG6 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG6)
#define ERR_ST_MASK_TAG7_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG7 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG7 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG7)
#define ERR_ST_MASK_TAG8_ERR (\
					CAMSVCENTRAL_GRAB_ERR_TAG8 |\
					CAMSVCENTRAL_OVRUN_ERR_TAG8 |\
					CAMSVCENTRAL_DB_LOAD_ERR_TAG8)

#define ERR_ST_MASK_CQ_ERR (\
					CAMSVCQTOP_SCQ_SUB_CODE_ERR |\
					CAMSVCQTOP_SCQ_SUB_VB_ERR |\
					CAMSVCQTOP_DMA_ERR)


#endif	/* _CAMSV_REGS_H */
