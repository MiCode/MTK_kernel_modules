// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */

#ifndef __MT_MET_EMI_BM_BASIC_H__
#define __MT_MET_EMI_BM_BASIC_H__

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "sspm_met_ipi_handle.h"
#endif
#endif


/*define MARGAUX*/

#define EMI_VER_MAJOR  3
#define EMI_VER_MINOR  5

#define MET_MAX_DRAM_CH_NUM     4

#define FILE_NODE_DATA_LEN 512
#define WSCT_AMOUNT 6
#define TSCT_AMOUNT 3


#define DRAM_EMI_BASECLOCK_RATE_LP4     4
#define DRAM_EMI_BASECLOCK_RATE_LP3     2

#define DRAM_IO_BUS_WIDTH_LP5           16
#define DRAM_IO_BUS_WIDTH_LP4           16
#define DRAM_IO_BUS_WIDTH_LP3           32

#define DRAM_DATARATE   2

#define DRAM_FREQ_DEFAULT  3733
#define DDR_RATIO_DEFAULT  8
#define DRAM_TYPE_DEFAULT  3

#define ADDR_EMI        ((unsigned long)BaseAddrEMI)

#define MASK_MASTER     0xFF
#define MASK_TRANS_TYPE 0xFF

#define BM_MASTER_M0            (0x01)
#define BM_MASTER_M1            (0x02)
#define BM_MASTER_M2            (0x04)
#define BM_MASTER_M3            (0x08)
#define BM_MASTER_M4            (0x10)
#define BM_MASTER_M5            (0x20)
#define BM_MASTER_M6            (0x40)
#define BM_MASTER_M7            (0x80)
#define BM_MASTER_ALL           (0xFF)


enum BM_RW_Type {
	BM_BOTH_READ_WRITE,
	BM_READ_ONLY,
	BM_WRITE_ONLY
};

enum {
	BM_TRANS_TYPE_1BEAT = 0x0,
	BM_TRANS_TYPE_2BEAT,
	BM_TRANS_TYPE_3BEAT,
	BM_TRANS_TYPE_4BEAT,
	BM_TRANS_TYPE_5BEAT,
	BM_TRANS_TYPE_6BEAT,
	BM_TRANS_TYPE_7BEAT,
	BM_TRANS_TYPE_8BEAT,
	BM_TRANS_TYPE_9BEAT,
	BM_TRANS_TYPE_10BEAT,
	BM_TRANS_TYPE_11BEAT,
	BM_TRANS_TYPE_12BEAT,
	BM_TRANS_TYPE_13BEAT,
	BM_TRANS_TYPE_14BEAT,
	BM_TRANS_TYPE_15BEAT,
	BM_TRANS_TYPE_16BEAT,
	BM_TRANS_TYPE_1Byte = 0 << 4,
	BM_TRANS_TYPE_2Byte = 1 << 4,
	BM_TRANS_TYPE_4Byte = 2 << 4,
	BM_TRANS_TYPE_8Byte = 3 << 4,
	BM_TRANS_TYPE_16Byte = 4 << 4,
	BM_TRANS_TYPE_32Byte = 5 << 4,
	BM_TRANS_TYPE_BURST_WRAP = 0 << 7,
	BM_TRANS_TYPE_BURST_INCR = 1 << 7
};

enum {
	BM_TRANS_RW_DEFAULT = 0x0,
	BM_TRANS_RW_READONLY,
	BM_TRANS_RW_WRITEONLY,
	BM_TRANS_RW_RWBOTH
};

enum {
	BM_WSCT_RW_DISABLE = 0x0,
	BM_WSCT_RW_READONLY,
	BM_WSCT_RW_WRITEONLY,
	BM_WSCT_RW_RWBOTH
};

/*coda busid 12bit, but HW support 16 bit*/
#define EMI_BMID_MASK				(0xFFFF)
#define BM_COUNTER_MAX				(21)

enum {
	BUS_MON_EN_SHIFT = 0,
	BUS_MON_PAUSE_SHIFT = 1,
	BUS_MON_IDLE_SHIFT = 3,
	BC_OVERRUN_SHIFT = 8,
	DRAMC_CG_SHIFT = 9,
};

#define BM_REQ_OK				(0)
#define BM_ERR_WRONG_REQ			(-1)
#define BM_ERR_OVERRUN				(-2)

#define BM_WSCT_TSCT_IDSEL_ENABLE		(0)
#define BM_WSCT_TSCT_IDSEL_DISABLE		(-1)
#define BM_TTYPE1_16_ENABLE			(0)
#define BM_TTYPE1_16_DISABLE			(-1)
#define BM_TTYPE17_21_ENABLE			(0)
#define BM_TTYPE17_21_DISABLE			(-1)
#define BM_BW_LIMITER_ENABLE			(0)
#define BM_BW_LIMITER_DISABLE			(-1)

#define M0_DOUBLE_HALF_BW_1CH	(0x0)
#define M0_DOUBLE_HALF_BW_2CH	(0x1)
#define M0_DOUBLE_HALF_BW_4CH	(0x2)

