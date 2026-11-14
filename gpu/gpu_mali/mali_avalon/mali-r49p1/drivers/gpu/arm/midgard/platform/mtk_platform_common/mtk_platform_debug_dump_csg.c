// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/delay.h>

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg.h>
#endif /* CONFIG_MALI_CSF_SUPPORT */

#include "mtk_platform_debug.h"

__attribute__((unused)) int mtk_debug_trylock(struct mutex *lock)
{
    int count = 3;
    int ret;

    ret = mutex_trylock(lock);
    while (!ret && --count) {
        msleep(1);
        ret = mutex_trylock(lock);
    }

    return ret;
}

int mtk_debug_cs_queue_allocate_memory(struct kbase_device *kbdev);
void mtk_debug_cs_queue_free_memory(void);
void mtk_debug_cs_queue_data_dump(struct kbase_device *kbdev, struct mtk_debug_cs_queue_data *cs_queue_data);
void mtk_debug_csf_dump_cpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx);
void mtk_debug_csf_dump_kcpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx);
void mtk_debug_csf_csg_active_dump_group(struct kbase_queue_group *const group,
                                         struct mtk_debug_cs_queue_data *cs_queue_data);
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
    int dump_queue_data = 0;
    static struct mtk_debug_cs_queue_data cs_queue_data;
    if (!kbdev) {
        pr_info("[KBASE]%s Bad kbdev!\n", __func__);
        return;
    }
    if (!kctx) {
        mtk_log_critical_exception(kbdev, true, "%s kctx NULL!", __func__);
        return;
    }

    dump_queue_data = mtk_debug_cs_queue_allocate_memory(kbdev);
    if (dump_queue_data) {
        INIT_LIST_HEAD(&cs_queue_data.queue_list);
    }
    do {
        struct kbase_context *kctx_dump, *kctx_checker;
        int found;
        int ret;

        mtk_log_critical_exception(kbdev, true,
            "[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u", MALI_CSF_CSG_DUMP_VERSION);

        /* check kctx if exist */
        if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
            mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
            break;
        }
        found = false;
        list_for_each_entry(kctx_checker, &kbdev->kctx_list, kctx_list_link) {
            if (kctx == kctx_checker) {
                found = true;
                break;
            }
        }
        if (!found) {
            mtk_log_critical_exception(kbdev, true,
                    "%s kctx %d_%d isn't exist in kctx_list, kctx failed!",
                    __func__, kctx->tgid, kctx->id);
            mutex_unlock(&kbdev->kctx_list_lock);
            break;
        }
        mutex_unlock(&kbdev->kctx_list_lock);

        /* lock kctx->csf.lock */
        mutex_lock(&kctx->csf.lock);
        kbase_csf_scheduler_lock(kbdev);
        kbase_csf_csg_update_status(kbdev);

        /* REFERENCE FROM kbasep_csf_csg_active_dump_print() in in midgard/scf/mali_kbase_csf_csg.c*/
        {
            u32 csg_nr = 0;
            u32 num_groups = 0;

            num_groups = kbdev->csf.global_iface.group_num;

            for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
                struct kbase_queue_group *const group =
                    kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

                if (!group)
                    continue;

                mtk_log_critical_exception(kbdev, true,
                    "[active_groups] ##### Ctx %d_%d #####",
                    group->kctx->tgid, group->kctx->id);

                if (dump_queue_data) {
                    cs_queue_data.kctx = group->kctx;
                    cs_queue_data.group_type = 0;
                    cs_queue_data.handle = group->handle;
                    mtk_debug_csf_csg_active_dump_group(group,  &cs_queue_data);
                } else {
                    mtk_debug_csf_csg_active_dump_group(group, NULL);
                }
            }
        }

        /* dump groups */
        if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
            mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
            mutex_unlock(&kbdev->csf.scheduler.lock);
            mutex_unlock(&kctx->csf.lock);
            break;
        }

        /* REFERENCE FROM kbasep_csf_csg_dump_print() in in midgard/scf/mali_kbase_csf_csg.c*/
        list_for_each_entry(kctx_dump, &kbdev->kctx_list, kctx_list_link) {
            u32 gr;

            if (kctx_dump != kctx) {
                if (!mtk_debug_trylock(&kctx_dump->csf.lock)) {
                    mtk_log_critical_exception(kbdev, true,
                        "[%d_%d] %s lock csf.lock failed, skip group dump!",
                        kctx_dump->tgid, kctx_dump->id, __func__);
                    continue;
                }
            }
            for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
                struct kbase_queue_group *const group = kctx_dump->csf.queue_groups[gr];

                if (!group || kbase_csf_scheduler_group_get_slot(group) >= 0) {
                    continue;
                }

                mtk_log_critical_exception(kbdev, true,
                    "[groups] ##### Ctx %d_%d #####",
                    group->kctx->tgid, group->kctx->id);

                if (dump_queue_data) {
                    cs_queue_data.kctx = group->kctx;
                    cs_queue_data.group_type = 1;
                    cs_queue_data.handle = group->handle;
                    mtk_debug_csf_csg_active_dump_group(group, &cs_queue_data);
                }
                else {
                    mtk_debug_csf_csg_active_dump_group(group, NULL);
                }
            }
            if (kctx_dump != kctx)
                mutex_unlock(&kctx_dump->csf.lock);
        }
        mutex_unlock(&kbdev->kctx_list_lock);
        kbase_csf_scheduler_unlock(kbdev);
        mutex_unlock(&kctx->csf.lock);

        /* dump kcpu queues */
        mtk_debug_csf_dump_kcpu_queues(kbdev, kctx);

        /* dump cpu queues */
        mtk_debug_csf_dump_cpu_queues(kbdev, kctx);

        /* dump command stream buffer */
        if (dump_queue_data) {
            if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
                mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
                break;
            }
            mtk_debug_cs_queue_data_dump(kbdev, &cs_queue_data);
            mutex_unlock(&kbdev->kctx_list_lock);
        }

    } while (false);

    if (dump_queue_data) {
        mtk_debug_cs_queue_free_memory();
    }
}
