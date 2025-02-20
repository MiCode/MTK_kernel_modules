/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_REGS_DEF_H__
#define __MTK_CAM_SENINF_TSREC_REGS_DEF_H__

#include "mtk_cam-seninf-tsrec-def.h"


/******************************************************************************
 * TSREC registers address & offset
 *****************************************************************************/
/* for iomem operation type/method */
#define SENINF_UNIFY_IOMEM_MAPPING        (0)	// for isp8 and after

#if (SENINF_UNIFY_IOMEM_MAPPING)
#define ADDR_SHIFT_FROM_TSREC_TOP         (0)	// (1)
#else
#define ADDR_SHIFT_FROM_TSREC_TOP         (0)	// this case must set to false
#endif

// #define TSREC_DBG_PRINT_IOMEM_ADDR


/*--------------------------------------------------------------------------*/
// tsrec_top related address shift define
/*--------------------------------------------------------------------------*/
#define TSREC_BASE                        0x3A310000

#define TSREC_TIMER_CFG_OFFSET            0x0
#define TSREC_TOP_CFG_OFFSET              0x4
#define TSREC_TIMER_LAT_OFFSET            0x10

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_TIMER_LAT_M_OFFSET          0x14
#endif

#define TSREC_DEVICE_IRQ_SEL_0_OFFSET     0x20


/*--------------------------------------------------------------------------*/
// per tsrec related address shift define
/*--------------------------------------------------------------------------*/
#define TSREC_N_1ST_BASE_OFFSET           0x10000
#define TSREC_N_OFFSET                    0x10000

#define TSREC_N_CFG_OFFSET                0x0
#define TSREC_N_INT_EN_OFFSET             0x4
#define TSREC_N_INT_STATUS_OFFSET         0x8
#define TSREC_N_SW_RST_OFFSET             0x10
#define TSREC_N_TS_CNT_OFFSET             0x14
#define TSREC_N_TRIG_SRC_OFFSET           0x18

#define TSREC_N_EXP_VC_DT_BASE_OFFSET     0x1C
#define TSREC_N_EXP_CNT_BASE_OFFSET       0x28

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_N_EXP_CNT_M_BASE_OFFSET     0x58
#endif


/*--------------------------------------------------------------------------*/
// register address shift utilities define
/*--------------------------------------------------------------------------*/
#define TSREC_N_BASE_SHIFT(n) \
	((n >= 0) ? (TSREC_N_1ST_BASE_OFFSET+(n)*TSREC_N_OFFSET) : 0)


#if (ADDR_SHIFT_FROM_TSREC_TOP)

#define TSREC_ADDR(n, shift)       (TSREC_BASE+(shift))
#define TSREC_CFG_OFFSET(n)        (TSREC_N_BASE_SHIFT((n))+TSREC_N_CFG_OFFSET)

#define TSREC_INT_EN_OFFSET(n)     (TSREC_N_BASE_SHIFT((n))+TSREC_N_INT_EN_OFFSET)
#define TSREC_INT_STATUS_OFFSET(n) (TSREC_N_BASE_SHIFT((n))+TSREC_N_INT_STATUS_OFFSET)

#define TSREC_SW_RST_OFFSET(n)     (TSREC_N_BASE_SHIFT((n))+TSREC_N_SW_RST_OFFSET)
#define TSREC_TS_CNT_OFFSET(n)     (TSREC_N_BASE_SHIFT((n))+TSREC_N_TS_CNT_OFFSET)
#define TSREC_TRIG_SRC_OFFSET(n)   (TSREC_N_BASE_SHIFT((n))+TSREC_N_TRIG_SRC_OFFSET)

#define TSREC_EXP_VC_DT_OFFSET(n, exp_n) \
	(TSREC_N_BASE_SHIFT((n))+TSREC_N_EXP_VC_DT_BASE_OFFSET \
	+(exp_n)*(32/8))

#define TSREC_EXP_CNT_OFFSET(n, exp_n, rec_n) \
	(TSREC_N_BASE_SHIFT((n))+TSREC_N_EXP_CNT_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_EXP_CNT_M_OFFSET(n, exp_n, rec_n) \
	(TSREC_N_BASE_SHIFT((n))+TSREC_N_EXP_CNT_M_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))
#endif

#else // => !ADDR_SHIFT_FROM_TSREC_TOP

#define TSREC_ADDR(n, shift)       (TSREC_BASE+TSREC_N_BASE_SHIFT((n))+(shift))
#define TSREC_CFG_OFFSET(n)        (TSREC_N_CFG_OFFSET)

