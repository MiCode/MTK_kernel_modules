// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/perf_event.h>
#include <asm/sysreg.h>

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
#include <linux/platform_device.h>
#include <linux/perf/arm_pmu.h>
#endif

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/irqreturn.h>
#include <linux/irq_work.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "cpu_pmu.h"
#include "mtk_typedefs.h"
#include "core_plf_init.h"

#ifdef MET_TINYSYS
#include "tinysys_mgr.h" /* for ondiemet_module */

#ifdef MET_SSPM
#include "tinysys_sspm.h"
#include "sspm_met_ipi_handle.h"
#endif /*MET_SSPM*/

#ifdef MET_MCUPM
#include "tinysys_mcupm.h"
#include "mcupm_met_ipi_handle.h"
#endif/*MET_MCUPM*/

#endif /*MET_TINYSYS*/

struct cpu_pmu_hw *cpu_pmu;
static int counter_cnt[NR_CPUS];
static int nr_arg[NR_CPUS];
static int nr_ignored_arg[NR_CPUS];

int met_perf_cpupmu_status;

static int mtk_pmu_event_enable = 0;

/*
 * XXX: in perf_events framework,
 *      PMU hardware slots and `hw_perf_event::idx' was not directly mapped.
 *      Following table was the mapping used in kernel 4.14:
 *
 *      | idx | hardware slot |
 *      |-----+---------------|
 *      |   0 | pmccntr_el0   |
 *      |   1 | 0             |
 *      |   2 | 1             |
 *      |   3 | 2             |
 *      |   4 | 3             |
 *      |   5 | 4             |
 *      |   6 | 5             |
 *
 *      We used following variables to describe this mapping relation,
 *      with init values determined by the relation table listed above.
 *      They're exported as file nodes to serve urgent needs.
 */
static int met_perf_event_cyc_cnt_evt_idx = 0;
static int met_perf_event_evt_idx_offset = -1;


/*
 * Option `handle_irq_selective':
 * controls whether we selectively mask perf_events framework's
 * conter overflow interference.
 *
 * this option was defaulted to 1 since met_drv_v3.
 */
static int handle_irq_selective = 1;

/*
 * Option `override_handle_irq':
 * controls whether we suppress perf_events' overflow handling,
 * which repositions overflowed counters (usually =+ 0x7fffffff),
 * by overriding it's irq handler.
 *
 * this option was defaulted to true for backward compatibility.
 */
static int override_handle_irq = 1;

/*
 * Option `pmu_use_alloc_bitmap':
 * when enabled, use an allocation bitmap to denote polled pmu
 * register indices. when disabled, use (start index, length) pair.
 * this option does not do anything when ondiemet_mode is 0.
 *
 * for example, when pmu register 0/3/4 is allocated, the bitmap
 * will be 0x19.
 * at this moment, up to 32 pmu registers is supported.
 *
 * this option was introduced and defaulted to 1 since met_drv_v3.
 */
static int pmu_use_alloc_bitmap = 1;

/*
 * Option `dbg_annotate_cnt_val':
 * when enabled, this driver force write all pmu register's counter
 * to its event id, which would later used for offline validation order
 * debugging purpose.
 *
 * this intrusive option renders all pmu counters unusable for
 * performance analysis.
 * don't enable this option unless you're conducting a test or
 * verificaction

 * obviously, this option should always be defaulted to 0.
 */

static void pmu_pmcr_read(void *data) {
	int *cpu_pmu_num = data;
    int core_id = smp_processor_id(); /*0~max cpu*/
    /*u32 i = read_sysreg(pmcr_el0);*/
    if (core_id < NR_CPUS)
    	*(cpu_pmu_num + core_id) = (read_sysreg(pmcr_el0) >> ARMV8_PMU_PMCR_N_SHIFT) & ARMV8_PMU_PMCR_N_MASK;
    /*PR_BOOTMSG("[eric debug] core_id=%d, pmcr_el0=%d\n", core_id, i);*/
}

ssize_t pmu_count_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf)
{
	int cpu;
	int ret = 0;
	int cpu_pmu_num[NR_CPUS] = {0}; /*read from pmcr_el0*/

	for(cpu=0; cpu<NR_CPUS; cpu++)
	{
		cpu_pmu_num[cpu] = 0;
	}
#if 0
	for_each_possible_cpu(cpu) /*for_each_possible_cpu is not used  */
	{
		cpu_pmu_num[cpu] = (read_sysreg(pmcr_el0) >> ARMV8_PMU_PMCR_N_SHIFT) & ARMV8_PMU_PMCR_N_MASK;
	}
#else
    for_each_online_cpu(cpu) {
    	smp_call_function_single(cpu, pmu_pmcr_read, cpu_pmu_num, 1);
    }
#endif
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "perf_num_counters: %d\n", perf_num_counters());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "read from pmcr_el10\n");

	for(cpu=0; cpu<NR_CPUS; cpu++)
	{
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "cpu_%d:%d\n",cpu,cpu_pmu_num[cpu]);
	}

	return strlen(buf);
}

static int dbg_annotate_cnt_val = 0;

#define __met_perf_event_is_cyc_cnt_idx(idx) \
	((idx) == met_perf_event_cyc_cnt_evt_idx)
#define __met_perf_event_idx_to_pmu_idx(idx) \
	((idx) + met_perf_event_evt_idx_offset)
#define __met_pmu_idx_to_perf_event_idx(idx) \
	((idx) - met_perf_event_evt_idx_offset)

static struct kobject *kobj_cpu;
DECLARE_KOBJ_ATTR_INT(mtk_pmu_event_enable, mtk_pmu_event_enable);

DECLARE_KOBJ_ATTR_INT(perf_event_cyc_cnt_evt_idx, met_perf_event_cyc_cnt_evt_idx);
DECLARE_KOBJ_ATTR_INT(perf_event_evt_idx_offset, met_perf_event_evt_idx_offset);

DECLARE_KOBJ_ATTR_INT(handle_irq_selective, handle_irq_selective);
DECLARE_KOBJ_ATTR_INT(override_handle_irq, override_handle_irq);

DECLARE_KOBJ_ATTR_INT(pmu_use_alloc_bitmap, pmu_use_alloc_bitmap);

DECLARE_KOBJ_ATTR_INT(dbg_annotate_cnt_val, dbg_annotate_cnt_val);
DECLARE_KOBJ_ATTR_RO(pmu_count);




#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(mtk_pmu_event_enable); \
		KOBJ_ATTR_ITEM(perf_event_cyc_cnt_evt_idx); \
		KOBJ_ATTR_ITEM(perf_event_evt_idx_offset); \
		KOBJ_ATTR_ITEM(handle_irq_selective); \
		KOBJ_ATTR_ITEM(override_handle_irq); \
		KOBJ_ATTR_ITEM(pmu_use_alloc_bitmap); \
		KOBJ_ATTR_ITEM(dbg_annotate_cnt_val); \
		KOBJ_ATTR_ITEM(pmu_count); \
	} while (0)

