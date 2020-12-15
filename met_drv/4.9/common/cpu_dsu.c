/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/perf_event.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "cpu_dsu.h"
#include "core_plf_init.h"

struct cpu_dsu_hw *cpu_dsu;
static int counter_cnt;
static struct kobject *kobj_dsu;
static int nr_arg;
static unsigned long long perfCurr[MXNR_DSU_EVENTS];
static unsigned long long perfPrev[MXNR_DSU_EVENTS];
static int perfCntFirst[MXNR_DSU_EVENTS];
static struct perf_event * pevent[MXNR_DSU_EVENTS];
static struct perf_event_attr pevent_attr[MXNR_DSU_EVENTS];
static unsigned int perf_device_type = 7;
static ssize_t perf_type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", perf_device_type);
}

static ssize_t perf_type_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtouint(buf, 0, &perf_device_type) != 0)
		return -EINVAL;

	return n;
}
static struct kobj_attribute perf_type_attr = __ATTR(perf_type, 0664, perf_type_show, perf_type_store);

noinline void mp_dsu(unsigned char cnt, unsigned int *value)
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

static void perf_cpudsu_polling(unsigned long long stamp, int cpu)
{
	int	event_count = cpu_dsu->event_count;
	struct met_dsu	*pmu = cpu_dsu->pmu;
	int	i, count;
	unsigned long long	delta;
	struct perf_event	*ev;
	unsigned int pmu_value[MXNR_DSU_EVENTS];

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;

		ev = pevent[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			perfCurr[i] = met_perf_event_read_local_symbol(ev);
			delta = (perfCurr[i] - perfPrev[i]);
			perfPrev[i] = perfCurr[i];
			if (perfCntFirst[i] == 1) {
				/* we shall omit delta counter when we get first counter */
				perfCntFirst[i] = 0;
				continue;
			}
			pmu_value[count] = (unsigned int)delta;
			count++;
		}
	}

	if (count == counter_cnt)
		mp_dsu(count, pmu_value);
}

static int perf_thread_set_perf_events(unsigned int cpu)
{
	int			i, size;
	struct perf_event	*ev;
	struct perf_event_attr	*ev_attr;
	int event_count = cpu_dsu->event_count;
	struct met_dsu *pmu = cpu_dsu->pmu;

	size = sizeof(struct perf_event_attr);

	for (i = 0; i < event_count; i++) {
		pevent[i] = NULL;
		if (!pmu[i].mode)
			continue;	/* Skip disabled counters */
		perfPrev[i] = 0;
		perfCurr[i] = 0;
		ev_attr = pevent_attr+i;
		memset(ev_attr, 0, size);
		ev_attr->config = pmu[i].event;
		ev_attr->type = perf_device_type;
		ev_attr->size = size;
		ev_attr->sample_period = 0;
		ev_attr->pinned = 1;

		ev = perf_event_create_kernel_counter(ev_attr, cpu, NULL, dummy_handler, NULL);
		if (IS_ERR(ev))
			continue;
		if (ev->state != PERF_EVENT_STATE_ACTIVE) {
			perf_event_release_kernel(ev);
			continue;
		}
		pevent[i] = ev;
		if (ev != NULL)
			perf_event_enable(ev);
	}	/* for all PMU counter */
	return 0;
}

void met_perf_cpudsu_down(void)
{
	int i;
	struct perf_event *ev;
	int event_count;
	struct met_dsu *pmu;

	if (met_cpudsu.mode == 0)
		return;
	event_count = cpu_dsu->event_count;
	pmu = cpu_dsu->pmu;
	for (i = 0; i < event_count; i++) {
		if (!pmu[i].mode)
			continue;
		ev = pevent[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			perf_event_disable(ev);
			perf_event_release_kernel(ev);
		}
		pevent[i] = NULL;
	}
	//perf_delayed_work_setup = NULL;
}

inline static void met_perf_cpudsu_start(int cpu)
{
	if (met_cpudsu.mode == 0)
		return;
	if (cpu != 0)
		return;
	perf_thread_set_perf_events(cpu);
}

