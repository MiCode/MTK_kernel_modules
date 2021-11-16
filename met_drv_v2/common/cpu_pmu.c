// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/perf_event.h>

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
#include <linux/platform_device.h>
#include <linux/perf/arm_pmu.h>
#endif

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/perf/arm_pmu.h>
#include <linux/irqreturn.h>
#include <linux/irq_work.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "cpu_pmu.h"
#include "mtk_typedefs.h"

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "sspm_met_ipi_handle.h"
#endif
#endif

struct cpu_pmu_hw *cpu_pmu;
static int counter_cnt[MXNR_CPU];
static int nr_arg[MXNR_CPU];

int met_perf_cpupmu_status;

static int mtk_pmu_event_enable = 0;
static struct kobject *kobj_cpu;
DECLARE_KOBJ_ATTR_INT(mtk_pmu_event_enable, mtk_pmu_event_enable);
#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(mtk_pmu_event_enable); \
	} while (0)

DEFINE_MUTEX(handle_irq_lock);
irqreturn_t (*handle_irq_orig)(struct arm_pmu *pmu);

#if IS_ENABLED(CONFIG_CPU_PM)
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
static DEFINE_PER_CPU(int, cpu_status);

#ifdef CPUPMU_V8_2
#include <linux/of.h>
#include <linux/of_address.h>

#ifdef USE_KERNEL_SYNC_WRITE_H
#include <mt-plat/sync_write.h>
#else
#include "sync_write.h"
#endif

#ifdef USE_KERNEL_MTK_IO_H
#include <mt-plat/mtk_io.h>
#else
#include "mtk_io.h"
#endif

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
	u64 value;
	int ret;

	if (per_cpu(perfSet, cpu) == 0)
		return;

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;

		ev = per_cpu(pevent, cpu)[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			if (!met_export_api_symbol->met_perf_event_read_local)
				continue;

			ret = met_export_api_symbol->met_perf_event_read_local(ev, &value);
			if (ret < 0) {
				PR_BOOTMSG_ONCE("[MET_PMU] perf_event_read_local fail (ret=%d)\n", ret);
				pr_debug("[MET_PMU] perf_event_read_local fail (ret=%d)\n", ret);
				continue;
			}

			per_cpu(perfCurr, cpu)[i] = value;
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

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#define	PMU_OVERFLOWED_MASK	0xffffffff

static inline int pmu_has_overflowed(u32 pmovsr)
{
	return pmovsr & PMU_OVERFLOWED_MASK;
}

static irqreturn_t perf_event_handle_irq_ignore_overflow(struct arm_pmu *pmu)
{
	u32 pmovsr;

	pmovsr = cpu_pmu->pmu_read_clear_overflow_flag();

	if (!pmu_has_overflowed(pmovsr)) {
		return IRQ_NONE;
	}
	else {
		irq_work_run();
		return IRQ_HANDLED;
	}
}
#endif
#endif

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

				MET_TRACE("[MET_PMU] cpu %d failed to register pmu event %4x\n", cpu, pmu[i].event);
				pr_notice("[MET_PMU] cpu %d failed to register pmu event %4x\n", cpu, pmu[i].event);
				continue;
			}

			/*
			 * in perf-event implementation, hardware pmu slot and cycle counter
			 * was mapped to perf_event::hw::idx as follows:
			 *
			 * | idx | hardware slot |
			 * |-----+---------------|
			 * |   0 | pmccntr_el0   |
			 * |   1 | 0             |
			 * |   2 | 1             |
			 * |   3 | 2             |
			 * |   4 | 3             |
			 * |   5 | 4             |
			 * |   6 | 5             |
			 */
			if (ev->hw.idx != 0) {
				MET_TRACE("[MET_PMU] cpu %d registered in pmu slot: [%d] evt=%#04x\n",
					  cpu, ev->hw.idx-1, pmu[i].event);
				pr_debug("[MET_PMU] cpu %d registered in pmu slot: [%d] evt=%#04x\n",
					 cpu, ev->hw.idx-1, pmu[i].event);
			} else if (ev->hw.idx == 0) {
				MET_TRACE("[MET_PMU] cpu %d registered cycle count evt=%#04x\n",
					  cpu, pmu[i].event);
				pr_debug("[MET_PMU] cpu %d registered cycle count evt=%#04x\n",
					 cpu, pmu[i].event);
			}

			per_cpu(pevent, cpu)[i] = ev;
			per_cpu(perfPrev, cpu)[i] = 0;
			per_cpu(perfCurr, cpu)[i] = 0;
			perf_event_enable(ev);
			per_cpu(perfCntFirst, cpu)[i] = 1;

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
			if (met_cpupmu.ondiemet_mode) {
				struct arm_pmu *armpmu;
				armpmu = container_of(ev->pmu, struct arm_pmu, pmu);
				mutex_lock(&handle_irq_lock);
				if (armpmu && armpmu->handle_irq != perf_event_handle_irq_ignore_overflow) {
					pr_debug("[MET_PMU] replaced original handle_irq=%p with dummy function\n",
						 armpmu->handle_irq);
					handle_irq_orig = armpmu->handle_irq;
					armpmu->handle_irq = perf_event_handle_irq_ignore_overflow;
				}
				mutex_unlock(&handle_irq_lock);
			}
