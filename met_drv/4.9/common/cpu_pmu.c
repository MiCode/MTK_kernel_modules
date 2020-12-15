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
#include <linux/perf_event.h>
#include <linux/cpu_pm.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "cpu_pmu.h"

struct cpu_pmu_hw *cpu_pmu;
static int counter_cnt[MXNR_CPU];
static int nr_arg[MXNR_CPU];

int met_perf_cpupmu_status;

#ifdef CONFIG_CPU_PM
static int use_cpu_pm_pmu_notifier = 0;

/* helper notifier for maintaining pmu states before cpu state transition */
static int cpu_pm_pmu_notify(struct notifier_block *b,
			     unsigned long cmd,
			     void *p)
{
	int ii;
	int cpu, count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (!met_perf_cpupmu_status)
		return NOTIFY_OK;

	cpu = raw_smp_processor_id();

	switch (cmd) {
	case CPU_PM_ENTER:
		count = cpu_pmu->polling(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu], pmu_value);
		for (ii = 0; ii < count; ii ++)
			cpu_pmu->cpu_pm_unpolled_loss[cpu][ii] += pmu_value[ii];

		cpu_pmu->stop(cpu_pmu->event_count[cpu]);
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		cpu_pmu->start(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu]);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

struct notifier_block cpu_pm_pmu_notifier = {
	.notifier_call = cpu_pm_pmu_notify,
};
#endif

static DEFINE_PER_CPU(unsigned long long[MXNR_PMU_EVENTS], perfCurr);
static DEFINE_PER_CPU(unsigned long long[MXNR_PMU_EVENTS], perfPrev);
static DEFINE_PER_CPU(int[MXNR_PMU_EVENTS], perfCntFirst);
static DEFINE_PER_CPU(struct perf_event * [MXNR_PMU_EVENTS], pevent);
static DEFINE_PER_CPU(struct perf_event_attr [MXNR_PMU_EVENTS], pevent_attr);
static DEFINE_PER_CPU(int, perfSet);
static DEFINE_PER_CPU(unsigned int, perf_task_init_done);
static DEFINE_PER_CPU(int, perf_cpuid);
static DEFINE_PER_CPU(struct delayed_work, cpu_pmu_dwork_setup);
static DEFINE_PER_CPU(struct delayed_work*, perf_delayed_work_setup);
static DEFINE_PER_CPU(struct delayed_work, cpu_pmu_dwork_down);

#ifdef CPUPMU_V8_2
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>

static char mcucfg_desc[] = "mediatek,mcucfg";
static void __iomem *mcucfg_base = NULL;
#define DBG_CONTROL_CPU6	((unsigned long)mcucfg_base + 0x3000 + 0x308)  /* DBG_CONTROL */
#define DBG_CONTROL_CPU7	((unsigned long)mcucfg_base + 0x3800 + 0x308)  /* DBG_CONTROL */
#define ENABLE_MTK_PMU_EVENTS_OFFSET 1
static int restore_dbg_ctrl_cpu6;
static int restore_dbg_ctrl_cpu7;