#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
DEFINE_MUTEX(handle_irq_lock);
int armpmu_irq_hdlr_cnt;
struct armpmu_handle_irq armpmu_irq_hdlr[MX_CPU_CLUSTER];
#endif
#endif

#if IS_ENABLED(CONFIG_CPU_PM)
static int use_cpu_pm_pmu_notifier = 0;

/* helper notifier for maintaining pmu states before cpu state transition */
static int cpu_pm_pmu_notify(struct notifier_block *b,
			     unsigned long cmd,
			     void *p)
{
	unsigned int cpu;
	int ii, count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (!met_perf_cpupmu_status)
		return NOTIFY_OK;

	cpu = raw_smp_processor_id();

	switch (cmd) {
	case CPU_PM_ENTER:
		count = cpu_pmu->polling(cpu_pmu->pmu[cpu],
					 cpu_pmu->event_count[cpu], pmu_value);
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

struct pmu_perf_data {
	unsigned long long perfCurr[MXNR_PMU_EVENTS];
	unsigned long long perfPrev[MXNR_PMU_EVENTS];
	int perfCntFirst[MXNR_PMU_EVENTS];
	struct perf_event *pevent[MXNR_PMU_EVENTS];
	/*
	* TODO: pevent_attr's idx does not map to per-cpu pevent due to the
	*       following constraint:
	*          allocation of pmu register id is bound after pevent_attr
	*          is determined.
	* this does not affect anythings now as pevent_attr will never be
	* accessed after inited.
	*/
	struct perf_event_attr pevent_attr[MXNR_PMU_EVENTS];
	int init_failed_cnt;
	struct pmu_failed_desc init_failed_pmus[MXNR_PMU_EVENT_BUFFER_SZ];
};
static struct pmu_perf_data __percpu *pmu_perf_data;
static int __percpu *cpu_status;

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
	// MET_GENERAL_PRINT(MET_TRACE, cnt, value);
	MET_TRACE_FORMAT_H(cnt, value);
}

static void dummy_handler(struct perf_event *event, struct perf_sample_data *data,
			  struct pt_regs *regs)
{
	/*
	 * Required as perf_event_create_kernel_counter() requires an overflow handler,
	 * even though all we do is poll.
	 */
}

/*
 * Because there was no EXPORT_SYMBOL_GPL'd perf_events counter reader function
 * defined, we have ported upstream kernel's function `perf_event_read_local' defined
 * in source file kernel/events/core.c (revision 3a9b53b).
 *
 * As we always set counters to pinned=1 and only read them during
 * interrupt contexts (metdevice::type = MET_TYPE_PMU),
 * we have omitted upstream perf_event_read_local's checking of ...
 *     1. inherit bit
 *     2. per-task event conditions
 *     3. per-cpu event conditions (e.g., core id check)
 *     4. pinned event conditions (e.g., core id check)
 *     5. oncpu status
 *
 * As parameters enabled/running were used only for returning enable/running
 * time to caller, we just ignored them in function body.
 */
static int __met_perf_event_read_local(struct perf_event *event, u64 *value,
			  u64 *enabled, u64 *running)
{
	unsigned long flags;
	int ret = 0;

	/*
	 * Disabling interrupts avoids all counter scheduling (context
	 * switches, timer based rotation and IPIs).
	 */
	local_irq_save(flags);

	event->pmu->read(event);
	*value = local64_read(&event->count);

	local_irq_restore(flags);

	return ret;
}

static void perf_cpupmu_polling(unsigned long long stamp, unsigned int cpu)
{
	int			event_count = cpu_pmu->event_count[cpu];
	struct met_pmu		*pmu = cpu_pmu->pmu[cpu];
	int			i, count;
	unsigned long long	delta;
	struct perf_event	*ev;
	unsigned int		pmu_value[MXNR_PMU_EVENTS];
	u64 value;
	int ret;

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == MODE_DISABLED)
			continue;

		ev = per_cpu_ptr(pmu_perf_data, cpu)->pevent[i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {

			ret = __met_perf_event_read_local(ev, &value, NULL, NULL);
			if (ret < 0) {
				PR_BOOTMSG_ONCE("[MET_PMU] perf_event_read_local fail (ret=%d)\n", ret);
				pr_debug("[MET_PMU] perf_event_read_local fail (ret=%d)\n", ret);
				continue;
			}

			per_cpu_ptr(pmu_perf_data, cpu)->perfCurr[i] = value;
			delta = (per_cpu_ptr(pmu_perf_data, cpu)->perfCurr[i] - per_cpu_ptr(pmu_perf_data, cpu)->perfPrev[i]);
			per_cpu_ptr(pmu_perf_data, cpu)->perfPrev[i] = per_cpu_ptr(pmu_perf_data, cpu)->perfCurr[i];
			if (per_cpu_ptr(pmu_perf_data, cpu)->perfCntFirst[i] == 1) {
				/* we shall omit delta counter when we get first counter */
				per_cpu_ptr(pmu_perf_data, cpu)->perfCntFirst[i] = 0;
				continue;
			}
			pmu_value[count] = (unsigned int)delta;
			count++;
		}
	}

	if (count == counter_cnt[cpu])
		mp_cpu(count, pmu_value);
}

static struct perf_event* perf_event_create(int cpu, unsigned short event, int idx)
{
	struct perf_event_attr	*ev_attr;
	struct perf_event	*ev;