#endif
#endif
		}	/* for all PMU counter */
		per_cpu(perfSet, cpu) = 1;
	}	/* for perfSet */

	return 0;
}

static void met_perf_cpupmu_start(int cpu)
{
	if (met_cpupmu.mode == 0)
		return;

	perf_thread_set_perf_events(cpu);
}

static void perf_thread_down(int cpu)
{
	int			i;
	struct perf_event	*ev;
	int			event_count;
	struct met_pmu		*pmu;

	if (per_cpu(perfSet, cpu) == 0)
		return;

	per_cpu(perfSet, cpu) = 0;
	event_count = cpu_pmu->event_count[cpu];
	pmu = cpu_pmu->pmu[cpu];
	for (i = 0; i < event_count; i++) {
		ev = per_cpu(pevent, cpu)[i];
		if (ev != NULL) {

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
			if (met_cpupmu.ondiemet_mode) {
				struct arm_pmu *armpmu;
				armpmu = container_of(ev->pmu, struct arm_pmu, pmu);
				mutex_lock(&handle_irq_lock);
				if (armpmu && armpmu->handle_irq == perf_event_handle_irq_ignore_overflow) {
					pr_debug("[MET_PMU] restore original handle_irq=%p\n", handle_irq_orig);
					armpmu->handle_irq = handle_irq_orig;
					handle_irq_orig = NULL;
				}
				mutex_unlock(&handle_irq_lock);
			}
#endif
#endif

			perf_event_release(cpu, ev);
			per_cpu(pevent, cpu)[i] = NULL;
		}
	}
}

static void met_perf_cpupmu_stop(int cpu)
{
	perf_thread_down(cpu);
}

