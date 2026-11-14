// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <linux/perf_event.h>
#include "met_drv.h"
#include "met_kernel_symbol.h"
#include "interface.h"
#include "trace.h"
#include "smmu_pmu.h"
#include "core_plf_init.h"
#include "mtk_typedefs.h"
#include "mtk-smmu-v3.h"

#if 0
#define DIN()
#define DOUT()
#define DBG(__fmt__, ...)
#else
#undef TAG
#define TAG "[MET_SMMU_PMU]"
#define DIN() \
	do{\
		pr_debug(TAG"[%s][%d] IN", __func__, __LINE__); \
	}while(0)
#define DOUT() \
	do{\
		pr_debug(TAG"[%s][%d] OUT", __func__, __LINE__); \
	}while(0)
#define DBG(__fmt__, ...) \
	do{\
		pr_info(TAG"[%s][%d]" __fmt__, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#endif

struct smmu_mp_func smmu_mp_func_impl[SMMU_TYPE_NUM] = {0};

struct smmu_lmu_hw *lmu_hw[SMMU_TYPE_NUM];
static int counter_cnt[SMMU_TYPE_NUM];
static struct kobject *kobj_lmu[SMMU_TYPE_NUM];
static int nr_arg[SMMU_TYPE_NUM];
static unsigned long long perfCurr[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];
static unsigned long long perfPrev[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];
static int perfCntFirst[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];
static struct perf_event * pevent[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];
static struct perf_event_attr pevent_attr[SMMU_TYPE_NUM][MXNR_SMMU_LMU_EVENTS];

static unsigned int perf_device_type[SMMU_TYPE_NUM]; /* pmu type */

/* collect fail to init events */
static int nr_ignored_arg[SMMU_TYPE_NUM] = {0};
static unsigned int init_failed_cnt[SMMU_TYPE_NUM] = {0};
static struct smmu_lmu_failed_desc init_failed_smmu_lmu[SMMU_TYPE_NUM][MXNR_SMMU_EVENT_BUFFER_SZ];

static struct metdevice *get_metdevice(u32 smmu_id)
{
	struct metdevice *met_dev;

	switch (smmu_id) {
	case MM_SMMU:
		met_dev = &met_mm_smmu_lmu;
		break;
	case APU_SMMU:
		met_dev = &met_apu_smmu_lmu;
		break;
	case SOC_SMMU:
		met_dev = &met_soc_smmu_lmu;
		break;
	case GPU_SMMU:
		met_dev = &met_gpu_smmu_lmu;
		break;
	default:
		met_dev = NULL;
		break;
	}
	return met_dev;
}

static ssize_t perf_type_show(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      char *buf, u32 smmu_id)
{
	if (smmu_id >= SMMU_TYPE_NUM)
		return 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", perf_device_type[smmu_id]);
}

static ssize_t perf_type_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
			       const char *buf, size_t n,
			       u32 smmu_id)
{
	if (smmu_id >= SMMU_TYPE_NUM)
		return 0;

	if (kstrtouint(buf, 0, &perf_device_type[smmu_id]) != 0)
		return -EINVAL;

	return n;
}

static ssize_t perf_type_show_mm(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 char *buf)
{
	return perf_type_show(kobj, attr, buf, MM_SMMU);
}

static ssize_t perf_type_show_apu(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  char *buf)
{
	return perf_type_show(kobj, attr, buf, APU_SMMU);
}

static ssize_t perf_type_show_soc(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  char *buf)
{
	return perf_type_show(kobj, attr, buf, SOC_SMMU);
}

static ssize_t perf_type_show_gpu(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  char *buf)
{
	return perf_type_show(kobj, attr, buf, GPU_SMMU);
}

static ssize_t perf_type_store_mm(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t n)
{
	return perf_type_store(kobj, attr, buf, n, MM_SMMU);
}

static ssize_t perf_type_store_apu(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	return perf_type_store(kobj, attr, buf, n, APU_SMMU);
}

static ssize_t perf_type_store_soc(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	return perf_type_store(kobj, attr, buf, n, SOC_SMMU);
}

static ssize_t perf_type_store_gpu(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	return perf_type_store(kobj, attr, buf, n, GPU_SMMU);
}

static struct kobj_attribute perf_type_attr[SMMU_TYPE_NUM] = {
	__ATTR(perf_type, 0664, perf_type_show_mm, perf_type_store_mm),
	__ATTR(perf_type, 0664, perf_type_show_apu, perf_type_store_apu),
	__ATTR(perf_type, 0664, perf_type_show_soc, perf_type_store_soc),
	__ATTR(perf_type, 0664, perf_type_show_gpu, perf_type_store_gpu),
};

static u32 get_event_id(struct perf_event_attr *attr) {
	return  attr->config;
}

#define PRINT_SMMU_COUNTER(event_id, value) \
	do { \
		switch (event_id) { \
		case EVENT_R_LAT_MAX: \
			MET_TRACE("tcu_miss_latency_max=%u\n", value); \
			break; \
		case EVENT_R_LAT_AVG: \
			MET_TRACE("tcu_miss_latency_avg=%u\n", value); \
			break; \
		case EVENT_TBUS_LAT_AVG: \
			MET_TRACE("tbu_access_latency_avg=%u\n", value); \
			break; \
		case EVENT_TBUS_TRANS_TOT: \
			MET_TRACE("tbu_access_cnt=%u\n", value); \
			break; \
		case EVENT_R_TRANS_TOT: \
			MET_TRACE("tcu_miss_cnt=%u\n", value); \
			break; \
		default: \
			break; \
		} \
	} while (0)

#define PRINT_INTERNAL_COUNTER(event_id, value) \
	do { \
		switch (event_id) { \
		case EVENT_TLB_MISS_RATE: \
			MET_TRACE("tcu_miss_rate=%u\n", value); \
			break; \
		default: \
			break; \
		} \
	} while (0)

static void mm_smmu__latency(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void mm_smmu__tcu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void mm_smmu__tbu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void mm_smmu__hit_miss_rate(unsigned int event_id, unsigned int value)
{
	PRINT_INTERNAL_COUNTER(event_id, value);
}

static void apu_smmu__latency(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void apu_smmu__tcu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void apu_smmu__tbu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void apu_smmu__hit_miss_rate(unsigned int event_id, unsigned int value)
{
	PRINT_INTERNAL_COUNTER(event_id, value);
}

static void soc_smmu__latency(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void soc_smmu__tcu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void soc_smmu__tbu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void soc_smmu__hit_miss_rate(unsigned int event_id, unsigned int value)
{
	PRINT_INTERNAL_COUNTER(event_id, value);
}

static void gpu_smmu__latency(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void gpu_smmu__tcu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void gpu_smmu__tbu_transaction(unsigned int event_id, unsigned int value)
{
	PRINT_SMMU_COUNTER(event_id, value);
}

static void gpu_smmu__hit_miss_rate(unsigned int event_id, unsigned int value)
{
	PRINT_INTERNAL_COUNTER(event_id, value);
}

static void mp_smmu_lmu(unsigned char cnt, unsigned int *value, u32 smmu_id)
{
	bool tbu_trans_exist = false, tcu_trans_exist = false;
	u32 event_id, tbu_trans_id, tcu_trans_id;
	struct perf_event *ev;
	const struct smmu_mp_func *mp_func;
	int i;

	mp_func = &smmu_mp_func_impl[smmu_id];

	for (i = 0; i < cnt; i++) {
		ev = pevent[smmu_id][i];
		event_id = get_event_id(&ev->attr);
		if (event_id == EVENT_R_LAT_MAX ||
		    event_id == EVENT_R_LAT_AVG ||
		    event_id == EVENT_TBUS_LAT_AVG) {
			mp_func->smmu_latency(event_id, value[i]);
		} else if (event_id == EVENT_R_TRANS_TOT) {
			tcu_trans_exist = true;
			tcu_trans_id = i;
			mp_func->smmu_tcu_transaction(event_id,
						      value[i]);
		} else if (event_id == EVENT_TBUS_TRANS_TOT) {
			tbu_trans_exist = true;
			tbu_trans_id = i;
			mp_func->smmu_tbu_transaction(event_id,
						      value[i]);
		}
	}

	if (tcu_trans_exist && tbu_trans_exist)
		mp_func->smmu_hit_miss_rate(EVENT_TLB_MISS_RATE,
					    (value[tcu_trans_id] * 1000) / value[tbu_trans_id]);
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

static int perf_thread_set_perf_events(int cpu, u32 smmu_id)
{
	int			i, size;
	struct perf_event	*ev;
	struct perf_event_attr	*ev_attr;
	int event_count = lmu_hw[smmu_id]->event_count;
	struct met_smmu_lmu *pmu = lmu_hw[smmu_id]->pmu;

	size = sizeof(struct perf_event_attr);

	for (i = 0; i < event_count; i++) {
		pevent[smmu_id][i] = NULL;
		if (!pmu[i].mode)
			continue;	/* Skip disabled counters */
		perfPrev[smmu_id][i] = 0;
		perfCurr[smmu_id][i] = 0;
		ev_attr = pevent_attr[smmu_id]+i;
		memset(ev_attr, 0, size);
		ev_attr->config = pmu[i].event;
		ev_attr->type = perf_device_type[smmu_id];
		ev_attr->size = size;
		ev_attr->sample_period = 0;
		ev_attr->pinned = 1;

		ev = perf_event_create_kernel_counter(ev_attr, cpu, NULL, dummy_handler, NULL);
		if (!ev || IS_ERR(ev)){
			/*bookkeep and remove occupied event from event array*/
			init_failed_smmu_lmu[smmu_id][init_failed_cnt[smmu_id]].event = pmu[i].event;
			init_failed_smmu_lmu[smmu_id][init_failed_cnt[smmu_id]].init_failed = SMMU_LMU_INIT_FAIL_OCCUPIED;
			init_failed_cnt[smmu_id]++;
			//clear pmu[i]
			pmu[i].mode = MODE_DISABLED;
			pmu[i].event = 0;
			pmu[i].freq = 0;
			counter_cnt[smmu_id]--; //decrease avail count
			DBG("%s config:%llu, type:%d event create failed %ld\n", __func__, ev_attr->config, ev_attr->type,  PTR_ERR(ev));
			continue;
		}
		if (ev->state != PERF_EVENT_STATE_ACTIVE) {
			perf_event_release_kernel(ev);
			/*bookkeep and remove occupied event from event array*/
			init_failed_smmu_lmu[smmu_id][init_failed_cnt[smmu_id]].event = pmu[i].event;
			init_failed_smmu_lmu[smmu_id][init_failed_cnt[smmu_id]].init_failed = SMMU_LMU_INIT_FAIL_OCCUPIED;
			init_failed_cnt[smmu_id]++;
			//clear pmu[i]
			pmu[i].mode = MODE_DISABLED;
			pmu[i].event = 0;
			pmu[i].freq = 0;
			counter_cnt[smmu_id]--; //decrease avail count
			DBG("%s config:%llu, type:%d event not active\n", __func__, ev_attr->config, ev_attr->type);
			continue;
		}

		DBG("%s config:%llu, type:%d ok, enable event\n", __func__, ev_attr->config, ev_attr->type);
		pevent[smmu_id][i] = ev;
		perf_event_enable(ev);

		perfCntFirst[smmu_id][i] = 1;
	}	/* for all PMU counter */
	return 0;
}

void met_perf_smmu_lmu_down(u32 smmu_id)
{
	int i;
	struct perf_event *ev;
	int event_count;
	struct met_smmu_lmu *pmu;
	struct metdevice *met_dev;

	DBG("%s enter\n", __func__);
	if (smmu_id >= SMMU_TYPE_NUM)
		return;

	met_dev = get_metdevice(smmu_id);
	if (!met_dev || met_dev->mode == 0)
		return;

	event_count = lmu_hw[smmu_id]->event_count;
	pmu = lmu_hw[smmu_id]->pmu;
	for (i = 0; i < event_count; i++) {
		if (!pmu[i].mode)
			continue;
		ev = pevent[smmu_id][i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			perf_event_disable(ev);
			perf_event_release_kernel(ev);
		}
		pevent[smmu_id][i] = NULL;
	}
	//perf_delayed_work_setup = NULL;
}

inline static void met_perf_smmu_lmu_start(int cpu, u32 smmu_id)
{
	struct metdevice *met_dev;

	DBG("%s enter, smmu_id:%d\n", __func__, smmu_id);

	met_dev = get_metdevice(smmu_id);
	if (!met_dev || met_dev->mode == 0)
		return;

	perf_thread_set_perf_events(cpu, smmu_id);
}

static int smmu_lmu_create_subfs(struct kobject *parent, u32 smmu_id)
{
	int ret = 0;

	DBG("%s enter\n", __func__);

	if (smmu_id >= SMMU_TYPE_NUM)
		return -ENODEV;

	lmu_hw[smmu_id] = smmu_lmu_hw_init(smmu_id);
	if (lmu_hw[smmu_id] == NULL) {
		PR_BOOTMSG("Failed to init smmu lmw, smmu id=%d!\n", smmu_id);
		return -ENODEV;
	}
	kobj_lmu[smmu_id] = parent;
	ret = sysfs_create_file(kobj_lmu[smmu_id], &perf_type_attr[smmu_id].attr);
	if (ret != 0) {
		PR_BOOTMSG("Failed to create perf_type in sysfs, smmu_id:%d\n",
			   smmu_id);
		goto out;
	}
out:
	return ret;
}

static int smmu_lmu_create_subfs_mm(struct kobject *parent)
{
	smmu_mp_func_impl[MM_SMMU].smmu_hit_miss_rate = mm_smmu__hit_miss_rate;
	smmu_mp_func_impl[MM_SMMU].smmu_latency = mm_smmu__latency;
	smmu_mp_func_impl[MM_SMMU].smmu_tbu_transaction = mm_smmu__tbu_transaction;
	smmu_mp_func_impl[MM_SMMU].smmu_tcu_transaction = mm_smmu__tcu_transaction;
	return smmu_lmu_create_subfs(parent, MM_SMMU);
}

static int smmu_lmu_create_subfs_apu(struct kobject *parent)
{
	smmu_mp_func_impl[APU_SMMU].smmu_hit_miss_rate = apu_smmu__hit_miss_rate;
	smmu_mp_func_impl[APU_SMMU].smmu_latency = apu_smmu__latency;
	smmu_mp_func_impl[APU_SMMU].smmu_tbu_transaction = apu_smmu__tbu_transaction;
	smmu_mp_func_impl[APU_SMMU].smmu_tcu_transaction = apu_smmu__tcu_transaction;
	return smmu_lmu_create_subfs(parent, APU_SMMU);
}

static int smmu_lmu_create_subfs_soc(struct kobject *parent)
{
	smmu_mp_func_impl[SOC_SMMU].smmu_hit_miss_rate = soc_smmu__hit_miss_rate;
	smmu_mp_func_impl[SOC_SMMU].smmu_latency = soc_smmu__latency;
	smmu_mp_func_impl[SOC_SMMU].smmu_tbu_transaction = soc_smmu__tbu_transaction;
	smmu_mp_func_impl[SOC_SMMU].smmu_tcu_transaction = soc_smmu__tcu_transaction;
	return smmu_lmu_create_subfs(parent, SOC_SMMU);
}

static int smmu_lmu_create_subfs_gpu(struct kobject *parent)
{
	smmu_mp_func_impl[GPU_SMMU].smmu_hit_miss_rate = gpu_smmu__hit_miss_rate;
	smmu_mp_func_impl[GPU_SMMU].smmu_latency = gpu_smmu__latency;
	smmu_mp_func_impl[GPU_SMMU].smmu_tbu_transaction = gpu_smmu__tbu_transaction;
	smmu_mp_func_impl[GPU_SMMU].smmu_tcu_transaction = gpu_smmu__tcu_transaction;
	return smmu_lmu_create_subfs(parent, GPU_SMMU);
}

static void smmu_lmu_delete_subfs(void)
{
	DBG("%s enter\n", __func__);
}

void met_perf_smmu_lmu_polling(unsigned long long stamp, int cpu, u32 smmu_id)
{
	int event_count = lmu_hw[smmu_id]->event_count;
	struct met_smmu_lmu *pmu = lmu_hw[smmu_id]->pmu;
	int i, count;
	unsigned long long delta;
	struct perf_event *ev;
	unsigned int pmu_value[MXNR_SMMU_LMU_EVENTS] = {0};
	u64 value;
	int ret;

	if (smmu_id >= SMMU_TYPE_NUM)
		return;

	count = 0;
	for (i = 0; i < event_count; i++) {
		if (pmu[i].mode == 0)
			continue;

		ev = pevent[smmu_id][i];
		if ((ev != NULL) && (ev->state == PERF_EVENT_STATE_ACTIVE)) {
			ret = __met_perf_event_read_local(ev, &value, NULL, NULL);
			if (ret < 0) {
				PR_BOOTMSG_ONCE("[MET_SMMU_PMU] read fail (ret=%d)\n", ret);
				continue;
			}

			perfCurr[smmu_id][i] = value;
			delta = (perfCurr[smmu_id][i] - perfPrev[smmu_id][i]);
			perfPrev[smmu_id][i] = perfCurr[smmu_id][i];
			if (perfCntFirst[smmu_id][i] == 1) {
				/* we shall omit delta counter when we get first counter */
				perfCntFirst[smmu_id][i] = 0;
				continue;
			}
			pmu_value[count] = (unsigned int)delta;
			count++;
		}
	}

	if (count == counter_cnt[smmu_id])
		mp_smmu_lmu(count, pmu_value, smmu_id);
}

static void met_perf_smmu_lmu_polling_mm(unsigned long long stamp, int cpu)
{
	met_perf_smmu_lmu_polling(stamp, cpu, MM_SMMU);
}

static void met_perf_smmu_lmu_polling_apu(unsigned long long stamp, int cpu)
{
	met_perf_smmu_lmu_polling(stamp, cpu, APU_SMMU);
}

static void met_perf_smmu_lmu_polling_soc(unsigned long long stamp, int cpu)
{
	met_perf_smmu_lmu_polling(stamp, cpu, SOC_SMMU);
}

static void met_perf_smmu_lmu_polling_gpu(unsigned long long stamp, int cpu)
{
	met_perf_smmu_lmu_polling(stamp, cpu, GPU_SMMU);
}

static void smmu_lmu_start_mm(void)
{
	met_perf_smmu_lmu_start(0, MM_SMMU);
}

static void smmu_lmu_start_apu(void)
{
	met_perf_smmu_lmu_start(0, APU_SMMU);
}

static void smmu_lmu_start_soc(void)
{
	met_perf_smmu_lmu_start(0, SOC_SMMU);
}

static void smmu_lmu_start_gpu(void)
{
	met_perf_smmu_lmu_start(0, GPU_SMMU);
}

static void smmu_lmu_stop_mm(void)
{
	met_perf_smmu_lmu_down(MM_SMMU);
}

static void smmu_lmu_stop_apu(void)
{
	met_perf_smmu_lmu_down(APU_SMMU);
}

static void smmu_lmu_stop_soc(void)
{
	met_perf_smmu_lmu_down(SOC_SMMU);
}

static void smmu_lmu_stop_gpu(void)
{
	met_perf_smmu_lmu_down(GPU_SMMU);
}

static const char header[] =
	"met-info [000] 0.0: met_smmu_lmu_header: SMMU";

static const char help[] =
	"  --smmu-lmu=EVENT         select SMMU-LMU events.\n"
	"                      you can enable at most \"%d general purpose events\"\n";

static int smmu_lmu_print_help(char *buf, int len, u32 smmu_id)
{
	if (smmu_id >= SMMU_TYPE_NUM)
		return -1;

	return snprintf(buf, PAGE_SIZE, help, lmu_hw[smmu_id]->event_count);
}

static int smmu_lmu_print_help_mm(char *buf, int len)
{
	return smmu_lmu_print_help(buf, len, MM_SMMU);
}

static int smmu_lmu_print_help_apu(char *buf, int len)
{
	return smmu_lmu_print_help(buf, len, APU_SMMU);
}

static int smmu_lmu_print_help_soc(char *buf, int len)
{
	return smmu_lmu_print_help(buf, len, SOC_SMMU);
}

static int smmu_lmu_print_help_gpu(char *buf, int len)
{
	return smmu_lmu_print_help(buf, len, GPU_SMMU);
}

static int reset_driver_stat(u32 smmu_id)
{
	int i;
	int event_count;
	struct met_smmu_lmu *pmu;
	struct metdevice *met_dev;

	if (smmu_id >= SMMU_TYPE_NUM)
		return -1;

	met_dev = get_metdevice(smmu_id);
	if (!met_dev)
		return -1;

	met_dev->mode = 0;
	event_count = lmu_hw[smmu_id]->event_count;
	pmu = lmu_hw[smmu_id]->pmu;
	counter_cnt[smmu_id] = 0;
	nr_arg[smmu_id] = 0;
	for (i = 0; i < event_count; i++) {
		pmu[i].mode = MODE_DISABLED;
		pmu[i].event = 0;
		pmu[i].freq = 0;
		DBG("reset event:%d, mode = 0", i);
	}

	nr_ignored_arg[smmu_id] = 0;
	init_failed_cnt[smmu_id] = 0;

	return 0;
}

static int smmu_lmu_print_header(char *buf, int len, u32 smmu_id)
{
	int first;
	int i, ret;
	int event_count;
	struct met_smmu_lmu *pmu;
	ret = 0;

	if (smmu_id >= SMMU_TYPE_NUM)
		return -1;

	/*
	 * print error message when user requested more smmu lmu events than
	 * platform's capability.
	 * we currently only prompt how many events were ignored.
	 */
	if (nr_ignored_arg[smmu_id]) {
		ret += SNPRINTF(buf + ret,
				len - ret,
				"met-info [000] 0.0: ##_SMMU_LMU_INIT_FAIL: "
				"too many events requested (max = %d), %d events ignored\n",
				lmu_hw[smmu_id]->event_count, nr_ignored_arg[smmu_id]);
		DBG("too many events requested (max = %d), %d events ignored\n",
		    lmu_hw[smmu_id]->event_count, nr_ignored_arg[smmu_id]);
	}

	/*
	 * print error message of init failed events due cpu offline
	 */
	event_count = lmu_hw[smmu_id]->event_count;
	pmu = lmu_hw[smmu_id]->pmu;
	first = 1;

	for (i = 0; i < init_failed_cnt[smmu_id]; i++) {

		if (init_failed_smmu_lmu[smmu_id][i].init_failed != SMMU_LMU_INIT_FAIL_OCCUPIED)
			continue;

		if (first) {
			ret += SNPRINTF(buf + ret,
					len - ret,
					"met-info [000] 0.0: ##_SMMU_LMU_INIT_FAIL: "
					"Occupied SMMU LMU specified slots: 0x%x",
					init_failed_smmu_lmu[smmu_id][i].event);
			first = 0;
			DBG("%x is occupied\n", init_failed_smmu_lmu[smmu_id][i].event);
			continue;
		}

		ret += SNPRINTF(buf + ret, len - ret, ",0x%x",
				init_failed_smmu_lmu[smmu_id][i].event);
	}
	/*
	if (!first && init_failed_cnt >=
		ARRAY_SIZE(init_failed_smmu_lmu))
		ret += SNPRINTF(buf + ret, len - ret,
				"... (truncated if there's more)");
				*/
	if (!first)
		ret += SNPRINTF(buf + ret, len - ret, "\n");

	/*
	 * active events
	 */
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "# mp_smmu_lmu: pmu_value1, ...\n");
	event_count = lmu_hw[smmu_id]->event_count;
	pmu = lmu_hw[smmu_id]->pmu;
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
		DBG("set event:%d mode=0\n", i);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");

	reset_driver_stat(smmu_id);
	return ret;
}

static int smmu_lmu_print_header_mm(char *buf, int len)
{
	return smmu_lmu_print_header(buf, len, MM_SMMU);
}

static int smmu_lmu_print_header_apu(char *buf, int len)
{
	return smmu_lmu_print_header(buf, len, APU_SMMU);
}

static int smmu_lmu_print_header_soc(char *buf, int len)
{
	return smmu_lmu_print_header(buf, len, SOC_SMMU);
}

static int smmu_lmu_print_header_gpu(char *buf, int len)
{
	return smmu_lmu_print_header(buf, len, GPU_SMMU);
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

static int smmu_lmu_process_argument(const char *arg, int len, u32 smmu_id)
{
	int nr_events, event_list[MXNR_SMMU_EVENT_BUFFER_SZ] = {};
	int i;
	int nr_counters;
	struct met_smmu_lmu *pmu;
	int arg_nr;
	int event_no;
	struct metdevice *met_dev;

	if (smmu_id >= SMMU_TYPE_NUM)
		return -1;

	met_dev = get_metdevice(smmu_id);
	if (!met_dev)
		return -1;

	/* get event_list */
	if ((nr_events = met_parse_num_list((char*)arg, len, event_list,
					     ARRAY_SIZE(event_list))) <= 0)
		goto arg_out;

	/* for each cpu in cpu_list, add all the events in event_list */
	nr_counters = lmu_hw[smmu_id]->event_count;
	pmu = lmu_hw[smmu_id]->pmu;
	arg_nr = nr_arg[smmu_id];

	if (nr_counters == 0)
		goto arg_out;

	for (i = 0; i < nr_events; i++) {
		event_no = event_list[i];
		/*
		 * check if event is duplicate,
		 * but may not include 0xff when met_lmu_hw_method == 0.
		 */
		if (lmu_hw[smmu_id]->check_event(pmu, arg_nr, event_no) < 0){
			DBG("arg_nr=%d, event_no=%d, duplicated!\n", arg_nr,event_no);
			continue;
		}
		if (arg_nr >= nr_counters){
			DBG("arg_nr(%d) exceeds nr_counters(%d)\n", arg_nr, nr_counters);
			nr_ignored_arg[smmu_id]++;
			continue;
		}

		pmu[arg_nr].mode = MODE_POLLING;
		pmu[arg_nr].event = event_no;
		pmu[arg_nr].freq = 0;
		arg_nr++;
		counter_cnt[smmu_id]++;
		DBG("event:%d mode=2\n", arg_nr);
	}
	nr_arg[smmu_id] = arg_nr;
	met_dev->mode = 1;
	return 0;

arg_out:
	reset_driver_stat(smmu_id);
	return -EINVAL;
}

static int smmu_lmu_process_argument_mm(const char *arg, int len)
{
	return smmu_lmu_process_argument(arg, len, MM_SMMU);
}

static int smmu_lmu_process_argument_apu(const char *arg, int len)
{
	return smmu_lmu_process_argument(arg, len, APU_SMMU);
}

static int smmu_lmu_process_argument_soc(const char *arg, int len)
{
	return smmu_lmu_process_argument(arg, len, SOC_SMMU);
}

static int smmu_lmu_process_argument_gpu(const char *arg, int len)
{
	return smmu_lmu_process_argument(arg, len, GPU_SMMU);
}

struct metdevice met_mm_smmu_lmu = {
	.name = "mm-smmu-lmu",
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = smmu_lmu_create_subfs_mm,
	.delete_subfs = smmu_lmu_delete_subfs,
	.start = smmu_lmu_start_mm,
	.stop = smmu_lmu_stop_mm,
	.polling_interval = 1,
	.timed_polling = met_perf_smmu_lmu_polling_mm,
	.print_help = smmu_lmu_print_help_mm,
	.print_header = smmu_lmu_print_header_mm,
	.process_argument = smmu_lmu_process_argument_mm,
};
EXPORT_SYMBOL(met_mm_smmu_lmu);

struct metdevice met_apu_smmu_lmu = {
	.name = "apu-smmu-lmu",
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = smmu_lmu_create_subfs_apu,
	.delete_subfs = smmu_lmu_delete_subfs,
	.start = smmu_lmu_start_apu,
	.stop = smmu_lmu_stop_apu,
	.polling_interval = 1,
	.timed_polling = met_perf_smmu_lmu_polling_apu,
	.print_help = smmu_lmu_print_help_apu,
	.print_header = smmu_lmu_print_header_apu,
	.process_argument = smmu_lmu_process_argument_apu,
};
EXPORT_SYMBOL(met_apu_smmu_lmu);

struct metdevice met_soc_smmu_lmu = {
	.name = "soc-smmu-lmu",
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = smmu_lmu_create_subfs_soc,
	.delete_subfs = smmu_lmu_delete_subfs,
	.start = smmu_lmu_start_soc,
	.stop = smmu_lmu_stop_soc,
	.polling_interval = 1,
	.timed_polling = met_perf_smmu_lmu_polling_soc,
	.print_help = smmu_lmu_print_help_soc,
	.print_header = smmu_lmu_print_header_soc,
	.process_argument = smmu_lmu_process_argument_soc,
};
EXPORT_SYMBOL(met_soc_smmu_lmu);

struct metdevice met_gpu_smmu_lmu = {
	.name = "gpu-smmu-lmu",
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = smmu_lmu_create_subfs_gpu,
	.delete_subfs = smmu_lmu_delete_subfs,
	.start = smmu_lmu_start_gpu,
	.stop = smmu_lmu_stop_gpu,
	.polling_interval = 1,
	.timed_polling = met_perf_smmu_lmu_polling_gpu,
	.print_help = smmu_lmu_print_help_gpu,
	.print_header = smmu_lmu_print_header_gpu,
	.process_argument = smmu_lmu_process_argument_gpu,
};
EXPORT_SYMBOL(met_gpu_smmu_lmu);

