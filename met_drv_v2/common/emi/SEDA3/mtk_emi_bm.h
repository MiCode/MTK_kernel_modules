/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */

#ifndef __MT_MET_EMI_BM_H__
#define __MT_MET_EMI_BM_H__

#define EMI_VER_MAJOR  3
#define EMI_VER_MINOR  0


#define DRAM_EMI_BASECLOCK_RATE_LP4     4
#define DRAM_EMI_BASECLOCK_RATE_LP3     2

#define DRAM_IO_BUS_WIDTH_LP4           16
#define DRAM_IO_BUS_WIDTH_LP3           32

#define DRAM_DATARATE   2

#define	ADDR_EMI		((unsigned long) BaseAddrEMI)

/*========================================================*/
/*EMI configuration by project*/
/*Change config start*/
/*========================================================*/
#define _GP_1_Default	(_M0 | _M1)
#define _GP_2_Default	(_M2 | _M5)
#define _GP_3_Default	(_M6 | _M7)


/*========================================================*/
/*Change config end*/
/*========================================================*/


#define _M0		(0x01)
#define _M1		(0x02)
#define _M2		(0x04)
#define _M3		(0x08)
#define _M4		(0x10)
#define _M5		(0x20)
#define _M6		(0x40)
#define _M7		(0x80)
#define _ALL	(0xFF)

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


#define EMI_BMID_MASK				(0xFFFF)
#define BM_COUNTER_MAX				(21)

#define BM_REQ_OK						(0)
#define BM_ERR_WRONG_REQ				(-1)
#define BM_ERR_OVERRUN					(-2)

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

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
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
#define EMI_BMEN		(0x400-EMI_OFF)
#define EMI_MSEL		(0x440-EMI_OFF)
#define EMI_MSEL2		(0x468-EMI_OFF)
#define EMI_MSEL3		(0x470-EMI_OFF)
#define EMI_MSEL4		(0x478-EMI_OFF)
#define EMI_MSEL5		(0x480-EMI_OFF)
#define EMI_MSEL6		(0x488-EMI_OFF)
#define EMI_MSEL7		(0x490-EMI_OFF)
#define EMI_MSEL8		(0x498-EMI_OFF)
#define EMI_MSEL9		(0x4A0-EMI_OFF)
#define EMI_MSEL10		(0x4A8-EMI_OFF)

#define EMI_BMID0		(0x4B0-EMI_OFF)
#define EMI_BMID1		(0x4B4-EMI_OFF)
#define EMI_BMID2		(0x4B8-EMI_OFF)
#define EMI_BMID3		(0x4BC-EMI_OFF)
#define EMI_BMID4		(0x4C0-EMI_OFF)
#define EMI_BMID5		(0x4C4-EMI_OFF)
#define EMI_BMID6		(0x4C8-EMI_OFF)
#define EMI_BMID7		(0x4CC-EMI_OFF)
#define EMI_BMID8		(0x4D0-EMI_OFF)
#define EMI_BMID9		(0x4D4-EMI_OFF)
#define EMI_BMID10		(0x4D8-EMI_OFF)

#define EMI_BMEN1		(0x4E0-EMI_OFF)
#define EMI_BMEN2		(0x4E8-EMI_OFF)
#define EMI_BMRW0		(0x4F8-EMI_OFF)
#define EMI_BMRW1		(0x4FC-EMI_OFF)


extern unsigned int MET_EMI_GetDramRankNum(void);
extern unsigned int MET_EMI_GetDramRankNum_CHN1(void);


unsigned int MET_EMI_GetDramChannNum(void);

extern int MET_BM_Init(void);
extern void MET_BM_DeInit(void);
extern void MET_BM_SaveCfg(void);
extern void MET_BM_RestoreCfg(void);
extern int MET_BM_SetMonitorCounter(const unsigned int counter_num,
				    const unsigned int master, const unsigned int trans_type);
extern int MET_BM_SetTtypeCounterRW(unsigned int bmrw0_val, unsigned int bmrw1_val);
extern int MET_BM_Set_WsctTsct_id_sel(unsigned int counter_num, unsigned int enable);
extern int MET_BM_SetbusID_En(const unsigned int counter_num,
			      const unsigned int enable);
extern int MET_BM_SetbusID(const unsigned int counter_num,
			   const unsigned int id);
extern int MET_BM_SetUltraHighFilter(const unsigned int counter_num, const unsigned int enable);
extern int MET_BM_SetLatencyCounter(unsigned int enable);
extern unsigned int MET_EMI_Get_BaseClock_Rate(void);
#endif				/* !__MT_MET_EMI_BM_H__ */
