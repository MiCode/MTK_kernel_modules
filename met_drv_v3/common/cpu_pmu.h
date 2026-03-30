/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CPU_PMU_H_
#define _CPU_PMU_H_

#include <linux/device.h>
#include <linux/perf_event.h>

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
#include <linux/platform_device.h>
#include <linux/perf/arm_pmu.h>
#endif

#define MODE_DISABLED	0
#define MODE_INTERRUPT	1
#define MODE_POLLING	2

#define PMU_INIT_SUCC             0
#define PMU_INIT_FAIL_OCCUPIED    1
#define PMU_INIT_FAIL_CPU_OFFLINE 2

#define MXSIZE_PMU_DESC 32
#define MX_CPU_CLUSTER 8

/* max number of pmu counter for armv9 is 20+1 */
#define	MXNR_PMU_EVENTS          22
/* a roughly large enough size for pmu events buffers,       */
/* if an input length is rediculously too many, we drop them */
#define MXNR_PMU_EVENT_BUFFER_SZ ((MXNR_PMU_EVENTS) + 16)
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
	unsigned long (*perf_event_get_evttype)(struct perf_event *ev);
	u32 (*pmu_read_clear_overflow_flag)(void);
	void (*write_counter)(unsigned int idx,
			      unsigned int val, int is_cyc_cnt);
	void (*disable_intr)(unsigned int idx);
	void (*disable_cyc_intr)(void);
	struct met_pmu *pmu[NR_CPUS];
	int event_count[NR_CPUS];
	/*
	 * used for compensation of pmu counter loss
	 * between end of polling and start of cpu pm
	 */
	unsigned int cpu_pm_unpolled_loss[NR_CPUS][MXNR_PMU_EVENTS];
};

struct armpmu_handle_irq {
	cpumask_t supported_cpus;
	irqreturn_t (*handle_irq_orig)(struct arm_pmu *pmu);
};

struct pmu_failed_desc {
	unsigned int event;
	unsigned char init_failed;
};

struct pmu_desc {
	unsigned int event;
	char name[MXSIZE_PMU_DESC];
};

typedef enum {
	SET_PMU_EVT_CNT = 0x0,
	SET_PMU_CYCCNT_ENABLE = 0x1,
	SET_PMU_BASE_OFFSET = 0x02,
	SET_PMU_POLLING_BITMAP = 0x03
} PMU_IPI_Type;

struct cpu_pmu_hw *cpu_pmu_hw_init(void);

extern struct cpu_pmu_hw *cpu_pmu;
extern noinline void mp_cpu(unsigned char cnt, unsigned int *value);

extern int met_perf_cpupmu_status;
extern void met_perf_cpupmu_polling(unsigned long long stamp, int cpu);

#endif	/* _CPU_PMU_H_ */
