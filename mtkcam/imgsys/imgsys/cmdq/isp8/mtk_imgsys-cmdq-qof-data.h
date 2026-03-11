/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Nick.wen <nick.wen@mediatek.com>
 *
 */

#include "mtk_imgsys-cmdq-qof.h"

/**
 * @brief List all qof related data.
 * 		  such as larb golden.
 */

#define QOF_REG_BASE			(0x34005000)
#define MMPC_REG_BASE		   (0x31B50000)

/* MTCMOS related */
#define ISP_TRAW_PWR_CON		(MMPC_REG_BASE+0x000)
#define ISP_DIP_PWR_CON			(MMPC_REG_BASE+0x004)
#define ISP_MAIN_PWR_CON		(MMPC_REG_BASE+0x008)
#define ISP_VCORE_PWR_CON		(MMPC_REG_BASE+0x00C)
#define ISP_WPE_EIS_PWR_CON		(MMPC_REG_BASE+0x010)
#define ISP_WPE_TNR_PWR_CON		(MMPC_REG_BASE+0x014)
#define ISP_WPE_LITE_PWR_CON	(MMPC_REG_BASE+0x018)

/* CG macro */
#define IMG_CG_IMGSYS_MAIN		(0x34000000)
#define IMG_CG_DIP_NR1_DIP1		(0x34130000)
#define IMG_CG_DIP_NR2_DIP1		(0x34170000)
#define IMG_CG_DIP_TOP_DIP1		(0x34110000)
#define IMG_CG_TRAW_CAP_DIP1	(0x34740000)
#define IMG_CG_TRAW_DIP1		(0x34710000)
#define IMG_CG_WPE1_DIP1		(0x34220000)
#define IMG_CG_WPE2_DIP1		(0x34520000)
#define IMG_CG_WPE3_DIP1		(0x34620000)

/* Larb reg base */
#define QOF_SUPPORT_LARB_ID10	(10)
#define QOF_SUPPORT_LARB_ID11	(11)
#define QOF_SUPPORT_LARB_ID15	(15)
#define QOF_SUPPORT_LARB_ID22	(22)
#define QOF_SUPPORT_LARB_ID23	(23)
#define QOF_SUPPORT_LARB_ID28	(28)
#define QOF_SUPPORT_LARB_ID38	(38)
#define QOF_SUPPORT_LARB_ID39	(39)
#define QOF_SUPPORT_LARB_ID40	(40)
#define QOF_SUPPORT_LARB_ARR_MAX	(QOF_SUPPORT_LARB_ID40 + 1)
#define IMG_LARB10_BASE		 (0x34120000)
#define IMG_LARB11_BASE		 (0x34230000)
#define IMG_LARB15_BASE		 (0x34140000)
#define IMG_LARB22_BASE		 (0x34530000)
#define IMG_LARB23_BASE		 (0x34630000)
#define IMG_LARB28_BASE		 (0x34720000)
#define IMG_LARB38_BASE		 (0x34190000)
#define IMG_LARB39_BASE		 (0x34180000)
#define IMG_LARB40_BASE		 (0x34730000)

/* QOF related */
#define QOF_SPARE_VALUE_TO_BE_FIX		 (0)

#ifndef BIT_ULL
#define BIT_ULL(n)			  ((uint64_t) 1ULL << (n))
#endif /* BIT_ULL */
#define BIT_PERIOD(st, ed)	  (((BIT(ed) - 1) - (BIT(st) - 1)) + BIT(ed))

struct imgsys_cg_data common_cg_data = {
	.clr_ofs = 0x8,
	.sta_ofs = 0
};

const u64 qof_larb_id_bit = (0UL |
	BIT_ULL(QOF_SUPPORT_LARB_ID10)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID11)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID15)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID22)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID23)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID28)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID38)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID39)   |
	BIT_ULL(QOF_SUPPORT_LARB_ID40)
);

#define QOF_SPARE_REG_FOOTPRINT	(QOF_REG_BASE + 0x000000910)
enum QOF_FOOTPRINT_BIT {
	QOF_FOOTPRINT_BIT_GET_SMI_EVENT,
	QOF_FOOTPRINT_BIT_BEFORE_ADD,
	QOF_FOOTPRINT_BIT_AFTER_ADD,
	QOF_FOOTPRINT_BIT_BEFORE_SUB,
	QOF_FOOTPRINT_BIT_AFTER_SUB,
	QOF_FOOTPRINT_BIT_FINISH_LOGIC,
	QOF_FOOTPRINT_BIT_HSK_DONE,
};

enum QOF_USER {
	QOF_USER_AP,
	QOF_USER_GCE,
	QOF_USER_GCE_SWWA, // wa for gce thd lacked
	QOF_USER_MAX,
};

enum CHECK_MODE {
	CHECK_MTCMOS,
	CHECK_QOF,
	CHECK_MODE_MAX,
};

