/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __V6_PMU_HW_H__
#define __V6_PMU_HW_H__

extern struct cpu_pmu_hw armv6_pmu;
extern struct cpu_pmu_hw *v6_cpu_pmu_hw_init(int typeid);

#endif