	/* XXX: idx does not map to pmu_perf_data::pevent */
	ev_attr = per_cpu_ptr(pmu_perf_data, cpu)->pevent_attr + idx;
	memset(ev_attr, 0, sizeof(*ev_attr));
	if (event == 0xff || event == 0x11) {
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
	if (!ev || IS_ERR(ev))
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

#ifdef MET_TINYSYS
#define	PMU_OVERFLOWED_MASK	0xffffffff

static inline int pmu_has_overflowed(u32 pmovsr)
{
	return pmovsr & PMU_OVERFLOWED_MASK;
}

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
static irqreturn_t handle_irq_selective_ignore_overflow(struct arm_pmu *pmu)
{
	unsigned int cpu;
	int ii, idx, is_cyc_cnt, event_count;
	struct met_pmu *metpmu;
	struct perf_event *ev;

	cpu = raw_smp_processor_id();

	metpmu = cpu_pmu->pmu[cpu];
	event_count = cpu_pmu->event_count[cpu];

	for (ii = 0; ii < event_count; ii++) {
		ev = per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii];

		if (metpmu[ii].mode != MODE_DISABLED) {
			idx = __met_perf_event_idx_to_pmu_idx(ev->hw.idx);
			is_cyc_cnt = __met_perf_event_is_cyc_cnt_idx(ev->hw.idx);
			if (is_cyc_cnt) {
				cpu_pmu->disable_cyc_intr();
			} else {
				cpu_pmu->disable_intr(idx);
			}
		}
	}

	for (ii = 0; ii < armpmu_irq_hdlr_cnt; ii++) {
		if (cpumask_test_cpu(cpu, &armpmu_irq_hdlr[ii].supported_cpus))
			return armpmu_irq_hdlr[ii].handle_irq_orig(pmu);
	}

	return IRQ_NONE;
}

static irqreturn_t handle_irq_ignore_overflow(struct arm_pmu *pmu)
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

static struct perf_event * __met_perf_events_set_event(unsigned int cpu, unsigned short event, unsigned int idx)
{
	struct perf_event	*ev;
#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
	struct arm_pmu *armpmu;
	irqreturn_t (*handle_irq_fptr)(struct arm_pmu *pmu);
#endif
#endif

	ev = perf_event_create(cpu, event, idx);

	if (ev == NULL) {

		PR_BOOTMSG("[MET_PMU] cpu=%d online=%d failed to register event %#04x\n",
			   cpu, cpu_online(cpu), event);
		pr_notice("[MET_PMU] cpu=%d online=%d failed to register event %#04x\n",
			  cpu, cpu_online(cpu), event);

		return NULL;
	} else if (__met_perf_event_is_cyc_cnt_idx(ev->hw.idx)) {

		/* XXX: we always place cycle count event onto last slot */
		idx = cpu_pmu->event_count[cpu] - 1;

		pr_debug("[MET_PMU] cpu %d registered cycle count evt=%#04x, "
			 "perf_events id: %d\n",
			 cpu, event, ev->hw.idx);
	} else {
		pr_debug("[MET_PMU] cpu %d registered in pmu slot: [%d] evt=%#04x\n",
			 cpu, __met_perf_event_idx_to_pmu_idx(ev->hw.idx), event);
	}

	per_cpu_ptr(pmu_perf_data, cpu)->pevent[idx] = ev;
	per_cpu_ptr(pmu_perf_data, cpu)->perfPrev[idx] = 0;
	per_cpu_ptr(pmu_perf_data, cpu)->perfCurr[idx] = 0;
	perf_event_enable(ev);
	per_cpu_ptr(pmu_perf_data, cpu)->perfCntFirst[idx] = 1;

#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
	if (met_cpupmu.ondiemet_mode && override_handle_irq) {
		handle_irq_fptr = handle_irq_selective ?
			handle_irq_selective_ignore_overflow :
			handle_irq_ignore_overflow;
		armpmu = container_of(ev->pmu, struct arm_pmu, pmu);
		mutex_lock(&handle_irq_lock);
		if (armpmu && armpmu->handle_irq != handle_irq_fptr) {
			pr_debug("[MET_PMU] replaced original handle_irq=%p with dummy function\n",
				    armpmu->handle_irq);
			armpmu_irq_hdlr[armpmu_irq_hdlr_cnt].supported_cpus = armpmu->supported_cpus;
			armpmu_irq_hdlr[armpmu_irq_hdlr_cnt].handle_irq_orig = armpmu->handle_irq;
			armpmu_irq_hdlr_cnt++;
			armpmu->handle_irq = handle_irq_fptr;
		}
		mutex_unlock(&handle_irq_lock);
	}
#endif
#endif