static int cpudsu_create_subfs(struct kobject *parent)
{
	int ret = 0;
	cpu_dsu = cpu_dsu_hw_init();
	if (cpu_dsu == NULL) {
		PR_BOOTMSG("Failed to init CPU PMU HW!!\n");
		return -ENODEV;
	}
	kobj_dsu = parent;
	ret = sysfs_create_file(kobj_dsu, &perf_type_attr.attr);
	if (ret != 0) {
		PR_BOOTMSG("Failed to create perf_type in sysfs\n");
		goto out;
	}
 out:
	return ret;
}

static void cpudsu_delete_subfs(void)
{
}

void met_perf_cpudsu_polling(unsigned long long stamp, int cpu)
{
	perf_cpudsu_polling(stamp, cpu);
}

static void cpudsu_start(void)
{
	int	cpu = raw_smp_processor_id();
	for_each_online_cpu(cpu)
		met_perf_cpudsu_start(cpu);
}

static void cpudsu_stop(void)
{
	met_perf_cpudsu_down();
}

static const char header[] =
	"met-info [000] 0.0: met_dsu_pmu_header: DSU";

static const char help[] =
	"  --dsu=EVENT         select DSU-PMU events.\n"
	"                      you can enable at most \"%d general purpose events\"\n";

static int cpudsu_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help, cpu_dsu->event_count);
}

static int reset_driver_stat(void)
{
	int		i;
	int		event_count;
	struct met_dsu	*pmu;

	met_cpudsu.mode = 0;
	event_count = cpu_dsu->event_count;
	pmu = cpu_dsu->pmu;
	counter_cnt = 0;
	nr_arg = 0;
	for (i = 0; i < event_count; i++) {
		pmu[i].mode = MODE_DISABLED;
		pmu[i].event = 0;
		pmu[i].freq = 0;
	}
	return 0;
}

static int cpudsu_print_header(char *buf, int len)
{
	int		first;
	int		i, ret;
	int		event_count;
	struct met_dsu	*pmu;
	ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "# mp_dsu: pmu_value1, ...\n");
	event_count = cpu_dsu->event_count;
	pmu = cpu_dsu->pmu;
	first = 1;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;
		if (first) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret, header);
			first = 0;
		}
		ret += snprintf(buf + ret, PAGE_SIZE - ret, ",0x%x", pmu[i].event);
		pmu[i].mode = 0;
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
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

static int cpudsu_process_argument(const char *arg, int len)
{
	int		nr_events, event_list[MXNR_DSU_EVENTS];
	int		i;
	int		nr_counters;
	struct met_dsu	*pmu;
	int		arg_nr;
	int		counters;
	int		event_no;

	/* get event_list */
	if ((nr_events = met_parse_num_list((char*)arg, len, event_list, ARRAY_SIZE(event_list))) <= 0)
		goto arg_out;

	/* for each cpu in cpu_list, add all the events in event_list */
	nr_counters = cpu_dsu->event_count;
	pmu = cpu_dsu->pmu;
	arg_nr = nr_arg;

	/*
	 * setup nr_counters for linux native perf mode.
	 * because the selected events are stored in pmu,
	 * so nr_counters can't large then event count in pmu.
	 */
	counters = perf_num_counters();
	if (counters < nr_counters)
		nr_counters = counters;

	if (nr_counters == 0)
		goto arg_out;

	for (i = 0; i < nr_events; i++) {
		event_no = event_list[i];
		/*
		 * check if event is duplicate,
		 * but may not include 0xff when met_cpu_dsu_method == 0.
		 */
		if (cpu_dsu->check_event(pmu, arg_nr, event_no) < 0)
			goto arg_out;
		if (arg_nr >= nr_counters)
			goto arg_out;
		pmu[arg_nr].mode = MODE_POLLING;
		pmu[arg_nr].event = event_no;
		pmu[arg_nr].freq = 0;
		arg_nr++;
		counter_cnt++;
	}
	nr_arg = arg_nr;
	met_cpudsu.mode = 1;
	return 0;

arg_out:
	reset_driver_stat();
	return -EINVAL;
}

struct metdevice met_cpudsu = {
	.name = "dsu",
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = cpudsu_create_subfs,
	.delete_subfs = cpudsu_delete_subfs,
	.start = cpudsu_start,
	.stop = cpudsu_stop,
	.polling_interval = 1,
	.timed_polling = met_perf_cpudsu_polling,
	.print_help = cpudsu_print_help,
	.print_header = cpudsu_print_header,
	.process_argument = cpudsu_process_argument
};
