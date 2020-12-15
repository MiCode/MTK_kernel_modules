/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __MT_MET_EMI_BM_H__
#define __MT_MET_EMI_BM_H__

#define EMI_VER_MAJOR  3
#define EMI_VER_MINOR  5

#define FILE_NODE_DATA_LEN 512 
#define WSCT_AMOUNT 6
#define TSCT_AMOUNT 3


#define DRAM_EMI_BASECLOCK_RATE_LP4     4
#define DRAM_EMI_BASECLOCK_RATE_LP3     2

#define DRAM_IO_BUS_WIDTH_LP4           16
#define DRAM_IO_BUS_WIDTH_LP3           32

#define DRAM_DATARATE   2

#define ADDR_EMI        ((unsigned long)BaseAddrEMI)


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


#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(ONDIEMET_SUPPORT)
enum BM_EMI_IPI_Type {
	SET_BASE_EMI = 0x0,
	SET_EBM_CONFIGS1 = 0x7,
	SET_EBM_CONFIGS2 = 0x8,
	SET_REGISTER_CB = 0x9,
};
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


unsigned int MET_EMI_GetDramChannNum(void);


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

#endif                          /* !__MT_MET_EMI_BM_H__ */