int cpu_pmu_debug_init(void)
{
	struct device_node  *node = NULL;
	unsigned int value6,value7;

	 /*for A75 MTK internal event*/
	 if (mcucfg_base == NULL) {
		node = of_find_compatible_node(NULL, NULL, mcucfg_desc);
		if (node == NULL) {
			MET_TRACE("[MET_PMU_DB] of_find node == NULL\n");
			pr_debug("[MET_PMU_DB] of_find node == NULL\n");
			goto out;
		}
		mcucfg_base = of_iomap(node, 0);
		of_node_put(node);
		if (mcucfg_base == NULL) {
			MET_TRACE("[MET_PMU_DB] mcucfg_base == NULL\n");
			pr_debug("[MET_PMU_DB] mcucfg_base == NULL\n");
			goto out;
		}
		MET_TRACE("[MET_PMU_DB] regbase %08lx\n", DBG_CONTROL_CPU7);
		pr_debug("[MET_PMU_DB] regbase %08lx\n", DBG_CONTROL_CPU7);
	}

	value6 = readl(IOMEM(DBG_CONTROL_CPU6));
	if (value6 & (1 << ENABLE_MTK_PMU_EVENTS_OFFSET)) {
		restore_dbg_ctrl_cpu6 = 1;
	} else {
		restore_dbg_ctrl_cpu6 = 0;
		mt_reg_sync_writel(value6 | (1 << ENABLE_MTK_PMU_EVENTS_OFFSET), DBG_CONTROL_CPU6);
	}

	value7 = readl(IOMEM(DBG_CONTROL_CPU7));
	if (value7 & (1 << ENABLE_MTK_PMU_EVENTS_OFFSET)) {
		restore_dbg_ctrl_cpu7 = 1;
	} else {
		restore_dbg_ctrl_cpu7 = 0;
		mt_reg_sync_writel(value7 | (1 << ENABLE_MTK_PMU_EVENTS_OFFSET), DBG_CONTROL_CPU7);
	}

	value6 = readl(IOMEM(DBG_CONTROL_CPU6));
	value7 = readl(IOMEM(DBG_CONTROL_CPU7));
	MET_TRACE("[MET_PMU_DB]DBG_CONTROL_CPU6 = %08x,  DBG_CONTROL_CPU7 = %08x\n", value6, value7);
	pr_debug("[MET_PMU_DB]DBG_CONTROL_CPU6 = %08x,  DBG_CONTROL_CPU7 = %08x\n", value6, value7);
	return 1;

out:
	if (mcucfg_base != NULL) {
		iounmap(mcucfg_base);
		mcucfg_base = NULL;
	}
	MET_TRACE("[MET_PMU_DB]DBG_CONTROL init error");
	pr_debug("[MET_PMU_DB]DBG_CONTROL init error");
	return 0;
}

int cpu_pmu_debug_uninit(void)
{
	unsigned int value6,value7;

	if (restore_dbg_ctrl_cpu6 == 0) {
		value6 = readl(IOMEM(DBG_CONTROL_CPU6));
		mt_reg_sync_writel(value6 & (~(1 << ENABLE_MTK_PMU_EVENTS_OFFSET)), DBG_CONTROL_CPU6);
	}
	if (restore_dbg_ctrl_cpu7 == 0) {
		value7 = readl(IOMEM(DBG_CONTROL_CPU7));
		mt_reg_sync_writel(value7 & (~(1 << ENABLE_MTK_PMU_EVENTS_OFFSET)), DBG_CONTROL_CPU7);
	}

	value6 = readl(IOMEM(DBG_CONTROL_CPU6));
	value7 = readl(IOMEM(DBG_CONTROL_CPU7));
	MET_TRACE("[MET_PMU_DB]DBG_CONTROL_CPU6 = %08x,  DBG_CONTROL_CPU7 = %08x\n", value6, value7);
	pr_debug("[MET_PMU_DB]DBG_CONTROL_CPU6 = %08x,  DBG_CONTROL_CPU7 = %08x\n", value6, value7);

	if (mcucfg_base != NULL) {
		iounmap(mcucfg_base);
		mcucfg_base = NULL;
	}
	restore_dbg_ctrl_cpu6 = 0;
	restore_dbg_ctrl_cpu7 = 0;
	return 1;
}
#endif




noinline void mp_cpu(unsigned char cnt, unsigned int *value)
{
	MET_GENERAL_PRINT(MET_TRACE, cnt, value);
}

static void dummy_handler(struct perf_event *event, struct perf_sample_data *data,
			  struct pt_regs *regs)
{
	/*
	 * Required as perf_event_create_kernel_counter() requires an overflow handler,
	 * even though all we do is poll.
	 */
}

static void perf_cpupmu_polling(unsigned long long stamp, int cpu)
{
	int			event_count = cpu_pmu->event_count[cpu];
	struct met_pmu		*pmu = cpu_pmu->pmu[cpu];
	int			i, count;
	unsigned long long	delta;
	struct perf_event	*ev;
	unsigned int		pmu_value[MXNR_PMU_EVENTS];

	if (per_cpu(perfSet, cpu) == 0)
		return;

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;

		ev = per_cpu(pevent, cpu)[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			per_cpu(perfCurr, cpu)[i] = met_perf_event_read_local_symbol(ev);
			delta = (per_cpu(perfCurr, cpu)[i] - per_cpu(perfPrev, cpu)[i]);
			per_cpu(perfPrev, cpu)[i] = per_cpu(perfCurr, cpu)[i];
			if (per_cpu(perfCntFirst, cpu)[i] == 1) {
				/* we shall omit delta counter when we get first counter */
				per_cpu(perfCntFirst, cpu)[i] = 0;
				continue;
			}
			pmu_value[count] = (unsigned int)delta;
			count++;
		}
	}

	if (count == counter_cnt[cpu])
		mp_cpu(count, pmu_value);
}