enum IMG_GCE_PWR_THREAD_ID {
	IMG_GCE_THREAD_PWR_START,
	IMG_GCE_THREAD_DIP = IMG_GCE_THREAD_PWR_START,
	IMG_GCE_THREAD_TRAW,
	IMG_GCE_THREAD_WPE_12,
	IMG_GCE_THREAD_WPE_3_LITE,
	IMG_GCE_THREAD_SMI_DUMP,
	IMG_GCE_THREAD_PWR_END = IMG_GCE_THREAD_SMI_DUMP,
};
#define QOF_TOTAL_THREAD (IMG_GCE_THREAD_PWR_END - IMG_GCE_THREAD_PWR_START + 1)

enum QOF_SUPPORT_MODULE {
	QOF_SUPPORT_START,
	QOF_SUPPORT_DIP = QOF_SUPPORT_START,
	QOF_SUPPORT_TRAW,
	QOF_SUPPORT_WPE_EIS,
	QOF_SUPPORT_WPE_TNR,
	QOF_SUPPORT_WPE_LITE,

	QOF_TOTAL_MODULE,
};

/* QOF register */
enum MAPED_RG_LIST {
	MAPED_RG_LIST_START,
	/* MTCMOS related */
	MAPED_RG_ISP_TRAW_PWR_CON = MAPED_RG_LIST_START,
	MAPED_RG_ISP_DIP_PWR_CON,
	MAPED_RG_ISP_MAIN_PWR_CON,
	MAPED_RG_ISP_VCORE_PWR_CON,
	MAPED_RG_ISP_WPE_EIS_PWR_CON,
	MAPED_RG_ISP_WPE_TNR_PWR_CON,
	MAPED_RG_ISP_WPE_LITE_PWR_CON,
	/* CG related */
	MAPED_RG_IMG_CG_IMGSYS_MAIN,
	MAPED_RG_IMG_CG_DIP_NR1_DIP1,
	MAPED_RG_IMG_CG_DIP_NR2_DIP1,
	MAPED_RG_IMG_CG_DIP_TOP_DIP1,
	MAPED_RG_IMG_CG_TRAW_CAP_DIP1,
	MAPED_RG_IMG_CG_TRAW_DIP1,
	MAPED_RG_IMG_CG_WPE1_DIP1,
	MAPED_RG_IMG_CG_WPE2_DIP1,
	MAPED_RG_IMG_CG_WPE3_DIP1,
	/* Larb related */
	MAPED_RG_IMG_LARB10_BASE,
	MAPED_RG_IMG_LARB11_BASE,
	MAPED_RG_IMG_LARB15_BASE,
	MAPED_RG_IMG_LARB22_BASE,
	MAPED_RG_IMG_LARB23_BASE,
	MAPED_RG_IMG_LARB38_BASE,
	MAPED_RG_IMG_LARB39_BASE,
	MAPED_RG_IMG_LARB28_BASE,
	MAPED_RG_IMG_LARB40_BASE,
	/* qof related */
	MAPED_RG_QOF_REG_BASE,
	MAPED_RG_MMPC_REG_BASE,
	MAPED_RG_LIST_ED = MAPED_RG_MMPC_REG_BASE,
};
#define MAPED_RG_LIST_NUM (MAPED_RG_LIST_ED - MAPED_RG_LIST_START + 1)

/* Larb golden */
struct qof_reg_data qof_larb10_golden[] = {
	{0x24, 0x300256},
	{0x524, 0x300256},
	{0x40, 0x1},
	{0x70, 0xffffffff},
	{0x200, 0x2b},
	{0x204, 0x8},
	{0x208, 0x20},
	{0x20c, 0x1d},
	{0x210, 0x19},
	{0x214, 0xf},
	{0x218, 0x1},
	{0x21c, 0x3},
	/* cmd throttle */
	{0x380, 0x9},
	{0x384, 0x9},
	{0x388, 0x9},
	{0x38c, 0x9},
	{0x390, 0x9},
	{0x394, 0x9},
	{0x398, 0x9},
	{0x39c, 0x9},
};

struct qof_reg_data qof_larb11_golden[] = {
	{0x24, 0x300256},
	{0x524, 0x300256},
	{0x40, 0x1},
	{0x70, 0xffffffff},
	{0x200, 0x8},
	{0x204, 0x16},
	{0x208, 0x16},
	{0x20c, 0x24},
	{0x210, 0x1},
	{0x214, 0x1},
	{0x218, 0x1},
	{0x21c, 0x3},
	{0x220, 0x32},
	{0x224, 0x1},
	{0x228, 0x8},
	{0x22c, 0x10},
	{0x230, 0x16},
	{0x234, 0x2},
	{0x238, 0x38},
	{0x380, 0x9},
	{0x384, 0x9},
	{0x388, 0x9},
	{0x38c, 0x9},
	{0x390, 0x9},
	{0x394, 0x9},
	{0x398, 0x9},
	{0x39c, 0x9},
	{0x3a0, 0x9},
	{0x3a4, 0x9},
	{0x3a8, 0x9},
	{0x3ac, 0x9},
	{0x3b0, 0x9},
	{0x3b4, 0x9},
	{0x3b8, 0x9},

};