/* EMI Rank configuration */
enum {
	DISABLE_DUAL_RANK_MODE = 0,
	ENABLE_DUAL_RANK_MODE,
};

#define RANK_MASK 0x1
#define ONE_RANK 1
#define DUAL_RANK 2


#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
enum BM_EMI_IPI_Type {
	SET_BASE_EMI = 0x0,
	SET_EBM_CONFIGS1 = 0x7,
	SET_EBM_CONFIGS2 = 0x8,
	SET_REGISTER_CB = 0x9,
};
#endif
#endif


#define	EMI_OFF			0x0000
#define EMI_CONA		(0x000-EMI_OFF)
#define EMI_CONH		(0x038-EMI_OFF)
#define EMI_CONH_2ND		(0x03C-EMI_OFF)
#define EMI_CONM		(0x060-EMI_OFF)
#define EMI_CONO		(0x070-EMI_OFF)

#define EMI_MDCT		(0x078 - EMI_OFF)
#define EMI_MDCT_2ND		(0x07C - EMI_OFF)

#define EMI_ARBA		(0x100 - EMI_OFF)
#define EMI_ARBB		(0x108 - EMI_OFF)
#define EMI_ARBC		(0x110 - EMI_OFF)
#define EMI_ARBD		(0x118 - EMI_OFF)
#define EMI_ARBE		(0x120 - EMI_OFF)
#define EMI_ARBF		(0x128 - EMI_OFF)
#define EMI_ARBG		(0x130 - EMI_OFF)
#define EMI_ARBG_2ND		(0x134 - EMI_OFF)
#define EMI_ARBH		(0x138 - EMI_OFF)


#define EMI_BMEN		(0x400-EMI_OFF)
#define EMI_MSEL		(0x440 - EMI_OFF)
#define EMI_MSEL2		(0x468 - EMI_OFF)
#define EMI_MSEL3		(0x470 - EMI_OFF)
#define EMI_MSEL4		(0x478 - EMI_OFF)
#define EMI_MSEL5		(0x480 - EMI_OFF)
#define EMI_MSEL6		(0x488 - EMI_OFF)
#define EMI_MSEL7		(0x490 - EMI_OFF)
#define EMI_MSEL8		(0x498 - EMI_OFF)
#define EMI_MSEL9		(0x4A0 - EMI_OFF)
#define EMI_MSEL10		(0x4A8 - EMI_OFF)

#define EMI_BMID0		(0x4B0 - EMI_OFF)
#define EMI_BMID1		(0x4B4 - EMI_OFF)
#define EMI_BMID2		(0x4B8 - EMI_OFF)
#define EMI_BMID3		(0x4BC - EMI_OFF)
#define EMI_BMID4		(0x4C0 - EMI_OFF)
#define EMI_BMID5		(0x4C4 - EMI_OFF)
#define EMI_BMID6		(0x4C8 - EMI_OFF)
#define EMI_BMID7		(0x4CC - EMI_OFF)
#define EMI_BMID8		(0x4D0 - EMI_OFF)
#define EMI_BMID9		(0x4D4 - EMI_OFF)
#define EMI_BMID10		(0x4D8 - EMI_OFF)

#define EMI_BMEN1		(0x4E0 - EMI_OFF)
#define EMI_BMEN2		(0x4E8 - EMI_OFF)
#define EMI_BMRW0		(0x4F8 - EMI_OFF)
#define EMI_BMRW1		(0x4FC - EMI_OFF)


/* SEDA 3.5 New! reg*/
/* For WSCT setting*/
#define EMI_DBWA (0xF00 - EMI_OFF)
#define EMI_DBWB (0xF04 - EMI_OFF)
#define EMI_DBWC (0xF08 - EMI_OFF)
#define EMI_DBWD (0xF0C - EMI_OFF)
#define EMI_DBWE (0xF10 - EMI_OFF)
#define EMI_DBWF (0xF14 - EMI_OFF)


#define EMI_DBWA_2ND (0xF2C - EMI_OFF)
#define EMI_DBWB_2ND (0xF30 - EMI_OFF)
#define EMI_DBWC_2ND (0xF34 - EMI_OFF)
#define EMI_DBWD_2ND (0xF38 - EMI_OFF)
#define EMI_DBWE_2ND (0xF3C - EMI_OFF)
#define EMI_DBWF_2ND (0xF40 - EMI_OFF)

#define EMI_DBWI (0xF20 - EMI_OFF) /* SEL_ID_MSK*/
#define EMI_DBWJ (0xF24 - EMI_OFF)
#define EMI_DBWK (0xF28 - EMI_OFF)

