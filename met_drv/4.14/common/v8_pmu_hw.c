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

#include <linux/perf_event.h>
#include <asm/cpu.h>
#include "met_kernel_symbol.h"
#include "cpu_pmu.h"

/*******************************
 *      ARM v8 operations      *
 *******************************/
/*
 * Per-CPU PMCR: config reg
 */
#define ARMV8_PMCR_E		(1 << 0)	/* Enable all counters */
#define ARMV8_PMCR_P		(1 << 1)	/* Reset all counters */
#define ARMV8_PMCR_C		(1 << 2)	/* Cycle counter reset */
#define ARMV8_PMCR_D		(1 << 3)	/* CCNT counts every 64th cpu cycle */
#define ARMV8_PMCR_X		(1 << 4)	/* Export to ETM */
#define ARMV8_PMCR_DP		(1 << 5)	/* Disable CCNT if non-invasive debug */
#define	ARMV8_PMCR_N_SHIFT	11		/* Number of counters supported */
#define	ARMV8_PMCR_N_MASK	0x1f
#define	ARMV8_PMCR_MASK		0x3f		/* Mask for writable bits */

/*
 * PMOVSR: counters overflow flag status reg
 */
#define	ARMV8_OVSR_MASK		0xffffffff	/* Mask for writable bits */
#define	ARMV8_OVERFLOWED_MASK	ARMV8_OVSR_MASK

static inline void armv8_pmu_counter_select(unsigned int idx)
{
	asm volatile ("msr pmselr_el0, %0"::"r" (idx));
	isb();
}

static inline void armv8_pmu_type_select(unsigned int idx, unsigned int type)
{
	armv8_pmu_counter_select(idx);
	asm volatile ("msr pmxevtyper_el0, %0"::"r" (type));
}

static inline unsigned int armv8_pmu_read_count(unsigned int idx)
{
	unsigned int value;

	if (idx == 31) {
		asm volatile ("mrs %0, pmccntr_el0":"=r" (value));
	} else {
		armv8_pmu_counter_select(idx);
		asm volatile ("mrs %0, pmxevcntr_el0":"=r" (value));
	}
	return value;
}

static inline void armv8_pmu_enable_count(unsigned int idx)
{
	asm volatile ("msr pmcntenset_el0, %0"::"r" (1 << idx));
}

static inline void armv8_pmu_disable_count(unsigned int idx)
{
	asm volatile ("msr pmcntenclr_el0, %0"::"r" (1 << idx));
}

static inline void armv8_pmu_enable_intr(unsigned int idx)
{
	asm volatile ("msr pmintenset_el1, %0"::"r" (1 << idx));
}

static inline void armv8_pmu_disable_intr(unsigned int idx)
{
	asm volatile ("msr pmintenclr_el1, %0"::"r" (1 << idx));
	isb();
	asm volatile ("msr pmovsclr_el0, %0"::"r" (1 << idx));
	isb();
}

static inline unsigned int armv8_pmu_overflow(void)
{
	unsigned int val;

	asm volatile ("mrs %0, pmovsclr_el0":"=r" (val));	/* read */
	val &= ARMV8_OVSR_MASK;
	asm volatile ("mrs %0, pmovsclr_el0"::"r" (val));
	return val;
}

static inline unsigned int armv8_pmu_control_read(void)
{
	unsigned int val;

	asm volatile ("mrs %0, pmcr_el0":"=r" (val));
	return val;
}

static inline void armv8_pmu_control_write(u32 val)
{
	val &= ARMV8_PMCR_MASK;
	isb();
	asm volatile ("msr pmcr_el0, %0"::"r" (val));
}

static void armv8_pmu_hw_reset_all(int generic_counters)
{
	int i;

	armv8_pmu_control_write(ARMV8_PMCR_C | ARMV8_PMCR_P);
	/* generic counter */
	for (i = 0; i < generic_counters; i++) {
		armv8_pmu_disable_intr(i);
		armv8_pmu_disable_count(i);
	}
	/* cycle counter */
	armv8_pmu_disable_intr(31);
	armv8_pmu_disable_count(31);
	armv8_pmu_overflow();	/* clear overflow */
}

/***********************************
 *      MET ARM v8 operations      *
 ***********************************/
enum ARM_TYPE {
	CORTEX_A53 = 0xD03,
	CORTEX_A35 = 0xD04,
	CORTEX_A55 = 0xD05,
	CORTEX_A57 = 0xD07,
	CORTEX_A72 = 0xD08,
	CORTEX_A73 = 0xD09,
	CORTEX_A75 = 0xD0A,
	CORTEX_A76 = 0xD0B,
	CHIP_UNKNOWN = 0xFFF
};