struct qof_reg_data qof_larb15_golden[] = {
	{0x24,	0x300256},
	{0x524,	0x300256},
	{0x40,	0x1},
	{0x70,	0xffffffff},
	{0x200,	0x2b},
	{0x204,	0x7},
	{0x208,	0x31},
	{0x20c,	0xa},
	{0x210,	0x10},
	{0x214,	0x10},
	{0x218,	0x2b},
	{0x21c,	0x29},
	{0x220,	0x7},
	{0x224,	0x1},
	{0x380,	0x9},
	{0x384,	0x9},
	{0x388,	0x9},
	{0x38c,	0x9},
	{0x390,	0x9},
	{0x394,	0x9},
	{0x398,	0x9},
	{0x39c,	0x9},
	{0x3a0,	0x9},
	{0x3a4,	0x9},
};

struct qof_reg_data qof_larb22_golden[] = {
	{0x24,	0x300256},
	{0x524,	0x300256},
	{0x40,	0x1},
	{0x70,	0xffffffff},
	{0x200,	0x8},
	{0x204,	0x16},
	{0x208,	0x16},
	{0x20c,	0x24},
	{0x210,	0x1},
	{0x214,	0x1},
	{0x218,	0x1},
	{0x21c,	0x3},
	{0x220,	0x32},
	{0x224,	0x1},
	{0x228,	0x8},
	{0x22c,	0x10},
	{0x230,	0x16},
	{0x234,	0x2},
	{0x238,	0x38},
	{0x380,	0x9},
	{0x384,	0x9},
	{0x388,	0x9},
	{0x38c,	0x9},
	{0x390,	0x9},
	{0x394,	0x9},
	{0x398,	0x9},
	{0x39c,	0x9},
	{0x3a0,	0x9},
	{0x3a4,	0x9},
	{0x3a8,	0x9},
	{0x3ac,	0x9},
	{0x3b0,	0x9},
	{0x3b4,	0x9},
	{0x3b8,	0x9},
};

struct qof_reg_data qof_larb23_golden[] = {
	{0x24,	0x300256},
	{0x524,	0x300256},
	{0x40,	0x1},
	{0x70,	0xffffffff},
	{0x200,	0x8},
	{0x204,	0x16},
	{0x208,	0x16},
	{0x20c,	0x24},
	{0x210,	0x1},
	{0x214,	0x1},
	{0x218,	0x1},
	{0x21c,	0x3},
	{0x220,	0x32},
	{0x224,	0x1},
	{0x228,	0x8},
	{0x22c,	0x10},
	{0x230,	0x16},
	{0x234,	0x2},
	{0x238,	0x38},
	{0x380,	0x9},
	{0x384,	0x9},
	{0x388,	0x9},
	{0x38c,	0x9},
	{0x390,	0x9},
	{0x394,	0x9},
	{0x398,	0x9},
	{0x39c,	0x9},
	{0x3a0,	0x9},
	{0x3a4,	0x9},
	{0x3a8,	0x9},
	{0x3ac,	0x9},
	{0x3b0,	0x9},
	{0x3b4,	0x9},
	{0x3b8,	0x9},
};

struct qof_reg_data qof_larb38_golden[] = {
	{0x24,	0x300256},
	{0x524,	0x300256},
	{0x40,	0x1},
	{0x70,	0xffffffff},
	{0x200,	0x29},
	{0x204,	0x40},
	{0x208,	0x40},
	{0x20c,	0x7},
	{0x210,	0x4},
	{0x214,	0x40},
	{0x218,	0x4},
	{0x21c,	0x18},
	{0x220,	0x1},
	{0x224,	0x1},
	{0x228,	0x1},
	{0x22c,	0x7},
	{0x230,	0x4},
	{0x234,	0x1},
	{0x380,	0x9},
	{0x384,	0x9},
	{0x388,	0x9},
	{0x38c,	0x9},
	{0x390,	0x9},
	{0x394,	0x9},
	{0x398,	0x9},
	{0x39c,	0x9},
	{0x3a0,	0x9},
	{0x3a4,	0x9},
	{0x3a8,	0x9},
	{0x3ac,	0x9},
	{0x3b0,	0x9},
	{0x3b4,	0x9},
};

struct qof_reg_data qof_larb39_golden[] = {
	{0x24,	0x300256},
	{0x524,	0x300256},
	{0x40,	0x1},
	{0x70,	0xffffffff},
	{0x200, 0x16},
	{0x204, 0x4},
	{0x208, 0x4},
	{0x20c, 0x8},
	{0x210, 0x4},
	{0x214, 0x6},
	{0x218, 0x6},
	{0x21c, 0x13},
	{0x220, 0x11},
	{0x224, 0x20},
	{0x228, 0x11},
	{0x22c, 0x1},
	{0x230, 0x1},
	{0x234, 0x1},
	{0x238, 0x9},
	{0x23c, 0x8},
	{0x240, 0x4},
	{0x244, 0x6},
	{0x248, 0x6},
	{0x380, 0x9},
	{0x384, 0x9},
	{0x388, 0x9},
	{0x38c, 0x9},
	{0x390, 0x9},
	{0x394, 0x9},
	{0x398, 0x9},
	{0x39c, 0x9},
	{0x3a0, 0x9},
	{0x3a4, 0x9},
	{0x3a8, 0x9},
	{0x3ac, 0x9},
	{0x3b0, 0x9},
	{0x3b4, 0x9},
	{0x3b8,	0x9},
	{0x3bc,	0x9},
	{0x3c0,	0x9},
	{0x3c4,	0x9},
	{0x3c8,	0x9},
};

