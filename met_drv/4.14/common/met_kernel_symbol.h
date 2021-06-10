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

#ifndef MET_KERNEL_SYMBOL
#define MET_KERNEL_SYMBOL

/*lookup symbol*/
#include <asm/cpu.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/kthread.h>

#if	defined(CONFIG_MET_ARM_32BIT)
extern void met_get_cpuinfo(int cpu, struct cpuinfo_arm **cpuinfo);
extern void (*met_get_cpuinfo_symbol)(int cpu, struct cpuinfo_arm **cpuinfo);
#else
extern void met_get_cpuinfo(int cpu, struct cpuinfo_arm64 **cpuinfo);
extern void (*met_get_cpuinfo_symbol)(int cpu, struct cpuinfo_arm64 **cpuinfo);
#endif

extern void (*tracing_record_cmdline_symbol)(struct task_struct *tsk);
extern void met_cpu_frequency(unsigned int frequency, unsigned int cpu_id);
extern void (*met_cpu_frequency_symbol)(unsigned int frequency, unsigned int cpu_id);
extern void (*met_arch_setup_dma_ops_symbol)(struct device *dev);
extern int (*met_perf_event_read_local_symbol)(struct perf_event *ev, u64 *value);
extern struct task_struct *(*met_kthread_create_on_cpu_symbol)(int (*threadfn)(void *data),
				void *data, unsigned int cpu,
				const char *namefmt);
extern int (*met_smp_call_function_single_symbol)(int cpu, smp_call_func_t func, void *info, int wait);

extern void met_tracing_record_cmdline(struct task_struct *tsk);
extern int met_reg_switch(void);
extern int (*met_reg_switch_symbol)(void);
extern void met_unreg_switch(void);
extern void (*met_unreg_switch_symbol)(void);
#ifdef MET_EVENT_POWER_SUPPORT
extern int met_reg_event_power(void);
extern int (*met_reg_event_power_symbol)(void);
extern void met_unreg_event_power(void);
extern void (*met_unreg_event_power_symbol)(void);
#endif
extern void met_arch_setup_dma_ops(struct device *dev);
extern int met_perf_event_read_local(struct perf_event *ev, u64 *value);
extern struct task_struct *met_kthread_create_on_cpu(int (*threadfn)(void *data),
				void *data, unsigned int cpu,
				const char *namefmt);
extern int met_smp_call_function_single(int cpu, smp_call_func_t func, void *info, int wait);
extern void met_arch_send_call_function_single_ipi(int cpu);


#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#endif
#ifdef SSPM_VERSION_V2
extern struct mtk_ipi_device sspm_ipidev;
extern struct mtk_ipi_device *sspm_ipidev_symbol;
#endif
#endif

#endif	/* MET_KERNEL_SYMBOL */