static struct perf_event* perf_event_create(int cpu, unsigned short event, int count)
{
	struct perf_event_attr	*ev_attr;
	struct perf_event	*ev;

	ev_attr = per_cpu(pevent_attr, cpu)+count;
	memset(ev_attr, 0, sizeof(*ev_attr));
	if (event == 0xff) {
		ev_attr->config = PERF_COUNT_HW_CPU_CYCLES;
		ev_attr->type = PERF_TYPE_HARDWARE;
	} else {
		ev_attr->config = event;
		ev_attr->type = PERF_TYPE_RAW;
	}
	ev_attr->size = sizeof(*ev_attr);
	ev_attr->sample_period = 0;
	ev_attr->pinned = 1;

	ev = perf_event_create_kernel_counter(ev_attr, cpu, NULL, dummy_handler, NULL);
	if (IS_ERR(ev))
		return NULL;
	do {
		if (ev->state == PERF_EVENT_STATE_ACTIVE)
			break;
		if (ev->state == PERF_EVENT_STATE_ERROR) {
			perf_event_enable(ev);
			if (ev->state == PERF_EVENT_STATE_ACTIVE)
				break;
		}
		perf_event_release_kernel(ev);
		return NULL;
	} while (0);

	return ev;
}

static void perf_event_release(int cpu, struct perf_event *ev)
{
	if (ev->state == PERF_EVENT_STATE_ACTIVE)
		perf_event_disable(ev);
	perf_event_release_kernel(ev);
}

static int perf_thread_set_perf_events(int cpu)
{
	int			i, size;
	struct perf_event	*ev;

	size = sizeof(struct perf_event_attr);
	if (per_cpu(perfSet, cpu) == 0) {
		int event_count = cpu_pmu->event_count[cpu];
		struct met_pmu *pmu = cpu_pmu->pmu[cpu];
		for (i = 0; i < event_count; i++) {
			if (!pmu[i].mode)
				continue;	/* Skip disabled counters */
			ev = perf_event_create(cpu, pmu[i].event, i);
			if (ev == NULL) {
				met_cpupmu.mode = 0;
				met_perf_cpupmu_status = 0;

				MET_TRACE("[MET_PMU] failed to register pmu event %4x\n", pmu[i].event);
				pr_notice("[MET_PMU] failed to register pmu event %4x\n", pmu[i].event);
				continue;
			}

			MET_TRACE("[MET_PMU] registered pmu slot: [%d] evt=%#04x\n", ev->hw.idx, pmu[i].event);
			pr_debug("[MET_PMU] registered pmu slot: [%d] evt=%#04x\n", ev->hw.idx, pmu[i].event);

			per_cpu(pevent, cpu)[i] = ev;
			per_cpu(perfPrev, cpu)[i] = 0;
			per_cpu(perfCurr, cpu)[i] = 0;
			perf_event_enable(ev);
			per_cpu(perfCntFirst, cpu)[i] = 1;
		}	/* for all PMU counter */
		per_cpu(perfSet, cpu) = 1;
	}	/* for perfSet */

	return 0;
}

static void perf_thread_setup(struct work_struct *work)
{
	int			cpu;
	struct delayed_work	*dwork = to_delayed_work(work);

	cpu = dwork->cpu;
	if (per_cpu(perf_task_init_done, cpu) == 0) {
		per_cpu(perf_task_init_done, cpu) = 1;
		perf_thread_set_perf_events(cpu);
	}
}

static void met_perf_cpupmu_start(int cpu)
{
	if (met_cpupmu.mode == 0)
		return;

	per_cpu(perf_cpuid, cpu) = cpu;
	if (per_cpu(perf_delayed_work_setup, cpu) == NULL) {
		struct delayed_work *dwork = &per_cpu(cpu_pmu_dwork_setup, cpu);
		INIT_DELAYED_WORK(dwork, perf_thread_setup);
		dwork->cpu = cpu;
		schedule_delayed_work_on(cpu, dwork, 0);
		per_cpu(perf_delayed_work_setup, cpu) = dwork;
	}
}

