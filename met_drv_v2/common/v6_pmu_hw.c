// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "cpu_pmu.h"

/*******************************
 *      ARM v6 operations      *
 *******************************/
#define ARMV6_PMCR_ENABLE               (1 << 0)
#define ARMV6_PMCR_CTR01_RESET          (1 << 1)
#define ARMV6_PMCR_CCOUNT_RESET         (1 << 2)
#define ARMV6_PMCR_CCOUNT_DIV           (1 << 3)
#define ARMV6_PMCR_COUNT0_IEN           (1 << 4)
#define ARMV6_PMCR_COUNT1_IEN           (1 << 5)
#define ARMV6_PMCR_CCOUNT_IEN           (1 << 6)
#define ARMV6_PMCR_COUNT0_OVERFLOW      (1 << 8)
#define ARMV6_PMCR_COUNT1_OVERFLOW      (1 << 9)
#define ARMV6_PMCR_CCOUNT_OVERFLOW      (1 << 10)
#define ARMV6_PMCR_EVT_COUNT0_SHIFT     20
#define ARMV6_PMCR_EVT_COUNT0_MASK      (0xFF << ARMV6_PMCR_EVT_COUNT0_SHIFT)
#define ARMV6_PMCR_EVT_COUNT1_SHIFT     12
#define ARMV6_PMCR_EVT_COUNT1_MASK      (0xFF << ARMV6_PMCR_EVT_COUNT1_SHIFT)

#define ARMV6_PMCR_OVERFLOWED_MASK \
	(ARMV6_PMCR_COUNT0_OVERFLOW | ARMV6_PMCR_COUNT1_OVERFLOW | \
	ARMV6_PMCR_CCOUNT_OVERFLOW)

enum armv6_counters {
	ARMV6_COUNTER0 = 0,
	ARMV6_COUNTER1,
	ARMV6_CYCLE_COUNTER,
};

static inline unsigned long armv6_pmcr_read(void)
{
	u32 val;

	asm volatile ("mrc   p15, 0, %0, c15, c12, 0":"=r" (val));
	return val;
}

static inline void armv6_pmcr_write(unsigned long val)
{
	asm volatile ("mcr   p15, 0, %0, c15, c12, 0"::"r" (val));
}

static inline unsigned int armv6_pmu_read_count(unsigned int idx)
{
	unsigned long value = 0;

	if (idx == ARMV6_CYCLE_COUNTER)
		asm volatile ("mrc   p15, 0, %0, c15, c12, 1":"=r" (value));
	else if (idx == ARMV6_COUNTER0)
		asm volatile ("mrc   p15, 0, %0, c15, c12, 2":"=r" (value));
	else if (idx == ARMV6_COUNTER1)
		asm volatile ("mrc   p15, 0, %0, c15, c12, 3":"=r" (value));

	return value;
}

static inline void armv6_pmu_overflow(void)
{
	unsigned int val;

	val = armv6_pmcr_read();
	val |= ARMV6_PMCR_OVERFLOWED_MASK;
	armv6_pmcr_write(val);
}

static inline unsigned int armv6_pmu_control_read(void)
{
	u32 val;

	asm volatile ("mrc   p15, 0, %0, c15, c12, 0":"=r" (val));
	return val;
}

static inline void armv6_pmu_control_write(unsigned int setting)
{
	unsigned long val;

	val = armv6_pmcr_read();
	val |= setting;
	armv6_pmcr_write(val);
}

static void armv6_pmu_hw_reset_all(void)
{
	unsigned long val;

	val = armv6_pmcr_read();
	val &= ~ARMV6_PMCR_ENABLE;	/* disable all counters */
	val |= (ARMV6_PMCR_CTR01_RESET | ARMV6_PMCR_CCOUNT_RESET);	/* reset CCNT, PMNC1/2 counter to zero */
	armv6_pmcr_write(val);

	armv6_pmu_overflow();
}

static void armv6pmu_enable_event(int idx, unsigned short config)
{
	unsigned long val, mask, evt;

	if (idx == ARMV6_CYCLE_COUNTER) {
		mask = 0;
		evt = ARMV6_PMCR_CCOUNT_IEN;
	} else if (idx == ARMV6_COUNTER0) {
		mask = ARMV6_PMCR_EVT_COUNT0_MASK;
		evt = (config << ARMV6_PMCR_EVT_COUNT0_SHIFT) | ARMV6_PMCR_COUNT0_IEN;
	} else if (idx == ARMV6_COUNTER1) {
		mask = ARMV6_PMCR_EVT_COUNT1_MASK;
		evt = (config << ARMV6_PMCR_EVT_COUNT1_SHIFT) | ARMV6_PMCR_COUNT1_IEN;
	} else {
		pr_debug("invalid counter number (%d)\n", idx);
		return;
	}

	/*
	 * Mask out the current event and set the counter to count the event
	 * that we're interested in.
	 */
	val = armv6_pmcr_read();
	val &= ~mask;
	val |= evt;
	armv6_pmcr_write(val);
}

/***********************************
 *      MET ARM v6 operations      *
 ***********************************/
enum ARM_TYPE {
	ARM1136 = 0xB36,
	ARM1156 = 0xB56,
	ARM1176 = 0xB76,
	CHIP_UNKNOWN = 0xFFF
};

struct chip_pmu {
	enum ARM_TYPE type;
};

static struct chip_pmu chips[] = {
	{ARM1136},
	{ARM1156},
	{ARM1176},
};

static int armv6_pmu_hw_check_event(struct met_pmu *pmu, int idx, int event)
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

static void armv6_pmu_hw_start(struct met_pmu *pmu, int count)
{
	int i;
	int generic = count - 1;

	armv6_pmu_hw_reset_all();

	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING)
			armv6pmu_enable_event(i, pmu[i].event);
	}

	if (pmu[count - 1].mode == MODE_POLLING)
		armv6pmu_enable_event(2, pmu[2].event);

	armv6_pmu_control_write(ARMV6_PMCR_ENABLE);
}

static void armv6_pmu_hw_stop(int count)
{
	armv6_pmu_hw_reset_all();
}

static unsigned int armv6_pmu_hw_polling(struct met_pmu *pmu, int count, unsigned int *pmu_value)
{
	int i, cnt = 0;
	int generic = count - 1;

	for (i = 0; i < generic; i++) {
		if (pmu[i].mode == MODE_POLLING) {
			pmu_value[cnt] = armv6_pmu_read_count(i);
			cnt++;
		}
	}

	if (pmu[count - 1].mode == MODE_POLLING) {
		pmu_value[cnt] = armv6_pmu_read_count(2);
		cnt++;
	}

	armv6_pmu_control_write(ARMV6_PMCR_ENABLE | ARMV6_PMCR_CTR01_RESET |
				ARMV6_PMCR_CCOUNT_RESET);

	return cnt;
}

struct cpu_pmu_hw armv6_pmu = {
	.name = "armv6_pmu",
	.check_event = armv6_pmu_hw_check_event,
	.start = armv6_pmu_hw_start,
	.stop = armv6_pmu_hw_stop,
	.polling = armv6_pmu_hw_polling,
};

struct cpu_pmu_hw *v6_cpu_pmu_hw_init(int typeid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(chips); i++)
		if (chips[i].type == typeid)
			break;

	if (i == ARRAY_SIZE(chips))
		return NULL;

	armv6_pmu.nr_cnt = 3;

	return &armv6_pmu;
}
