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

#ifndef _CPU_DSU_H_
#define _CPU_DSU_H_

#include <linux/device.h>

#define MODE_DISABLED	0
#define MODE_INTERRUPT	1
#define MODE_POLLING	2

#define MXNR_CPU	NR_CPUS

#define	MXNR_DSU_EVENTS	8	/* max number of pmu counter for armv8 is 6+1 */
struct met_dsu {
	unsigned char mode;
	unsigned short event;
	unsigned long freq;
	struct kobject *kobj_cpu_dsu;
};

struct cpu_dsu_hw {
	const char *name;
	int nr_cnt;
	int (*check_event)(struct met_dsu *pmu, int idx, int event);
	void (*start)(struct met_dsu *pmu, int count);
	void (*stop)(int count);
	unsigned int (*polling)(struct met_dsu *pmu, int count, unsigned int *pmu_value);
	struct met_dsu *pmu;
	int event_count;
};


struct cpu_dsu_hw *cpu_dsu_hw_init(void);



#endif	/* _CPU_DSU_H_ */