struct qof_reg_data qof_larb28_golden[] = {
	{0x24, 0x300256},
	{0x524, 0x300256},
	{0x40, 0x1},
	{0x70, 0xffffffff},
	{0x200, 0x2b},
	{0x204, 0x8},
	{0x208, 0x31},
	{0x20c, 0x10},
	{0x210, 0x26},
	{0x214, 0x15},
	{0x218, 0x1},
	{0x21c, 0x10},
	{0x380, 0x9},
	{0x384, 0x9},
	{0x388, 0x9},
	{0x38c, 0x9},
	{0x390, 0x9},
	{0x394, 0x9},
	{0x398, 0x9},
	{0x39c, 0x9},
};

struct qof_reg_data qof_larb40_golden[] = {
	{0x24, 0x300256},
	{0x524, 0x300256},
	{0x40, 0x1},
	{0x70, 0xffffffff},
	{0x200, 0x9},
	{0x204, 0x7},
	{0x208, 0x7},
	{0x20c, 0xb},
	{0x210, 0xf},
	{0x214, 0x1d},
	{0x218, 0x13},
	{0x21c, 0x6},
	{0x220, 0x1},
	{0x224, 0x1},
	{0x228, 0x1},
	{0x22c, 0x6},
	{0x230, 0x9},
	{0x234, 0x7},
	{0x238, 0xe},
	{0x23c, 0x3},
	{0x380, 0x9},
	{0x384, 0x9},
	{0x388, 0x9},
	{0x38c, 0x9},
	{0x390, 0x9},
	{0x394, 0x9},
	{0x398, 0x9},
	{0x39c, 0x9},
	{0x3a0, 0x9},
	{0x3a4, 0x9},
	{0x3a8, 0x9},
	{0x3ac, 0x9},
	{0x3b0, 0x9},
	{0x3b4, 0x9},
	{0x3b8, 0x9},
	{0x3bc, 0x9},
};

/* Larb data */
struct qof_larb_info qof_larb10_info = {
	.reg_ba = IMG_LARB10_BASE,
	.reg_list_size = sizeof(qof_larb10_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb10_golden,
};

struct qof_larb_info qof_larb11_info = {
	.reg_ba = IMG_LARB11_BASE,
	.reg_list_size = sizeof(qof_larb11_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb11_golden,
};

struct qof_larb_info qof_larb15_info = {
	.reg_ba = IMG_LARB15_BASE,
	.reg_list_size = sizeof(qof_larb15_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb15_golden,
};

struct qof_larb_info qof_larb22_info = {
	.reg_ba = IMG_LARB22_BASE,
	.reg_list_size = sizeof(qof_larb22_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb22_golden,
};

struct qof_larb_info qof_larb23_info = {
	.reg_ba = IMG_LARB23_BASE,
	.reg_list_size = sizeof(qof_larb23_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb23_golden,
};

struct qof_larb_info qof_larb38_info = {
	.reg_ba = IMG_LARB38_BASE,
	.reg_list_size = sizeof(qof_larb38_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb38_golden,
};

struct qof_larb_info qof_larb39_info = {
	.reg_ba = IMG_LARB39_BASE,
	.reg_list_size = sizeof(qof_larb39_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb39_golden,
};

struct qof_larb_info qof_larb28_info = {
	.reg_ba = IMG_LARB28_BASE,
	.reg_list_size = sizeof(qof_larb28_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb28_golden,
};

struct qof_larb_info qof_larb40_info = {
	.reg_ba = IMG_LARB40_BASE,
	.reg_list_size = sizeof(qof_larb40_golden)/sizeof(struct qof_reg_data),
	.larb_reg_list = qof_larb40_golden,
};

enum MTCMOS_REG_NAME {
	MTCMOS_ISP_PWR_CON,