	return ev;
}

static void met_perf_cpupmu_start(int cpu)
{
	if (met_cpupmu.mode == 0)
		return;
}

static void perf_thread_down(unsigned int cpu)
{
	int			i, j;
	struct perf_event	*ev;
	int			event_count;
	struct met_pmu		*pmu;
#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
	struct arm_pmu *armpmu;
	irqreturn_t (*handle_irq_fptr)(struct arm_pmu *pmu);
#endif
#endif

	event_count = cpu_pmu->event_count[cpu];
	pmu = cpu_pmu->pmu[cpu];
	for (i = 0; i < event_count; i++) {
		ev = per_cpu_ptr(pmu_perf_data, cpu)->pevent[i];
		if (ev != NULL) {

#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
			if (met_cpupmu.ondiemet_mode && override_handle_irq) {
				handle_irq_fptr = handle_irq_selective ?
					handle_irq_selective_ignore_overflow :
					handle_irq_ignore_overflow;
				armpmu = container_of(ev->pmu, struct arm_pmu, pmu);
				mutex_lock(&handle_irq_lock);
				if (armpmu && armpmu->handle_irq == handle_irq_fptr) {
					for (j = 0; j < armpmu_irq_hdlr_cnt; j++) {
						if (cpumask_test_cpu(cpu, &armpmu_irq_hdlr[j].supported_cpus)) {
							pr_debug("[MET_PMU] restore original handle_irq=%p\n",
								armpmu_irq_hdlr[j].handle_irq_orig);
							armpmu->handle_irq = armpmu_irq_hdlr[j].handle_irq_orig;
							armpmu_irq_hdlr[j].handle_irq_orig = NULL;
							break;
						}
					}
				}
				mutex_unlock(&handle_irq_lock);
			}
#endif
#endif
			perf_event_release(cpu, ev);
			per_cpu_ptr(pmu_perf_data, cpu)->pevent[i] = NULL;
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

	pmu_perf_data = alloc_percpu(typeof(*pmu_perf_data));
	if (!pmu_perf_data) {
		PR_BOOTMSG("[MET_PMU] percpu pmu_perf_data allocate fail\n");
		pr_debug("[MET_PMU] percpu pmu_perf_data allocate fail\n");
		/* don't return fail here, we don't break mounting process */
	}

	cpu_status = alloc_percpu(typeof(*cpu_status));
	if (!cpu_status) {
		PR_BOOTMSG("percpu cpu_status allocate fail\n");
		pr_debug("percpu cpu_status allocate fail\n");
	}

	return 0;
}

static void cpupmu_delete_subfs(void)
{
	if (pmu_perf_data) {
		free_percpu(pmu_perf_data);
	}
	if (cpu_status) {
		free_percpu(cpu_status);
	}

#define KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_cpu, &attr_name ## _attr.attr)

	if (kobj_cpu != NULL) {
		KOBJ_ATTR_LIST;
		kobj_cpu = NULL;
	}
#undef  KOBJ_ATTR_ITEM
}

void met_perf_cpupmu_polling(unsigned long long stamp, int _cpu_)
{
	unsigned int cpu = _cpu_;
	int count;
	unsigned int pmu_value[MXNR_PMU_EVENTS];

	if (*per_cpu_ptr(cpu_status, cpu) != MET_CPU_ONLINE)
		return;

	if (met_cpu_pmu_method) {
		if (!pmu_perf_data) {
			return;
		}

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

#ifdef MET_TINYSYS
#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
static void __annotate_allocated_pmu_counter(unsigned int cpu) {

	struct perf_event *ev;
	struct met_pmu *metpmu;
	struct arm_pmu *armpmu = NULL;
	int event_count, ii, first;

	event_count = cpu_pmu->event_count[cpu];
	for (ii = 0, first = 1; ii < event_count; ii++) {

		metpmu = cpu_pmu->pmu[cpu];

		if (metpmu[ii].mode == MODE_DISABLED) {
			continue;
		}

		if (met_cpu_pmu_method) {
			ev = per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii];
			if (!ev) {
				pr_debug("(cpu=%d, idx=%d): perf_event handle was null\n",
					 cpu, ii);
				continue;
			}

			armpmu = to_arm_pmu(ev->pmu);
			if (!armpmu || !armpmu->write_counter || !armpmu->stop) {
				pr_debug("(cpu=%d, idx=%d): armpmu=%p, write_counter=%p, stop=%p\n",
					 cpu, ii, armpmu, armpmu?armpmu->write_counter:NULL, armpmu?armpmu->stop:NULL);
				continue;
			}

			/* force stop counting of all pmu regs on current cpu */
			armpmu->stop(armpmu);
			/* force write its event id to pmu counter reg */
			armpmu->write_counter(ev, metpmu[ii].event);

			pr_debug("(cpu=%d, idx=%d): pmu counter force set to 0x%x\n",
				 cpu, ii, metpmu[ii].event);
		} else {
			if (first) {
				/* disable counting/interrupts of all registers */
				cpu_pmu->stop(event_count);
				first = 0;
			}
			if (!cpu_pmu->write_counter) {
				pr_debug("(cpu=%d, idx=%d): write_counter not implemented\n");
				continue;
			}
			cpu_pmu->write_counter(ii, metpmu[ii].event,
					       ii == event_count-1);
		}
	}
}
#endif
#endif

static int reset_driver_stat(void)
{
	int		cpu, i;
	int		event_count;
	struct met_pmu	*pmu;

	met_cpupmu.mode = 0;
	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		counter_cnt[cpu] = 0;
		nr_arg[cpu] = 0;
		nr_ignored_arg[cpu] = 0;
		for (i = 0; i < event_count; i++) {
			pmu[i].mode = MODE_DISABLED;
			pmu[i].event = 0;
			pmu[i].freq = 0;
		}
		if (pmu_perf_data) {
			per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt = 0;
		}
	}

	armpmu_irq_hdlr_cnt = 0;

	return 0;
}

static void cpupmu_start(void)
{
	unsigned int cpu = raw_smp_processor_id();

	if (!pmu_perf_data || !cpu_status) {
		MET_TRACE("percpu pmu_perf_data/cpu_status allocate fail\n");
		reset_driver_stat();
		return;
	}

	if (!met_cpu_pmu_method) {
		nr_arg[cpu] = 0;
		cpu_pmu->start(cpu_pmu->pmu[cpu], cpu_pmu->event_count[cpu]);

		met_perf_cpupmu_status = 1;
		*per_cpu_ptr(cpu_status, cpu) = MET_CPU_ONLINE;
	}

	pr_debug("dbg_annotate_cnt_val = %d\n", dbg_annotate_cnt_val);
	if (dbg_annotate_cnt_val) {
#if defined(MET_TINYSYS) && (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
		__annotate_allocated_pmu_counter(cpu);
#else
		pr_debug("__annotate_allocated_pmu_counter not implemented, ignored\n");
#endif
	}
}

static void __dump_perf_event_info(unsigned int cpu)
{
	int ii, idx;
	struct perf_event *ev;
	struct met_pmu *pmu = cpu_pmu->pmu[cpu];

	for (ii = 0; ii < cpu_pmu->event_count[cpu]; ii++) {

		if (pmu[ii].mode == MODE_DISABLED) {
			continue;
		}

		ev = per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii];
		if (!ev || (ev->state != PERF_EVENT_STATE_ACTIVE)) {
			continue;
		}

		if (__met_perf_event_is_cyc_cnt_idx(ev->hw.idx)) {

			/* XXX: we always place cycle count event onto last slot */
			idx = cpu_pmu->event_count[cpu] - 1;

			MET_TRACE("[MET_PMU] cpu %d registered cycle count evt=%#04x, "
				  "perf_events id: %d\n", cpu, pmu[ii].event, ev->hw.idx);
			pr_debug("[MET_PMU] cpu %d registered cycle count evt=%#04x, "
				 "perf_events id: %d\n", cpu, pmu[ii].event, ev->hw.idx);
		} else {
			MET_TRACE("[MET_PMU] cpu %d registered in pmu slot: [%d] evt=%#04x\n",
				  cpu, __met_perf_event_idx_to_pmu_idx(ev->hw.idx),
				  pmu[ii].event);
			pr_debug("[MET_PMU] cpu %d registered in pmu slot: [%d] evt=%#04x\n",
				 cpu, __met_perf_event_idx_to_pmu_idx(ev->hw.idx),
				 pmu[ii].event);
		}
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
			if (cpu<0 || cpu>=NR_CPUS)
				continue;

			__dump_perf_event_info(cpu);
		}

		for_each_possible_cpu(cpu) {
			if (cpu<0 || cpu>=NR_CPUS)
				continue;

			met_perf_cpupmu_start(cpu);

			met_perf_cpupmu_status = 1;
			*per_cpu_ptr(cpu_status, cpu) = MET_CPU_ONLINE;
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
	return SNPRINTF(buf, PAGE_SIZE, help, cpu_pmu->cpu_name);
}

#ifdef MET_TINYSYS
static int cycle_count_mode_enabled(unsigned int cpu) {

	int event_cnt;
	struct met_pmu	*pmu;

	pmu = cpu_pmu->pmu[cpu];
	event_cnt = cpu_pmu->event_count[cpu];
	return pmu[event_cnt-1].mode == MODE_POLLING;
}

static int __is_pmu_regular_reg_allocated(unsigned int cpu, int hw_idx)
{
	int ii;
	int event_count;
	struct met_pmu	*pmu;
	struct hw_perf_event *hwc;

	event_count = cpu_pmu->event_count[cpu];

	/* bound checking */
	if (hw_idx >= (event_count - 1) || hw_idx < 0) {
		return 0;
	}

	pmu = cpu_pmu->pmu[cpu];

	if (met_cpu_pmu_method) {
		/* no need to check cycle count, thus use event_count-1 */
		for (ii = 0; ii < event_count - 1; ii ++) {

			if (pmu[ii].mode == MODE_DISABLED ||
			    per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii] == NULL)
				continue;

			hwc = &(per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii]->hw);
			if (hw_idx == __met_perf_event_idx_to_pmu_idx(hwc->idx)) {
				return 1;
			}
		}
	} else {
		if (pmu[hw_idx].mode != MODE_DISABLED) {
			return 1;
		}
	}

	return 0;
}

static int __pmu_event_on_hw_idx(unsigned int cpu, int hw_idx)
{
	int ii;
	int event_count;
	struct met_pmu	*pmu;
	struct hw_perf_event *hwc;

	event_count = cpu_pmu->event_count[cpu];

	/* bound checking */
	if (hw_idx >= (event_count - 1) || hw_idx < 0) {
		return 0;
	}

	pmu = cpu_pmu->pmu[cpu];

	if (met_cpu_pmu_method) {
		/* no need to check cycle count, thus use event_count-1 */
		for (ii = 0; ii < event_count - 1; ii ++) {

			if (pmu[ii].mode == MODE_DISABLED ||
			    per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii] == NULL)
				continue;

			hwc = &(per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii]->hw);
			if (hw_idx == __met_perf_event_idx_to_pmu_idx(hwc->idx)) {
				return pmu[ii].event;
			}
		}
	} else {
		if (pmu[hw_idx].mode != MODE_DISABLED) {
			return pmu[hw_idx].event;
		}
	}

	/* just return a dummy value when not found */
	return 0;
}
#endif

