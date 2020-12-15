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

#include "cpu_pmu.h"
#include "v6_pmu_hw.h"

/*******************************
 *      ARM v7 operations      *
 *******************************/
#define ARMV7_PMCR_E		(1 << 0)	/* enable all counters */
#define ARMV7_PMCR_P		(1 << 1)
#define ARMV7_PMCR_C		(1 << 2)
#define ARMV7_PMCR_D		(1 << 3)
#define ARMV7_PMCR_X		(1 << 4)
#define ARMV7_PMCR_DP		(1 << 5)
#define ARMV7_PMCR_N_SHIFT	11		/* Number of counters supported */
#define ARMV7_PMCR_N_MASK	0x1f
#define ARMV7_PMCR_MASK		0x3f		/* mask for writable bits */

static unsigned int armv7_get_ic(void)
{
	unsigned int value;
	/* Read Main ID Register */
	asm volatile ("mrc p15, 0, %0, c0, c0, 0":"=r" (value));

	value = (value & 0xffff) >> 4;	/* primary part number */
	return value;
}

static inline void armv7_pmu_counter_select(unsigned int idx)
{
	asm volatile ("mcr p15, 0, %0, c9, c12, 5"::"r" (idx));
	isb();
}

static inline void armv7_pmu_type_select(unsigned int idx, unsigned int type)
{
	armv7_pmu_counter_select(idx);
	asm volatile ("mcr p15, 0, %0, c9, c13, 1"::"r" (type));
}

static inline unsigned int armv7_pmu_read_count(unsigned int idx)
{
	unsigned int value;

	if (idx == 31) {
		asm volatile ("mrc p15, 0, %0, c9, c13, 0":"=r" (value));
	} else {
		armv7_pmu_counter_select(idx);
		asm volatile ("mrc p15, 0, %0, c9, c13, 2":"=r" (value));
	}
	return value;
}

static inline void armv7_pmu_write_count(int idx, u32 value)
{
	if (idx == 31) {
		asm volatile ("mcr p15, 0, %0, c9, c13, 0"::"r" (value));
	} else {
		armv7_pmu_counter_select(idx);
		asm volatile ("mcr p15, 0, %0, c9, c13, 2"::"r" (value));
	}
}

static inline void armv7_pmu_enable_count(unsigned int idx)
{
	asm volatile ("mcr p15, 0, %0, c9, c12, 1"::"r" (1 << idx));
}

static inline void armv7_pmu_disable_count(unsigned int idx)
{
	asm volatile ("mcr p15, 0, %0, c9, c12, 2"::"r" (1 << idx));
}

static inline void armv7_pmu_enable_intr(unsigned int idx)
{
	asm volatile ("mcr p15, 0, %0, c9, c14, 1"::"r" (1 << idx));
}

static inline void armv7_pmu_disable_intr(unsigned int idx)
{
	asm volatile ("mcr p15, 0, %0, c9, c14, 2"::"r" (1 << idx));
}

static inline unsigned int armv7_pmu_overflow(void)
{
	unsigned int val;

	asm volatile ("mrc p15, 0, %0, c9, c12, 3":"=r" (val));	/* read */
	asm volatile ("mcr p15, 0, %0, c9, c12, 3"::"r" (val));
	return val;
}

static inline unsigned int armv7_pmu_control_read(void)
{
	u32 val;

	asm volatile ("mrc p15, 0, %0, c9, c12, 0":"=r" (val));
	return val;
}

static inline void armv7_pmu_control_write(unsigned int val)
{
	val &= ARMV7_PMCR_MASK;
	isb();
	asm volatile ("mcr p15, 0, %0, c9, c12, 0"::"r" (val));
}

static int armv7_pmu_hw_get_counters(void)
{
	int count = armv7_pmu_control_read();
	/* N, bits[15:11] */
	count = ((count >> ARMV7_PMCR_N_SHIFT) & ARMV7_PMCR_N_MASK);
	return count;
}

