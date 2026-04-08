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


#define SLC_PMU_CNT_AMOUNT 32

/*read from dts*/
extern int EMI_NUM;
extern int DRAMC_VER;
extern int DRAM_CH_NUM_PER_EMI;
extern int SLC_PMU_CH_NUM;
extern int MET_EMI_support_list;
extern int ddrphy_ao_misc_cg_ctrl0;
extern int ddrphy_ao_misc_cg_ctrl2;
// extern int DRAM_FREQ_DEFAULT;
// extern int DDR_RATIO_DEFAULT;
// extern int DRAM_TYPE_DEFAULT;
// extern unsigned int reserve_wsct_setting;
extern unsigned int slc_pmu_cnt_setting_enable_val_[MET_MAX_EMI_NUM][SLC_PMU_CNT_AMOUNT];
extern int EMIPLL_CON1;

#endif

