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

/* include <asm/percpu.h> */
#include <trace/events/sched.h>
#include <linux/module.h>
#include <trace/events/irq.h>
#include <trace/events/power.h>

#include "interface.h"
#include "met_drv.h"
#include "cpu_pmu.h"
#include "switch.h"
#include "sampler.h"
#include "met_kernel_symbol.h"
/* #include "trace.h" */

/*
 * IRQ_TIRGGER and CPU_IDLE_TRIGGER
 */
/* #define IRQ_TRIGGER */
/* #define CPU_IDLE_TRIGGER */

static DEFINE_PER_CPU(unsigned int, first_log);

#ifdef __aarch64__
/* #include <asm/compat.h> */
#include <linux/compat.h>
#endif

noinline void mt_switch(struct task_struct *prev, struct task_struct *next)
{
	int cpu;
	int prev_state = 0, next_state = 0;

#ifdef __aarch64__
	prev_state = !(is_compat_thread(task_thread_info(prev)));
	next_state = !(is_compat_thread(task_thread_info(next)));
#endif

	cpu = smp_processor_id();
	if (per_cpu(first_log, cpu)) {
		MET_TRACE("%d, %d, %d, %d\n", prev->pid, prev_state, next->pid, next_state);
		per_cpu(first_log, cpu) = 0;
	}
	else if (prev_state != next_state)
		MET_TRACE("%d, %d, %d, %d\n", prev->pid, prev_state, next->pid, next_state);
}


#if 0 /* move to kernel space */
MET_DEFINE_PROBE(sched_switch,
		 TP_PROTO(bool preempt, struct task_struct *prev, struct task_struct *next))
{
	/* speedup sched_switch callback handle */
	if (met_switch.mode == 0)
		return;

	if (met_switch.mode & MT_SWITCH_EVENT_TIMER)
		met_event_timer_notify();

	if (met_switch.mode & MT_SWITCH_64_32BIT)
		mt_switch(prev, next);

	if (met_switch.mode & MT_SWITCH_SCHEDSWITCH) {
		if (get_pmu_profiling_version() == 1)
			cpupmu_polling(0, smp_processor_id());
#ifdef MET_SUPPORT_CPUPMU_V2
		else if (get_pmu_profiling_version() == 2)
			cpupmu_polling_v2(0, smp_processor_id());
#endif
	}
}
#endif

void met_sched_switch(struct task_struct *prev, struct task_struct *next)
{
	/* speedup sched_switch callback handle */
	if (met_switch.mode == 0)
		return;

	if (met_switch.mode & MT_SWITCH_EVENT_TIMER)
		met_event_timer_notify();

	if (met_switch.mode & MT_SWITCH_64_32BIT)
		mt_switch(prev, next);

	/* met_perf_cpupmu_status: 0: stop, others: polling */
	if ((met_switch.mode & MT_SWITCH_SCHEDSWITCH) && met_perf_cpupmu_status)
		met_perf_cpupmu_polling(0, smp_processor_id());
}

#ifdef IRQ_TRIGGER
MET_DEFINE_PROBE(irq_handler_entry, TP_PROTO(int irq, struct irqaction *action))
{
	if (met_switch.mode & MT_SWITCH_EVENT_TIMER) {
		met_event_timer_notify();
		return;
	}
}
#endif

#ifdef CPU_IDLE_TRIGGER
MET_DEFINE_PROBE(cpu_idle, TP_PROTO(unsigned int state, unsigned int cpu_id))
{
	if (met_switch.mode & MT_SWITCH_EVENT_TIMER) {
		met_event_timer_notify();
		return;
	}
}
#endif

#ifdef MET_ANYTIME
/*
 * create related subfs file node
 */

static ssize_t default_on_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1\n");
}

static struct kobj_attribute default_on_attr = __ATTR(default_on, 0664, default_on_show, NULL);
static struct kobject *kobj_cpu;
#endif

