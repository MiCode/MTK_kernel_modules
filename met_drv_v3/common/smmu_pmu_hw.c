// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <asm/cpu.h>
#include "met_kernel_symbol.h"
#include "smmu_pmu.h"
#include "mtk-smmu-v3.h"

#define CLUSTERPMCR_N_SHIFT		11
#define CLUSTERPMCR_N_MASK		0x1f

#if 0
#define DBG(__fmt__, ...)
#else
#undef TAG
#define TAG "[MET_SMMU_PMU]"
#define DBG(__fmt__, ...) \
	do{\
		pr_info(TAG"[%s][%d]" __fmt__, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#endif

static int smmu_lmu_hw_check_event(struct met_smmu_lmu *pmu, int idx, int event);

static struct met_smmu_lmu lmu_pmus[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];

struct smmu_lmu_hw smmuv3_lmu[SMMU_TYPE_NUM] = {
	{
		.name = "mm_smmu_lmu",
		.check_event = smmu_lmu_hw_check_event,
		.smmu_id = MM_SMMU,
	},
	{
		.name = "apu_smmu_lmu",
		.check_event = smmu_lmu_hw_check_event,
		.smmu_id = APU_SMMU,
	},
	{
		.name = "soc_smmu_lmu",
		.check_event = smmu_lmu_hw_check_event,
		.smmu_id = SOC_SMMU,
	},
	{
		.name = "gpu_smmu_lmu",
		.check_event = smmu_lmu_hw_check_event,
		.smmu_id = GPU_SMMU,
	},
};

static inline int get_smmu_hw_id(struct met_smmu_lmu *pmu)
{
	int i;

	if (!pmu)
		return -1;

	for (i = 0; i < SMMU_TYPE_NUM; i++) {
		if (smmuv3_lmu[i].pmu && smmuv3_lmu[i].pmu == pmu)
			break;
	}

	if (i == SMMU_TYPE_NUM)
		return -1;

	return smmuv3_lmu[i].smmu_id;
}

static int smmu_lmu_hw_check_event(struct met_smmu_lmu *pmu, int idx, int event)
{
	int smmu_id, i;

	smmu_id = get_smmu_hw_id(pmu);
	if (smmu_id < 0 || smmu_id >= SMMU_TYPE_NUM)
		return -1;

	/* Check if event is duplicate */
	for (i = 0; i < idx; i++) {
		if (pmu[i].event == event)
			break;
	}
	if (i < idx) {
		DBG("++++++ found duplicate event 0x%02x i=%d smmu_id=%d\n",
		    event, i, smmu_id);
		return -1;
	}
	return 0;
}

static void set_smmu_lmu_event_count(u32 smmu_id)
{
	/* Set to defaut max value */
	smmuv3_lmu[smmu_id].event_count = MXNR_SMMU_LMU_EVENTS;
}

static void init_smmu_lmu(u32 smmu_id)
{
	set_smmu_lmu_event_count(smmu_id);
}

struct smmu_lmu_hw *smmu_lmu_hw_init(u32 smmu_id)
{

	init_smmu_lmu(smmu_id);
	smmuv3_lmu[smmu_id].pmu = lmu_pmus[smmu_id];
	DBG("name=%s\n", smmuv3_lmu[smmu_id].name);
	return &smmuv3_lmu[smmu_id];
}
