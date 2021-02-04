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

#ifndef _CPU_PMU_H_
#define _CPU_PMU_H_

#include <linux/device.h>

#define MODE_DISABLED	0
#define MODE_INTERRUPT	1
#define MODE_POLLING	2

#define MXSIZE_PMU_DESC 32
#define MXNR_CPU	NR_CPUS

#define	MXNR_PMU_EVENTS	8	/* max number of pmu counter for armv8 is 6+1 */
struct met_pmu {
	unsigned char mode;
	unsigned short event;
	unsigned long freq;
	struct kobject *kobj_cpu_pmu;
};

struct cpu_pmu_hw {
	const char *name;
	const char *cpu_name;
	int nr_cnt;
	int (*get_event_desc)(int idx, int event, char *event_desc);
	int (*check_event)(struct met_pmu *pmu, int idx, int event);
	void (*start)(struct met_pmu *pmu, int count);
	void (*stop)(int count);
	unsigned int (*polling)(struct met_pmu *pmu, int count, unsigned int *pmu_value);
	struct met_pmu *pmu[MXNR_CPU];
	int event_count[MXNR_CPU];
	/*
	 * used for compensation of pmu counter loss
	 * between end of polling and start of cpu pm
	 */
	unsigned int cpu_pm_unpolled_loss[MXNR_CPU][MXNR_PMU_EVENTS];
};

struct pmu_desc {
	unsigned int event;
	char name[MXSIZE_PMU_DESC];
};

struct cpu_pmu_hw *cpu_pmu_hw_init(void);

extern struct cpu_pmu_hw *cpu_pmu;
extern noinline void mp_cpu(unsigned char cnt, unsigned int *value);

extern int met_perf_cpupmu_status;
extern void met_perf_cpupmu_polling(unsigned long long stamp, int cpu);

#endif	/* _CPU_PMU_H_ */
