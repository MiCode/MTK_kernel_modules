// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */

#ifndef __MT_MET_EMI_BM_HW_H__
#define __MT_MET_EMI_BM_HW_H__

#define EMI_VER_MAJOR  3
#define EMI_VER_MINOR  51

#define MET_MAX_EMI_NUM         2
#define MET_MAX_DRAM_CH_NUM     2

#define DRAMC_VER 2

/*read from dts*/
extern int EMI_NUM;
extern int DRAM_CH_NUM_PER_EMI;
extern int MET_EMI_support_list;
extern int ddrphy_ao_misc_cg_ctrl0;
extern int ddrphy_ao_misc_cg_ctrl2;
// extern int DRAM_FREQ_DEFAULT;
// extern int DDR_RATIO_DEFAULT;
// extern int DRAM_TYPE_DEFAULT;
// extern unsigned int reserve_wsct_setting;


#endif

