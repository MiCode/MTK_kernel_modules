/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <asm/system.h>
#include <linux/smp.h>

#include "cpu_pmu.h"
#include "mips_pmu_name.h"

struct chip_pmu {
	enum cpu_type_enum type;
	struct pmu_desc **desc;
	void *refptr;
	const char *cpu_name;
	unsigned int pmu_desc_size;
	unsigned int max_hw_events;
	unsigned int max_reg_count;
};

struct pmu_desc *mips_pmu_desc[MIPS_MAX_HWEVENTS];

static struct chip_pmu chips[] = {
	{CPU_1004K, mips_pmu_desc, (void *)mips_1004k_pmu_desc, "MIPS_1004K",
	 MIPS_1004K_PMU_DESC_SIZE, MIPS_1004K_PMU_DESC_COUNT, PMU_1004K_MAX_HW_REGS},
};

static struct chip_pmu chip_unknown = { CPU_UNKNOWN, NULL, NULL, "Unknown CPU", 0, 0, 0 };

#define CHIP_PMU_COUNT (sizeof(chips) / sizeof(struct chip_pmu))
static struct chip_pmu *chip;
#define M_CONFIG1_PC    (1 << 4)

#define M_PERFCTL_EXL           (1  <<  0)
#define M_PERFCTL_KERNEL        (1  <<  1)
#define M_PERFCTL_SUPERVISOR        (1  <<  2)
#define M_PERFCTL_USER          (1  <<  3)
#define M_PERFCTL_INTERRUPT_ENABLE  (1  <<  4)
#define M_PERFCTL_EVENT(event)      (((event) & 0x3ff)  << 5)
#define M_PERFCTL_VPEID(vpe)        ((vpe)    << 16)

#ifdef CONFIG_CPU_BMIPS5000
#define M_PERFCTL_MT_EN(filter)     0
#else				/* !CONFIG_CPU_BMIPS5000 */
#define M_PERFCTL_MT_EN(filter)     ((filter) << 20)
#endif				/* CONFIG_CPU_BMIPS5000 */

#define    M_TC_EN_ALL          M_PERFCTL_MT_EN(0)
#define    M_TC_EN_VPE          M_PERFCTL_MT_EN(1)
#define    M_TC_EN_TC           M_PERFCTL_MT_EN(2)
#define M_PERFCTL_TCID(tcid)        ((tcid)   << 22)
#define M_PERFCTL_WIDE          (1  << 30)
#define M_PERFCTL_MORE          (1  << 31)
#define M_PERFCTL_TC            (1  << 30)

#define M_PERFCTL_COUNT_EVENT_WHENEVER  (M_PERFCTL_EXL |        \
		M_PERFCTL_KERNEL |      \
		M_PERFCTL_USER |        \
		M_PERFCTL_SUPERVISOR |      \
		M_PERFCTL_INTERRUPT_ENABLE)

#ifdef CONFIG_MIPS_MT_SMP
#define M_PERFCTL_CONFIG_MASK       0x3fff801f
#else
#define M_PERFCTL_CONFIG_MASK       0x1f
#endif
#define M_PERFCTL_EVENT_MASK        0xfe0

#define vpe_id()    0

/* To get current TCID*/
#define read_c0_tcbind() __read_32bit_c0_register($2, 2)

struct cpu_hw_events {
	unsigned int config_base[MIPS_MAX_HWEVENTS];
	unsigned int saved_ctrl[MIPS_MAX_HWEVENTS];
};

DEFINE_PER_CPU(struct cpu_hw_events, cpu_hw_events) = {
	.config_base = {
	0, 0, 0, 0}, .saved_ctrl = {
0, 0, 0, 0},};

static enum cpu_type_enum mips_get_ic(void)
{
	unsigned int value = current_cpu_type();

	/* pr_debug("ic value: %X\n", value); */
	return value;
}

static int __n_counters(void)
{
	if (!(read_c0_config1() & M_CONFIG1_PC))
		return 0;
	if (!(read_c0_perfctrl0() & M_PERFCTL_MORE))
		return 1;
	if (!(read_c0_perfctrl1() & M_PERFCTL_MORE))
		return 2;
	if (!(read_c0_perfctrl2() & M_PERFCTL_MORE))
		return 3;

	return 4;
}

static int n_counters(void)
{
	int counters;

	switch (current_cpu_type()) {
	case CPU_R10000:
		counters = 2;
		break;
	case CPU_R12000:
	case CPU_R14000:
		counters = 4;
		break;
	default:
		counters = __n_counters();
		break;
	}

	return counters;
}