/* For Ttype setting */
#define EMI_TTYPE1_CONA (0xF50 - EMI_OFF)
#define EMI_TTYPE1_CONB (0xF54 - EMI_OFF)
#define EMI_TTYPE2_CONA (0xF58 - EMI_OFF)
#define EMI_TTYPE2_CONB (0xF5C - EMI_OFF)
#define EMI_TTYPE3_CONA (0xF60 - EMI_OFF)
#define EMI_TTYPE3_CONB (0xF64 - EMI_OFF)
#define EMI_TTYPE4_CONA (0xF68 - EMI_OFF)
#define EMI_TTYPE4_CONB (0xF6C - EMI_OFF)
#define EMI_TTYPE5_CONA (0xF70 - EMI_OFF)
#define EMI_TTYPE5_CONB (0xF74 - EMI_OFF)
#define EMI_TTYPE6_CONA (0xF78 - EMI_OFF)
#define EMI_TTYPE6_CONB (0xF7C - EMI_OFF)
#define EMI_TTYPE7_CONA (0xF80 - EMI_OFF)
#define EMI_TTYPE7_CONB (0xF84 - EMI_OFF)
#define EMI_TTYPE8_CONA (0xF88 - EMI_OFF)
#define EMI_TTYPE8_CONB (0xF8C - EMI_OFF)
#define EMI_TTYPE9_CONA (0xF90 - EMI_OFF)
#define EMI_TTYPE9_CONB (0xF94 - EMI_OFF)
#define EMI_TTYPE10_CONA (0xF98 - EMI_OFF)
#define EMI_TTYPE10_CONB (0xF9C - EMI_OFF)
#define EMI_TTYPE11_CONA (0xFA0 - EMI_OFF)
#define EMI_TTYPE11_CONB (0xFA4 - EMI_OFF)
#define EMI_TTYPE12_CONA (0xFA8 - EMI_OFF)
#define EMI_TTYPE12_CONB (0xFAC - EMI_OFF)
#define EMI_TTYPE13_CONA (0xFB0 - EMI_OFF)
#define EMI_TTYPE13_CONB (0xFB4 - EMI_OFF)
#define EMI_TTYPE14_CONA (0xFB8 - EMI_OFF)
#define EMI_TTYPE14_CONB (0xFBC - EMI_OFF)
#define EMI_TTYPE15_CONA (0xFC0 - EMI_OFF)
#define EMI_TTYPE15_CONB (0xFC4 - EMI_OFF)
#define EMI_TTYPE16_CONA (0xFC8 - EMI_OFF)
#define EMI_TTYPE16_CONB (0xFCC - EMI_OFF)
#define EMI_TTYPE17_CONA (0xFD0 - EMI_OFF)
#define EMI_TTYPE17_CONB (0xFD4 - EMI_OFF)
#define EMI_TTYPE18_CONA (0xFD8 - EMI_OFF)
#define EMI_TTYPE18_CONB (0xFDC - EMI_OFF)
#define EMI_TTYPE19_CONA (0xFE0 - EMI_OFF)
#define EMI_TTYPE19_CONB (0xFE4 - EMI_OFF)
#define EMI_TTYPE20_CONA (0xFE8 - EMI_OFF)
#define EMI_TTYPE20_CONB (0xFEC - EMI_OFF)
#define EMI_TTYPE21_CONA (0xFF0 - EMI_OFF)
#define EMI_TTYPE21_CONB (0xFF4 - EMI_OFF)

/* low effeciency */
#define CHN_EMI_LOWEFF_CTL0 (0x500)
// #define WMASK_PID_OFFSET
// #define AGEXP_PID_OFFSET


/* met_drv define  & global var */
#define CNT_COUNTDOWN   (0)

/* extern struct metdevice met_sspm_emi; */

extern int emi_tsct_enable;
extern int emi_mdct_enable;
extern int emi_TP_busfiltr_enable;
extern int mdmcu_sel_enable;
extern unsigned int rd_mdmcu_rsv_num;
extern int metemi_func_opt;

extern int met_emi_regdump;

extern int msel_enable;
extern unsigned int msel_group1;
extern unsigned int msel_group2;
extern unsigned int msel_group3;

extern struct kobject *kobj_emi;
extern int rwtype;

                 /* 1ms */
extern int bw_limiter_enable;

extern int ttype1_16_en;
extern int ttype17_21_en;

extern int dramc_pdir_enable;
extern int dram_chann_num;

extern int high_priority_filter;

extern int ttype_master_val[21];
extern int ttype_busid_val[21];
extern int ttype_nbeat_val[21];
extern int ttype_nbyte_val[21];
extern int ttype_burst_val[21];
extern int ttype_rw_val[21];

extern unsigned int msel_group_ext_val[WSCT_AMOUNT];
extern unsigned int wsct_rw_val[WSCT_AMOUNT];
extern char* const delim_comma;
extern char* const delim_coclon;

extern char msel_group_ext[FILE_NODE_DATA_LEN];

extern unsigned int WSCT_HPRI_DIS[WSCT_AMOUNT];
extern unsigned int WSCT_HPRI_SEL[WSCT_AMOUNT];
extern char wsct_high_priority_enable[FILE_NODE_DATA_LEN];

/*wsct_busid*/
extern unsigned int wsct_busid_val[WSCT_AMOUNT];
extern unsigned int wsct_idMask_val[WSCT_AMOUNT];
extern char wsct_busid[FILE_NODE_DATA_LEN];

