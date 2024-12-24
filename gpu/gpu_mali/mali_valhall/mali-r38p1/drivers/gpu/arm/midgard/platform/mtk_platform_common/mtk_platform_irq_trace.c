// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_irq_trace.h>
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>

struct mtk_irq_trace_recorder mtk_irq_trace_recorders[KBASE_IRQ_NUM][MAX_PHASE_NUM];
struct mtk_irq_trace_recorder mtk_l2_trace_recorders[MAX_PHASE_NUM];
unsigned long long mtk_irq_trace_data[MAX_DATA_NUM];
unsigned int irq_monitor_default_enabled = false;
unsigned int irq_monitor_override_threshold_ms = DEFAULT_GPU_TH1_MS;

void mtk_debug_irq_trace_record_data(unsigned int data_id, unsigned long long data)
{
	if (data_id < MAX_DATA_NUM)
		mtk_irq_trace_data[data_id] = data;
	return;
}

void mtk_debug_irq_trace_record_start(enum KBASE_IRQ_ID irq_id, unsigned int phase_id)
{
	if ((irq_id < KBASE_IRQ_NUM) && (phase_id < MAX_PHASE_NUM))
		mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns = ktime_get_raw_ns();
	return;
}

void mtk_debug_irq_trace_record_end(enum KBASE_IRQ_ID irq_id, unsigned int phase_id)
{
	if ((irq_id < KBASE_IRQ_NUM) && (phase_id < MAX_PHASE_NUM))
		mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns = ktime_get_raw_ns();
	return;
}
void mtk_debug_irq_trace_l2_record(unsigned long long start_time, unsigned int from_state,
    unsigned int to_state)
{
	static unsigned int phase_id = 0;

	phase_id %= MAX_PHASE_NUM;

	mtk_l2_trace_recorders[phase_id].start_record_time_ns = start_time;
	mtk_l2_trace_recorders[phase_id].end_record_time_ns = ktime_get_raw_ns();
	mtk_l2_trace_recorders[phase_id].data_1 = (unsigned long long)from_state;
	mtk_l2_trace_recorders[phase_id].data_2 = (unsigned long long)to_state;

	phase_id++;

	return;
}

static void mtk_debug_irq_trace_show(void)
{
	int irq_id = 0;
	int phase_id = 0;
	long long int interval = 0;
	pr_info("IRQ Trace INFO:\n");
	pr_info("IRQ Monitor enabled: %s\n", (irq_monitor_default_enabled) ? "true" : "false");
	pr_info("IRQ Monitor th1 ms: %dms\n", irq_monitor_override_threshold_ms);
	pr_info("GPU_IRQ_count: %llu\n", mtk_irq_trace_data[0]);
	pr_info("GPU_IRQ_status: 0x%llx\n", mtk_irq_trace_data[1]);
	pr_info("JOB_IRQ_count: %llu\n", mtk_irq_trace_data[2]);
	pr_info("JOB_IRQ_status: 0x%llx\n", mtk_irq_trace_data[3]);

	for (irq_id = 0; irq_id < KBASE_IRQ_NUM; irq_id ++) {
		for (phase_id = 0; phase_id < MAX_PHASE_NUM; phase_id ++){
			interval = (mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns -
				mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns) / 1000;

			pr_info("irq_id[%d], phase_id[%d], start_time[%llu], end_time[%llu], interval[%llu us]\n",
				irq_id, phase_id, mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns,
				mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns, interval);
		}
	}

	pr_info("L2 status INFO:\n");
	for (phase_id = 0; phase_id < MAX_PHASE_NUM; phase_id ++){
		interval = (mtk_l2_trace_recorders[phase_id].end_record_time_ns -
			mtk_l2_trace_recorders[phase_id].start_record_time_ns) / 1000;

		pr_info("phase_id[%d], start_time[%llu], end_time[%llu], interval[%llu us], from[%llu], to[%llu]\n",
			phase_id, mtk_l2_trace_recorders[phase_id].start_record_time_ns,
			mtk_l2_trace_recorders[phase_id].end_record_time_ns, interval,
			mtk_l2_trace_recorders[phase_id].data_1,
			mtk_l2_trace_recorders[phase_id].data_2);
	}

	return;
}

