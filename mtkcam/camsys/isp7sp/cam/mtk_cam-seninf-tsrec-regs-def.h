/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2022 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_TSREC_REGS_DEF_H__
#define __MTK_CAM_SENINF_TSREC_REGS_DEF_H__

#include "mtk_cam-seninf-tsrec-def.h"


/******************************************************************************
 * TSREC registers address & offset
 *****************************************************************************/
#define SENINF_BASE                       0x1A00E000
#define TSREC_BASE                        0x1A024000

#define TSREC_TOP_CFG_OFFSET              0x0
#define TSREC_TIMER_CFG_OFFSET            0x4
#define TSREC_TIMER_LAT_OFFSET            0x8

#define TSREC_INT_EN_OFFSET               0x10
#define TSREC_INT_STATUS_OFFSET           0x14

#define TSREC_INT_EN_2_OFFSET             0x1C
#define TSREC_INT_STATUS_2_OFFSET         0x20

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_TIMER_LAT_M_OFFSET          0x28
#endif


/*--------------------------------------------------------------------------*/
#define TSREC_N_OFFSET                    0x180

#define TSREC_N_CFG_OFFSET                0x40
#define TSREC_N_SW_RST_OFFSET             0x44
#define TSREC_N_TS_CNT_OFFSET             0x48
#define TSREC_N_TRIG_SRC_OFFSET           0x4C

#define TSREC_N_EXP_VC_DT_BASE_OFFSET     0x50
#define TSREC_N_EXP_CNT_BASE_OFFSET       0x70

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_N_EXP_CNT_M_BASE_OFFSET     0xA0
#endif


#define TSREC_ADDR_BY_OFFSET(n)    (TSREC_BASE+(n))
#define TSREC_CFG_OFFSET(n)        ((n)*TSREC_N_OFFSET+TSREC_N_CFG_OFFSET)
#define TSREC_SW_RST_OFFSET(n)     ((n)*TSREC_N_OFFSET+TSREC_N_SW_RST_OFFSET)
#define TSREC_TS_CNT_OFFSET(n)     ((n)*TSREC_N_OFFSET+TSREC_N_TS_CNT_OFFSET)
#define TSREC_TRIG_SRC_OFFSET(n)   ((n)*TSREC_N_OFFSET+TSREC_N_TRIG_SRC_OFFSET)

#define TSREC_EXP_VC_DT_OFFSET(n, exp_n) \
	((n)*TSREC_N_OFFSET+TSREC_N_EXP_VC_DT_BASE_OFFSET \
	+(exp_n)*(32/8))

#define TSREC_EXP_CNT_OFFSET(n, exp_n, rec_n) \
	((n)*TSREC_N_OFFSET+TSREC_N_EXP_CNT_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))

#if (TSREC_WITH_64_BITS_TIMER_RG)
#define TSREC_EXP_CNT_M_OFFSET(n, exp_n, rec_n) \
	((n)*TSREC_N_OFFSET+TSREC_N_EXP_CNT_M_BASE_OFFSET \
	+((exp_n)*TSREC_TS_REC_MAX_CNT+(rec_n))*(32/8))
#endif


/******************************************************************************
 * TSREC registers union structure
 *****************************************************************************/
#define REG_TSREC_CFG_VALID_MASK          0x111111
union REG_TSREC_CFG { /* 0x1A02400 */
	struct {
		unsigned int TSREC_A_CK_EN               :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_B_CK_EN               :  1;  /*  1.. 1, 0x00000002 */
		unsigned int TSREC_C_CK_EN               :  1;  /*  2.. 2, 0x00000004 */
		unsigned int TSREC_D_CK_EN               :  1;  /*  3.. 3, 0x00000008 */
		unsigned int TSREC_E_CK_EN               :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_F_CK_EN               :  1;  /*  5.. 5, 0x00000020 */
		unsigned int rsv_6                       : 10;  /*  6..15, 0x0000FFC0 */
		unsigned int TSREC_A_SRC_SELECT          :  1;  /* 16..16, 0x00010000 */
		unsigned int TSREC_B_SRC_SELECT          :  1;  /* 17..17, 0x00020000 */
		unsigned int TSREC_C_SRC_SELECT          :  1;  /* 18..18, 0x00040000 */
		unsigned int TSREC_D_SRC_SELECT          :  1;  /* 19..19, 0x00080000 */
		unsigned int TSREC_E_SRC_SELECT          :  1;  /* 20..20, 0x00100000 */
		unsigned int TSREC_F_SRC_SELECT          :  1;  /* 21..21, 0x00200000 */
		unsigned int rsv_22                      : 10;  /* 22..31, 0xFFC00000 */
	} bits;
	unsigned int val;
};