static void armv7_pmu_hw_reset_all(int generic_counters)
{
	int i;

	armv7_pmu_control_write(ARMV7_PMCR_C | ARMV7_PMCR_P);
	/* generic counter */
	for (i = 0; i < generic_counters; i++) {
		armv7_pmu_disable_intr(i);
		armv7_pmu_disable_count(i);
	}
	/* cycle counter */
	armv7_pmu_disable_intr(31);
	armv7_pmu_disable_count(31);
	armv7_pmu_overflow();	/* clear overflow */
}

/***********************************
 *      MET ARM v7 operations      *
 ***********************************/
enum ARM_TYPE {
	CORTEX_A7 = 0xC07,
	CORTEX_A9 = 0xC09,
	CORTEX_A12 = 0xC0D,
	CORTEX_A15 = 0xC0F,
	CORTEX_A17 = 0xC0E,
	CORTEX_A53 = 0xD03,
	CORTEX_A57 = 0xD07,
	CHIP_UNKNOWN = 0xFFF
};

struct chip_pmu {
	enum ARM_TYPE type;
};

static struct chip_pmu chips[] = {
	{CORTEX_A7},
	{CORTEX_A9},
	{CORTEX_A12},
	{CORTEX_A15},
	{CORTEX_A17},
	{CORTEX_A53},
	{CORTEX_A57},
};

static int armv7_pmu_hw_check_event(struct met_pmu *pmu, int idx, int event)
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

static void armv7_pmu_hw_start(struct met_pmu *pmu, int count)
{
	int i;
	int generic = count - 1;

	armv7_pmu_hw_reset_all(generic);
	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			armv7_pmu_type_select(i, pmu[i].event);
			armv7_pmu_enable_count(i);
		}
	}
	if (pmu[count - 1].mode == MODE_POLLING) {	/* cycle counter */
		armv7_pmu_enable_count(31);
	}
	armv7_pmu_control_write(ARMV7_PMCR_E);
}

static void armv7_pmu_hw_stop(int count)
{
	int generic = count - 1;

	armv7_pmu_hw_reset_all(generic);
}

static unsigned int armv7_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value)
{
	int i, cnt = 0;
	int generic = count - 1;

	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pmu_value[cnt] = armv7_pmu_read_count(i);
			cnt++;
		}
	}
	if (pmu[count - 1].mode == MODE_POLLING) {
		pmu_value[cnt] = armv7_pmu_read_count(31);
		cnt++;
	}
	armv7_pmu_control_write(ARMV7_PMCR_C | ARMV7_PMCR_P | ARMV7_PMCR_E);

	return cnt;
}

static struct met_pmu	pmus[MXNR_CPU][MXNR_PMU_EVENTS];

struct cpu_pmu_hw armv7_pmu = {
	.name = "armv7_pmu",
	.check_event = armv7_pmu_hw_check_event,
	.start = armv7_pmu_hw_start,
	.stop = armv7_pmu_hw_stop,
	.polling = armv7_pmu_hw_polling,
};

struct cpu_pmu_hw *cpu_pmu_hw_init(void)
{
	int i;
	enum ARM_TYPE type;
	struct cpu_pmu_hw *pmu;

	type = (enum ARM_TYPE)armv7_get_ic();
	for (i = 0; i < ARRAY_SIZE(chips); i++)
		if (chips[i].type == type)
			break;

	if (i < ARRAY_SIZE(chips)) {
		armv7_pmu.nr_cnt = armv7_pmu_hw_get_counters() + 1;
		pmu = &armv7_pmu;
	} else
		pmu = v6_cpu_pmu_hw_init(type);

	if (pmu == NULL)
		return NULL;

	for (i = 0; i < MXNR_CPU; i++) {
		pmu->event_count[i] = pmu->nr_cnt;
		pmu->pmu[i] = pmus[i];
	}

	return pmu;
}