static int cpupmu_create_subfs(struct kobject *parent)
{
	int ret = 0;

	cpu_pmu = cpu_pmu_hw_init();
	if (cpu_pmu == NULL) {
		PR_BOOTMSG("Failed to init CPU PMU HW!!\n");
		return -ENODEV;
	}

	kobj_cpu = parent;

#define KOBJ_ATTR_ITEM(attr_name) \
	do { \
		ret = sysfs_create_file(kobj_cpu, &attr_name ## _attr.attr); \
		if (ret != 0) { \
			pr_notice("Failed to create " #attr_name " in sysfs\n"); \
			return ret; \
		} \
	} while (0)
	KOBJ_ATTR_LIST;
#undef  KOBJ_ATTR_ITEM

	return 0;
}

static void cpupmu_delete_subfs(void)
{
#define KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_cpu, &attr_name ## _attr.attr)

	if (kobj_cpu != NULL) {
		KOBJ_ATTR_LIST;
		kobj_cpu = NULL;
	}
#undef  KOBJ_ATTR_ITEM
}

void met_perf_cpupmu_polling(unsigned long long stamp, int cpu)
{
	int count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (per_cpu(cpu_status, cpu) != MET_CPU_ONLINE)
		return;

	if (met_cpu_pmu_method) {
		perf_cpupmu_polling(stamp, cpu);
	} else {
		count = cpu_pmu->polling(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu], pmu_value);

#if IS_ENABLED(CONFIG_CPU_PM)
		if (met_cpu_pm_pmu_reconfig) {
			int ii;
			for (ii = 0; ii < count; ii ++)
				pmu_value[ii] += cpu_pmu->cpu_pm_unpolled_loss[cpu][ii];
		}
#endif

		mp_cpu(count, pmu_value);

#if IS_ENABLED(CONFIG_CPU_PM)
		if (met_cpu_pm_pmu_reconfig) {
			memset(cpu_pmu->cpu_pm_unpolled_loss[cpu], 0, sizeof (cpu_pmu->cpu_pm_unpolled_loss[0]));
		}
#endif
	}
}

static void cpupmu_start(void)
{
	int	cpu = raw_smp_processor_id();

	if (!met_cpu_pmu_method) {
		nr_arg[cpu] = 0;
		cpu_pmu->start(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu]);

		met_perf_cpupmu_status = 1;
		per_cpu(cpu_status, cpu) = MET_CPU_ONLINE;
	}
}


static void cpupmu_unique_start(void)
{
	int cpu;

#ifdef CPUPMU_V8_2
	int ret = 0;
	if (mtk_pmu_event_enable == 1){
		ret = cpu_pmu_debug_init();
		if (ret == 0)
			PR_BOOTMSG("Failed to init CPU PMU debug!!\n");
	}
#endif

#if IS_ENABLED(CONFIG_CPU_PM)
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

	if (met_cpu_pmu_method) {
		for_each_possible_cpu(cpu) {
			met_perf_cpupmu_start(cpu);

			met_perf_cpupmu_status = 1;
			per_cpu(cpu_status, cpu) = MET_CPU_ONLINE;
		}
	}

	return;
}

static void cpupmu_stop(void)
{
	int	cpu = raw_smp_processor_id();

	met_perf_cpupmu_status = 0;

	if (!met_cpu_pmu_method)
		cpu_pmu->stop(cpu_pmu->event_count[cpu]);
}

