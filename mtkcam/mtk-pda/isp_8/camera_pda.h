/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_PDA_HW_H__
#define __MTK_PDA_HW_H__

#include <linux/completion.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/ioctl.h>

#include <linux/firmware.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/io.h>

#define PDA_MAGIC               'P'

#define PDA_KERNEL_VERSION 4000

#define kFlexibleROIMaxNum 128
#define kImageBufferNum 4

#define OUT_BYTE_PER_ROI 1200

#define PDA_MAXROI_PER_ROUND 24

#define PDA_MAX_QUANTITY 4

#define PDA_CFG_0_REG (0x000)
#define PDA_CFG_1_REG (0x004)
#define PDA_CFG_2_REG (0x008)
#define PDA_CFG_3_REG (0x00c)
#define PDA_CFG_4_REG (0x010)
#define PDA_CFG_5_REG (0x014)
#define PDA_CFG_6_REG (0x018)
#define PDA_CFG_7_REG (0x01c)
#define PDA_CFG_8_REG (0x020)
#define PDA_CFG_9_REG (0x024)
#define PDA_CFG_10_REG (0x028)
#define PDA_CFG_11_REG (0x02c)
#define PDA_CFG_12_REG (0x030)
#define PDA_CFG_13_REG (0x034)
#define PDA_CFG_14_REG (0x038)
#define PDA_CFG_15_REG (0x03c)
#define PDA_CFG_16_REG (0x040)
#define PDA_CFG_17_REG (0x044)
#define PDA_CFG_18_REG (0x048)
#define PDA_CFG_19_REG (0x04c)
#define PDA_CFG_20_REG (0x050)
#define PDA_CFG_21_REG (0x054)
#define PDA_CFG_22_REG (0x058)
#define PDA_CFG_23_REG (0x05c)
#define PDA_CFG_24_REG (0x060)
#define PDA_CFG_25_REG (0x064)
#define PDA_CFG_26_REG (0x068)
#define PDA_CFG_27_REG (0x06c)
#define PDA_CFG_28_REG (0x070)
#define PDA_PDAI_P1_BASE_ADDR_REG (0x300)
#define PDA_PDATI_P1_BASE_ADDR_REG (0x304)
#define PDA_PDAI_P2_BASE_ADDR_REG (0x308)
#define PDA_PDATI_P2_BASE_ADDR_REG (0x30c)
#define PDA_PDAI_STRIDE_REG (0x310)
#define PDA_PDAI_P1_CON0_REG (0x314)
#define PDA_PDAI_P1_CON1_REG (0x318)
#define PDA_PDAI_P1_CON2_REG (0x31c)
#define PDA_PDAI_P1_CON3_REG (0x320)
#define PDA_PDAI_P1_CON4_REG (0x324)
#define PDA_PDATI_P1_CON0_REG (0x328)
#define PDA_PDATI_P1_CON1_REG (0x32c)
#define PDA_PDATI_P1_CON2_REG (0x330)
#define PDA_PDATI_P1_CON3_REG (0x334)
#define PDA_PDATI_P1_CON4_REG (0x338)
#define PDA_PDAI_P2_CON0_REG (0x33c)
#define PDA_PDAI_P2_CON1_REG (0x340)
#define PDA_PDAI_P2_CON2_REG (0x344)
#define PDA_PDAI_P2_CON3_REG (0x348)
#define PDA_PDAI_P2_CON4_REG (0x34c)
#define PDA_PDATI_P2_CON0_REG (0x350)
#define PDA_PDATI_P2_CON1_REG (0x354)
#define PDA_PDATI_P2_CON2_REG (0x358)
#define PDA_PDATI_P2_CON3_REG (0x35c)
#define PDA_PDATI_P2_CON4_REG (0x360)
#define PDA_PDAO_P1_BASE_ADDR_REG (0x364)
#define PDA_PDAO_P1_XSIZE_REG (0x368)
#define PDA_PDAO_P1_CON0_REG (0x36c)
#define PDA_PDAO_P1_CON1_REG (0x370)
#define PDA_PDAO_P1_CON2_REG (0x374)
#define PDA_PDAO_P1_CON3_REG (0x378)
#define PDA_PDAO_P1_CON4_REG (0x37c)
#define PDA_PDA_DMA_EN_REG (0x380)
#define PDA_PDA_DMA_RST_REG (0x384)
#define PDA_PDA_DMA_TOP_REG (0x388)
#define PDA_PDA_SECURE_REG (0x38c)
#define PDA_PDA_TILE_STATUS_REG (0x390)
#define PDA_PDA_DCM_DIS_REG (0x394)
#define PDA_PDA_DCM_ST_REG (0x398)
#define PDA_PDAI_P1_ERR_STAT_REG (0x39c)
#define PDA_PDATI_P1_ERR_STAT_REG (0x3a0)
#define PDA_PDAI_P2_ERR_STAT_REG (0x3a4)
#define PDA_PDATI_P2_ERR_STAT_REG (0x3a8)
#define PDA_PDAO_P1_ERR_STAT_REG (0x3ac)
#define PDA_PDA_ERR_STAT_EN_REG (0x3b0)
#define PDA_PDA_ERR_STAT_REG (0x3b4)
#define PDA_PDA_TOP_CTL_REG (0x3b8)
#define PDA_PDA_DEBUG_SEL_REG (0x3bc)
#define PDA_PDA_IRQ_TRIG_REG (0x3c0)
#define PDA_PDAI_P1_BASE_ADDR_MSB_REG (0x3c4)
#define PDA_PDATI_P1_BASE_ADDR_MSB_REG (0x3c8)
#define PDA_PDAI_P2_BASE_ADDR_MSB_REG (0x3cc)
#define PDA_PDATI_P2_BASE_ADDR_MSB_REG (0x3d0)
#define PDA_PDAO_P1_BASE_ADDR_MSB_REG (0x3d4)
#define PDA_PDAO_DMA_EXISTED_ECO_REG (0x3d8)
#define PDA_PDA_DEBUG_DATA_REG (0x3dc)
#define PDA_PDALI_P3_BASE_ADDR_REG (0x604)
#define PDA_PDARI_P3_BASE_ADDR_REG (0x608)
#define PDA_PDALI_P4_BASE_ADDR_REG (0x60c)
#define PDA_PDARI_P4_BASE_ADDR_REG (0x610)
#define PDA_PDALI_P5_BASE_ADDR_REG (0x614)
#define PDA_PDARI_P5_BASE_ADDR_REG (0x618)
#define PDA_PDALI_P3_CON0_REG (0x61c)
#define PDA_PDALI_P3_CON1_REG (0x620)
#define PDA_PDALI_P3_CON2_REG (0x624)
#define PDA_PDALI_P3_CON3_REG (0x628)
#define PDA_PDALI_P3_CON4_REG (0x62c)
#define PDA_PDARI_P3_CON0_REG (0x630)
#define PDA_PDARI_P3_CON1_REG (0x634)
#define PDA_PDARI_P3_CON2_REG (0x638)
#define PDA_PDARI_P3_CON3_REG (0x63c)
#define PDA_PDARI_P3_CON4_REG (0x640)
#define PDA_PDALI_P4_CON0_REG (0x644)
#define PDA_PDALI_P4_CON1_REG (0x648)
#define PDA_PDALI_P4_CON2_REG (0x64c)
#define PDA_PDALI_P4_CON3_REG (0x650)
#define PDA_PDALI_P4_CON4_REG (0x654)
#define PDA_PDARI_P4_CON0_REG (0x658)
#define PDA_PDARI_P4_CON1_REG (0x65C)
#define PDA_PDARI_P4_CON2_REG (0x660)
#define PDA_PDARI_P4_CON3_REG (0x664)
#define PDA_PDARI_P4_CON4_REG (0x668)
#define PDA_PDALI_P5_CON0_REG (0x66C)
#define PDA_PDALI_P5_CON1_REG (0x670)
#define PDA_PDALI_P5_CON2_REG (0x674)
#define PDA_PDALI_P5_CON3_REG (0x678)
#define PDA_PDALI_P5_CON4_REG (0x67C)
#define PDA_PDARI_P5_CON0_REG (0x680)
#define PDA_PDARI_P5_CON1_REG (0x684)
#define PDA_PDARI_P5_CON2_REG (0x688)
#define PDA_PDARI_P5_CON3_REG (0x68C)
#define PDA_PDARI_P5_CON4_REG (0x690)
#define PDA_PDA_SECURE_1_REG (0x694)
#define PDA_PDA_SECURE_2_REG (0x698)
#define PDA_PDA_TILE_STATUS_1_REG (0x69c)
#define PDA_PDALI_P3_ERR_STAT_REG (0x6a0)
#define PDA_PDARI_P3_ERR_STAT_REG (0x6a4)
#define PDA_PDALI_P4_ERR_STAT_REG (0x6a8)
#define PDA_PDARI_P4_ERR_STAT_REG (0x6ac)
#define PDA_PDALI_P5_ERR_STAT_REG (0x6b0)
#define PDA_PDARI_P5_ERR_STAT_REG (0x6b4)
#define PDA_PDALI_P3_BASE_ADDR_MSB_REG (0x6b8)
#define PDA_PDARI_P3_BASE_ADDR_MSB_REG (0x6bc)
#define PDA_PDALI_P4_BASE_ADDR_MSB_REG (0x6c0)
#define PDA_PDARI_P4_BASE_ADDR_MSB_REG (0x6c4)
#define PDA_PDALI_P5_BASE_ADDR_MSB_REG (0x6c8)
#define PDA_PDARI_P5_BASE_ADDR_MSB_REG (0x6cc)
#define PDA_PDA_SECURE_3_REG (0x6d0)
#define PDA_PDA_AUTO_TRIG_REG (0x6d4)
#define PDA_DDREN_CFG_REG (0x6fc)

