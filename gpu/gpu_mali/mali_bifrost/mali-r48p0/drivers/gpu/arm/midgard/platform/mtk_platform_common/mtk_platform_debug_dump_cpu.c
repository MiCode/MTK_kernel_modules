// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/delay.h>

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
//#include <csf/mali_kbase_csf_csg.h>
#include <csf/mali_kbase_csf_cpu_queue.h>
#endif /* CONFIG_MALI_CSF_SUPPORT */

#include "mtk_platform_debug.h"

__attribute__((unused)) extern int mtk_debug_trylock(struct mutex *lock);

/* REFERENCE FROM kbasep_csf_cpu_queue_dump_print() in midgard/scf/mali_kbase_csf_cpu_queue.c */
void mtk_debug_csf_dump_cpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
    mutex_lock(&kctx->csf.lock);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
    mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
    if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] Dump request already started!",
            kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
        mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
        mutex_unlock(&kctx->csf.lock);
        return;
    }
    do {
        atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
        init_completion(&kctx->csf.cpu_queue.dump_cmp);
        kbase_event_wakeup(kctx);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
        mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
        mutex_unlock(&kctx->csf.lock);

        mtk_log_critical_exception(kbdev, true,
            "[cpu_queue] CPU Queues table (version:v%u):",
            MALI_CSF_CPU_QUEUE_DUMP_VERSION);
        mtk_log_critical_exception(kbdev, true,
            "[cpu_queue] ##### Ctx %d_%d #####",
            kctx->tgid, kctx->id);

        if (!wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp, msecs_to_jiffies(3000))) {
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] Timeout waiting for dump completion",
                kctx->tgid, kctx->id);
                mutex_lock(&kctx->csf.lock);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
                mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
                break;
        }

        mutex_lock(&kctx->csf.lock);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
        mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
        if (kctx->csf.cpu_queue.buffer) {
            int i;
            int next_str_idx = 0;

            WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_PENDING);

            for (i = 0; i < kctx->csf.cpu_queue.buffer_size; i++) {
                if (kctx->csf.cpu_queue.buffer[i] == '\n') {
                    kctx->csf.cpu_queue.buffer[i] = '\0';
                    mtk_log_critical_exception(kbdev, true,
                        "%s",
                        &(kctx->csf.cpu_queue.buffer[next_str_idx]));
                    next_str_idx = i + 1;
                }
            }

            kfree(kctx->csf.cpu_queue.buffer);
            kctx->csf.cpu_queue.buffer = NULL;
            kctx->csf.cpu_queue.buffer_size = 0;
        } else {
            mtk_log_critical_exception(kbdev, true,
                    "[%d_%d] Dump error! (time out)",
                    kctx->tgid, kctx->id);
        }
    } while (false);
    atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
    mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
    mutex_unlock(&kctx->csf.lock);
}
