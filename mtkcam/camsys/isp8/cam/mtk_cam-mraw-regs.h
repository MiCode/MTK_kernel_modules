/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MRAW_REGS_H
#define _MRAW_REGS_H

/* camsys mraw */
#define REG_CAMSYS_MRAW_SW_RST					0xA0

/* mraw */
#define REG_MRAW_CTL_MOD_EN					    0x0000

#define REG_MRAW_CTL_MOD2_EN				    0x0004
union MRAW_CTL_MOD2_EN {
	struct {
		unsigned int MRAWCTL_CPI_M1_EN			:  1;
		unsigned int MRAWCTL_MBN_M1_EN			:  1;
		unsigned int MRAWCTL_FHG_M1_EN			:  1;
		unsigned int rsv_3						: 29;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_CTL_MOD3_EN				    0x0008
union MRAW_CTL_MOD3_EN {
	struct {
		unsigned int MRAWCTL_IMGO_M1_EN			:  1;
		unsigned int MRAWCTL_IMGBO_M1_EN		:  1;
		unsigned int MRAWCTL_CPIO_M1_EN			:  1;
		unsigned int MRAWCTL_FHO_M1_EN			:  1;
		unsigned int rsv_4						: 28;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_CTL_MOD4_EN				    0x000C
union MRAW_CTL_MOD4_EN {
	struct {
		unsigned int MRAWCTL_CQI_M1_EN			:  1;
		unsigned int MRAWCTL_CQI_M2_EN			:  1;
		unsigned int rsv_2						: 30;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_CTL_MISC						0x0010
union MRAW_CTL_MISC {
	struct {
		unsigned int MRAWCTL_DB_LOAD_HOLD		:  1;
		unsigned int MRAWCTL_DB_LOAD_HOLD_SUB	:  1;
		unsigned int MRAWCTL_DB_LOAD_SRC		:  2;
		unsigned int MRAWCTL_DB_EN				:  1;
		unsigned int MRAWCTL_DB_LOAD_FORCE		:  1;
		unsigned int rsv_6						:  2;
		unsigned int MRAWCTL_APB_CLK_GATE_BYPASS		:  1;
		unsigned int MRAWCTL_DB_LOAD_TRIG		:  1;
		unsigned int rsv_10						: 18;
		unsigned int MRAWCTL_PERF_MEAS_EN		:  1;
		unsigned int rsv_29						:  3;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_CTL_MODE_CTL				    0x0014

#define REG_MRAW_CTL_SEL					    0x0020
#define REG_MRAW_CTL_FMT_SEL			        0x0024
#define REG_MRAW_CTL_DONE_SEL			        0x0028

#define REG_MRAW_CTL_FBC_GROUP					0x0040

#define REG_MRAW_CTL_WFBC_EN					0x0048
#define REG_MRAW_CTL_WFBC_INC					0x004C

#define REG_MRAW_CTL_START						0x0050

#define REG_MRAW_CTL_SW_CTL						0x0058

#define REG_MRAW_CTL_DDREN_CTL					0x0080
union MRAW_CTL_DDREN_CTL {
	struct {
		unsigned int MRAWCTL_DDREN_HW_EN		:  1;
		unsigned int rsv_1						:  3;
		unsigned int MRAWCTL_DDREN_SW_SET		:  1;
		unsigned int rsv_5						:  3;
		unsigned int MRAWCTL_DDREN_SW_CLR		:  1;
		unsigned int rsv_9						: 23;
	} Bits;
	unsigned int Raw;
};
#define REG_MRAW_CTL_DDREN_ST					0x0084

#define REG_MRAW_CTL_BW_QOS_CTL					0x0088
union MRAW_CTL_BW_QOS_CTL {
	struct {
		unsigned int MRAWCTL_BW_QOS_HW_EN		:  1;
		unsigned int rsv_1						:  3;
		unsigned int MRAWCTL_BW_QOS_SW_SET		:  1;
		unsigned int rsv_5						:  3;
		unsigned int MRAWCTL_BW_QOS_SW_CLR		:  1;
		unsigned int rsv_9						: 23;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_CTL_INT_EN						0x0100
#define REG_MRAW_CTL_INT_STATUS					0x0104
#define MRAWCTL_VS_INT_ST						BIT(0)
#define MRAWCTL_TG_ERR_ST						BIT(6)
#define MRAWCTL_TG_GBERR_ST						BIT(7)
#define MRAWCTL_SOF_INT_ST						BIT(8)
#define MRAWCTL_CQ_DB_LOAD_ERR_ST				BIT(12)
#define MRAWCTL_CQ_MAX_START_DLY_ERR_ST			BIT(13)
#define MRAWCTL_CQ_MAIN_CODE_ERR_ST				BIT(14)
#define MRAWCTL_CQ_MAIN_VS_ERR_ST				BIT(15)
#define MRAWCTL_CQ_TRIG_DLY_INT_ST				BIT(16)
#define MRAWCTL_CQ_SUB_CODE_ERR_ST				BIT(17)
#define MRAWCTL_CQ_SUB_VS_ERR_ST				BIT(18)
#define MRAWCTL_PASS1_DONE_ST					BIT(19)
#define MRAWCTL_SW_PASS1_DONE_ST				BIT(20)
#define MRAWCTL_SUB_PASS1_DONE_ST				BIT(21)
#define MRAWCTL_DMA_ERR_ST						BIT(22)
#define MRAWCTL_SW_ENQUE_ERR_ST					BIT(23)

#define REG_MRAW_CTL_INT2_EN					0x0110
#define REG_MRAW_CTL_INT2_STATUS				0x0114

#define REG_MRAW_CTL_INT3_EN					0x0120
#define REG_MRAW_CTL_INT3_STATUS				0x0124

#define REG_MRAW_CTL_INT5_EN					0x0140
#define REG_MRAW_CTL_INT5_STATUS				0x0144
#define REG_MRAW_CTL_INT5_STATUSX				0x0148
#define MRAWCTL_IMGO_M1_OTF_OVERFLOW_ST			BIT(0)
#define MRAWCTL_IMGBO_M1_OTF_OVERFLOW_ST		BIT(1)
#define MRAWCTL_CPIO_M1_OTF_OVERFLOW_ST			BIT(2)
#define MRAWCTL_FHO_M1_OTF_OVERFLOW_ST			BIT(3)

#define REG_MRAW_CTL_INT6_EN					0x0150
#define MRAWCTL_CQ_SUB_THR0_DONE_EN				BIT(1)

#define REG_MRAW_CTL_INT6_STATUS				0x0154
#define MRAWCTL_CQ_SUB_THR0_DONE_ST				BIT(9)

#define REG_MRAW_CQ_EN							0x0400
#define MRAWCQ_SOF_SEL							BIT(2)
#define MRAWCQ_DB_EN							BIT(4)
#define MRAWSCQ_SUBSAMPLE_EN					BIT(21)

#define REG_MRAW_SCQ_START_PERIOD				0x0408

#define REG_MRAW_CQ_SUB_EN						0x0430
#define MRAWCQ_SUB_DB_EN						BIT(4)

#define REG_MRAW_CQ_SUB_THR0_CTL				0x0440
#define MRAWCQ_SUB_THR0_EN						BIT(0)
#define MRAWCQ_SUB_THR0_MODE_IMMEDIATE			BIT(4)

#define REG_MRAW_CQ_SUB_THR0_BASEADDR_2		    0x044C
#define REG_MRAW_CQ_SUB_THR0_BASEADDR_2_MSB	    0x0450
#define REG_MRAW_CQ_SUB_THR0_DESC_SIZE_2		0x0458

#define REG_MRAW_TG_SEN_MODE					0x0500
#define MRAWTG_CMOS_EN							BIT(0)
#define MRAWTG_CMOS_RDY_SEL						BIT(14)
union MRAW_TG_SEN_MODE {
	struct {
		unsigned int TG_CMOS_EN					:  1;
		unsigned int rsv_1						:  1;
		unsigned int TG_SOT_MODE				:  1;
		unsigned int TG_SOT_CLR_MODE			:  1;
		unsigned int TG_DBL_DATA_BUS			:  3;
		unsigned int rsv_7						:  1;
		unsigned int TG_SOF_SRC					:  2;
		unsigned int TG_EOF_SRC					:  2;
		unsigned int TG_PXL_CNT_RST_SRC			:  1;
		unsigned int rsv_13						:  1;
		unsigned int TG_M1_CMOS_RDY_SEL			:  1;
		unsigned int TG_FIFO_FULL_CTL_EN		:  1;
		unsigned int TG_TIME_STP_EN				:  1;
		unsigned int TG_VS_SUB_EN				:  1;
		unsigned int TG_SOF_SUB_EN				:  1;
		unsigned int TG_VSYNC_INT_POL			:  1;
		unsigned int TG_EOF_ALS_RDY_EN			:  1;
		unsigned int rsv_21						:  1;
		unsigned int TG_M1_STAGGER_EN			:  1;
		unsigned int TG_HDR_EN					:  1;
		unsigned int TG_HDR_SEL					:  1;
		unsigned int TG_SOT_DLY_EN				:  1;
		unsigned int TG_VS_IGNORE_STALL_EN		:  1;
		unsigned int TG_LINE_OK_CHECK			:  1;
		unsigned int TG_SOF_SUB_CNT_EXCEPTION_DIS		:  1;
		unsigned int rsv_29						:  3;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_TG_VF_CON						0x0504
#define MRAWTG_VFDATA_EN						BIT(0)
union MRAW_TG_VF_CON {
	struct {
		unsigned int TG_M1_VFDATA_EN			:  1;
		unsigned int TG_SINGLE_MODE				:  1;
		unsigned int rsv_2						:  2;
		unsigned int TG_FR_CON					:  3;
		unsigned int rsv_7						:  1;
		unsigned int TG_SP_DELAY				:  3;
		unsigned int rsv_11						:  1;
		unsigned int TG_SPDELAY_MODE			:  1;
		unsigned int TG_VFDATA_EN_MUX_0_SEL		:  1;
		unsigned int TG_VFDATA_EN_MUX_1_SEL		:  1;
		unsigned int rsv_15						: 17;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_TG_SEN_GRAB_PXL				0x0508
#define REG_MRAW_TG_SEN_GRAB_LIN				0x050C

#define REG_MRAW_TG_PATH_CFG					0x0510
#define MRAWTG_FULL_SEL							BIT(15)
union MRAW_TG_PATH_CFG {
	struct {
		unsigned int TG_SEN_IN_LSB				:  3;
		unsigned int rsv_3						:  1;
		unsigned int TG_JPGINF_EN				:  1;
		unsigned int TG_MEMIN_EN				:  1;
		unsigned int rsv_6						:  1;
		unsigned int TG_JPG_LINEND_EN			:  1;
		unsigned int TG_M1_DB_LOAD_DIS			:  1;
		unsigned int TG_DB_LOAD_SRC				:  1;
		unsigned int TG_DB_LOAD_VSPOL			:  1;
		unsigned int TG_DB_LOAD_HOLD			:  1;
		unsigned int TG_YUV_U2S_DIS				:  1;
		unsigned int TG_YUV_BIN_EN				:  1;
		unsigned int TG_ERR_SEL					:  1;
		unsigned int TG_FULL_SEL				:  1;
		unsigned int TG_FULL_SEL2				:  1;
		unsigned int TG_FLUSH_DISABLE			:  1;
		unsigned int TG_INT_BLANK_DISABLE		:  1;
		unsigned int TG_EXP_ESC					:  1;
		unsigned int TG_SUB_SOF_SRC_SEL			:  2;
		unsigned int rsv_22						: 10;
	} Bits;
	unsigned int Raw;
};

#define REG_MRAW_TG_FRMSIZE_ST					0x0538

#define REG_MRAW_TG_INTER_ST					0x053C

#define MRAWTG_CS_MASK							0x3F00
#define MRAWTG_IDLE_ST							BIT(8)

#define REG_MRAW_TG_FRMSIZE_ST_R				0x056C

#define REG_MRAW_TG_TIME_STAMP_CTL				0x0570
#define REG_MRAW_TG_TIME_STAMP					0x0578
#define REG_MRAW_TG_TIME_STAMP_CNT				0x057C

#define REG_MRAW_TG_HW_TIMER_CTL					0x05C0
union MRAW_TG_TIMER_CTL {
	struct {
		unsigned int TG_HW_TIMER_EN				:  1;
		unsigned int rsv_1						: 31;
	} Bits;
	unsigned int Raw;
};
#define REG_MRAW_TG_HW_TIMER_INC_PERIOD			0x05C4
#define REG_MRAW_TG_HW_DDR_GEN_PLUS_CNT			0x05C8
#define REG_MRAW_TG_HW_QOS_GEN_PLUS_CNT			0x05CC

#define REG_MRAW_TG_HW_TIMER_CNT				0x05d0

#define REG_MRAW_SEP_CTL						0x0600
#define REG_MRAW_SEP_CROP						0x0604
#define REG_MRAW_SEP_VSIZE						0x0608

#define REG_MRAW_MQE_CFG						0x0640
#define REG_MRAW_MQE_IN_IMG						0x0644

#define REG_MRAW_CPI_CFG_0						0x1000
#define REG_MRAW_CPI_CFG_1						0x1004

#define REG_MRAW_MBN_CFG_0						0x1040
#define REG_MRAW_MBN_CFG_1						0x1044
#define REG_MRAW_MBN_CFG_2						0x1048

/* fhg spare 3 for seq num use */
#define REG_MRAW_FRAME_SEQ_NUM					0x1188

#define REG_MRAW_DMA_DBG_SEL					0x2070
#define REG_MRAW_DMA_DBG_PORT					0x2074
#define REG_MRAW_IMGO_BASE_ADDR					0x22E0
#define REG_MRAW_IMGO_BASE_ADDR_MSB				0x22E4
#define REG_MRAW_IMGO_OFST_ADDR					0x22E8
#define REG_MRAW_IMGO_OFST_ADDR_MSB				0x22EC
#define REG_MRAW_IMGO_XSIZE						0x22F0
#define REG_MRAW_IMGO_YSIZE						0x22F4
#define REG_MRAW_IMGO_STRIDE					0x22F8

#define REG_MRAW_IMGO_ORIWDMA_CON0				0x2300
#define REG_MRAW_IMGO_ORIWDMA_CON1				0x2304
#define REG_MRAW_IMGO_ORIWDMA_CON2				0x2308
#define REG_MRAW_IMGO_ORIWDMA_CON3				0x230C
#define REG_MRAW_IMGO_ORIWDMA_CON4				0x2310

#define REG_MRAW_IMGO_ERR_STAT					0x2318

#define REG_MRAW_IMGBO_BASE_ADDR				0x2330
#define REG_MRAW_IMGBO_BASE_ADDR_MSB			0x2334
#define REG_MRAW_IMGBO_OFST_ADDR				0x2338
#define REG_MRAW_IMGBO_OFST_ADDR_MSB			0x233C
#define REG_MRAW_IMGBO_XSIZE					0x2340
#define REG_MRAW_IMGBO_YSIZE					0x2344
#define REG_MRAW_IMGBO_STRIDE					0x2348

#define REG_MRAW_IMGBO_ORIWDMA_CON0				0x2350
#define REG_MRAW_IMGBO_ORIWDMA_CON1				0x2354
#define REG_MRAW_IMGBO_ORIWDMA_CON2				0x2358
#define REG_MRAW_IMGBO_ORIWDMA_CON3				0x235C
#define REG_MRAW_IMGBO_ORIWDMA_CON4				0x2360

#define REG_MRAW_IMGBO_ERR_STAT					0x2368

#define REG_MRAW_CPIO_BASE_ADDR					0x2380
#define REG_MRAW_CPIO_BASE_ADDR_MSB				0x2384
#define REG_MRAW_CPIO_OFST_ADDR					0x2388
#define REG_MRAW_CPIO_OFST_ADDR_MSB				0x238C
#define REG_MRAW_CPIO_XSIZE						0x2390
#define REG_MRAW_CPIO_YSIZE						0x2394
#define REG_MRAW_CPIO_STRIDE					0x2398

#define REG_MRAW_CPIO_ORIWDMA_CON0				0x23A0
#define REG_MRAW_CPIO_ORIWDMA_CON1				0x23A4
#define REG_MRAW_CPIO_ORIWDMA_CON2				0x23A8
#define REG_MRAW_CPIO_ORIWDMA_CON3				0x23AC
#define REG_MRAW_CPIO_ORIWDMA_CON4				0x23B0

#define REG_MRAW_CPIO_ERR_STAT					0x23B8

#define REG_MRAW_FHO_ORIWDMA_CON0				0x23F0
#define REG_MRAW_FHO_ORIWDMA_CON1				0x23F4
#define REG_MRAW_FHO_ORIWDMA_CON2				0x23F8
#define REG_MRAW_FHO_ORIWDMA_CON3				0x23FC
#define REG_MRAW_FHO_ORIWDMA_CON4				0x2400

#define REG_MRAW_M1_CQI_ORIRDMA_CON0			0x2220
#define REG_MRAW_M1_CQI_ORIRDMA_CON1			0x2224
#define REG_MRAW_M1_CQI_ORIRDMA_CON2			0x2228
#define REG_MRAW_M1_CQI_ORIRDMA_CON3			0x222C
#define REG_MRAW_M1_CQI_ORIRDMA_CON4			0x2230

#define REG_MRAW_M2_CQI_ORIRDMA_CON0			0x2290
#define REG_MRAW_M2_CQI_ORIRDMA_CON1			0x2294
#define REG_MRAW_M2_CQI_ORIRDMA_CON2			0x2298
#define REG_MRAW_M2_CQI_ORIRDMA_CON3			0x229C
#define REG_MRAW_M2_CQI_ORIRDMA_CON4			0x22A0

/* STG */
#define REG_MRAW_STG_EN_CTRL					0x2420
#define REG_MRAW_STG_NONE_SAME_PG_SEND_EN_CTRL	0x2440

/* error mask */
#define INT_ST_MASK_MRAW_ERR (\
					MRAWCTL_TG_ERR_ST |\
					MRAWCTL_TG_GBERR_ST |\
					MRAWCTL_CQ_DB_LOAD_ERR_ST |\
					MRAWCTL_DMA_ERR_ST)

#define DMA_ST_MASK_MRAW_ERR (\
					MRAWCTL_DMA_ERR_ST)

#endif	/* _MRAW_REGS_H */