void mtk_debug_irq_trace_check_timeout(struct kbase_device *kbdev, enum KBASE_IRQ_ID irq_id, unsigned int phase_id)
{
	long long int interval = 0;

	interval = mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns -
				mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns;

	if ((interval / 1000000) >= irq_monitor_override_threshold_ms) {
		mtk_logbuffer_print(&kbdev->logbuf_exception, "[GODZILLA] irq: %d timeout, interval: %lld \n",
			irq_id, (interval / 1000000));
		mtk_debug_irq_trace_show();
	}

	return;
}

int mtk_debug_irq_trace_init(struct kbase_device *kbdev)
{
	struct device_node *node;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	/* check if irq moniter dts property exists */
	node = of_find_node_by_name(NULL, "gpufreq");
	if (node) {
		/* irq monitor only enabled in aging load */
		if (!of_property_read_u32(node, "aging-load", &irq_monitor_default_enabled)) {
			if (irq_monitor_default_enabled) {
				/* irq monitor will override IRQ LONG trigger threshold */
				irq_monitor_override_threshold_ms = DEFAULT_IRQ_MONITOR_OVERRIDE_TH1_MS;
			}
		}
	}

	return 0;
}

int mtk_debug_irq_trace_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	return 0;
}

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_FS)
static int mtk_debug_irq_trace(struct seq_file *file, void *data)
{
	int irq_id = 0;
	int phase_id = 0;
	long long int interval = 0;
	seq_printf(file, "IRQ Trace INFO:\n");
	seq_printf(file, "IRQ Monitor enabled: %s\n", (irq_monitor_default_enabled) ? "true" : "false");
	seq_printf(file, "IRQ Monitor th1 ms: %dms\n", irq_monitor_override_threshold_ms);
	seq_printf(file, "GPU_IRQ_count: %llu\n", mtk_irq_trace_data[0]);
	seq_printf(file, "GPU_IRQ_status: 0x%llx\n", mtk_irq_trace_data[1]);
	seq_printf(file, "JOB_IRQ_count: %llu\n", mtk_irq_trace_data[2]);
	seq_printf(file, "JOB_IRQ_status: 0x%llx\n", mtk_irq_trace_data[3]);

	for (irq_id = 0; irq_id < KBASE_IRQ_NUM; irq_id ++) {
		for (phase_id = 0; phase_id < MAX_PHASE_NUM; phase_id ++){
			interval = (mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns -
				mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns) / 1000;

			seq_printf(file, "irq_id[%d], phase_id[%d], start_time[%llu], end_time[%llu], interval[%llu us]\n",
				irq_id, phase_id, mtk_irq_trace_recorders[irq_id][phase_id].start_record_time_ns,
				mtk_irq_trace_recorders[irq_id][phase_id].end_record_time_ns, interval);
		}
	}

	seq_printf(file, "L2 status INFO:\n");
	for (phase_id = 0; phase_id < MAX_PHASE_NUM; phase_id ++){
		interval = (mtk_l2_trace_recorders[phase_id].end_record_time_ns -
			mtk_l2_trace_recorders[phase_id].start_record_time_ns) / 1000;

		seq_printf(file, "phase_id[%d], start_time[%llu], end_time[%llu], interval[%llu us], from[%llu], to[%llu]\n",
			phase_id, mtk_l2_trace_recorders[phase_id].start_record_time_ns,
			mtk_l2_trace_recorders[phase_id].end_record_time_ns, interval,
			mtk_l2_trace_recorders[phase_id].data_1,
			mtk_l2_trace_recorders[phase_id].data_2);
	}

	return 0;
}

static int mtk_irq_debugfs_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_irq_trace,
	                   in->i_private);
}

static const struct file_operations mtk_irq_debugfs_fops = {
	.open = mtk_irq_debugfs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_irq_trace_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("irq_trace", 0440,
		kbdev->mali_debugfs_directory, kbdev,
		&mtk_irq_debugfs_fops);

	return 0;
}


#endif /* CONFIG_MALI_MTK_DEBUG_FS */