	MTCMOS_REG_TOTAL_NUM,
};

enum QOF_REG_NAME {
	QOF_IMG_EVENT_CNT_ADD,
	QOF_IMG_ITC_SRC_SEL,
	QOF_IMG_HW_CLR_EN,
	QOF_IMG_HW_SET_EN,
	QOF_IMG_OFF_ITC_W_EN,
	QOF_IMG_ON_ITC_W_EN,
	QOF_IMG_HW_SEQ_EN,
	QOF_IMG_RTC_EN,
	QOF_IMG_GCE_RESTORE_EN,
	QOF_IMG_GCE_SAVE_EN,
	QOF_IMG_PWR_ACK_2ND_WAIT_TH,
	QOF_IMG_PWR_ACK_WAIT_TH,
	QOF_IMG_QOF_ENG_EN,
	QOF_IMG_INTX_STATUS,
	QOF_IMG_EVENT_CNT_SUB,
	QOF_IMG_GCE_SAVE_DONE,
	QOF_IMG_POWER_STATE,
	QOF_IMG_GCE_RESTORE_DONE,
	QOF_IMG_APMCU_SET,
	QOF_IMG_APMCU_CLR,
	QOF_IMG_INTX_STATUS_FOR_CQ,
	QOF_IMG_QOF_EVENT_CNT,
	QOF_IMG_QOF_VOTER_DBG,
	QOF_IMG_QOF_DONE_STATUS,
	QOF_IMG_ITC_STATUS,
	QOF_IMG_QOF_STATE_DBG,
	QOF_IMG_QOF_MTC_ST_LSB,
	QOF_IMG_QOF_MTC_ST_MSB2,