/* wsct_chn_rank_sel */
extern unsigned int  wsct_chn_rank_sel_val[WSCT_AMOUNT];
extern char wsct_chn_rank_sel[FILE_NODE_DATA_LEN];

/* wsct_burst_range */
extern unsigned int  wsct_byte_low_bnd_val[WSCT_AMOUNT];
extern unsigned int  wsct_byte_up_bnd_val[WSCT_AMOUNT];
extern unsigned int  wsct_byte_bnd_dis[WSCT_AMOUNT];
extern char wsct_burst_range[FILE_NODE_DATA_LEN];

/* tsct_busid_enable */
extern unsigned int tsct_busid_enable_val[TSCT_AMOUNT];
extern char tsct_busid_enable[FILE_NODE_DATA_LEN];
/* ttype_high_priority_ext */
extern unsigned int TTYPE_HPRI_SEL[BM_COUNTER_MAX];
extern char ttype_high_priority_ext[FILE_NODE_DATA_LEN];
/* ttype_busid_ext */
extern unsigned int ttype_idMask_val[BM_COUNTER_MAX];
extern char ttype_busid_ext[FILE_NODE_DATA_LEN];
/* ttype_chn_rank_sel */
extern unsigned int  ttype_chn_rank_sel_val[BM_COUNTER_MAX];
extern char ttype_chn_rank_sel[FILE_NODE_DATA_LEN];

/* ttype_burst_range */
extern unsigned int  ttype_byte_low_bnd_val[BM_COUNTER_MAX];
extern unsigned int  ttype_byte_up_bnd_val[BM_COUNTER_MAX];
extern unsigned int  ttype_byte_bnd_dis[BM_COUNTER_MAX];
extern char ttype_burst_range[FILE_NODE_DATA_LEN];


extern unsigned int wmask_msel_val[MET_MAX_DRAM_CH_NUM];
extern char wmask_msel[FILE_NODE_DATA_LEN];

extern unsigned int ageexp_msel_val[MET_MAX_DRAM_CH_NUM];
extern unsigned int ageexp_rw_val[MET_MAX_DRAM_CH_NUM];
extern char ageexp_msel_rw[FILE_NODE_DATA_LEN];

extern char default_val[FILE_NODE_DATA_LEN];

extern int reserve_wsct_setting;

extern struct kobj_attribute clear_setting_attr;
extern struct kobj_attribute msel_group_ext_attr;
extern struct kobj_attribute wsct_rw_attr;
extern struct kobj_attribute wsct_high_priority_enable_attr;
extern struct kobj_attribute wsct_busid_attr;
extern struct kobj_attribute wsct_chn_rank_sel_attr;
extern struct kobj_attribute wsct_burst_range_attr;
extern struct kobj_attribute tsct_busid_enable_attr;
extern struct kobj_attribute ttype_high_priority_ext_attr;
extern struct kobj_attribute ttype_busid_ext_attr;
extern struct kobj_attribute ttype_chn_rank_sel_attr;
extern struct kobj_attribute ttype_burst_range_attr;

/* for header print*/
#define MAX_HEADER_LEN (1024 * 6)
extern char header_str[MAX_HEADER_LEN];
extern unsigned int output_header_len;
extern unsigned int output_str_len;

#define emi_help_msg "  --emi                                 monitor EMI banwidth\n"

extern int emi_use_ondiemet;
extern int emi_inited;

enum SSPM_Mode {
	CUSTOMER_MODE = 0x0,
	UNDEFINE_MODE = 0x1,
	INTERNAL_MODE = 0X2780
};


/*======================================================================*/
/*	KOBJ Declarations						*/
/*======================================================================*/
DECLARE_KOBJ_ATTR_INT(emi_TP_busfiltr_enable, emi_TP_busfiltr_enable);
DECLARE_KOBJ_ATTR_INT(emi_regdump, met_emi_regdump);
DECLARE_KOBJ_ATTR_INT(msel_enable, msel_enable);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group1, msel_group1, msel_group1 > 0 && msel_group1 <= BM_MASTER_ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group2, msel_group2, msel_group2 > 0 && msel_group2 <= BM_MASTER_ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group3, msel_group3, msel_group3 > 0 && msel_group3 <= BM_MASTER_ALL);


/* KOBJ: rwtype */
DECLARE_KOBJ_ATTR_INT_CHECK(rwtype, rwtype, rwtype >= 0 && rwtype <= BM_WRITE_ONLY);


/* KOBJ: ttype1_16_en */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype1_16_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE1_16_ENABLE,   "ENABLE" },
		{ BM_TTYPE1_16_DISABLE,  "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype1_16_en, ttype1_16_en, ttype1_16_en);

/* KOBJ: ttype17_21_en */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype17_21_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE17_21_ENABLE,  "ENABLE" },
		{ BM_TTYPE17_21_DISABLE, "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype17_21_en, ttype17_21_en, ttype17_21_en);

/* KOBJ: bw_limiter_enable */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	bw_limiter_enable,
	KOBJ_ITEM_LIST(
		{ BM_BW_LIMITER_ENABLE,  "ENABLE" },
		{ BM_BW_LIMITER_DISABLE, "DISABLE" }
		)
	);