union _REG_PDA_CFG_0_ {
	struct /* 0x0000 */
	{
		unsigned int  PDA_WIDTH        :  16;   /*  0.. 16, 0x0000FFFF */
		unsigned int  PDA_HEIGHT       :  16;   /*  16.. 32, 0xFFFF0080 */
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_1_ {
	struct /* 0x0004 */
	{
		unsigned int  PDA_EN           :  1;    /*  0.. 0, 0x00000001 */
		unsigned int  PDA_PR_XNUM      :  5;    /*  1.. 5, 0x0000003E */
		unsigned int  PDA_PR_YNUM      :  5;    /*  6.. 10, 0x00007C0 */
		unsigned int  PDA_PAT_WIDTH    :  10;   /*  11.. 20, 0x001FF800 */
		unsigned int  PDA_BIN_FCTR     :  2;    /*  21.. 22, 0x00600000 */
		unsigned int  PDA_RNG_ST       :  6;    /*  23.. 28, 0x1F800080 */
		unsigned int  PDA_SHF_0        :  3;    /*  29.. 31, 0xE0000000 */
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_2_ {
	struct /* 0x0008 */
	{
		unsigned int  PDA_SHF_1        :  3;
		unsigned int  PDA_SHF_2        :  3;
		unsigned int  PDA_RGN_NUM      :  7;
		unsigned int  PDA_TBL_STRIDE   :  16;
		unsigned int  rsv_29           :  3;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_3_ {
	struct /* 0x000C */
	{
		unsigned int  PDA_POS_L_0      :  9;
		unsigned int  PDA_POS_L_1      :  9;
		unsigned int  PDA_POS_L_2      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_4_ {
	struct /* 0x0010 */
	{
		unsigned int  PDA_POS_L_3      :  9;
		unsigned int  PDA_POS_L_4      :  9;
		unsigned int  PDA_POS_L_5      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_5_ {
	struct /* 0x0014 */
	{
		unsigned int  PDA_POS_L_6      :  9;
		unsigned int  PDA_POS_L_7      :  9;
		unsigned int  PDA_POS_L_8      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_6_ {
	struct /* 0x0018 */
	{
		unsigned int  PDA_POS_L_9      :  9;
		unsigned int  PDA_POS_L_10     :  9;
		unsigned int  PDA_POS_L_11     :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_7_ {
	struct /* 0x001C */
	{
		unsigned int  PDA_POS_L_12     :  9;
		unsigned int  PDA_POS_L_13     :  9;
		unsigned int  PDA_POS_L_14     :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_8_ {
	struct /* 0x0020 */
	{
		unsigned int  PDA_POS_L_15     :  9;
		unsigned int  PDA_POS_R_0      :  9;
		unsigned int  PDA_POS_R_1      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_9_ {
	struct /* 0x0024 */
	{
		unsigned int  PDA_POS_R_2      :  9;
		unsigned int  PDA_POS_R_3      :  9;
		unsigned int  PDA_POS_R_4      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_10_ {
	struct /* 0x0028 */
	{
		unsigned int  PDA_POS_R_5      :  9;
		unsigned int  PDA_POS_R_6      :  9;
		unsigned int  PDA_POS_R_7      :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_11_ {
	struct /* 0x002C */
	{
		unsigned int  PDA_POS_R_8      :  9;
		unsigned int  PDA_POS_R_9      :  9;
		unsigned int  PDA_POS_R_10     :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_12_ {
	struct /* 0x0030 */
	{
		unsigned int  PDA_POS_R_11     :  9;
		unsigned int  PDA_POS_R_12     :  9;
		unsigned int  PDA_POS_R_13     :  9;
		unsigned int  rsv_27           :  5;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_13_ {
	struct /* 0x0034 */
	{
		unsigned int  PDA_POS_R_14     :  9;
		unsigned int  PDA_POS_R_15     :  9;
		unsigned int  rsv_18           :  14;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_14_ {
	struct /* 0x0038 */
	{
		unsigned int  PDA_B_N          :  2;
		unsigned int  PDA_B_F0         :  5;
		unsigned int  PDA_B_F1         :  5;
		unsigned int  PDA_B_F2         :  5;
		unsigned int  PDA_B_F3         :  5;
		unsigned int  PDA_TBL_INV      :  1;
		unsigned int  PDA_TDBIN_FX     :  2;
		unsigned int  PDA_TDBIN_FY     :  2;
		unsigned int  PDA_FILT_EN0     :  1;
		unsigned int  rsv_28           :  4;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_15_ {
	struct /* 0x003C */
	{
		unsigned int  PDA_FILT_C_0     :  7;
		unsigned int  PDA_FILT_C_1     :  7;
		unsigned int  PDA_FILT_C_2     :  7;
		unsigned int  PDA_FILT_C_3     :  7;
		unsigned int  rsv_28           :  4;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_16_ {
	struct /* 0x0040 */
	{
		unsigned int  PDA_FILT_C_4     :  7;
		unsigned int  PDA_FILT_C_5     :  7;
		unsigned int  PDA_FILT_C_6     :  7;
		unsigned int  PDA_FILT_C_7     :  7;
		unsigned int  rsv_28           :  4;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_17_ {
	struct /* 0x0044 */
	{
		unsigned int  PDA_FILT_C_8     :  7;
		unsigned int  PDA_FILT_C_9     :  7;
		unsigned int  PDA_FILT_C_10    :  7;
		unsigned int  PDA_FILT_C_11    :  7;
		unsigned int  rsv_28           :  4;
	} Bits;
	unsigned int Raw;
};

union _REG_PDA_CFG_18_ {
	struct /* 0x0048 */
	{
		unsigned int  PDA_FILT_C_12    :  7;
		unsigned int  PDA_GRAD_DST     :  4;
		unsigned int  PDA_GRAD_THD    :  12;
		unsigned int  rsv_23           :  9;
	} Bits;
	unsigned int Raw;
};

struct PDA_HW_Register_Group {
	union _REG_PDA_CFG_0_            PDA_CFG_0;
	union _REG_PDA_CFG_1_            PDA_CFG_1;
	union _REG_PDA_CFG_2_            PDA_CFG_2;
	union _REG_PDA_CFG_3_            PDA_CFG_3;
	union _REG_PDA_CFG_4_            PDA_CFG_4;
	union _REG_PDA_CFG_5_            PDA_CFG_5;
	union _REG_PDA_CFG_6_            PDA_CFG_6;
	union _REG_PDA_CFG_7_            PDA_CFG_7;
	union _REG_PDA_CFG_8_            PDA_CFG_8;
	union _REG_PDA_CFG_9_            PDA_CFG_9;
	union _REG_PDA_CFG_10_           PDA_CFG_10;
	union _REG_PDA_CFG_11_           PDA_CFG_11;
	union _REG_PDA_CFG_12_           PDA_CFG_12;
	union _REG_PDA_CFG_13_           PDA_CFG_13;
	union _REG_PDA_CFG_14_           PDA_CFG_14;
	union _REG_PDA_CFG_15_           PDA_CFG_15;
	union _REG_PDA_CFG_16_           PDA_CFG_16;
	union _REG_PDA_CFG_17_           PDA_CFG_17;
	union _REG_PDA_CFG_18_           PDA_CFG_18;
};

struct PDA_Init_Data {
	unsigned int Kversion;
};

//Datastructure for 1024 ROI
struct PDA_Data_t {
	// flexible roi
	unsigned int roi_x[kFlexibleROIMaxNum];
	unsigned int roi_y[kFlexibleROIMaxNum];
	unsigned int roi_h[kFlexibleROIMaxNum];
	unsigned int roi_w[kFlexibleROIMaxNum];
	unsigned int roi_iw[kFlexibleROIMaxNum];
	unsigned int roi_obx[kFlexibleROIMaxNum];
	unsigned int roi_oby[kFlexibleROIMaxNum];
	unsigned int roi_nbx[kFlexibleROIMaxNum];
	unsigned int roi_nby[kFlexibleROIMaxNum];
	unsigned int roi_num;

	// common data
	unsigned int image_size;
	unsigned int table_size;
	unsigned int output_size;

	int fd_left_image[kImageBufferNum];
	int fd_right_image[kImageBufferNum];
	int fd_left_table;
	int fd_right_table;
	int fd_output;

	struct PDA_HW_Register_Group PDA_HW_Register;

	int status;
	unsigned int timeout;
	int sensor_dev;
	int is_inputBuffer_updated;
	int is_outputBuffer_updated;

	unsigned int address_offset[kImageBufferNum];
};

struct pda_mmu {
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attach;
	struct sg_table			*sgt;
};

// PDA clock info.
struct PDA_CLK_STRUCT {
	struct clk *CG_PDA_TOP_MUX;
};

// pda device information
struct PDA_device {
	int HWstatus;
	void __iomem *m_pda_base;
	int irq;
};

enum PDA_CMD_ENUM {
	PDA_CMD_RESET,		/* Reset */
	PDA_CMD_ENQUE_WAITIRQ,	/* PDA Enque And Wait Irq */
	PDA_CMD_GET_VERSION,	/* PDA Get Kernel Version */
	PDA_CMD_PUT_DMA_BUF,
	PDA_CMD_TOTAL,
};

#define PDA_RESET	_IO(PDA_MAGIC, PDA_CMD_RESET)
#define PDA_ENQUE_WAITIRQ    \
	_IOWR(PDA_MAGIC, PDA_CMD_ENQUE_WAITIRQ, struct PDA_Data_t)
#define PDA_GET_VERSION    \
	_IOWR(PDA_MAGIC, PDA_CMD_GET_VERSION, struct PDA_Init_Data)
#define PDA_PUT_DMA_BUF    \
	_IO(PDA_MAGIC, PDA_CMD_PUT_DMA_BUF)

// pda api function
void pda_mmqos_init(struct device *pdev);
void pda_mmqos_bw_set(struct PDA_Data_t *pda_Pdadata);
void pda_mmqos_bw_reset(void);
void pda_init_larb(struct platform_device *pdev);
int pda_devm_clk_get(struct platform_device *pdev);
void pda_clk_prepare_enable(void);
void pda_clk_disable_unprepare(void);
void __iomem *pda_get_camsys_address(void);
unsigned int GetResetBitMask(int PDA_Index);
void pda_debug_log(int32_t debug_log_en);

#endif/*__MTK_PDA_HW_H__*/
