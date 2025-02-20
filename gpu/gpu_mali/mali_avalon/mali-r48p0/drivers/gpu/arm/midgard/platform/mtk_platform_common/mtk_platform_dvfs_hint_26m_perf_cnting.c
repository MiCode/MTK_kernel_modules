// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <platform/mtk_platform_common/mtk_platform_dvfs_hint_26m_perf_cnting.h>

static void __iomem *io_dvfs_top_base_addr;
static void __iomem *io_top_base_addr;
static void __iomem *io_sc_base_addr;
static void __iomem *io_gpu_sysram_addr;

static u64 g_last_us;
static unsigned int g_last_active_counter;
unsigned int g_before_power_off_counter;

bool dvfs_hint_26m_perf_cnting_enable;
bool Enable_IPA;
bool gpu_power_status;

static void dvfs_hint_dvfs_top_base_reg_write(u32 offset, u32 value)
{
	writel(value, io_dvfs_top_base_addr + offset);
}

static u32 dvfs_hint_dvfs_top_base_reg_read(u32 offset)
{

	if (!io_dvfs_top_base_addr)
		return -1;

	if ((offset % 4) != 0)
		return -1;

	return readl(io_dvfs_top_base_addr + offset);
}

static void dvfs_hint_top_base_reg_write(u32 offset, u32 value)
{
	writel(value, io_top_base_addr + offset);

}

static u32 dvfs_hint_top_base_reg_read(u32 offset)
{
	if (!io_top_base_addr)
		return -1;

	if ((offset % 4) != 0)
		return -1;

	return readl(io_top_base_addr + offset);
}

void dvfs_hint_sc_base_reg_write(u32 offset, u32 value)
{
	writel(value, io_sc_base_addr + offset);
}

u32 dvfs_hint_sc_base_reg_read(u32 offset)
{

	if (!io_sc_base_addr)
		return -1;

	if ((offset % 4) != 0)
		return -1;

	return readl(io_sc_base_addr + offset);
}

unsigned int mtk_dvfs_hint_26m_sc_prfcnt_query(u32 reg)
{
	unsigned int active_counter;

	//snapshot performance counter to register bank
	dvfs_hint_sc_base_reg_read(0x700);

	//get active counter value
	active_counter = dvfs_hint_sc_base_reg_read(reg);

	//if get value fail return 100 to debug
	if (active_counter < 0)
		return 100;
	else
		return active_counter;

}

unsigned int mtk_dvfs_hint_26m_prfcnt_query(u32 reg)
{
	unsigned int active_counter;

	//snapshot performance counter to register bank
	dvfs_hint_dvfs_top_base_reg_read(0x0068);

	//get active counter value
	active_counter = dvfs_hint_dvfs_top_base_reg_read(reg);

	//if get value fail return 100 to debug
	if (active_counter < 0)
		return 100;
	else
		return active_counter;
}

unsigned int mtk_dvfs_hint_26m_cal_prfcnt_utilization(u32 reg, bool is_gpu_powered)
{
	unsigned int cur_active_counter;
	ktime_t current_timestamp;
	u64 cur_us;
	unsigned long long delta_time;
	unsigned int delta_dvfs_hint_26M_counter;
	unsigned int active_26M_loading;

	if(is_gpu_powered)
		cur_active_counter = mtk_dvfs_hint_26m_prfcnt_query(reg);
	else
		cur_active_counter = 0;

	current_timestamp = ktime_get();
	cur_us =(u64)(ktime_to_us(current_timestamp));
	delta_time = cur_us - g_last_us;

	//save counter value before GPU power off ,then counter reg will be clear.
	//add before_power_off_counter to calculate correct loading.
	if (g_before_power_off_counter){

		//cur counter may equal before_power_off_counter, because get before gpu power off
		if (cur_active_counter == g_before_power_off_counter)
			delta_dvfs_hint_26M_counter = cur_active_counter - g_last_active_counter;
		else {
			if (g_before_power_off_counter >= g_last_active_counter)
				delta_dvfs_hint_26M_counter = cur_active_counter + (g_before_power_off_counter - g_last_active_counter);
			else
				delta_dvfs_hint_26M_counter = cur_active_counter + g_before_power_off_counter + (MAX_26M_PRFCNT_VALUE - g_last_active_counter);
			g_before_power_off_counter = 0;
		}

	} else {
		if (cur_active_counter >= g_last_active_counter)
			delta_dvfs_hint_26M_counter = cur_active_counter - g_last_active_counter;
		else
			delta_dvfs_hint_26M_counter = cur_active_counter + (MAX_26M_PRFCNT_VALUE - g_last_active_counter);
	}

	g_last_us = cur_us;
	g_last_active_counter = cur_active_counter;

	active_26M_loading = (100 * delta_dvfs_hint_26M_counter) / (delta_time * DVFS_HINT_26M_FREQ);

	if (active_26M_loading > 100)
		active_26M_loading = 100;

	return active_26M_loading;

}

int mtk_dvfs_hint_26m_setting(void)
{
	unsigned int dvfs_hint_cg, dvfs_hint_cg_en;

	// Enable dvfs hint to count cg event 26m
	dvfs_hint_dvfs_top_base_reg_write(0x48,0x7F);

	// event mask[2:0] = reg($DVFS_TOP + 0x44)[2:0]
	dvfs_hint_dvfs_top_base_reg_write(0x44,0x0007);

	// dvfs_hint_cg_en
	dvfs_hint_cg = dvfs_hint_top_base_reg_read(0xB0);
	dvfs_hint_cg_en = dvfs_hint_cg|0x200000;
	dvfs_hint_top_base_reg_write(0xB0,dvfs_hint_cg_en);

	// reg master id dis
	dvfs_hint_dvfs_top_base_reg_write(0x10,0x4000);

	//dvfs_hint_top_base_reg_write(0x5E8, 0x1);
	//dvfs_hint_top_base_reg_write(0x5EC, 0x5);

	return 0;
}

int mtk_dvfs_hint_26m_init(struct kbase_device *kbdev ,phys_addr_t top_base_addr ,phys_addr_t dvfs_top_addr)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	io_dvfs_top_base_addr = ioremap(dvfs_top_addr  , 0x1000);
	io_top_base_addr = ioremap(top_base_addr  , 0x1000);

	dvfs_hint_26m_perf_cnting_enable = true;
	Enable_IPA = false;
	gpu_power_status = true;

	return 0;
}

