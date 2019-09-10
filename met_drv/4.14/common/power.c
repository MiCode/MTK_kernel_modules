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

#include <linux/cpufreq.h>
#include <trace/events/power.h>

#include "power.h"
#include "met_drv.h"
#include "met_kernel_symbol.h"

noinline void cpu_frequency(unsigned int frequency, unsigned int cpu_id)
{
	/* suppose this symbol is available, otherwise, the met.ko will fail */
	met_cpu_frequency_symbol(frequency, cpu_id);
}

void force_power_log(int cpu)
{
	struct cpufreq_policy *p;

	if (cpu == POWER_LOG_ALL) {
		for_each_possible_cpu(cpu) {
			p = cpufreq_cpu_get(cpu);
			if (p != NULL) {
				cpu_frequency(p->cur, cpu);
				cpufreq_cpu_put(p);
			} else {
				cpu_frequency(0, cpu);
			}
		}
	} else {
		p = cpufreq_cpu_get(cpu);
		if (p != NULL) {
			cpu_frequency(p->cur, cpu);
			cpufreq_cpu_put(p);
		} else {
			cpu_frequency(0, cpu);
		}
	}
}

void force_power_log_val(unsigned int frequency, int cpu)
{
	cpu_frequency(frequency, cpu);
}