static int cpupmu_print_header(char *buf, int len)
{
	int		cpu, i, ret, first;
	int		event_count;
	struct met_pmu	*pmu;
	struct pmu_failed_desc *failed_pmu_ptr;

	ret = 0;

	/*append CPU PMU access method*/
	if (met_cpu_pmu_method)
		ret += SNPRINTF(buf + ret, len,
			"met-info [000] 0.0: CPU_PMU_method: perf APIs\n");
	else
		ret += SNPRINTF(buf + ret, len,
			"met-info [000] 0.0: CPU_PMU_method: MET pmu driver\n");

	/*append cache line size*/
	ret += SNPRINTF(buf + ret, len - ret, cache_line_header, cache_line_size());
	ret += SNPRINTF(buf + ret, len - ret, "# mp_cpu: pmu_value1, ...\n");

	/*
	 * print error message when user requested more pmu events than
	 * platform's capability.
	 * we currently only prompt how many events were ignored.
	 */
	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		if (nr_ignored_arg[cpu]) {
			ret += SNPRINTF(buf + ret,
					len - ret,
					"met-info [000] 0.0: ##_PMU_INIT_FAIL: "
					"too many events requested on CPU %d (max = %d+1), %d events ignored\n",
					cpu, cpu_pmu->event_count[cpu]-1, nr_ignored_arg[cpu]);
		}
	}

	/*
	 * print error message of init failed events due to lack of
	 * hardware register slots, it usually happened when they were occupied
	 * by other perf_events users
	 */
	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;

		for (i = 0; i < per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt; i++) {

			failed_pmu_ptr = per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus + i;

			if (failed_pmu_ptr->init_failed != PMU_INIT_FAIL_CPU_OFFLINE)
				continue;

			if (first) {
				ret += SNPRINTF(buf + ret,
						len - ret,
						"met-info [000] 0.0: ##_PMU_INIT_FAIL: "
						"CPU %d offline, unable to allocate following PMU event(s) 0x%x",
						cpu, failed_pmu_ptr->event);
				first = 0;
				continue;
			}

			ret += SNPRINTF(buf + ret, len - ret, ",0x%x", failed_pmu_ptr->event);
		}
		if (!first && per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt >=
		    ARRAY_SIZE(per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus))
			ret += SNPRINTF(buf + ret, len - ret,
					"... (truncated if there's more)");
		if (!first)
			ret += SNPRINTF(buf + ret, len - ret, "\n");
	}

	/*
	 * print error message of init failed events due cpu offline
	 */
	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;

		for (i = 0; i < per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt; i++) {

			failed_pmu_ptr = per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus + i;

			if (failed_pmu_ptr->init_failed != PMU_INIT_FAIL_OCCUPIED)
				continue;

			if (first) {
				ret += SNPRINTF(buf + ret,
						len - ret,
						"met-info [000] 0.0: ##_PMU_INIT_FAIL: "
						"on CPU %d, no enough PMU register slots to allocate events 0x%x",
						cpu, failed_pmu_ptr->event);
				first = 0;
				continue;
			}

			ret += SNPRINTF(buf + ret, len - ret, ",0x%x", failed_pmu_ptr->event);
		}
		if (!first && per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt >=
		    ARRAY_SIZE(per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus))
			ret += SNPRINTF(buf + ret, len - ret,
					"... (truncated if there's more)");
		if (!first)
			ret += SNPRINTF(buf + ret, len - ret, "\n");
	}

	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		event_count = cpu_pmu->event_count[cpu];
		pmu = cpu_pmu->pmu[cpu];
		first = 1;
#ifdef MET_TINYSYS
		/*
		 * only when bitmap mode is enabled, we print headers in the order of
		 * actual physical register ids
		 * bitmap does not do anything when ondimet_mode=0.
		 */
		if (met_cpupmu.ondiemet_mode && pmu_use_alloc_bitmap) {
			for (i = 0; i < event_count - 1; i++) {
				if (!__is_pmu_regular_reg_allocated(cpu, i))
					continue;

				if (first) {
					ret += SNPRINTF(buf + ret, len - ret, header, cpu);
					first = 0;
				}
				ret += SNPRINTF(buf + ret, len - ret, ",0x%x",
						__pmu_event_on_hw_idx(cpu, i));
			}
			if (pmu[event_count-1].mode != MODE_DISABLED) {
				if (first) {
					ret += SNPRINTF(buf + ret, len - ret, header, cpu);
					first = 0;
				}
				ret += SNPRINTF(buf + ret, len - ret, ",0x%x",
						pmu[event_count-1].event);
			}
			if (!first)
				ret += SNPRINTF(buf + ret, len - ret, "\n");
		}
#else
		pr_debug("sspm not enabled, ignore pmu_use_alloc_bitmap\n");
		MET_TRACE("sspm not enabled, ignore pmu_use_alloc_bitmap\n");
		if (0) {
		}