static int met_switch_create_subfs(struct kobject *parent)
{
	int ret = 0;

	/* register tracepoints */
#if 0
	if (MET_REGISTER_TRACE(sched_switch)) {
		pr_debug("can not register callback of sched_switch\n");
		return -ENODEV;
	}
#else
	if (met_reg_switch_symbol)
		ret = met_reg_switch_symbol();
#endif
#ifdef CPU_IDLE_TRIGGER
	if (MET_REGISTER_TRACE(cpu_idle)) {
		pr_debug("can not register callback of irq_handler_entry\n");
		return -ENODEV;
	}
#endif
#ifdef IRQ_TRIGGER
	if (MET_REGISTER_TRACE(irq_handler_entry)) {
		pr_debug("can not register callback of irq_handler_entry\n");
		return -ENODEV;
	}
#endif

#ifdef MET_ANYTIME
	/*
	 * to create default_on file node
	 * let user space can know we can support MET default on
	 */
	kobj_cpu = parent;
	ret = sysfs_create_file(kobj_cpu, &default_on_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create default_on in sysfs\n");
		return -1;
	}
#endif

	return ret;
}


static void met_switch_delete_subfs(void)
{
#ifdef MET_ANYTIME
	if (kobj_cpu != NULL) {
		sysfs_remove_file(kobj_cpu, &default_on_attr.attr);
		kobj_cpu = NULL;
	}
#endif
#ifdef IRQ_TRIGGER
	MET_UNREGISTER_TRACE(irq_handler_entry);
#endif
#ifdef CPU_IDLE_TRIGGER
	MET_UNREGISTER_TRACE(cpu_idle);
#endif
#if 0
	MET_UNREGISTER_TRACE(sched_switch);
#else
	if (met_unreg_switch_symbol)
		met_unreg_switch_symbol();
#endif

}


static void (*cpu_timed_polling)(unsigned long long stamp, int cpu);
/* static void (*cpu_tagged_polling)(unsigned long long stamp, int cpu); */

static void met_switch_start(void)
{
	int cpu;

	if (met_switch.mode & MT_SWITCH_SCHEDSWITCH) {
		cpu_timed_polling = met_cpupmu.timed_polling;
		/* cpu_tagged_polling = met_cpupmu.tagged_polling; */
		met_cpupmu.timed_polling = NULL;
		/* met_cpupmu.tagged_polling = NULL; */
	}

	for_each_possible_cpu(cpu) {
		per_cpu(first_log, cpu) = 1;
	}

}


static void met_switch_stop(void)
{
	int cpu;

	if (met_switch.mode & MT_SWITCH_SCHEDSWITCH) {
		met_cpupmu.timed_polling = cpu_timed_polling;
		/* met_cpupmu.tagged_polling = cpu_tagged_polling; */
	}

	for_each_possible_cpu(cpu) {
		per_cpu(first_log, cpu) = 0;
	}

}


static int met_switch_process_argument(const char *arg, int len)
{
	unsigned int value;
	/*ex: mxitem is 0x0005, max value should be (5-1) + (5-2) = 0x100 + 0x11 = 7 */
	unsigned int max_value = ((MT_SWITCH_MX_ITEM * 2) - 3);


	if (met_parse_num(arg, &value, len) < 0)
		goto arg_switch_exit;

	if ((value < 1) || (value > max_value))
		goto arg_switch_exit;

	met_switch.mode = value;
	return 0;

arg_switch_exit:
	met_switch.mode = 0;
	return -EINVAL;
}

static const char header[] =
	"met-info [000] 0.0: met_switch_header: prev_pid,prev_state,next_pid,next_state\n";

static const char help[] =
"  --switch=mode                         mode:0x1 - output CPUPMU whenever sched_switch\n"
"                                        mode:0x2 - output Aarch 32/64 state whenever state changed (no CPUPMU)\n"
"                                        mode:0x4 - force output count at tag_start/tag_end\n"
"                                        mode:0x8 - task switch timer\n"
"                                        mode:0xF - mode 0x1 + 0x2 + 04 + 08\n";

static int met_switch_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int met_switch_print_header(char *buf, int len)
{
	int ret = 0;

	ret =
	    snprintf(buf, PAGE_SIZE, "met-info [000] 0.0: mp_cpu_switch_base: %d\n",
		     met_switch.mode);
	if (met_switch.mode & MT_SWITCH_64_32BIT)
		ret += snprintf(buf + ret, PAGE_SIZE, header);

	return ret;
}


struct metdevice met_switch = {
	.name = "switch",
	.type = MET_TYPE_PMU,
	.create_subfs = met_switch_create_subfs,
	.delete_subfs = met_switch_delete_subfs,
	.start = met_switch_start,
	.stop = met_switch_stop,
	.process_argument = met_switch_process_argument,
	.print_help = met_switch_print_help,
	.print_header = met_switch_print_header,
};