struct chip_pmu {
	enum ARM_TYPE	type;
	unsigned int	event_count;
};

static struct chip_pmu	chips[] = {
	{CORTEX_A35, 6+1},
	{CORTEX_A53, 6+1},
	{CORTEX_A55, 6+1},
	{CORTEX_A57, 6+1},
	{CORTEX_A72, 6+1},
	{CORTEX_A73, 6+1},
	{CORTEX_A75, 6+1},
	{CORTEX_A76, 6+1},
};

static int armv8_pmu_hw_check_event(struct met_pmu *pmu, int idx, int event)
{
	int i;

	/* Check if event is duplicate */
	for (i = 0; i < idx; i++) {
		if (pmu[i].event == event)
			break;
	}
	if (i < idx) {
		/* pr_debug("++++++ found duplicate event 0x%02x i=%d\n", event, i); */
		return -1;
	}

	return 0;
}

static void armv8_pmu_hw_start(struct met_pmu *pmu, int count)
{
	int i;
	int generic = count - 1;

	armv8_pmu_hw_reset_all(generic);
	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			armv8_pmu_type_select(i, pmu[i].event);
			armv8_pmu_enable_count(i);
		}
	}
	if (pmu[count - 1].mode == MODE_POLLING) {	/* cycle counter */
		armv8_pmu_enable_count(31);
	}
	armv8_pmu_control_write(ARMV8_PMCR_E);
}

static void armv8_pmu_hw_stop(int count)
{
	int generic = count - 1;

	armv8_pmu_hw_reset_all(generic);
}

static unsigned int armv8_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value)
{
	int i, cnt = 0;
	int generic = count - 1;

	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pmu_value[cnt] = armv8_pmu_read_count(i);
			cnt++;
		}
	}
	if (pmu[count - 1].mode == MODE_POLLING) {
		pmu_value[cnt] = armv8_pmu_read_count(31);
		cnt++;
	}
	armv8_pmu_control_write(ARMV8_PMCR_C | ARMV8_PMCR_P | ARMV8_PMCR_E);

	return cnt;
}

static unsigned long armv8_perf_event_get_evttype(struct perf_event *ev) {

	struct hw_perf_event *hwc;

	hwc = &ev->hw;
	return hwc->config_base & ARMV8_PMU_EVTYPE_EVENT;
}

#define	PMU_OVSR_MASK		0xffffffff     /* Mask for writable bits */

static u32 armv8_pmu_read_clear_overflow_flag(void)
{
	u32 value;

	asm volatile ("mrs %0, pmovsclr_el0":"=r" (value));

	/* Write to clear flags */
	value &= PMU_OVSR_MASK;
	asm volatile ("msr pmovsclr_el0, %0"::"r" (value));

	return value;
}

static struct met_pmu	pmus[MXNR_CPU][MXNR_PMU_EVENTS];

struct cpu_pmu_hw armv8_pmu = {
	.name = "armv8_pmu",
	.check_event = armv8_pmu_hw_check_event,
	.start = armv8_pmu_hw_start,
	.stop = armv8_pmu_hw_stop,
	.polling = armv8_pmu_hw_polling,
	.perf_event_get_evttype = armv8_perf_event_get_evttype,
	.pmu_read_clear_overflow_flag = armv8_pmu_read_clear_overflow_flag,
};

static void init_pmus(void)
{
	int	cpu;
	int	i;

	for_each_possible_cpu(cpu) {
		struct cpuinfo_arm64 *cpuinfo;
		if (cpu >= MXNR_CPU)
			continue;
		met_get_cpuinfo_symbol(cpu, &cpuinfo);
		/* PR_BOOTMSG("CPU[%d]: reg_midr = %x\n", cpu, cpuinfo->reg_midr); */
		for (i = 0; i < ARRAY_SIZE(chips); i++) {
			if (chips[i].type == (cpuinfo->reg_midr & 0xffff) >> 4) {
				armv8_pmu.event_count[cpu] = chips[i].event_count;
				break;
			}
		}
	}
}

struct cpu_pmu_hw *cpu_pmu_hw_init(void)
{
	int	cpu;

	init_pmus();
	for (cpu = 0; cpu < MXNR_CPU; cpu++)
		armv8_pmu.pmu[cpu] = pmus[cpu];

	return &armv8_pmu;
}