static void perf_thread_down(struct work_struct *work)
{
	struct delayed_work	*dwork = to_delayed_work(work);
	int			cpu, i;
	struct perf_event	*ev;
	int			event_count;
	struct met_pmu		*pmu;

	cpu = dwork->cpu;
	if (per_cpu(perfSet, cpu) == 0)
		return;

	per_cpu(perfSet, cpu) = 0;
	event_count = cpu_pmu->event_count[cpu];
	pmu = cpu_pmu->pmu[cpu];
	for (i = 0; i < event_count; i++) {
		ev = per_cpu(pevent, cpu)[i];
		if (ev != NULL) {
			perf_event_release(cpu, ev);
			per_cpu(pevent, cpu)[i] = NULL;
		}
	}
	per_cpu(perf_task_init_done, cpu) = 0;
	per_cpu(perf_delayed_work_setup, cpu) = NULL;
}

static void met_perf_cpupmu_stop(int cpu)
{
	struct delayed_work	*dwork;

	per_cpu(perf_cpuid, cpu) = cpu;
	dwork = &per_cpu(cpu_pmu_dwork_down, cpu);
	INIT_DELAYED_WORK(dwork, perf_thread_down);
	dwork->cpu = cpu;
	schedule_delayed_work_on(cpu, dwork, 0);
}

static int cpupmu_create_subfs(struct kobject *parent)
{
	cpu_pmu = cpu_pmu_hw_init();
	if (cpu_pmu == NULL) {
		PR_BOOTMSG("Failed to init CPU PMU HW!!\n");
		return -ENODEV;
	}

	return 0;
}

static void cpupmu_delete_subfs(void)
{
}

void met_perf_cpupmu_polling(unsigned long long stamp, int cpu)
{
	int count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (met_cpu_pmu_method) {
		perf_cpupmu_polling(stamp, cpu);
	} else {
		count = cpu_pmu->polling(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu], pmu_value);

#ifdef CONFIG_CPU_PM
		if (met_cpu_pm_pmu_reconfig) {
			int ii;
			for (ii = 0; ii < count; ii ++)
				pmu_value[ii] += cpu_pmu->cpu_pm_unpolled_loss[cpu][ii];
		}
#endif

		mp_cpu(count, pmu_value);

#ifdef CONFIG_CPU_PM
		if (met_cpu_pm_pmu_reconfig) {
			memset(cpu_pmu->cpu_pm_unpolled_loss[cpu], 0, sizeof (cpu_pmu->cpu_pm_unpolled_loss[0]));
		}
#endif
	}
}

static void cpupmu_start(void)
{
	int	cpu = raw_smp_processor_id();

	if (met_cpu_pmu_method)
		met_perf_cpupmu_start(cpu);
	else {
		nr_arg[cpu] = 0;
		cpu_pmu->start(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu]);
	}
	met_perf_cpupmu_status = 1;
}


static void cpupmu_unique_start(void)
{
#ifdef CPUPMU_V8_2
	int ret = 0;
	ret = cpu_pmu_debug_init();
	if (ret == 0)
		PR_BOOTMSG("Failed to init CPU PMU debug!!\n");
#endif

#ifdef CONFIG_CPU_PM
	use_cpu_pm_pmu_notifier = 0;
	if (met_cpu_pm_pmu_reconfig) {
		if (met_cpu_pmu_method) {
			met_cpu_pm_pmu_reconfig = 0;
			MET_TRACE("[MET_PMU] met_cpu_pmu_method=%d, met_cpu_pm_pmu_reconfig forced disabled\n", met_cpu_pmu_method);
			pr_debug("[MET_PMU] met_cpu_pmu_method=%d, met_cpu_pm_pmu_reconfig forced disabled\n", met_cpu_pmu_method);
		} else {
			memset(cpu_pmu->cpu_pm_unpolled_loss, 0, sizeof (cpu_pmu->cpu_pm_unpolled_loss));
			cpu_pm_register_notifier(&cpu_pm_pmu_notifier);
			use_cpu_pm_pmu_notifier = 1;
		}
	}
#else
	if (met_cpu_pm_pmu_reconfig) {
		met_cpu_pm_pmu_reconfig = 0;
		MET_TRACE("[MET_PMU] CONFIG_CPU_PM=%d, met_cpu_pm_pmu_reconfig forced disabled\n", CONFIG_CPU_PM);
		pr_debug("[MET_PMU] CONFIG_CPU_PM=%d, met_cpu_pm_pmu_reconfig forced disabled\n", CONFIG_CPU_PM);
	}
#endif
	MET_TRACE("[MET_PMU] met_cpu_pm_pmu_reconfig=%u\n", met_cpu_pm_pmu_reconfig);
	pr_debug("[MET_PMU] met_cpu_pm_pmu_reconfig=%u\n", met_cpu_pm_pmu_reconfig);

	return;
}