#endif
		else {
			for (i = 0; i < event_count; i++) {
				if (pmu[i].mode == MODE_DISABLED)
					continue;

				if (first) {
					ret += SNPRINTF(buf + ret, len - ret, header, cpu);
					first = 0;
				}
				ret += SNPRINTF(buf + ret, len - ret, ",0x%x", pmu[i].event);
			}
			if (!first)
				ret += SNPRINTF(buf + ret, len - ret, "\n");
		}
	}

	if (met_cpu_pmu_method) {
		for_each_possible_cpu(cpu) {
			if (cpu<0 || cpu>=NR_CPUS)
				continue;
			/*
			 * XXX: we still need perf_event metadata until here,
			 *      so we postpone runmet_perf_cpupmu_stop until now
			 */
			met_perf_cpupmu_stop(cpu);
		}
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
	struct perf_event *ev;
	char		*arg1 = (char*)arg;
	int		len1 = len;
	int		cpu, cpu_list[NR_CPUS];
	int		nr_events;
	/* overprovision for users input */
	int		event_list[MXNR_PMU_EVENT_BUFFER_SZ] = {};
	int		i;
	int		nr_counters;
	int		offset;
	struct met_pmu	*pmu;
	unsigned int arg_nr;
	int		event_no;
	int		is_cpu_cycle_evt;
	struct pmu_failed_desc *failed_pmu_ptr;

	if (!pmu_perf_data || !cpu_status)
		return 0;

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
		/*
		 * when argument list CONTAINS core id,
		 * e.g.,
		 *     --pmu-cpu-evt=0,1,6:0x2b,0x08,0x16,0x2a,0xff
		 */
		int list[NR_CPUS] = {}, cnt;
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
	else {
		/*
		 * when argument list contains no core id,
		 * e.g.,
		 *     --pmu-cpu-evt=0x2b,0x08,0x16,0x2a,0xff
		 */
		memset(cpu_list, 1, sizeof(cpu_list));
	}

	/* get event_list */
	if ((nr_events = met_parse_num_list(arg1, len1, event_list, ARRAY_SIZE(event_list))) <= 0)
		goto arg_out;

	/* for each cpu in cpu_list, add all the events in event_list */
	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		pmu = cpu_pmu->pmu[cpu];
		/*
		 * restore `nr_arg' from previous iteration,
		 * for cases when certain core's arguments consists more than one clauses
		 * e.g.,
		 *     --pmu-cpu-evt=0:0x2b
		 *     --pmu-cpu-evt=0:0x08
		 *     --pmu-cpu-evt=0:0x16
		 */
		arg_nr = nr_arg[cpu];

		if (cpu_list[cpu] == 0)
			continue;

		nr_counters = cpu_pmu->event_count[cpu];
		pr_debug("[MET_PMU] pmu slot count=%d\n", nr_counters);
		PR_BOOTMSG("[MET_PMU]CPU%d pmu slot count=%d\n",cpu, nr_counters);

		if (nr_counters == 0)
			goto arg_out;

		for (i = 0; i < nr_events; i++) {
			event_no = event_list[i];

			/*
			 * there're three possible cause of a failed pmu allocation:
			 *     1. user asked more events than chip's capability
			 *     2. part of the pmu registers was occupied by other users
			 *     3. the requested cpu has been offline
			 *
			 * (we treat 1 and 2 as different cases for easier trouble shooting)
			 */

			/*
			 * skip duplicate events
			 * not treated as warning/error, this event was already
			 * registered anyway
			 */
			/* check cycle count event */
			if (event_no == 0x11 || event_no == 0xff) {
				/* allocate onto cycle count register (pmccntr_el0) ? */
				if (pmu[nr_counters-1].mode == MODE_POLLING) {
					continue;
				}
				/* allocated onto regular register as 0x11 ? */
				if (cpu_pmu->check_event(pmu, arg_nr, 0x11) < 0) {
					continue;
				}
				/* allocated onto regular register as 0xff ? */
				if (cpu_pmu->check_event(pmu, arg_nr, 0xff) < 0) {
					continue;
				}
			} else if (cpu_pmu->check_event(pmu, arg_nr, event_no) < 0) {
				/*
				 * check regular registers (pmccntr_el0 not checked)
				 */
				continue;
			}

			/*
			 * handle case (1) user asked more events than chip's capability.
			 *
			 * we just ignore it and display warning message in
			 * trace header
			 *
			 * as we removed all duplicate events, so we could never have a
			 * failed cycle counter allocation due to case (1), but only
			 * case (2)
			 */
			if (event_no != 0xff && event_no != 0x11 &&
			    pmu[nr_counters-2].mode != MODE_DISABLED) {
				nr_ignored_arg[cpu] ++;
				continue;
			}

			is_cpu_cycle_evt = 0;
			if (met_cpu_pmu_method) {

				ev = __met_perf_events_set_event(cpu, event_no, arg_nr);

				if (!ev) {
					/* bound protection */
					if (per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt >=
					    ARRAY_SIZE(per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus)) {
						continue;
					}
					failed_pmu_ptr = per_cpu_ptr(pmu_perf_data, cpu)->init_failed_pmus;
					failed_pmu_ptr += per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt;

					/*
					 * handle case (2) part of the pmu registers was
					 * occupied by other users
					 */
					if (cpu_online(cpu)) {
						failed_pmu_ptr->event = event_no;
						failed_pmu_ptr->init_failed = PMU_INIT_FAIL_OCCUPIED;

					}
					/* handle case (3) the requested cpu has been turned offline */
					else {
						failed_pmu_ptr->event = event_no;
						failed_pmu_ptr->init_failed = PMU_INIT_FAIL_CPU_OFFLINE;
					}

					per_cpu_ptr(pmu_perf_data, cpu)->init_failed_cnt ++;

					continue;
				}

				/*
				 * perf_event prefers to allocate cycle count in pmccntr_el0
				 * (reserved as struct perf_event::hw.idx), but when
				 * it was occupied, a regular pmu register is used instead.
				 *
				 * here we identify cycle count/regular event by observing
				 * the value of struct perf_event::hw.idx.
				 *
				 * in cases when event 0xff/0x11 is allocated on regular register,
				 * we treat it as regular register, as cycle counter is special only
				 * when it is allocated on pmccntr_el0.
				 */
				is_cpu_cycle_evt = __met_perf_event_is_cyc_cnt_idx(ev->hw.idx);
			} else {
				/*
				 * there's no actual "failed allocation" in our arm asm
				 * version implementation.
				 */
				is_cpu_cycle_evt = (event_no == 0x11 || event_no == 0xff);
			}

			if (is_cpu_cycle_evt) {
				offset = nr_counters-1;
			} else {
				offset = arg_nr;
				arg_nr++;
			}

			pmu[offset].mode = MODE_POLLING;
			pmu[offset].event = event_no;
			pmu[offset].freq = 0;

			counter_cnt[cpu]++;
		} /* for i: 0 -> nr_events */

		nr_arg[cpu] = arg_nr;

	} /* for_each_possible_cpu(cpu) */

	met_cpupmu.mode = 1;
	return 0;

