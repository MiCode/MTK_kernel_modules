/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _SMMU_PMU_H_
#define _SMMU_PMU_H_

#include <linux/device.h>

#define MODE_DISABLED	0
#define MODE_INTERRUPT	1
#define MODE_POLLING	2

#define	MXNR_SMMU_LMU_EVENTS	16
#define MXNR_SMMU_EVENT_BUFFER_SZ ((MXNR_SMMU_LMU_EVENTS) + 0)

/* event id list from smmu lmu */
#define EVENT_R_LAT_MAX			0
#define EVENT_R_LAT_MAX_AXID		1
#define EVENT_R_LAT_MAX_PEND_BY_EMI	2
#define EVENT_R_LAT_TOT			3
#define EVENT_R_TRANS_TOT		4
#define EVENT_R_LAT_AVG			5
#define EVENT_R_OOS_TRANS_TOT		6
#define EVENT_W_LAT_MAX			7
#define EVENT_W_LAT_MAX_AXID		8
#define EVENT_W_LAT_MAX_PEND_BY_EMI	9
#define EVENT_W_LAT_TOT			10
#define EVENT_W_TRNAS_TOT		11
#define EVENT_W_LAT_AVG			12
#define EVENT_W_OOS_TRANS_TOT		13
#define EVENT_TBUS_TRANS_TOT		14
#define EVENT_TBUS_LAT_AVG		15

/* local event list */
#define EVENT_TLB_MISS_RATE		0

#define SMMU_LMU_INIT_SUCC		0
#define SMMU_LMU_INIT_FAIL_OCCUPIED	1

struct smmu_lmu_failed_desc {
	unsigned int event;
	unsigned char init_failed;
};

struct met_smmu_lmu {
	unsigned char mode;
	unsigned short event;
	unsigned long freq;
	struct kobject *kobj_smmu_lmu;
};

struct smmu_lmu_hw {
	const char *name;
	int nr_cnt;
	int (*check_event)(struct met_smmu_lmu *pmu, int idx, int event);
	void (*start)(struct met_smmu_lmu *pmu, int count);
	void (*stop)(int count);
	unsigned int (*polling)(struct met_smmu_lmu *pmu, int count, unsigned int *pmu_value);
	struct met_smmu_lmu *pmu;
	int event_count;
	int smmu_id;
};

struct smmu_mp_func {
	void (*smmu_latency)(unsigned int event_id, unsigned int value);
	void (*smmu_tbu_transaction)(unsigned int event_id, unsigned int value);
	void (*smmu_tcu_transaction)(unsigned int event_id, unsigned int value);
	void (*smmu_hit_miss_rate)(unsigned int event_id, unsigned int value);
};

struct smmu_lmu_hw *smmu_lmu_hw_init(u32 smmu_id);


#endif	/* _SMMU_PMU_H_ */