static void cpupmu_unique_stop(void)
{
	int cpu;

	if (met_cpu_pmu_method) {
		for_each_possible_cpu(cpu) {
			met_perf_cpupmu_stop(cpu);
		}
	}

#ifdef CPUPMU_V8_2
	if (mtk_pmu_event_enable == 1)
		cpu_pmu_debug_uninit();
#endif

#if IS_ENABLED(CONFIG_CPU_PM)
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
		ret += snprintf(buf + ret, len,
			"met-info [000] 0.0: CPU_PMU_method: perf APIs\n");
	else
		ret += snprintf(buf + ret, len,
			"met-info [000] 0.0: CPU_PMU_method: MET pmu driver\n");

	/*append cache line size*/
	ret += snprintf(buf + ret, len - ret, cache_line_header, cache_line_size());
	ret += snprintf(buf + ret, len - ret, "# mp_cpu: pmu_value1, ...\n");

	for_each_possible_cpu(cpu) {
		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;
		for (i = 0; i < event_count; i++) {
			if (pmu[i].mode == 0)
				continue;
			if (first) {
				ret += snprintf(buf + ret, len - ret, header, cpu);
				first = 0;
			}
			ret += snprintf(buf + ret, len - ret, ",0x%x", pmu[i].event);
			pmu[i].mode = 0;
		}
		if (!first)
			ret += snprintf(buf + ret, len - ret, "\n");
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

static const struct perf_pmu_events_attr *
perf_event_get_evt_attr_by_name(const struct perf_event *ev,
			       const char *name) {
	struct arm_pmu *arm_pmu;
	struct attribute **attrp;
	struct device_attribute *dev_attr_p;
	struct perf_pmu_events_attr *ev_attr_p;

	arm_pmu = container_of(ev->pmu, struct arm_pmu, pmu);

	for (attrp = arm_pmu->attr_groups[ARMPMU_ATTR_GROUP_EVENTS]->attrs;
	     *attrp != NULL;
	     attrp ++) {

		dev_attr_p = container_of(*attrp, struct device_attribute, attr);
		ev_attr_p = container_of(dev_attr_p, struct perf_pmu_events_attr, attr);

		if (0 == strcmp((*attrp)->name, name)) {
			return ev_attr_p;
		}
	}

	return NULL;
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
	int		is_cpu_cycle_evt;
	const struct perf_pmu_events_attr *ev_attr_p;

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
			is_cpu_cycle_evt = 0;
			/*
			 * check if event is duplicate, but does not include 0xff
			 */
			if (cpu_pmu->check_event(pmu, arg_nr, event_no) < 0)
				goto arg_out;

			/*
			 * test if this event is available when in perf_APIs mode
			 */
			if (met_cpu_pmu_method) {
				struct perf_event *ev;

				if (!cpu_pmu->perf_event_get_evttype) {
					MET_TRACE("[MET_PMU] cpu_pmu->perf_event_get_evttype=NULL, "
						  "met pmu on perf-event was not supported on this platform\n");
					pr_debug("[MET_PMU] cpu_pmu->perf_event_get_evttype=NULL, "
						 "met pmu on perf-event was not supported on this platform\n");
					goto arg_out;
				}

				ev = perf_event_create(cpu, event_no, arg_nr);
				if (ev == NULL) {
					pr_debug("!!!!!!!! [MET_PMU] failed pmu alloction test (event_no=%#04x)\n", event_no);
					goto arg_out;
				} else {
					perf_event_release(cpu, ev);
				}

				ev_attr_p = perf_event_get_evt_attr_by_name(ev, "cpu_cycles");
				if (ev_attr_p && cpu_pmu->perf_event_get_evttype(ev) == ev_attr_p->id)
					is_cpu_cycle_evt = 1;
			}

			if (met_cpu_pmu_method) {
				if (is_cpu_cycle_evt) {
					if (pmu[nr_counters-1].mode == MODE_POLLING)
						goto arg_out;
					pmu[nr_counters-1].mode = MODE_POLLING;
					pmu[nr_counters-1].event = event_no;
					pmu[nr_counters-1].freq = 0;
				} else {
					if (arg_nr >= (nr_counters - 1))
						goto arg_out;
					pmu[arg_nr].mode = MODE_POLLING;
					pmu[arg_nr].event = event_no;
					pmu[arg_nr].freq = 0;
					arg_nr++;
				}
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

static void cpupmu_cpu_state_notify(long cpu, unsigned long action)
{
	per_cpu(cpu_status, cpu) = action;

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
	if (met_cpu_pmu_method && action == MET_CPU_OFFLINE) {
		struct perf_event *event = NULL;
		struct arm_pmu *armpmu = NULL;
		struct platform_device *pmu_device = NULL;
		int irq = 0;

		event = per_cpu(pevent, cpu)[0];
		if (event)
			armpmu = to_arm_pmu(event->pmu);
		pr_debug("!!!!!!!! %s_%ld, event=%p\n", __FUNCTION__, cpu, event);

		if (armpmu)
			pmu_device = armpmu->plat_device;
		pr_debug("!!!!!!!! %s_%ld, armpmu=%p\n", __FUNCTION__, cpu, armpmu);

		if (pmu_device)
			irq = platform_get_irq(pmu_device, 0);
		pr_debug("!!!!!!!! %s_%ld, pmu_device=%p\n", __FUNCTION__, cpu, pmu_device);

		if (irq > 0)
			disable_percpu_irq(irq);
		pr_debug("!!!!!!!! %s_%ld, irq=%d\n", __FUNCTION__, cpu, irq);
	}
#endif
}

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
static void sspm_pmu_start(void)
{
	ondiemet_module[ONDIEMET_SSPM] |= ID_PMU;

	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_start();
}

static int cycle_count_mode_enabled(int cpu) {

	int event_cnt;
	struct met_pmu	*pmu;

	pmu = cpu_pmu->pmu[cpu];

	if (met_cpu_pmu_method) {
		event_cnt = perf_num_counters();
	} else {
		event_cnt = cpu_pmu->event_count[cpu];
	}

	return pmu[event_cnt-1].mode == MODE_POLLING;
}

static void ipi_config_pmu_counter_cnt(void) {

	int ret, cpu, ii, cnt_num;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	struct hw_perf_event *hwc;
	unsigned int base_offset;

	for_each_possible_cpu(cpu) {
		for (ii = 0; ii < 4; ii++)
			ipi_buf[ii] = 0;

		ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_EVT_CNT;
		/*
		 *  XXX: on sspm side, cycle counter was not counted in
		 *       total event number `counter_cnt', but controlled by
		 *       an addtional argument `SET_PMU_CYCCNT_ENABLE' instead
		 */
		cnt_num = (cycle_count_mode_enabled(cpu) ?
			   (counter_cnt[cpu]-1) : counter_cnt[cpu]);
		ipi_buf[1] = (cpu << 16) | (cnt_num & 0xffff);

		MET_TRACE("[MET_PMU][IPI_CONFIG] core=%d, pmu_counter_cnt=%d\n", cpu, cnt_num);
		pr_debug("[MET_PMU][IPI_CONFIG] core=%d, pmu_counter_cnt=%d\n", cpu, cnt_num);

		MET_TRACE("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d, in_interrupt()=%lu\n", sspm_buf_available, in_interrupt());
		pr_debug("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d, in_interrupt()=%lu\n", sspm_buf_available, in_interrupt());

		if (sspm_buf_available == 1) {
			ret = met_ipi_to_sspm_command((void *) ipi_buf, 0, &rdata, 1);
		}

		for (ii = 0; ii < 4; ii++)
			ipi_buf[ii] = 0;

		if (per_cpu(pevent, cpu)[0]) {
			hwc = &(per_cpu(pevent, cpu)[0]->hw);
			base_offset = hwc->idx-1;
		} else {
			base_offset = 0;
		}

		ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_BASE_OFFSET;
		ipi_buf[1] = (cpu << 16) | (base_offset & 0xffff);

		MET_TRACE("[MET_PMU][IPI_CONFIG] core=%d, base offset set to %lu\n", cpu, base_offset);
		pr_debug("[MET_PMU][IPI_CONFIG] core=%d, base offset set to %lu\n", cpu, base_offset);

		if (sspm_buf_available == 1) {
			ret = met_ipi_to_sspm_command((void *) ipi_buf, 0, &rdata, 1);
		}

		if (cycle_count_mode_enabled(cpu)) {

			for (ii = 0; ii < 4; ii++)
				ipi_buf[ii] = 0;

			ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_CYCCNT_ENABLE;
			ipi_buf[1] = cpu & 0xffff;

			MET_TRACE("[MET_PMU][IPI_CONFIG] core=%d, pmu cycle cnt enable\n", cpu);
			pr_debug("[MET_PMU][IPI_CONFIG] core=%d, pmu cycle cnt enable\n", cpu);

			if (sspm_buf_available == 1) {
				ret = met_ipi_to_sspm_command((void *) ipi_buf, 0, &rdata, 1);
			}
		}
	}
}

static int __is_perf_event_hw_slot_seq_order(int cpu) {

	struct hw_perf_event *hwc, *hwc_prev;
	int event_count = cpu_pmu->event_count[cpu];
	int ii;

	/*
	 * perf-event descriptor list would not have any hole
	 * (excepts special 0xff, which will always be the last element)
	 */
	if (per_cpu(pevent, cpu)[0] == NULL)
		return 1;

	/*
	 * XXX: no need to check the last slot,
	 *      which is reserved for 0xff
	 */
	for (ii = 1; ii < event_count - 1; ii++) {

		if (per_cpu(pevent, cpu)[ii] == NULL)
			return 1;

		hwc = &(per_cpu(pevent, cpu)[ii]->hw);
		hwc_prev = &(per_cpu(pevent, cpu)[ii-1]->hw);

		if (hwc->idx != hwc_prev->idx + 1)
			return 0;
	}

	return 1;
}

static int __validate_sspm_compatibility(void) {

	int cpu;

	for_each_possible_cpu(cpu) {

		if (!__is_perf_event_hw_slot_seq_order(cpu)) {
			MET_TRACE("[MET_PMU] pmu not sequentially allocated on cpu %d\n"
				  ,cpu);
			pr_debug("[MET_PMU] pmu not sequentially allocated on cpu %d\n"
				 ,cpu);
			return -1;
		}
	}

	return 0;
}

static void sspm_pmu_unique_start(void) {

	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_unique_start();

	if (met_cpupmu.ondiemet_mode == 1) {
		if (__validate_sspm_compatibility() == -1) {
			MET_TRACE("[MET_PMU] turned off sspm side polling\n");
			pr_debug("[MET_PMU] turned off sspm side polling\n");
			/* return without sending init IPIs, leaving sspm side to poll nothing */
			return;
		}
	}

	ipi_config_pmu_counter_cnt();
}

static void sspm_pmu_unique_stop(void)
{
	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_unique_stop();
	return;
}

static void sspm_pmu_stop(void)
{
	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_stop();
}

static const char sspm_pmu_header[] = "met-info [000] 0.0: pmu_sampler: sspm\n";

static int sspm_pmu_print_header(char *buf, int len)
{
	int ret;

	ret = snprintf(buf, len, sspm_pmu_header);

	if (met_cpupmu.ondiemet_mode == 1)
		ret += cpupmu_print_header(buf + ret, len - ret);

	return ret;
}

static int sspm_pmu_process_argument(const char *arg, int len)
{
	if (met_cpupmu.ondiemet_mode == 1) {

		if (!cpu_pmu->pmu_read_clear_overflow_flag) {
			MET_TRACE("[MET_PMU] cpu_pmu->pmu_read_clear_overflow_flag=NULL, "
				  "pmu on sspm was not supported on this platform\n");
			pr_debug("[MET_PMU] cpu_pmu->pmu_read_clear_overflow_flag=NULL, "
				 "pmu on sspm was not supported on this platform\n");
			return -EINVAL;
		}

		return cpupmu_process_argument(arg, len);
	}
	return 0;
}
#endif /* end of #if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT) */
#endif /* end of #if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) */

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
	.process_argument = cpupmu_process_argument,
	.cpu_state_notify = cpupmu_cpu_state_notify,
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	.ondiemet_mode = 1,
	.ondiemet_start = sspm_pmu_start,
	.uniq_ondiemet_start = sspm_pmu_unique_start,
	.uniq_ondiemet_stop = sspm_pmu_unique_stop,
	.ondiemet_stop = sspm_pmu_stop,
	.ondiemet_print_header = sspm_pmu_print_header,
	.ondiemet_process_argument = sspm_pmu_process_argument
#endif
#endif
};