arg_out:
	reset_driver_stat();
	return -EINVAL;
}

static void cpupmu_cpu_state_notify(long cpu, unsigned long action)
{
	if (!pmu_perf_data || !cpu_status)
		return;

	*per_cpu_ptr(cpu_status, cpu) = action;

#if (IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_ARM))
	if (met_cpu_pmu_method && action == MET_CPU_OFFLINE) {
		struct perf_event *event = NULL;
		struct arm_pmu *armpmu = NULL;
		struct platform_device *pmu_device = NULL;
		int irq = 0;

		if (pmu_perf_data) {
			event = per_cpu_ptr(pmu_perf_data, cpu)->pevent[0];
		}

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

#ifdef MET_TINYSYS
#ifdef MET_SSPM
static void sspm_pmu_start(void)
{
	ondiemet_module[ONDIEMET_SSPM] |= ID_PMU;
}
#endif
#ifdef MET_MCUPM
static void mcupm_pmu_start(void)
{
	ondiemet_module[ONDIEMET_MCUPM] |= ID_PMU;
}
#endif

static void tinysys_pmu_start(void)
{
#ifdef MET_SSPM
	if (met_cpupmu.tinysys_type == 0)
		sspm_pmu_start();
#endif

#ifdef MET_MCUPM
	if (met_cpupmu.tinysys_type == 1)
		mcupm_pmu_start();
#endif

	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_start();
}

static unsigned int __get_pmu_hw_reg_alloc_bitmap(int cpu) {

	int hw_idx, bitmap;

	bitmap = 0;
	for (hw_idx = 0; hw_idx < cpu_pmu->event_count[cpu] - 1 ; hw_idx ++) {
		if (__is_pmu_regular_reg_allocated(cpu, hw_idx)){
			bitmap |= 1 << hw_idx;
		}
	}

	return bitmap;
}

static void ipi_config_pmu_counter_cnt(void) {

	int ret, cpu, ii, cnt_num;
	unsigned int rdata;
	unsigned int ipi_buf[4] = {0, 0, 0, 0};
	struct hw_perf_event *hwc;
	unsigned int base_offset;

	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

		MET_TRACE("core=%d, pmu_use_alloc_bitmap=%d\n",
			  cpu, pmu_use_alloc_bitmap);
		pr_debug("core=%d, pmu_use_alloc_bitmap=%d\n",
			 cpu, pmu_use_alloc_bitmap);

		if (pmu_use_alloc_bitmap) {  /* use allocation bitmap */
			for (ii = 0; ii < 4; ii++)
				ipi_buf[ii] = 0;

			ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_POLLING_BITMAP;
			ipi_buf[1] = cpu & 0xffffffff;
			/*
			 *  XXX: on sspm side, polling of cycle counter was not controlled
			 *       by enum `SET_PMU_POLLING_BITMAP', but an addtional argument
			 *       `SET_PMU_CYCCNT_ENABLE' instead
			 *
			 *  and yes, currently the max supported nuber of pmu register is *32*
			 */
			ipi_buf[2] = __get_pmu_hw_reg_alloc_bitmap(cpu) & 0xffffffff;

			MET_TRACE("core=%d, polling_bitmap=0x%x\n",
				  cpu, ipi_buf[2]);
			pr_debug("core=%d, polling_bitmap=0x%x\n",
				 cpu, ipi_buf[2]);

#ifdef MET_SSPM
			if (met_cpupmu.tinysys_type == 0) {
				if (sspm_buf_available == 1) {
					ret = met_scmi_to_sspm_command((void *) ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
				} else {
					MET_TRACE("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d\n",
						  sspm_buf_available);
					pr_debug("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d\n",
						 sspm_buf_available);

				}
			}
#endif
#ifdef MET_MCUPM
			if (met_cpupmu.tinysys_type == 1) {
				if (mcupm_buf_available == 1) {
					ret = met_ipi_to_mcupm_command((void *) ipi_buf, 0, &rdata, 1);
				} else {
					MET_TRACE("[MET_PMU][IPI_CONFIG] mcupm_buf_available=%d\n",
						  mcupm_buf_available);
					pr_debug("[MET_PMU][IPI_CONFIG] mcupm_buf_available=%d\n",
						 mcupm_buf_available);

				}
			}
#endif

		} else {  /* use (start index, length) pair */
			for (ii = 0; ii < 4; ii++)
				ipi_buf[ii] = 0;

			ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_EVT_CNT;
			/*
			 *  XXX: on sspm side, cycle counter was not counted in
			 *       total event number `counter_cnt', but controlled by
			 *       an addtional argument `SET_PMU_CYCCNT_ENABLE' instead
			 */
			cnt_num = cycle_count_mode_enabled(cpu) ?
				counter_cnt[cpu]-1 : counter_cnt[cpu];
			ipi_buf[1] = (cpu << 16) | (cnt_num & 0xffff);

#ifdef MET_SSPM
			if (met_cpupmu.tinysys_type == 0) {
				if (sspm_buf_available == 1) {
					ret = met_scmi_to_sspm_command((void *) ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
				} else {
					MET_TRACE("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d\n",
						  sspm_buf_available);
					pr_debug("[MET_PMU][IPI_CONFIG] sspm_buf_available=%d\n",
						 sspm_buf_available);
				}
			}
#endif
#ifdef MET_MCUPM
			if (met_cpupmu.tinysys_type == 1) {
				if (mcupm_buf_available == 1) {
					ret = met_ipi_to_mcupm_command((void *) ipi_buf, 0, &rdata, 1);
				} else {
					MET_TRACE("[MET_PMU][IPI_CONFIG] mcupm_buf_available=%d\n",
						  mcupm_buf_available);
					pr_debug("[MET_PMU][IPI_CONFIG] mcupm_buf_available=%d\n",
						 mcupm_buf_available);
				}
			}
#endif
			for (ii = 0; ii < 4; ii++)
				ipi_buf[ii] = 0;

			if (per_cpu_ptr(pmu_perf_data, cpu)->pevent[0]) {
				hwc = &(per_cpu_ptr(pmu_perf_data, cpu)->pevent[0]->hw);
				base_offset = __met_perf_event_idx_to_pmu_idx(hwc->idx);
			} else {
				base_offset = 0;
			}

			ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_BASE_OFFSET;
			ipi_buf[1] = (cpu << 16) | (base_offset & 0xffff);

			MET_TRACE("[MET_PMU][IPI_CONFIG] core=%d, base offset set to %u\n", cpu, base_offset);
			pr_debug("[MET_PMU][IPI_CONFIG] core=%d, base offset set to %u\n", cpu, base_offset);

#ifdef MET_SSPM
			if (met_cpupmu.tinysys_type == 0) {
				if (sspm_buf_available == 1) {
					ret = met_scmi_to_sspm_command((void *) ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
				}
			}
#endif
#ifdef MET_MCUPM
			if (met_cpupmu.tinysys_type == 1) {
				if (mcupm_buf_available == 1) {
					ret = met_ipi_to_mcupm_command((void *) ipi_buf, 0, &rdata, 1);
				}
			}
#endif
		}

		if (cycle_count_mode_enabled(cpu)) {

			for (ii = 0; ii < 4; ii++)
				ipi_buf[ii] = 0;

			ipi_buf[0] = MET_MAIN_ID | (MID_PMU << MID_BIT_SHIFT) | MET_ARGU | SET_PMU_CYCCNT_ENABLE;
			ipi_buf[1] = cpu & 0xffff;

			MET_TRACE("[MET_PMU][IPI_CONFIG] core=%d, pmu cycle cnt enable\n", cpu);
			pr_debug("[MET_PMU][IPI_CONFIG] core=%d, pmu cycle cnt enable\n", cpu);

#ifdef MET_SSPM
			if (met_cpupmu.tinysys_type == 0) {
				if (sspm_buf_available == 1) {
					ret = met_scmi_to_sspm_command((void *) ipi_buf, sizeof(ipi_buf)/sizeof(unsigned int), &rdata, 1);
				}
			}
#endif
#ifdef MET_MCUPM
			if (met_cpupmu.tinysys_type == 1) {
				if (mcupm_buf_available == 1) {
					ret = met_ipi_to_mcupm_command((void *) ipi_buf, 0, &rdata, 1);
				}
			}
#endif
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
	if (per_cpu_ptr(pmu_perf_data, cpu)->pevent[0] == NULL)
		return 1;

	/*
	 * XXX: no need to check the last slot,
	 *      which is reserved for 0xff
	 */
	for (ii = 1; ii < event_count - 1; ii++) {

		/* this condition check also works when met_cpu_pmu_method == 0 */
		if (per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii] == NULL)
			return 1;

		hwc = &(per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii]->hw);
		hwc_prev = &(per_cpu_ptr(pmu_perf_data, cpu)->pevent[ii-1]->hw);

		if (hwc->idx != hwc_prev->idx + 1)
			return 0;
	}

	return 1;
}

static int __validate_sspm_compatibility(void) {

	int cpu;

	for_each_possible_cpu(cpu) {
		if (cpu<0 || cpu>=NR_CPUS)
			continue;

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

static void tinysys_pmu_unique_start(void) {

	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_unique_start();

	if (met_cpupmu.ondiemet_mode == 1) {

                if (!pmu_use_alloc_bitmap &&
		    __validate_sspm_compatibility() == -1) {
			MET_TRACE("[MET_PMU] turned off sspm side polling\n");
			pr_debug("[MET_PMU] turned off sspm side polling\n");
			/* return without sending init IPIs, leaving sspm side to poll nothing */
			return;
		}
	}

	ipi_config_pmu_counter_cnt();
}

static void tinysys_pmu_unique_stop(void)
{
	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_unique_stop();
	return;
}

static void tinysys_pmu_stop(void)
{
	if (met_cpupmu.ondiemet_mode == 1)
		cpupmu_stop();
}

static const char sspm_pmu_header[] = "met-info [000] 0.0: pmu_sampler: sspm\n";
static const char mcupm_pmu_header[] = "met-info [000] 0.0: pmu_sampler: mcupm\n";

static int tinysys_pmu_print_header(char *buf, int len)
{
	int ret = 0;

	if (met_cpupmu.tinysys_type == 0)
	{
		ret = SNPRINTF(buf, len, sspm_pmu_header);
	} else if (met_cpupmu.tinysys_type == 1)  {
		ret = SNPRINTF(buf, len, mcupm_pmu_header);
	}

	if (met_cpupmu.ondiemet_mode == 1)
		ret += cpupmu_print_header(buf + ret, len - ret);

	return ret;
}

static int tinysys_pmu_process_argument(const char *arg, int len)
{
	if (met_cpupmu.ondiemet_mode == 1) {
                if (override_handle_irq) {
                        if (handle_irq_selective) {
                                if (!(cpu_pmu->disable_intr && cpu_pmu->disable_cyc_intr)) {
                                        MET_TRACE("[MET_PMU] missing function disable_intr and disable_cyc_intr, "
                                                  "pmu on sspm was not supported on this platform\n");
                                        pr_debug("[MET_PMU] missing function disable_intr and disable_cyc_intr, "
                                                 "pmu on sspm was not supported on this platform\n");
                                        return -EINVAL;
                                }
                        } else {
                                if (!cpu_pmu->pmu_read_clear_overflow_flag) {
                                        MET_TRACE("[MET_PMU] missing function pmu_read_clear_overflow_flag, "
                                                  "pmu on sspm was not supported on this platform\n");
                                        pr_debug("[MET_PMU] missing function pmu_read_clear_overflow_flag, "
                                                 "pmu on sspm was not supported on this platform\n");
                                        return -EINVAL;
                                }
                        }
                }

                return cpupmu_process_argument(arg, len);
        }
	return 0;
}
#endif /* end of #ifdef MET_TINYSYS */

MET_DEFINE_DEPENDENCY_BY_NAME(dependencies) = {
	{.symbol=(void**)&met_scmi_api_ready, .init_once=0, .cpu_related=1, .ondiemet_mode=1, .tinysys_type=0},
	{.symbol=(void**)&met_sspm_api_ready, .init_once=0, .cpu_related=1, .ondiemet_mode=1, .tinysys_type=0},
	{.symbol=(void**)&met_ipi_api_ready, .init_once=0, .cpu_related=1, .ondiemet_mode=1, .tinysys_type=1},
	{.symbol=(void**)&met_mcupm_api_ready, .init_once=0, .cpu_related=1, .ondiemet_mode=1, .tinysys_type=1},
	{.symbol=(void**)&met_scmi_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=0},
	{.symbol=(void**)&met_sspm_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=0},
	{.symbol=(void**)&met_ipi_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=1},
	{.symbol=(void**)&met_mcupm_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=1, .tinysys_type=1},
};

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
#ifdef MET_TINYSYS
	.ondiemet_mode = 0,
	.tinysys_type = 0,
	.ondiemet_start = tinysys_pmu_start,
	.uniq_ondiemet_start = tinysys_pmu_unique_start,
	.uniq_ondiemet_stop = tinysys_pmu_unique_stop,
	.ondiemet_stop = tinysys_pmu_stop,
	.ondiemet_print_header = tinysys_pmu_print_header,
	.ondiemet_process_argument = tinysys_pmu_process_argument,
#endif
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(dependencies)
};