union REG_TSREC_TIMER_CFG { /* 0x1A024004 */
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


#define TSREC_INT_WCLR_EN_BIT     BIT(31)
union REG_TSREC_INT_EN { /* 0x1A024010 */
	struct {
		unsigned int TSREC_A_EXP0_VSYNC_INT_EN   :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_A_EXP1_VSYNC_INT_EN   :  1;  /*  1.. 1, 0x00000002 */
		unsigned int TSREC_A_EXP2_VSYNC_INT_EN   :  1;  /*  2.. 2, 0x00000004 */
		unsigned int TSREC_B_EXP0_VSYNC_INT_EN   :  1;  /*  3.. 3, 0x00000008 */
		unsigned int TSREC_B_EXP1_VSYNC_INT_EN   :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_B_EXP2_VSYNC_INT_EN   :  1;  /*  5.. 5, 0x00000020 */
		unsigned int TSREC_C_EXP0_VSYNC_INT_EN   :  1;  /*  6.. 6, 0x00000040 */
		unsigned int TSREC_C_EXP1_VSYNC_INT_EN   :  1;  /*  7.. 7, 0x00000080 */
		unsigned int TSREC_C_EXP2_VSYNC_INT_EN   :  1;  /*  8.. 8, 0x00000100 */
		unsigned int TSREC_D_EXP0_VSYNC_INT_EN   :  1;  /*  9.. 9, 0x00000200 */
		unsigned int TSREC_D_EXP1_VSYNC_INT_EN   :  1;  /* 10..10, 0x00000400 */
		unsigned int TSREC_D_EXP2_VSYNC_INT_EN   :  1;  /* 11..11, 0x00000800 */
		unsigned int rsv_12                      :  4;  /* 12..15, 0x0000F000 */
		unsigned int TSREC_A_EXP0_HSYNC_INT_EN   :  1;  /* 16..16, 0x00010000 */
		unsigned int TSREC_A_EXP1_HSYNC_INT_EN   :  1;  /* 17..17, 0x00020000 */
		unsigned int TSREC_A_EXP2_HSYNC_INT_EN   :  1;  /* 18..18, 0x00040000 */
		unsigned int TSREC_B_EXP0_HSYNC_INT_EN   :  1;  /* 19..19, 0x00080000 */
		unsigned int TSREC_B_EXP1_HSYNC_INT_EN   :  1;  /* 20..20, 0x00100000 */
		unsigned int TSREC_B_EXP2_HSYNC_INT_EN   :  1;  /* 21..21, 0x00200000 */
		unsigned int TSREC_C_EXP0_HSYNC_INT_EN   :  1;  /* 22..22, 0x00400000 */
		unsigned int TSREC_C_EXP1_HSYNC_INT_EN   :  1;  /* 23..23, 0x00800000 */
		unsigned int TSREC_C_EXP2_HSYNC_INT_EN   :  1;  /* 24..24, 0x01000000 */
		unsigned int TSREC_D_EXP0_HSYNC_INT_EN   :  1;  /* 25..25, 0x02000000 */
		unsigned int TSREC_D_EXP1_HSYNC_INT_EN   :  1;  /* 26..26, 0x04000000 */
		unsigned int TSREC_D_EXP2_HSYNC_INT_EN   :  1;  /* 27..27, 0x08000000 */
		unsigned int rsv_28                      :  3;  /* 28..30, 0x70000000 */
		unsigned int TSREC_INT_WCLR_EN           :  1;  /* 31..31, 0x80000000 */
	} bits;
	unsigned int val;
};


union REG_TSREC_INT_STATUS { /* 0x1A024014 */
	struct {
		unsigned int TSREC_A_EXP0_VSYNC_INT      :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_A_EXP1_VSYNC_INT      :  1;  /*  1.. 1, 0x00000002 */
		unsigned int TSREC_A_EXP2_VSYNC_INT      :  1;  /*  2.. 2, 0x00000004 */
		unsigned int TSREC_B_EXP0_VSYNC_INT      :  1;  /*  3.. 3, 0x00000008 */
		unsigned int TSREC_B_EXP1_VSYNC_INT      :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_B_EXP2_VSYNC_INT      :  1;  /*  5.. 5, 0x00000020 */
		unsigned int TSREC_C_EXP0_VSYNC_INT      :  1;  /*  6.. 6, 0x00000040 */
		unsigned int TSREC_C_EXP1_VSYNC_INT      :  1;  /*  7.. 7, 0x00000080 */
		unsigned int TSREC_C_EXP2_VSYNC_INT      :  1;  /*  8.. 8, 0x00000100 */
		unsigned int TSREC_D_EXP0_VSYNC_INT      :  1;  /*  9.. 9, 0x00000200 */
		unsigned int TSREC_D_EXP1_VSYNC_INT      :  1;  /* 10..10, 0x00000400 */
		unsigned int TSREC_D_EXP2_VSYNC_INT      :  1;  /* 11..11, 0x00000800 */
		unsigned int rsv_12                      :  4;  /* 12..15, 0x0000F000 */
		unsigned int TSREC_A_EXP0_HSYNC_INT      :  1;  /* 16..16, 0x00010000 */
		unsigned int TSREC_A_EXP1_HSYNC_INT      :  1;  /* 17..17, 0x00020000 */
		unsigned int TSREC_A_EXP2_HSYNC_INT      :  1;  /* 18..18, 0x00040000 */
		unsigned int TSREC_B_EXP0_HSYNC_INT      :  1;  /* 19..19, 0x00080000 */
		unsigned int TSREC_B_EXP1_HSYNC_INT      :  1;  /* 20..20, 0x00100000 */
		unsigned int TSREC_B_EXP2_HSYNC_INT      :  1;  /* 21..21, 0x00200000 */
		unsigned int TSREC_C_EXP0_HSYNC_INT      :  1;  /* 22..22, 0x00400000 */
		unsigned int TSREC_C_EXP1_HSYNC_INT      :  1;  /* 23..23, 0x00800000 */
		unsigned int TSREC_C_EXP2_HSYNC_INT      :  1;  /* 24..24, 0x01000000 */
		unsigned int TSREC_D_EXP0_HSYNC_INT      :  1;  /* 25..25, 0x02000000 */
		unsigned int TSREC_D_EXP1_HSYNC_INT      :  1;  /* 26..26, 0x04000000 */
		unsigned int TSREC_D_EXP2_HSYNC_INT      :  1;  /* 27..27, 0x08000000 */
		unsigned int rsv_28                      :  4;  /* 28..31, 0xF0000000 */
	} bits;
	unsigned int val;
};


union REG_TSREC_INT_EN_2 { /* 0x1A02401C */
	struct {
		unsigned int TSREC_E_EXP0_VSYNC_INT_EN   :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_E_EXP1_VSYNC_INT_EN   :  1;  /*  1.. 1, 0x00000002 */
		unsigned int TSREC_E_EXP2_VSYNC_INT_EN   :  1;  /*  2.. 2, 0x00000004 */
		unsigned int TSREC_F_EXP0_VSYNC_INT_EN   :  1;  /*  3.. 3, 0x00000008 */
		unsigned int TSREC_F_EXP1_VSYNC_INT_EN   :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_F_EXP2_VSYNC_INT_EN   :  1;  /*  5.. 5, 0x00000020 */
		unsigned int rsv_6                       : 10;  /*  6..15, 0x0000FFC0 */
		unsigned int TSREC_E_EXP0_HSYNC_INT_EN   :  1;  /* 16..16, 0x00010000 */
		unsigned int TSREC_E_EXP1_HSYNC_INT_EN   :  1;  /* 17..17, 0x00020000 */
		unsigned int TSREC_E_EXP2_HSYNC_INT_EN   :  1;  /* 18..18, 0x00040000 */
		unsigned int TSREC_F_EXP0_HSYNC_INT_EN   :  1;  /* 19..19, 0x00080000 */
		unsigned int TSREC_F_EXP1_HSYNC_INT_EN   :  1;  /* 20..20, 0x00100000 */
		unsigned int TSREC_F_EXP2_HSYNC_INT_EN   :  1;  /* 21..21, 0x00200000 */
		unsigned int rsv_22                      : 10;  /* 22..31, 0xFFC00000 */
	} bits;
	unsigned int val;
};


union REG_TSREC_INT_STATUS_2 { /* 0x1A024020 */
	struct {
		unsigned int TSREC_E_EXP0_VSYNC_INT      :  1;  /*  0.. 0, 0x00000001 */
		unsigned int TSREC_E_EXP1_VSYNC_INT      :  1;  /*  1.. 1, 0x00000002 */
		unsigned int TSREC_E_EXP2_VSYNC_INT      :  1;  /*  2.. 2, 0x00000004 */
		unsigned int TSREC_F_EXP0_VSYNC_INT      :  1;  /*  3.. 3, 0x00000008 */
		unsigned int TSREC_F_EXP1_VSYNC_INT      :  1;  /*  4.. 4, 0x00000010 */
		unsigned int TSREC_F_EXP2_VSYNC_INT      :  1;  /*  5.. 5, 0x00000020 */
		unsigned int rsv_6                       : 10;  /*  6..15, 0x0000FFC0 */
		unsigned int TSREC_E_EXP0_HSYNC_INT      :  1;  /* 16..16, 0x00010000 */
		unsigned int TSREC_E_EXP1_HSYNC_INT      :  1;  /* 17..17, 0x00020000 */
		unsigned int TSREC_E_EXP2_HSYNC_INT      :  1;  /* 18..18, 0x00040000 */
		unsigned int TSREC_F_EXP0_HSYNC_INT      :  1;  /* 19..19, 0x00080000 */
		unsigned int TSREC_F_EXP1_HSYNC_INT      :  1;  /* 20..20, 0x00100000 */
		unsigned int TSREC_F_EXP2_HSYNC_INT      :  1;  /* 21..21, 0x00200000 */
		unsigned int rsv_22                      : 10;  /* 22..31, 0xFFC00000 */
	} bits;
	unsigned int val;
};


/******************************************************************************
 * TSREC registers general union structure / define
 *****************************************************************************/
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