static void cpupmu_stop(void)
{
	int	cpu = raw_smp_processor_id();

	met_perf_cpupmu_status = 0;
	if (met_cpu_pmu_method)
		met_perf_cpupmu_stop(cpu);
	else
		cpu_pmu->stop(cpu_pmu->event_count[cpu]);
}

static void cpupmu_unique_stop(void)
{
#ifdef CPUPMU_V8_2
	cpu_pmu_debug_uninit();
#endif

#ifdef CONFIG_CPU_PM
	if (use_cpu_pm_pmu_notifier) {
		cpu_pm_unregister_notifier(&cpu_pm_pmu_notifier);
	}
#endif
	return;
}

static const char cache_line_header[] =
	"met-info [000] 0.0: met_cpu_cache_line_size: %d\n";
static const char header[] =
	"met-info [000] 0.0: met_cpu_header_v2: %d";

static const char help[] =
	"  --pmu-cpu-evt=[cpu_list:]event_list   select CPU-PMU events in %s\n"
	"                                        cpu_list: specify the cpu_id list or apply to all the cores\n"
	"                                            example: 0,1,2\n"
	"                                        event_list: specify the event number\n"
	"                                            example: 0x8,0xff\n";

static int cpupmu_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help, cpu_pmu->cpu_name);
}

static int reset_driver_stat(void)
{
	int		cpu, i;
	int		event_count;
	struct met_pmu	*pmu;

	met_cpupmu.mode = 0;
	for_each_possible_cpu(cpu) {
		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		counter_cnt[cpu] = 0;
		nr_arg[cpu] = 0;
		for (i = 0; i < event_count; i++) {
			pmu[i].mode = MODE_DISABLED;
			pmu[i].event = 0;
			pmu[i].freq = 0;
		}
	}

	return 0;
}

static int cpupmu_print_header(char *buf, int len)
{
	int		cpu, i, ret, first;
	int		event_count;
	struct met_pmu	*pmu;

	ret = 0;

	/*append CPU PMU access method*/
	if (met_cpu_pmu_method)
		ret += snprintf(buf + ret, PAGE_SIZE,
			"met-info [000] 0.0: CPU_PMU_method: perf APIs\n");
	else
		ret += snprintf(buf + ret, PAGE_SIZE,
			"met-info [000] 0.0: CPU_PMU_method: MET pmu driver\n");

	/*append cache line size*/
	ret += snprintf(buf + ret, PAGE_SIZE - ret, cache_line_header, cache_line_size());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "# mp_cpu: pmu_value1, ...\n");

	for_each_possible_cpu(cpu) {
		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;
		for (i = 0; i < event_count; i++) {
			if (pmu[i].mode == 0)
				continue;
			if (first) {
				ret += snprintf(buf + ret, PAGE_SIZE - ret, header, cpu);
				first = 0;
			}
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",0x%x", pmu[i].event);
			pmu[i].mode = 0;
		}
		if (!first)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}

	reset_driver_stat();

	return ret;
}

static int met_parse_num_list(char *arg, int len, int *list, int list_cnt)
{
	int	nr_num = 0;
	char	*num;
	int	num_len;

	/* search ',' as the splitter */
	while (len) {
		num = arg;
		num_len = 0;
		if (list_cnt <= 0)
			return -1;
		while (len) {
			len--;
			if (*arg == ',') {
				*(arg++) = '\0';
				break;
			}
			arg++;
			num_len++;
		}
		if (met_parse_num(num, list, num_len) < 0)
			return -1;
		list++;
		list_cnt--;
		nr_num++;
	}

	return nr_num;
}

