// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_DVFS_HINT_26M_PERF_CNTING_H__
#define __MTK_PLATFORM_DVFS_HINT_26M_PERF_CNTING_H__

#define MAX_26M_PRFCNT_VALUE   0xFFFFFFFF //reg 32bit
#define DVFS_HINT_26M_FREQ 		  26//us

#define SELECT_UNION_ITER_MCU    0x318 /* read union(iter,mcu) performance counter reg*/
#define SELECT_ITER              0x30C /* read iter(frag|compute|tile) performance counter reg */

unsigned int mtk_dvfs_hint_26m_prfcnt_query(u32 reg);
void dvfs_hint_sc_base_reg_write(u32 offset, u32 value);
u32 dvfs_hint_sc_base_reg_read(u32 offset);
unsigned int mtk_dvfs_hint_26m_sc_prfcnt_query(u32 reg);
unsigned int mtk_dvfs_hint_26m_cal_prfcnt_utilization(u32 reg, bool is_gpu_powered);
int mtk_dvfs_hint_26m_setting(void);
int mtk_dvfs_hint_26m_init(struct kbase_device *kbdev ,phys_addr_t top_base_addr ,phys_addr_t dvfs_top_addr);


#endif /* __MTK_PLATFORM_DVFS_HINT_26M_PERF_CNTING_H__ */