	QOF_REG_TOTAL_NUM,
};

const struct reg_table_unit mtcmos_reg_table[QOF_TOTAL_MODULE][MTCMOS_REG_TOTAL_NUM] = {
	// DIP
	{
		{
			// MTCMOS_ISP_PWR_CON
			.addr = ISP_DIP_PWR_CON,
			.val = BIT(30)|BIT(31),
			.mask = BIT(30)|BIT(31),
		},
	},
	// TRAW
	{
		{
			// MTCMOS_ISP_PWR_CON
			.addr = ISP_TRAW_PWR_CON,
			.val = BIT(30)|BIT(31),
			.mask = BIT(30)|BIT(31),
		},
	},
	// WPE_1_EIS
	{
		{
			// MTCMOS_ISP_PWR_CON
			.addr = ISP_WPE_EIS_PWR_CON,
			.val = BIT(30)|BIT(31),
			.mask = BIT(30)|BIT(31),
		},
	},
	// WPE_2_TNR
	{
		{
			// MTCMOS_ISP_PWR_CON
			.addr = ISP_WPE_TNR_PWR_CON,
			.val = BIT(30)|BIT(31),
			.mask = BIT(30)|BIT(31),
		},
	},
	// WPE_3_LITE
	{
		{
			// MTCMOS_ISP_PWR_CON
			.addr = ISP_WPE_LITE_PWR_CON,
			.val = BIT(30)|BIT(31),
			.mask = BIT(30)|BIT(31),
		},
	},

};

const struct reg_table_unit qof_reg_table[QOF_TOTAL_MODULE][QOF_REG_TOTAL_NUM] = {
	// QOF_SUPPORT_DIP
	{
		{
			// QOF_IMG_EVENT_CNT_ADD
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(7),
			.mask = BIT(7),
		},
		{
			// QOF_IMG_ITC_SRC_SEL
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(25),
			.mask = BIT(25),
		},
		{
			// QOF_IMG_HW_CLR_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(14),
			.mask = BIT(14),
		},
		{
			// QOF_IMG_HW_SET_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(13),
			.mask = BIT(13),
		},
		{
			// QOF_IMG_OFF_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(12),
			.mask = BIT(12),
		},
		{
			// QOF_IMG_ON_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(11),
			.mask = BIT(11),
		},
		{
			// QOF_IMG_HW_SEQ_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(10),
			.mask = BIT(10),
		},
		{
			// QOF_IMG_RTC_EN
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(9),
			.mask = BIT(9),
		},
		{
			// QOF_IMG_GCE_RESTORE_EN
			.addr = QOF_REG_BASE + 0x00000034,
			.val = BIT(2),
			.mask = BIT(2),
		},
		{
			// QOF_IMG_GCE_SAVE_EN
			.addr = QOF_REG_BASE + 0x00000034,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_PWR_ACK_2ND_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000040,
			.val = 0x5140000, // bit 16~31 =‘h514
			.mask = BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|
				BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|
				BIT(30)|BIT(31)//BIT_PERIOD(16, 31),
		},
		{
			// QOF_IMG_PWR_ACK_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000040,
			.val = 0x514, //  =‘h514
			.mask = BIT_PERIOD(0, 15)
		},
		{
			// QOF_IMG_QOF_ENG_EN
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_INTX_STATUS
			.addr = QOF_REG_BASE + 0x000000A4,
			.val = 0xFFFFFFFF,
			.mask = 0xFFFFFFFF
		},
		{
			// QOF_IMG_EVENT_CNT_SUB
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(8),
			.mask = BIT(8),
		},
		{
			// QOF_IMG_GCE_SAVE_DONE
			.addr = QOF_REG_BASE + 0x00000034,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_POWER_STATE
			.addr = QOF_REG_BASE + 0x00000038,
			.val = BIT_PERIOD(0, 7),
			.mask = BIT_PERIOD(0, 7),
		},
		{
			// QOF_IMG_GCE_RESTORE_DONE
			.addr = QOF_REG_BASE + 0x00000034,
			.val = BIT(3),
			.mask = BIT(3),
		},
		{
			// QOF_IMG_APMCU_SET
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_APMCU_CLR
			.addr = QOF_REG_BASE + 0x00000020,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// DIP_REG_CQ_INT2_STATUS
			.addr = QOF_REG_BASE + 0x000000AC,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_QOF_EVENT_CNT
			.addr = QOF_REG_BASE + 0x0000003C,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_VOTER_DBG
			.addr = QOF_REG_BASE + 0x00000028,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_DONE_STATUS
			.addr = QOF_REG_BASE + 0x00000024,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_ITC_STATUS
			.addr = QOF_REG_BASE + 0x00000004,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_STATE_DBG
			.addr = QOF_REG_BASE + 0x00000038,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_LSB
			.addr = QOF_REG_BASE + 0x00000200,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_MSB2
			.addr = QOF_REG_BASE + 0x00000204,
			.val = BIT(3),
			.mask = BIT(3)
		},
	},

	//QOF_SUPPORT_TRAW
	{
		{
			// QOF_IMG_EVENT_CNT_ADD
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(7),
			.mask = BIT(7),
		},
		{
			// QOF_IMG_ITC_SRC_SEL
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(26),
			.mask = BIT(26),
		},
		{
			// QOF_IMG_HW_CLR_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(14),
			.mask = BIT(14),
		},
		{
			// QOF_IMG_HW_SET_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(13),
			.mask = BIT(13),
		},
		{
			// QOF_IMG_OFF_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(12),
			.mask = BIT(12),
		},
		{
			// QOF_IMG_ON_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(11),
			.mask = BIT(11),
		},
		{
			// QOF_IMG_HW_SEQ_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(10),
			.mask = BIT(10),
		},
		{
			// QOF_IMG_RTC_EN
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(9),
			.mask = BIT(9),
		},
		{
			// QOF_IMG_GCE_RESTORE_EN
			.addr = QOF_REG_BASE + 0x00000224,
			.val = BIT(2),
			.mask = BIT(2),
		},
		{
			// QOF_IMG_GCE_SAVE_EN
			.addr = QOF_REG_BASE + 0x00000224,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_PWR_ACK_2ND_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000230,
			.val = 0x5140000, // bit 16~31 =‘h514
			.mask = BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|
				BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|
				BIT(30)|BIT(31)//BIT_PERIOD(16, 31),
		},
		{
			// QOF_IMG_PWR_ACK_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000230,
			.val = 0x514, //  =‘h514
			.mask = BIT_PERIOD(0, 15)
		},
		{
			// QOF_IMG_QOF_ENG_EN
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(4), // NEED TO = 1
			.mask = BIT(4)
		},
		{
			// QOF_IMG_INTX_STATUS
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX
		},
		{
			// QOF_IMG_EVENT_CNT_SUB
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(8),
			.mask = BIT(8),
		},
		{
			// QOF_IMG_GCE_SAVE_DONE
			.addr = QOF_REG_BASE + 0x00000224,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_POWER_STATE
			.addr = QOF_REG_BASE + 0x00000228,
			.val = BIT_PERIOD(0, 7),
			.mask = BIT_PERIOD(0, 7),
		},
		{
			// QOF_IMG_GCE_RESTORE_DONE
			.addr = QOF_REG_BASE + 0x00000224,
			.val = BIT(3),
			.mask = BIT(3),
		},
		{
			// QOF_IMG_APMCU_SET
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_APMCU_CLR
			.addr = QOF_REG_BASE + 0x00000210,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_INTX_STATUS
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX,
		},
		{
			// QOF_IMG_QOF_EVENT_CNT
			.addr = QOF_REG_BASE + 0x0000022C,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_VOTER_DBG
			.addr = QOF_REG_BASE + 0x00000218,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_DONE_STATUS
			.addr = QOF_REG_BASE + 0x00000214,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_ITC_STATUS
			.addr = QOF_REG_BASE + 0x00000004,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_STATE_DBG
			.addr = QOF_REG_BASE + 0x00000228,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_LSB
			.addr = QOF_REG_BASE + 0x00000400,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_MSB2
			.addr = QOF_REG_BASE + 0x00000404,
			.val = BIT(3),
			.mask = BIT(3)
		},
	},

	// QOF_SUPPORT_WPE_EIS
	{
		{
			// QOF_IMG_EVENT_CNT_ADD
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(7),
			.mask = BIT(7),
		},
		{
			// QOF_IMG_ITC_SRC_SEL
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(27),
			.mask = BIT(27),
		},
		{
			// QOF_IMG_HW_CLR_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(14),
			.mask = BIT(14),
		},
		{
			// QOF_IMG_HW_SET_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(13),
			.mask = BIT(13),
		},
		{
			// QOF_IMG_OFF_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(12),
			.mask = BIT(12),
		},
		{
			// QOF_IMG_ON_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(11),
			.mask = BIT(11),
		},
		{
			// QOF_IMG_HW_SEQ_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(10),
			.mask = BIT(10),
		},
		{
			// QOF_IMG_RTC_EN
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(9),
			.mask = BIT(9),
		},
		{
			// QOF_IMG_GCE_RESTORE_EN
			.addr = QOF_REG_BASE + 0x00000424,
			.val = BIT(2),
			.mask = BIT(2),
		},
		{
			// QOF_IMG_GCE_SAVE_EN
			.addr = QOF_REG_BASE + 0x00000424,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_PWR_ACK_2ND_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000430,
			.val = 0x5140000, // bit 16~31 =‘h514
			.mask = BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|
				BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|
				BIT(30)|BIT(31)//BIT_PERIOD(16, 31),
		},
		{
			// QOF_IMG_PWR_ACK_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000430,
			.val = 0x514, //  =‘h514
			.mask = BIT_PERIOD(0, 15)
		},
		{
			// QOF_IMG_QOF_ENG_EN
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(5), // NEED TO = 1
			.mask = BIT(5)
		},
		{
			// QOF_IMG_INTX_STATUS = PQDIP_A_REG_INT2_STATUS = QOF_IMG_INT6_STATUS_3
			.addr = QOF_REG_BASE + 0x000004A4,
			.val = 0xFFFFFFFF,
			.mask = 0xFFFFFFFF
		},
		{
			// QOF_IMG_EVENT_CNT_SUB
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(8),
			.mask = BIT(8),
		},
		{
			// QOF_IMG_GCE_SAVE_DONE
			.addr = QOF_REG_BASE + 0x00000424,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_POWER_STATE
			.addr = QOF_REG_BASE + 0x00000428,
			.val = BIT_PERIOD(0, 7),
			.mask = BIT_PERIOD(0, 7),
		},
		{
			// QOF_IMG_GCE_RESTORE_DONE
			.addr = QOF_REG_BASE + 0x00000424,
			.val = BIT(3),
			.mask = BIT(3),
		},
		{
			// QOF_IMG_APMCU_SET
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_APMCU_CLR
			.addr = QOF_REG_BASE + 0x00000410,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_INTX_STATUS
			.addr = QOF_REG_BASE + 0x000004AC,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_QOF_EVENT_CNT
			.addr = QOF_REG_BASE + 0x0000042C,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_VOTER_DBG
			.addr = QOF_REG_BASE + 0x00000418,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_DONE_STATUS
			.addr = QOF_REG_BASE + 0x00000414,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_ITC_STATUS
			.addr = QOF_REG_BASE + 0x00000004,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_STATE_DBG
			.addr = QOF_REG_BASE + 0x00000428,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_LSB
			.addr = QOF_REG_BASE + 0x00000600,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_MSB2
			.addr = QOF_REG_BASE + 0x00000604,
			.val = BIT(3),
			.mask = BIT(3)
		},
	},

	// QOF_SUPPORT_WPE_TNR
	{
		{
			// QOF_IMG_EVENT_CNT_ADD
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(7),
			.mask = BIT(7),
		},
		{
			// QOF_IMG_ITC_SRC_SEL
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(28),
			.mask = BIT(28),
		},
		{
			// QOF_IMG_HW_CLR_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(14),
			.mask = BIT(14),
		},
		{
			// QOF_IMG_HW_SET_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(13),
			.mask = BIT(13),
		},
		{
			// QOF_IMG_OFF_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(12),
			.mask = BIT(12),
		},
		{
			// QOF_IMG_ON_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(11),
			.mask = BIT(11),
		},
		{
			// QOF_IMG_HW_SEQ_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(10),
			.mask = BIT(10),
		},
		{
			// QOF_IMG_RTC_EN
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(9),
			.mask = BIT(9),
		},
		{
			// QOF_IMG_GCE_RESTORE_EN
			.addr = QOF_REG_BASE + 0x00000624,
			.val = BIT(2),
			.mask = BIT(2),
		},
		{
			// QOF_IMG_GCE_SAVE_EN
			.addr = QOF_REG_BASE + 0x00000624,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_PWR_ACK_2ND_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000630,
			.val = 0x5140000, // bit 16~31 =‘h514
			.mask = BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|
				BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|
				BIT(30)|BIT(31)//BIT_PERIOD(16, 31),
		},
		{
			// QOF_IMG_PWR_ACK_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000630,
			.val = 0x514, //  =‘h514
			.mask = BIT_PERIOD(0, 15)
		},
		{
			// QOF_IMG_QOF_ENG_EN
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(6), // NEED TO = 1
			.mask = BIT(6)
		},
		{
			// QOF_IMG_INTX_STATUS
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX
		},
		{
			// QOF_IMG_EVENT_CNT_SUB
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(8),
			.mask = BIT(8),
		},
		{
			// QOF_IMG_GCE_SAVE_DONE
			.addr = QOF_REG_BASE + 0x00000624,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_POWER_STATE
			.addr = QOF_REG_BASE + 0x00000628,
			.val = BIT_PERIOD(0, 7),
			.mask = BIT_PERIOD(0, 7),
		},
		{
			// QOF_IMG_GCE_RESTORE_DONE
			.addr = QOF_REG_BASE + 0x00000624,
			.val = BIT(3),
			.mask = BIT(3),
		},
		{
			// QOF_IMG_APMCU_SET
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_APMCU_CLR
			.addr = QOF_REG_BASE + 0x00000610,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_INTX_STATUS_FOR_CQ: QOF_SPARE_VALUE_TO_BE_FIX
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX,
		},
		{
			// QOF_IMG_QOF_EVENT_CNT
			.addr = QOF_REG_BASE + 0x0000062C,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_VOTER_DBG
			.addr = QOF_REG_BASE + 0x00000618,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_DONE_STATUS
			.addr = QOF_REG_BASE + 0x00000614,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_ITC_STATUS
			.addr = QOF_REG_BASE + 0x00000004,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_STATE_DBG
			.addr = QOF_REG_BASE + 0x00000628,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_LSB
			.addr = QOF_REG_BASE + 0x00000800,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_MSB2
			.addr = QOF_REG_BASE + 0x00000804,
			.val = BIT(3),
			.mask = BIT(3)
		},
	},

	// QOF_SUPPORT_WPE_LITE
	{
		{
			// QOF_IMG_EVENT_CNT_ADD
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(7),
			.mask = BIT(7),
		},
		{
			// QOF_IMG_ITC_SRC_SEL
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(29),
			.mask = BIT(29),
		},
		{
			// QOF_IMG_HW_CLR_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(14),
			.mask = BIT(14),
		},
		{
			// QOF_IMG_HW_SET_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(13),
			.mask = BIT(13),
		},
		{
			// QOF_IMG_OFF_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(12),
			.mask = BIT(12),
		},
		{
			// QOF_IMG_ON_ITC_W_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(11),
			.mask = BIT(11),
		},
		{
			// QOF_IMG_HW_SEQ_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(10),
			.mask = BIT(10),
		},
		{
			// QOF_IMG_RTC_EN
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(9),
			.mask = BIT(9),
		},
		{
			// QOF_IMG_GCE_RESTORE_EN
			.addr = QOF_REG_BASE + 0x00000824,
			.val = BIT(2),
			.mask = BIT(2),
		},
		{
			// QOF_IMG_GCE_SAVE_EN
			.addr = QOF_REG_BASE + 0x00000824,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_PWR_ACK_2ND_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000830,
			.val = 0x5140000, // bit 16~31 =‘h514
			.mask = BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|
				BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|
				BIT(30)|BIT(31)//BIT_PERIOD(16, 31),
		},
		{
			// QOF_IMG_PWR_ACK_WAIT_TH
			.addr = QOF_REG_BASE + 0x00000830,
			.val = 0x514, //  =‘h514
			.mask = BIT_PERIOD(0, 15)
		},
		{
			// QOF_IMG_QOF_ENG_EN
			.addr = QOF_REG_BASE + 0x00000000,
			.val = BIT(7), // NEED TO = 1
			.mask = BIT(7)
		},
		{
			// QOF_IMG_INTX_STATUS = QOF_SPARE_VALUE_TO_BE_FIX
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX
		},
		{
			// QOF_IMG_EVENT_CNT_SUB
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(8),
			.mask = BIT(8),
		},
		{
			// QOF_IMG_GCE_SAVE_DONE
			.addr = QOF_REG_BASE + 0x00000824,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_POWER_STATE
			.addr = QOF_REG_BASE + 0x00000828,
			.val = BIT_PERIOD(0, 7),
			.mask = BIT_PERIOD(0, 7),
		},
		{
			// QOF_IMG_GCE_RESTORE_DONE
			.addr = QOF_REG_BASE + 0x00000824,
			.val = BIT(3),
			.mask = BIT(3),
		},
		{
			// QOF_IMG_APMCU_SET
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(0),
			.mask = BIT(0),
		},
		{
			// QOF_IMG_APMCU_CLR
			.addr = QOF_REG_BASE + 0x00000810,
			.val = BIT(1),
			.mask = BIT(1),
		},
		{
			// QOF_IMG_INTX_STATUS_FOR_CQ: QOF_SPARE_VALUE_TO_BE_FIX
			.addr = QOF_REG_BASE + QOF_SPARE_VALUE_TO_BE_FIX,
			.val = QOF_SPARE_VALUE_TO_BE_FIX,
			.mask = QOF_SPARE_VALUE_TO_BE_FIX,
		},
		{
			// QOF_IMG_QOF_EVENT_CNT
			.addr = QOF_REG_BASE + 0x0000082C,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_VOTER_DBG
			.addr = QOF_REG_BASE + 0x00000818,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_DONE_STATUS
			.addr = QOF_REG_BASE + 0x00000814,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_ITC_STATUS
			.addr = QOF_REG_BASE + 0x00000004,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_STATE_DBG
			.addr = QOF_REG_BASE + 0x00000828,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_LSB
			.addr = QOF_REG_BASE + 0x00000A00,
			.val = BIT(3),
			.mask = BIT(3)
		},
		{
			// QOF_IMG_QOF_MTC_ST_MSB2
			.addr = QOF_REG_BASE + 0x00000A04,
			.val = BIT(3),
			.mask = BIT(3)
		},
	},
};