static int cpupmu_process_argument(const char *arg, int len)
{
	char		*arg1 = (char*)arg;
	int		len1 = len;
	int		cpu, cpu_list[MXNR_CPU];
	int		nr_events, event_list[MXNR_PMU_EVENTS];
	int		i;
	int		nr_counters;
	struct met_pmu	*pmu;
	int		arg_nr;
	int		event_no;

	/*
	 * split cpu_list and event_list by ':'
	 *   arg, len: cpu_list when found (i < len)
	 *   arg1, len1: event_list
	 */
	for (i = 0; i < len; i++) {
		if (arg[i] == ':') {
			arg1[i] = '\0';
			arg1 += i+1;
			len1 = len - i - 1;
			len = i;
			break;
		}
	}

	/*
	 * setup cpu_list array
	 *   1: selected
	 *   0: unselected
	 */
	if (arg1 != arg) {	/* is cpu_id list specified? */
		int list[MXNR_CPU], cnt;
		int cpu_id;
		if ((cnt = met_parse_num_list((char*)arg, len, list, ARRAY_SIZE(list))) <= 0)
			goto arg_out;
		memset(cpu_list, 0, sizeof(cpu_list));
		for (i = 0; i < cnt; i++) {
			cpu_id = list[i];
			if (cpu_id < 0 || cpu_id >= ARRAY_SIZE(cpu_list))
				goto arg_out;
			cpu_list[cpu_id] = 1;
		}
	}
	else
		memset(cpu_list, 1, sizeof(cpu_list));

	/* get event_list */
	if ((nr_events = met_parse_num_list(arg1, len1, event_list, ARRAY_SIZE(event_list))) <= 0)
		goto arg_out;

	/* for each cpu in cpu_list, add all the events in event_list */
	for_each_possible_cpu(cpu) {
		pmu = cpu_pmu->pmu[cpu];
		arg_nr = nr_arg[cpu];

		if (cpu_list[cpu] == 0)
			continue;

		if (met_cpu_pmu_method) {
			nr_counters = perf_num_counters();
		} else {
			nr_counters = cpu_pmu->event_count[cpu];
		}

		pr_debug("[MET_PMU] pmu slot count=%d\n", nr_counters);

		if (nr_counters == 0)
			goto arg_out;

		for (i = 0; i < nr_events; i++) {
			event_no = event_list[i];
			/*
			 * check if event is duplicate,
			 * but may not include 0xff when met_cpu_pmu_method == 0.
			 */
			if (cpu_pmu->check_event(pmu, arg_nr, event_no) < 0)
				goto arg_out;

			/*
			 * test if this event is available when in perf_APIs mode
			 */
			if (met_cpu_pmu_method) {
				struct perf_event *ev;
				ev = perf_event_create(cpu, event_no, arg_nr);
				if (ev == NULL) {
					pr_notice("[MET_PMU] failed pmu alloction test: event_no=%#04x\n", event_no);
					goto arg_out;
				}
				perf_event_release(cpu, ev);
			}

			if (met_cpu_pmu_method) {
				if (arg_nr >= nr_counters)
					goto arg_out;
				pmu[arg_nr].mode = MODE_POLLING;
				pmu[arg_nr].event = event_no;
				pmu[arg_nr].freq = 0;
				arg_nr++;
			} else {
				if (event_no == 0xff) {
					if (pmu[nr_counters-1].mode == MODE_POLLING)
						goto arg_out;
					pmu[nr_counters-1].mode = MODE_POLLING;
					pmu[nr_counters-1].event = 0xff;
					pmu[nr_counters-1].freq = 0;
				} else {
					if (arg_nr >= (nr_counters - 1))
						goto arg_out;
					pmu[arg_nr].mode = MODE_POLLING;
					pmu[arg_nr].event = event_no;
					pmu[arg_nr].freq = 0;
					arg_nr++;
				}
			}
			counter_cnt[cpu]++;
		}
		nr_arg[cpu] = arg_nr;
	}

	met_cpupmu.mode = 1;
	return 0;

arg_out:
	reset_driver_stat();
	return -EINVAL;
}

struct metdevice met_cpupmu = {
	.name = "cpu",
	.type = MET_TYPE_PMU,
	.cpu_related = 1,
	.create_subfs = cpupmu_create_subfs,
	.delete_subfs = cpupmu_delete_subfs,
	.start = cpupmu_start,
	.uniq_start = cpupmu_unique_start,
	.stop = cpupmu_stop,
	.uniq_stop = cpupmu_unique_stop,
	.polling_interval = 1,
	.timed_polling = met_perf_cpupmu_polling,
	.print_help = cpupmu_print_help,
	.print_header = cpupmu_print_header,
	.process_argument = cpupmu_process_argument
};