static int mips_pmu_hw_get_counters(void)
{
	int count = n_counters();

	/* pr_debug("pmu hw event nr: %d\n", count); */
	return count;
}

static unsigned int mipsxx_pmu_swizzle_perf_idx(unsigned int idx)
{
	if (vpe_id() == 1)
		idx = (idx + 2) & 3;
	return idx;
}

static void mipsxx_pmu_write_counter(unsigned int idx, u64 val)
{
	idx = mipsxx_pmu_swizzle_perf_idx(idx);

	switch (idx) {
	case 0:
		write_c0_perfcntr0(val);
		return;
	case 1:
		write_c0_perfcntr1(val);
		return;
	case 2:
		write_c0_perfcntr2(val);
		return;
	case 3:
		write_c0_perfcntr3(val);
		return;
	}
}

static u64 mipsxx_pmu_read_counter(unsigned int idx)
{
	idx = mipsxx_pmu_swizzle_perf_idx(idx);

	switch (idx) {
	case 0:
		/*
		 * The counters are unsigned, we must cast to truncate
		 * off the high bits.
		 */
		return (u32) read_c0_perfcntr0();
	case 1:
		return (u32) read_c0_perfcntr1();
	case 2:
		return (u32) read_c0_perfcntr2();
	case 3:
		return (u32) read_c0_perfcntr3();
	default:
		WARN_ONCE(1, "Invalid performance counter number (%d)\n", idx);
		return 0;
	}
}


static unsigned int mipsxx_pmu_read_control(unsigned int idx)
{
	idx = mipsxx_pmu_swizzle_perf_idx(idx);

	switch (idx) {
	case 0:
		return read_c0_perfctrl0();
	case 1:
		return read_c0_perfctrl1();
	case 2:
		return read_c0_perfctrl2();
	case 3:
		return read_c0_perfctrl3();
	default:
		WARN_ONCE(1, "Invalid performance counter number (%d)\n", idx);
		return 0;
	}
}

static void mipsxx_pmu_write_control(unsigned int idx, unsigned int val)
{
	idx = mipsxx_pmu_swizzle_perf_idx(idx);

	switch (idx) {
	case 0:
		write_c0_perfctrl0(val);
		return;
	case 1:
		write_c0_perfctrl1(val);
		return;
	case 2:
		write_c0_perfctrl2(val);
		return;
	case 3:
		write_c0_perfctrl3(val);
		return;
	}
}

static int mipsxx_pmu_get_vpeid(void)
{
	return read_c0_tcbind() & 0xF;
}

static void mipsxx_pmu_reset_counters(int idx)
{
	switch (idx) {
	case 3:
		mipsxx_pmu_write_control(3, 0);
		mipsxx_pmu_write_counter(3, 0);
		break;
	case 2:
		mipsxx_pmu_write_control(2, 0);
		mipsxx_pmu_write_counter(2, 0);
		break;
	case 1:
		mipsxx_pmu_write_control(1, 0);
		mipsxx_pmu_write_counter(1, 0);
		break;
	case 0:
		mipsxx_pmu_write_control(0, 0);
		mipsxx_pmu_write_counter(0, 0);
		break;
	}
}

static void mipsxx_pmu_enable_event(int idx, int event)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	unsigned long flags;

	WARN_ON(idx < 0 || idx >= chip->max_hw_events);
	cpuc->saved_ctrl[idx] = M_PERFCTL_EVENT(event & 0xff) |
	    M_PERFCTL_VPEID(mipsxx_pmu_get_vpeid()) |
	    (cpuc->config_base[idx] & M_PERFCTL_CONFIG_MASK);
#ifdef CONFIG_CPU_BMIPS5000
	/* if (IS_ENABLED(CONFIG_CPU_BMIPS5000)) */
	/* enable the counter for the calling thread */
	cpuc->saved_ctrl[idx] |= (1 << (12 + vpe_id())) | M_PERFCTL_TC;
#endif
	/*
	 * To enable pmu count
	 */
	local_irq_save(flags);
	mipsxx_pmu_write_control(idx, cpuc->saved_ctrl[idx]);
	local_irq_restore(flags);
}

static void mipsxx_pmu_disable_event(int idx)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	unsigned long flags;

	/* WARN_ON(idx < 0 || idx >= mipspmu.num_counters); */
	WARN_ON(idx < 0 || idx >= chip->max_hw_events);

	local_irq_save(flags);
	cpuc->saved_ctrl[idx] = mipsxx_pmu_read_control(idx) & ~M_PERFCTL_COUNT_EVENT_WHENEVER;
	mipsxx_pmu_write_control(idx, cpuc->saved_ctrl[idx]);
	local_irq_restore(flags);
}