DECLARE_KOBJ_ATTR_STR_LIST(bw_limiter_enable, bw_limiter_enable, bw_limiter_enable);

/* KOBJ: ttype_master */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_master,
	KOBJ_ITEM_LIST(
		{ BM_MASTER_M0,  "M0" },
		{ BM_MASTER_M1,  "M1" },
		{ BM_MASTER_M2,  "M2" },
		{ BM_MASTER_M3,  "M3" },
		{ BM_MASTER_M4,  "M4" },
		{ BM_MASTER_M5,  "M5" },
		{ BM_MASTER_M6,  "M6" },
		{ BM_MASTER_M7,  "M7" }
		)
	);


/* KOBJ: ttypeX_nbeat, ttypeX_nbyte, ttypeX_burst */
DECLARE_KOBJ_ATTR_INT_LIST_ITEM(
	ttype_nbeat,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_1BEAT,   1 },
		{ BM_TRANS_TYPE_2BEAT,   2 },
		{ BM_TRANS_TYPE_3BEAT,   3 },
		{ BM_TRANS_TYPE_4BEAT,   4 },
		{ BM_TRANS_TYPE_5BEAT,   5 },
		{ BM_TRANS_TYPE_6BEAT,   6 },
		{ BM_TRANS_TYPE_7BEAT,   7 },
		{ BM_TRANS_TYPE_8BEAT,   8 },
		{ BM_TRANS_TYPE_9BEAT,   9 },
		{ BM_TRANS_TYPE_10BEAT,  10 },
		{ BM_TRANS_TYPE_11BEAT,  11 },
		{ BM_TRANS_TYPE_12BEAT,  12 },
		{ BM_TRANS_TYPE_13BEAT,  13 },
		{ BM_TRANS_TYPE_14BEAT,  14 },
		{ BM_TRANS_TYPE_15BEAT,  15 },
		{ BM_TRANS_TYPE_16BEAT,  16 }
		)
	);
DECLARE_KOBJ_ATTR_INT_LIST_ITEM(
	ttype_nbyte,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_1Byte,   1 },
		{ BM_TRANS_TYPE_2Byte,   2 },
		{ BM_TRANS_TYPE_4Byte,   4 },
		{ BM_TRANS_TYPE_8Byte,   8 },
		{ BM_TRANS_TYPE_16Byte,  16 },
		{ BM_TRANS_TYPE_32Byte,  32 }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_burst,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_BURST_INCR,      "INCR" },
		{ BM_TRANS_TYPE_BURST_WRAP,      "WRAP" }
		)
	);

DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_rw,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_RW_DEFAULT,   "DEFAULT" },
		{ BM_TRANS_RW_READONLY,  "R" },
		{ BM_TRANS_RW_WRITEONLY, "W" },
		{ BM_TRANS_RW_RWBOTH,    "BOTH" }
		)
	);


DECLARE_KOBJ_ATTR_INT(dramc_pdir_enable, dramc_pdir_enable);

DECLARE_KOBJ_ATTR_HEX(high_priority_filter, high_priority_filter);

#define DECLARE_KOBJ_TTYPE_MASTER(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _master, ttype_master_val[nr - 1], ttype_master)

