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


#ifndef __V6_PMU_HW_H__
#define __V6_PMU_HW_H__

extern struct cpu_pmu_hw armv6_pmu;
extern struct cpu_pmu_hw *v6_cpu_pmu_hw_init(int typeid);

#endif
