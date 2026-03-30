// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
#include "str_util.h"

/* #include "trace.h" */
#include "mtk_typedefs.h"

/*
 * IRQ_TIRGGER and CPU_IDLE_TRIGGER
 */
/* #define IRQ_TRIGGER */
/* #define CPU_IDLE_TRIGGER */

static unsigned int __percpu *first_log;
static int __percpu *per_cpu_last_need_polling_pid;

#define SWITCH_MODE_LENGTH 5
#define KWORKER_STR_LENGTH 7


static int met_switch_get_switch_mode_arg(const char *input_str, const char *delim_ptr, char *switch_mode_arg);
static void met_switch_get_process_filter_arg(const char *input_str);
static int check_if_need_polling(struct task_struct *prev, struct task_struct *next);


#ifdef __aarch64__
/* #include <asm/compat.h> */
#include <linux/compat.h>
#endif

struct met_str_array *process_filter_list;
static const char kworker_str[]="kworker";
int process_filter_is_empty =1;


noinline void mt_switch(struct task_struct *prev, struct task_struct *next)
{
	int cpu;
	int prev_state = 0, next_state = 0;

#ifdef __aarch64__
	prev_state = !(is_compat_thread(task_thread_info(prev)));
	next_state = !(is_compat_thread(task_thread_info(next)));
#endif

	cpu = smp_processor_id();
	if (*per_cpu_ptr(first_log, cpu)) {
		MET_TRACE("%d, %d, %d, %d\n", prev->pid, prev_state, next->pid, next_state);
		*per_cpu_ptr(first_log, cpu) = 0;
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

void met_sched_switch(void *data, bool preempt, struct task_struct *prev, struct task_struct *next)
{
	/* speedup sched_switch callback handle */
	if (met_switch.mode == 0)
		return;

	if (met_switch.mode & MT_SWITCH_EVENT_TIMER)
		met_event_timer_notify();

	if (met_switch.mode & MT_SWITCH_64_32BIT)
		mt_switch(prev, next);

	/* met_perf_cpupmu_status: 0: stop, others: polling */
	if ((met_switch.mode & MT_SWITCH_SCHEDSWITCH) && met_perf_cpupmu_status){
		if(process_filter_is_empty || check_if_need_polling(prev, next)){
			met_perf_cpupmu_polling(0, smp_processor_id());
		}
	}

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
	return SNPRINTF(buf, PAGE_SIZE, "1\n");
}

static struct kobj_attribute default_on_attr = __ATTR(default_on, 0664, default_on_show, NULL);
static struct kobject *kobj_cpu;
#endif

static int met_switch_create_subfs(struct kobject *parent)
{
	int ret = 0;

	first_log = alloc_percpu(typeof(*first_log));
	if (!first_log) {
		PR_BOOTMSG("percpu first_log allocate fail\n");
		pr_debug("percpu first_log allocate fail\n");
		return 0;
	}

	per_cpu_last_need_polling_pid = alloc_percpu(typeof(*per_cpu_last_need_polling_pid));
	if (!per_cpu_last_need_polling_pid) {
		PR_BOOTMSG("percpu per_cpu_last_need_polling_pid allocate fail\n");
		pr_debug("percpu per_cpu_last_need_polling_pid allocate fail\n");
		return 0;
	}

	/* register tracepoints */
	ret = met_tracepoint_probe_reg("sched_switch", met_sched_switch);

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
	if (first_log) {
		free_percpu(first_log);
	}

	if(per_cpu_last_need_polling_pid) {
		free_percpu(per_cpu_last_need_polling_pid);
	}

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
}


static void (*cpu_timed_polling)(unsigned long long stamp, int cpu);
/* static void (*cpu_tagged_polling)(unsigned long long stamp, int cpu); */

static void met_switch_start(void)
{
	int cpu;

	if (!first_log) {
		MET_TRACE("percpu first_log allocate fail\n");
		met_switch.mode = 0;
		return;
	}

	if(!per_cpu_last_need_polling_pid){
		MET_TRACE("percpu per_cpu_last_need_polling_pid allocate fail\n");
		met_switch.mode = 0;
		return;
	}

	if (met_switch.mode & MT_SWITCH_SCHEDSWITCH) {
		cpu_timed_polling = met_cpupmu.timed_polling;
		/* cpu_tagged_polling = met_cpupmu.tagged_polling; */
		met_cpupmu.timed_polling = NULL;
		/* met_cpupmu.tagged_polling = NULL; */
	}

	for_each_possible_cpu(cpu) {
		if(cpu<0 || cpu>=NR_CPUS)
			continue;

		*per_cpu_ptr(first_log, cpu) = 1;
		*per_cpu_ptr(per_cpu_last_need_polling_pid, cpu)=-1;
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
		if(cpu<0 || cpu>=NR_CPUS)
			continue;

		*per_cpu_ptr(first_log, cpu) = 0;
		*per_cpu_ptr(per_cpu_last_need_polling_pid, cpu)=-1;
	}

}

static int met_switch_process_argument(const char *arg, int len)
{
	unsigned int value = 0;
	/*ex: mxitem is 0x0005, max value should be (5-1) + (5-2) = 0x100 + 0x11 = 7 */
	unsigned int max_value = ((MT_SWITCH_MX_ITEM * 2) - 3);
	char switch_mode_arg[SWITCH_MODE_LENGTH+1];
	char *delim_ptr = NULL;

	pr_info("[met_switch_process_argument]arg=%s, len=%d\n", arg, len);

	/*reset variable*/
	met_util_str_array_clean(process_filter_list);
	process_filter_list = NULL;
	process_filter_is_empty = 1;

	if (!first_log)
		return 0;
	/*
		input argu may have following types:
		1. --switch=3 (original)
		2. --switch=17:mali-cmar-backe,mali-utility-wo,mali-mem-purge
		3. --switch=17
	*/
	delim_ptr = strchr(arg, ':');

	if(delim_ptr != NULL){
		/*get [switch mode] part from arg*/
		if(met_switch_get_switch_mode_arg(arg, delim_ptr, switch_mode_arg)<=0){
			goto arg_switch_exit;
		}
		arg =  switch_mode_arg;

		/*get [process_filter] part from arg*/
		met_switch_get_process_filter_arg(delim_ptr+1);
	}

	if (met_parse_num(arg, &value, len) < 0)
		goto arg_switch_exit;

	if ((value < 1) || (value > max_value))
		goto arg_switch_exit;

	met_switch.mode = value;

	if((met_switch.mode & MT_SWITCH_GPU_DDK_OVERHEAD) && process_filter_is_empty){
		process_filter_is_empty = 0;
	}
		
	return 0;

arg_switch_exit:
	met_switch.mode = 0;
	return -EINVAL;
}

static int met_switch_get_switch_mode_arg(const char *input_str, const char *delim_ptr, char *switch_mode_arg){

	int switch_mode_arg_length = delim_ptr - input_str;

	if(switch_mode_arg_length > SWITCH_MODE_LENGTH){
		pr_info("[met_switch_get_switch_mode_arg]Error: invalid switch mode number=%s\n", input_str);
		return -1;
	}

	strncpy(switch_mode_arg, input_str, switch_mode_arg_length);
	*(switch_mode_arg+switch_mode_arg_length) ='\0';
	pr_info("[met_switch_get_switch_mode_arg]switch_mode_arg=%s\n", switch_mode_arg);

	return switch_mode_arg_length;
}


static void met_switch_get_process_filter_arg(const char *input_str){

	/*use str array*/
	process_filter_list = met_util_str_split(input_str, ',');
	if(process_filter_list==NULL){
		process_filter_is_empty=1;
	}else{
		process_filter_is_empty = process_filter_list->str_ptr_array_length==0?1:0;
	}
}


static int check_if_need_polling(struct task_struct *prev, struct task_struct *next){

	int prev_is_need =0;
	int next_is_need = 0;

	int cpu = smp_processor_id();

	if(*per_cpu_ptr(per_cpu_last_need_polling_pid, cpu) == prev->pid ){
		prev_is_need = 1;
	}

	next_is_need = (strncmp(next->comm, kworker_str, KWORKER_STR_LENGTH)==0) || met_util_in_str_array(next->comm, 0, process_filter_list);
	if(next_is_need){
		*per_cpu_ptr(per_cpu_last_need_polling_pid, cpu) =  next->pid;
	}else{
		*per_cpu_ptr(per_cpu_last_need_polling_pid, cpu)  =  -1;
	}

	return (prev_is_need || next_is_need);
}


static const char header[] =
	"met-info [000] 0.0: met_switch_header: prev_pid,prev_state,next_pid,next_state\n";

static const char help[] =
"  --switch=mode                         mode:0x1  - output CPUPMU whenever sched_switch\n"
"                                        mode:0x2  - output Aarch 32/64 state whenever state changed (no CPUPMU)\n"
"                                        mode:0x4  - force output count at tag_start/tag_end\n"
"                                        mode:0x8  - task switch timer\n"
"                                        mode:0xF  - mode 0x1 + 0x2 + 04 + 08\n"
"                                        mode:0x10 - enable filter mechanism, only specific process will dump CPUPMU\n";


static int met_switch_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, help);
}

static int met_switch_print_header(char *buf, int len)
{
	int ret = 0;
	int i =0;
	char *start_point = buf;

	ret =
	    SNPRINTF(buf, PAGE_SIZE, "met-info [000] 0.0: mp_cpu_switch_base: %d\n",
		     met_switch.mode);

	if(ret>0){
		start_point = buf+ret;
	}

	/*use str array*/
	if(process_filter_list !=NULL && process_filter_list->str_ptr_array_length>0){
		i = process_filter_list->str_ptr_array_length-1;
		for(; i>=0; i--){
			int n = snprintf(start_point, PAGE_SIZE, "met-info [000] 0.0: switch_filter: %s\n", process_filter_list->str_ptr_array[i]);
			pr_info("[met_switch_print_header] n=%d\n", n);
			start_point += n;
		}
	}

	if (met_switch.mode & MT_SWITCH_64_32BIT)
		start_point += SNPRINTF(start_point, PAGE_SIZE, header);

	return start_point-buf;
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