#define DECLARE_KOBJ_TTYPE_NBEAT(nr) \
	DECLARE_KOBJ_ATTR_INT_LIST(ttype ## nr ## _nbeat, ttype_nbeat_val[nr - 1], ttype_nbeat)

#define DECLARE_KOBJ_TTYPE_NBYTE(nr) \
	DECLARE_KOBJ_ATTR_INT_LIST(ttype ## nr ## _nbyte, ttype_nbyte_val[nr - 1], ttype_nbyte)

#define DECLARE_KOBJ_TTYPE_BURST(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _burst, ttype_burst_val[nr - 1], ttype_burst)

#define DECLARE_KOBJ_TTYPE_RW(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _rw, ttype_rw_val[nr - 1], ttype_rw)

#define DECLARE_KOBJ_TTYPE_BUSID_VAL(nr) \
	DECLARE_KOBJ_ATTR_HEX(ttype ## nr ## _busid, ttype_busid_val[nr - 1])

DECLARE_KOBJ_TTYPE_MASTER(1);
DECLARE_KOBJ_TTYPE_NBEAT(1);
DECLARE_KOBJ_TTYPE_NBYTE(1);
DECLARE_KOBJ_TTYPE_BURST(1);
DECLARE_KOBJ_TTYPE_RW(1);
DECLARE_KOBJ_TTYPE_BUSID_VAL(1);

DECLARE_KOBJ_TTYPE_MASTER(2);
DECLARE_KOBJ_TTYPE_NBEAT(2);
DECLARE_KOBJ_TTYPE_NBYTE(2);
DECLARE_KOBJ_TTYPE_BURST(2);
DECLARE_KOBJ_TTYPE_RW(2);
DECLARE_KOBJ_TTYPE_BUSID_VAL(2);

DECLARE_KOBJ_TTYPE_MASTER(3);
DECLARE_KOBJ_TTYPE_NBEAT(3);
DECLARE_KOBJ_TTYPE_NBYTE(3);
DECLARE_KOBJ_TTYPE_BURST(3);
DECLARE_KOBJ_TTYPE_RW(3);
DECLARE_KOBJ_TTYPE_BUSID_VAL(3);

DECLARE_KOBJ_TTYPE_MASTER(4);
DECLARE_KOBJ_TTYPE_NBEAT(4);
DECLARE_KOBJ_TTYPE_NBYTE(4);
DECLARE_KOBJ_TTYPE_BURST(4);
DECLARE_KOBJ_TTYPE_RW(4);
DECLARE_KOBJ_TTYPE_BUSID_VAL(4);

DECLARE_KOBJ_TTYPE_MASTER(5);
DECLARE_KOBJ_TTYPE_NBEAT(5);
DECLARE_KOBJ_TTYPE_NBYTE(5);
DECLARE_KOBJ_TTYPE_BURST(5);
DECLARE_KOBJ_TTYPE_RW(5);
DECLARE_KOBJ_TTYPE_BUSID_VAL(5);

DECLARE_KOBJ_TTYPE_MASTER(6);
DECLARE_KOBJ_TTYPE_NBEAT(6);
DECLARE_KOBJ_TTYPE_NBYTE(6);
DECLARE_KOBJ_TTYPE_BURST(6);
DECLARE_KOBJ_TTYPE_RW(6);
DECLARE_KOBJ_TTYPE_BUSID_VAL(6);

DECLARE_KOBJ_TTYPE_MASTER(7);
DECLARE_KOBJ_TTYPE_NBEAT(7);
DECLARE_KOBJ_TTYPE_NBYTE(7);
DECLARE_KOBJ_TTYPE_BURST(7);
DECLARE_KOBJ_TTYPE_RW(7);
DECLARE_KOBJ_TTYPE_BUSID_VAL(7);

DECLARE_KOBJ_TTYPE_MASTER(8);
DECLARE_KOBJ_TTYPE_NBEAT(8);
DECLARE_KOBJ_TTYPE_NBYTE(8);
DECLARE_KOBJ_TTYPE_BURST(8);
DECLARE_KOBJ_TTYPE_RW(8);
DECLARE_KOBJ_TTYPE_BUSID_VAL(8);

DECLARE_KOBJ_TTYPE_MASTER(9);
DECLARE_KOBJ_TTYPE_NBEAT(9);
DECLARE_KOBJ_TTYPE_NBYTE(9);
DECLARE_KOBJ_TTYPE_BURST(9);
DECLARE_KOBJ_TTYPE_RW(9);
DECLARE_KOBJ_TTYPE_BUSID_VAL(9);

DECLARE_KOBJ_TTYPE_MASTER(10);
DECLARE_KOBJ_TTYPE_NBEAT(10);
DECLARE_KOBJ_TTYPE_NBYTE(10);
DECLARE_KOBJ_TTYPE_BURST(10);
DECLARE_KOBJ_TTYPE_RW(10);
DECLARE_KOBJ_TTYPE_BUSID_VAL(10);

DECLARE_KOBJ_TTYPE_MASTER(11);
DECLARE_KOBJ_TTYPE_NBEAT(11);
DECLARE_KOBJ_TTYPE_NBYTE(11);
DECLARE_KOBJ_TTYPE_BURST(11);
DECLARE_KOBJ_TTYPE_RW(11);
DECLARE_KOBJ_TTYPE_BUSID_VAL(11);

DECLARE_KOBJ_TTYPE_MASTER(12);
DECLARE_KOBJ_TTYPE_NBEAT(12);
DECLARE_KOBJ_TTYPE_NBYTE(12);
DECLARE_KOBJ_TTYPE_BURST(12);
DECLARE_KOBJ_TTYPE_RW(12);
DECLARE_KOBJ_TTYPE_BUSID_VAL(12);

DECLARE_KOBJ_TTYPE_MASTER(13);
DECLARE_KOBJ_TTYPE_NBEAT(13);
DECLARE_KOBJ_TTYPE_NBYTE(13);
DECLARE_KOBJ_TTYPE_BURST(13);
DECLARE_KOBJ_TTYPE_RW(13);
DECLARE_KOBJ_TTYPE_BUSID_VAL(13);

DECLARE_KOBJ_TTYPE_MASTER(14);
DECLARE_KOBJ_TTYPE_NBEAT(14);
DECLARE_KOBJ_TTYPE_NBYTE(14);
DECLARE_KOBJ_TTYPE_BURST(14);
DECLARE_KOBJ_TTYPE_RW(14);
DECLARE_KOBJ_TTYPE_BUSID_VAL(14);

DECLARE_KOBJ_TTYPE_MASTER(15);
DECLARE_KOBJ_TTYPE_NBEAT(15);
DECLARE_KOBJ_TTYPE_NBYTE(15);
DECLARE_KOBJ_TTYPE_BURST(15);
DECLARE_KOBJ_TTYPE_RW(15);
DECLARE_KOBJ_TTYPE_BUSID_VAL(15);

DECLARE_KOBJ_TTYPE_MASTER(16);
DECLARE_KOBJ_TTYPE_NBEAT(16);
DECLARE_KOBJ_TTYPE_NBYTE(16);
DECLARE_KOBJ_TTYPE_BURST(16);
DECLARE_KOBJ_TTYPE_RW(16);
DECLARE_KOBJ_TTYPE_BUSID_VAL(16);

DECLARE_KOBJ_TTYPE_MASTER(17);
DECLARE_KOBJ_TTYPE_NBEAT(17);
DECLARE_KOBJ_TTYPE_NBYTE(17);
DECLARE_KOBJ_TTYPE_BURST(17);
DECLARE_KOBJ_TTYPE_RW(17);
DECLARE_KOBJ_TTYPE_BUSID_VAL(17);

DECLARE_KOBJ_TTYPE_MASTER(18);
DECLARE_KOBJ_TTYPE_NBEAT(18);
DECLARE_KOBJ_TTYPE_NBYTE(18);
DECLARE_KOBJ_TTYPE_BURST(18);
DECLARE_KOBJ_TTYPE_RW(18);
DECLARE_KOBJ_TTYPE_BUSID_VAL(18);

DECLARE_KOBJ_TTYPE_MASTER(19);
DECLARE_KOBJ_TTYPE_NBEAT(19);
DECLARE_KOBJ_TTYPE_NBYTE(19);
DECLARE_KOBJ_TTYPE_BURST(19);
DECLARE_KOBJ_TTYPE_RW(19);
DECLARE_KOBJ_TTYPE_BUSID_VAL(19);

DECLARE_KOBJ_TTYPE_MASTER(20);
DECLARE_KOBJ_TTYPE_NBEAT(20);
DECLARE_KOBJ_TTYPE_NBYTE(20);
DECLARE_KOBJ_TTYPE_BURST(20);
DECLARE_KOBJ_TTYPE_RW(20);
DECLARE_KOBJ_TTYPE_BUSID_VAL(20);

DECLARE_KOBJ_TTYPE_MASTER(21);
DECLARE_KOBJ_TTYPE_NBEAT(21);
DECLARE_KOBJ_TTYPE_NBYTE(21);
DECLARE_KOBJ_TTYPE_BURST(21);
DECLARE_KOBJ_TTYPE_RW(21);
DECLARE_KOBJ_TTYPE_BUSID_VAL(21);


#define KOBJ_ATTR_ITEM_SERIAL_FNODE(nr) \
	do { \
		KOBJ_ATTR_ITEM(ttype ## nr ## _master); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _nbeat); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _nbyte); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _burst); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _busid); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _rw); \
	} while (0)

#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(high_priority_filter); \
		KOBJ_ATTR_ITEM(emi_TP_busfiltr_enable); \
		KOBJ_ATTR_ITEM(msel_enable); \
		KOBJ_ATTR_ITEM(msel_group1); \
		KOBJ_ATTR_ITEM(msel_group2); \
		KOBJ_ATTR_ITEM(msel_group3); \
		KOBJ_ATTR_ITEM(rwtype); \
		KOBJ_ATTR_ITEM(ttype17_21_en); \
		KOBJ_ATTR_ITEM(ttype1_16_en); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(1); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(2); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(3); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(4); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(5); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(6); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(7); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(8); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(9); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(10); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(11); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(12); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(13); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(14); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(15); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(16); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(17); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(18); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(19); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(20); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(21); \
		KOBJ_ATTR_ITEM(bw_limiter_enable); \
		KOBJ_ATTR_ITEM(dramc_pdir_enable); \
		KOBJ_ATTR_ITEM(clear_setting);\
		KOBJ_ATTR_ITEM(msel_group_ext);\
		KOBJ_ATTR_ITEM(wsct_rw);\
		KOBJ_ATTR_ITEM(wsct_high_priority_enable);\
		KOBJ_ATTR_ITEM(wsct_busid);\
		KOBJ_ATTR_ITEM(wsct_chn_rank_sel);\
		KOBJ_ATTR_ITEM(wsct_burst_range);\
		KOBJ_ATTR_ITEM(tsct_busid_enable);\
		KOBJ_ATTR_ITEM(ttype_high_priority_ext);\
		KOBJ_ATTR_ITEM(ttype_busid_ext);\
		KOBJ_ATTR_ITEM(ttype_chn_rank_sel);\
		KOBJ_ATTR_ITEM(ttype_burst_range);\
		KOBJ_ATTR_ITEM(reserve_wsct_setting);\
		KOBJ_ATTR_ITEM(emi_regdump); \
		KOBJ_ATTR_ITEM(wmask_msel); \
		KOBJ_ATTR_ITEM(ageexp_msel_rw); \
		KOBJ_ATTR_ITEM(default_val); \
		KOBJ_ATTR_ITEM(sspm_support_feature); \
	} while (0)


DECLARE_KOBJ_ATTR_INT(reserve_wsct_setting, reserve_wsct_setting);

extern int MET_BM_Init(void);
extern void MET_BM_DeInit(void);
extern void MET_BM_SaveCfg(void);
extern void MET_BM_RestoreCfg(void);



extern int MET_BM_SetMonitorCounter(const unsigned int counter_num,
				    const unsigned int master, const unsigned int trans_type);
extern int MET_BM_SetTtypeCounterRW(unsigned int bmrw0_val, unsigned int bmrw1_val);
extern int MET_BM_Set_WsctTsct_id_sel(unsigned int counter_num, unsigned int enable);
extern int MET_BM_SetMaster(const unsigned int counter_num, const unsigned int master);
extern int MET_BM_SetbusID_En(const unsigned int counter_num,
			      const unsigned int enable);
extern int MET_BM_SetbusID(const unsigned int counter_num,
			   const unsigned int id);
extern int MET_BM_SetUltraHighFilter(const unsigned int counter_num, const unsigned int enable);
extern int MET_BM_SetLatencyCounter(unsigned int enable);
extern void MET_BM_SetReadWriteType(const unsigned int ReadWriteType);

extern unsigned int MET_EMI_GetDramRankNum(void);
extern unsigned int MET_EMI_GetDramRankNum_CHN1(void);


extern unsigned int MET_EMI_GetDramChannNum(void);
extern unsigned int MET_EMI_Get_CONH_2ND(void);

/* SEDA 3.5 NEW */
extern int MET_BM_SetWSCT_master_rw(unsigned int *master , unsigned int *rw);
extern int MET_BM_SetWSCT_high_priority(unsigned int *disable, unsigned int *select);
extern int MET_BM_SetWSCT_busid_idmask(unsigned int *busid, unsigned int *idMask);
extern int MET_BM_SetWSCT_chn_rank_sel(unsigned int *chn_rank_sel);
extern int MET_BM_SetWSCT_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd);
extern int MET_BM_SetTSCT_busid_enable(unsigned int *enable);
extern int MET_BM_SetTtype_high_priority_sel(unsigned int _high_priority_filter, unsigned int *select);
extern int MET_BM_SetTtype_busid_idmask(unsigned int *busid, unsigned int *idMask, int _ttype1_16_en, int _ttype17_21_en);
extern int MET_BM_SetTtype_chn_rank_sel(unsigned int *chn_rank_sel);
extern int MET_BM_SetTtype_burst_range(unsigned int *bnd_dis, unsigned int *low_bnd, unsigned int *up_bnd);
extern unsigned int MET_EMI_Get_BaseClock_Rate(void);


/* file node controll */
extern void _clear_msel_group_ext(void);
extern ssize_t msel_group_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t msel_group_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_wsct_rw(void);
extern ssize_t wsct_rw_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wsct_rw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_wsct_high_priority_enable(void);
extern ssize_t wsct_high_priority_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wsct_high_priority_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_wsct_busid(void);
extern ssize_t wsct_busid_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wsct_busid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_wsct_chn_rank_sel(void);
extern ssize_t wsct_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wsct_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_wsct_burst_range(void);
extern ssize_t wsct_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wsct_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_tsct_busid_enable(void);
extern ssize_t tsct_busid_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t tsct_busid_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_ttype_high_priority_ext(void);
extern ssize_t ttype_high_priority_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t ttype_high_priority_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_ttype_busid_ext(void);
extern ssize_t ttype_busid_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t ttype_busid_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_ttype_chn_rank_sel(void);
extern ssize_t ttype_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t ttype_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
extern void _clear_ttype_burst_range(void);
extern ssize_t ttype_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t ttype_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

extern ssize_t wmask_msel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t wmask_msel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

extern ssize_t ageexp_msel_rw_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n);
extern ssize_t ageexp_msel_rw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

extern void _clear_setting(void);
extern ssize_t clear_setting_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n);

extern void emi_init(void);
extern void emi_uninit(void);

extern void MET_BM_IPI_configs(void);
extern void MET_BM_IPI_REGISTER_CB(void);


extern unsigned int get_sspm_support_feature(void);
extern unsigned check_sspm_support(unsigned int module_id);
extern int emi_create_header(char *buf, int buf_len);

extern int met_emi_create_basic(struct kobject *parent, struct metdevice *emi_dev);
extern void met_emi_delete_basic(void);
extern void met_emi_resume_basic(void);

extern int do_emi(void);
extern int emi_print_header_basic(char *buf, int len);
extern void ondiemet_emi_start_basic(void);
extern void ondiemet_emi_stop_basic(void);

extern unsigned met_get_dram_data_rate(void);
extern unsigned int MET_GET_DRAM_TYPE(void);

#endif                          /* !__MT_MET_EMI_BM_BASIC_H__ */
