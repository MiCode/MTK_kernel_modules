// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_IRQ_TRACE_H__
#define __MTK_PLATFORM_IRQ_TRACE_H__

#define MAX_PHASE_NUM 20
#define MAX_DATA_NUM 100
#define DEFAULT_GPU_TH1_MS 50 /* 50ms */
#define DEFAULT_IRQ_MONITOR_OVERRIDE_TH1_MS 5 /* 5ms  */

enum KBASE_IRQ_ID {
    KBASE_IRQ_JOB,
    KBASE_IRQ_MMU,
    KBASE_IRQ_GPU,
    KBASE_IRQ_NUM,
};

struct mtk_irq_trace_recorder {
    enum KBASE_IRQ_ID irq_id;
    unsigned int phase_id;
    unsigned long long start_record_time_ns;
    unsigned long long end_record_time_ns;
    unsigned long long data_1;
    unsigned long long data_2;
};

void mtk_debug_irq_trace_record_start(enum KBASE_IRQ_ID irq_id, unsigned int phase_id);
void mtk_debug_irq_trace_record_end(enum KBASE_IRQ_ID irq_id, unsigned int phase_id);
void mtk_debug_irq_trace_record_data(unsigned int data_id, unsigned long long data);
void mtk_debug_irq_trace_check_timeout(struct kbase_device *kbdev, enum KBASE_IRQ_ID irq_id, unsigned int phase_id);
void mtk_debug_irq_trace_l2_record(unsigned long long start_time, unsigned int from_state,
    unsigned int to_state);
int mtk_debug_irq_trace_init(struct kbase_device *kbdev);
int mtk_debug_irq_trace_term(struct kbase_device *kbdev);

int mtk_debug_irq_trace_debugfs_init(struct kbase_device *kbdev);

#endif /* __MTK_PLATFORM_IRQ_TRACE_H__ */