#define TSREC_INT_EN_OFFSET(n)     (TSREC_N_INT_EN_OFFSET)
#define TSREC_INT_STATUS_OFFSET(n) (TSREC_N_INT_STATUS_OFFSET)

#define TSREC_SW_RST_OFFSET(n)     (TSREC_N_SW_RST_OFFSET)
#define TSREC_TS_CNT_OFFSET(n)     (TSREC_N_TS_CNT_OFFSET)
#define TSREC_TRIG_SRC_OFFSET(n)   (TSREC_N_TRIG_SRC_OFFSET)

#define TSREC_EXP_VC_DT_OFFSET(n, exp_n) \
	(TSREC_N_EXP_VC_DT_BASE_OFFSET \
	+(exp_n)*(32/8))

#define TSREC_EXP_CNT_OFFSET(n, exp_n, rec_n) \
	(TSREC_N_EXP_CNT_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_EXP_CNT_M_OFFSET(n, exp_n, rec_n) \
	(TSREC_N_EXP_CNT_M_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))
#endif

#endif // SENINF_UNIFY_IOMEM_MAPPING


/******************************************************************************
 * TSREC registers union structure
 *****************************************************************************/
union REG_TSREC_TIMER_CFG { /* 0x1A024004 or 0x3A310000 */
	struct {
		unsigned int TSREC_TIMER_FIX_CLK_EN      :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_TIMER_CK_EN           :  1;  /*  1.. 1, 0x00000002 */
		unsigned int rsv_2                       :  2;  /*  2.. 3, 0x0000000C */
		unsigned int TSREC_TIMER_CLR             :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_TIMER_LAT             :  1;  /*  5.. 5, 0x00000020 */
#if !(TSREC_WITH_GLOBAL_TIMER)
		unsigned int rsv_6                       : 26;  /*  6..31, 0xFFFFFFC0 */
#else
		unsigned int TSREC_TIMER_SEL             :  1;  /*  6.. 6, 0x00000040 */
		unsigned int TSREC_TIMER_EN              :  1;  /*  7.. 7, 0x00000080 */
		unsigned int rsv_8                       : 24;  /*  8..31, 0xFFFFFF00 */
#endif // !TSREC_WITH_GLOBAL_TIMER
	} bits;
	unsigned int val;
};


/******************************************************************************
 * TSREC registers general union structure / define
 *****************************************************************************/
#define TSREC_INT_WCLR_EN_BIT             31


union REG_TSREC_N_TS_CNT {
	struct {
		unsigned int TSREC_N_EXP0_TS_CNT         :  2;  /*  0.. 1, 0x00000003 */
		unsigned int rsv_2                       :  2;  /*  2.. 3, 0x0000000C */
		unsigned int TSREC_N_EXP1_TS_CNT         :  2;  /*  4.. 5, 0x00000030 */
		unsigned int rsv_6                       :  2;  /*  6.. 7, 0x000000C0 */
		unsigned int TSREC_N_EXP2_TS_CNT         :  2;  /*  8.. 9, 0x00000300 */
		unsigned int rsv_10                      : 22;  /* 10..31, 0xFFFFFC00 */
	} bits;
	unsigned int val;
};


// for TSREC_N_TRIG_SRC register.
#define TSREC_TRIG_SRC_VSYNC              0
#define TSREC_TRIG_SRC_HSYNC              1
#define TSREC_EXP_TRIG_SRC_SHIFT          4


#define TSREC_N_EXP_VSYNC_VC_BIT_WIDTH    5
#define TSREC_N_EXP_HSYNC_VC_BIT_WIDTH    5
#define TSREC_N_EXP_HSYNC_DT_BIT_WIDTH    6
union REG_TSREC_N_EXP_VC_DT {
	struct {
		unsigned int TSREC_N_EXP_VSYNC_VC        :  5;  /*  0.. 4, 0x0000001F */
		unsigned int rsv_5                       :  3;  /*  5.. 7, 0x000000E0 */
		unsigned int TSREC_N_EXP_HSYNC_VC        :  5;  /*  8..12, 0x00001F00 */
		unsigned int rsv_13                      :  3;  /* 13..15, 0x0000E000 */
		unsigned int TSREC_N_EXP_HSYNC_DT        :  6;  /* 16..21, 0x003F0000 */
		unsigned int rsv_22                      : 10;  /* 22..31, 0xFFC00000 */
	} bits;
	unsigned int val;
};


#endif