static int mips_pmu_hw_get_event_desc(int idx, int event, char *event_desc)
{
	int i;

	if (event_desc == NULL) {
		pr_debug("event_desc is NULL\n");
		return -1;
	}

	for (i = 0; i < chip->max_reg_count; i++) {
		if (chip->desc[idx][i].event == event) {
			strncpy(event_desc, chip->desc[idx][i].name, MXSIZE_PMU_DESC - 1);
			break;
		}
	}
	if (i == chip->max_reg_count)
		return -1;

	return 0;
}


static int mips_pmu_hw_check_event(struct met_pmu *pmu, int idx, int event)
{
	int i;

	/* to check index over run */
	if (!chip)
		return -1;

	if (idx >= chip->max_hw_events)
		return -1;

	for (i = 0; i < chip->max_reg_count; i++) {
		if (chip->desc[idx][i].event == event)
			break;
	}
	if (i == chip->max_reg_count)
		return -1;

	return 0;
}

static void mips_pmu_hw_start(struct met_pmu *pmu, int count)
{
	int i;
	int generic = count - 1;
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);

	/* pr_debug("hw_start generic: %d\n", generic); */
	for (i = 0; i < generic; i++) {
		/* init config */
		cpuc->config_base[i] = 0;
		cpuc->config_base[i] |= M_TC_EN_VPE;
		cpuc->config_base[i] |= M_PERFCTL_USER;
		cpuc->config_base[i] |= M_PERFCTL_KERNEL;
		cpuc->config_base[i] |= M_PERFCTL_EXL;
		cpuc->config_base[i] |= M_PERFCTL_SUPERVISOR;
		cpuc->config_base[i] &= M_PERFCTL_CONFIG_MASK;
		 /**/ mipsxx_pmu_reset_counters(i);
		if (pmu[i].mode == MODE_POLLING)
			mipsxx_pmu_enable_event(i, pmu[i].event);
	}
	if (pmu[count - 1].mode == MODE_POLLING)
		pr_debug("%s %d BUG!!! index over run!!\n", __func__, __LINE__);
}

static void mips_pmu_hw_stop(int count)
{
	int idx = 0;
	int generic = count - 1;
	/* pr_debug("reset %d\n", generic); */
	for (idx = 0; idx < generic; idx++) {
		mipsxx_pmu_reset_counters(idx);
		mipsxx_pmu_disable_event(idx);
	}
}


static unsigned int mips_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value)
{
	int i, cnt = 0;
	int generic = count - 1;

	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pmu_value[cnt] = mipsxx_pmu_read_counter(i);
			cnt++;
			mipsxx_pmu_reset_counters(i);
			mipsxx_pmu_enable_event(i, pmu[i].event);
		}
	}
	if (pmu[count - 1].mode == MODE_POLLING) {
		pr_debug("%s %d BUG!!! index over run!!\n", __func__, __LINE__);
		pmu_value[cnt] = 0xFFFF;
		cnt++;
	}

	return cnt;
}



struct cpu_pmu_hw mips_pmu = {
	.name = "mips_pmu",
	.get_event_desc = mips_pmu_hw_get_event_desc,
	.check_event = mips_pmu_hw_check_event,
	.start = mips_pmu_hw_start,
	.stop = mips_pmu_hw_stop,
	.polling = mips_pmu_hw_polling,
};

struct cpu_pmu_hw *cpu_pmu_hw_init(void)
{
	int i = 0;
	enum cpu_type_enum type;
	int pmu_hw_count = 0;

	type = mips_get_ic();

	if (CPU_UNKNOWN == type || CPU_LAST == type) {
		chip = &chip_unknown;
		return NULL;
	}
	for (i = 0; i < CHIP_PMU_COUNT; i++) {
		if (chips[i].type == type) {
			chip = &(chips[i]);
			break;
		}
	}
	if (i == CHIP_PMU_COUNT) {
		chip = &chip_unknown;
		return NULL;
	}

	pmu_hw_count = mips_pmu_hw_get_counters();
	for (i = 0; i < pmu_hw_count; i++)
		chip->desc[i] = chip->refptr + (chip->pmu_desc_size * i);

	mips_pmu.nr_cnt = pmu_hw_count + 1;
	mips_pmu.cpu_name = chip->cpu_name;
	return &mips_pmu;
}
