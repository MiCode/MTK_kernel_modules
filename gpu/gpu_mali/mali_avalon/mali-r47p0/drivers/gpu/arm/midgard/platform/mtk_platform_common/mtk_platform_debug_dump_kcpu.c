// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <linux/delay.h>
#include <mali_kbase_sync.h>
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg.h>
#include <csf/mali_kbase_csf_kcpu_debugfs.h>
#include <csf/mali_kbase_csf_kcpu.h>
#endif /* CONFIG_MALI_CSF_SUPPORT */

#include "mtk_platform_debug.h"

__attribute__((unused)) extern int mtk_debug_trylock(struct mutex *lock);

static void mtk_debug_csf_dump_kcpu_cmd_fence_signal(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    struct kbase_sync_fence_info info = { 0 };

    if (cmd->info.fence.fence)
        kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
    else
        scnprintf(info.name, sizeof(info.name), "NULL");

    mtk_log_critical_exception(kbdev, true,
        "[%d_%d] %9lu(  %s ), %4d(-), Fence Signal, %pK %s %s",
        kctx->tgid, kctx->id,
        idx,
        queue->has_error ? "InErr" : "NoErr",
        cmd_idx,
        info.fence,
        info.name,
        kbase_sync_status_string(info.status));
}

static void mtk_debug_csf_dump_kcpu_cmd_fence_wait(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    struct kbase_sync_fence_info info = { 0 };

    if (cmd->info.fence.fence)
        kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
    else
        scnprintf(info.name, sizeof(info.name), "NULL");

    mtk_log_critical_exception(kbdev, true,
        "[%d_%d] %9lu(  %s ), %4d(-),   Fence Wait, %pK %s %s",
        kctx->tgid, kctx->id,
        idx,
        queue->has_error ? "InErr" : "NoErr",
        cmd_idx,
        info.fence,
        info.name,
        kbase_sync_status_string(info.status));
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_wait(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    unsigned int i;
    struct kbase_kcpu_command_cqs_wait_info *waits = &cmd->info.cqs_wait;

    if (waits->nr_objs == 0) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),     CQS Wait, nr_objs == 0\n",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    if (waits->objs == NULL) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),     CQS Wait, objs == NULL\n",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    for (i = 0; i < waits->nr_objs; i++) {
        struct kbase_vmap_struct *mapping;
        u32 val;
        char const *msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";
        u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);

        if (cpu_ptr) {
            val = *cpu_ptr;
            kbase_phy_alloc_mapping_put(kctx, mapping);

            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] %9lu(  %s ), %4d(%d),     CQS Wait, %llx(%u > %u, inherit_err: %s)",
                kctx->tgid, kctx->id,
                idx,
                queue->has_error ? "InErr" : "NoErr",
                cmd_idx, i,
                waits->objs[i].addr,
                val,
                waits->objs[i].val,
                msg);
        } else {
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] %9lu(  %s ), %4d(%d),     CQS Wait, %llx(??val?? > %u, inherit_err: %s)",
                kctx->tgid, kctx->id,
                idx,
                queue->has_error ? "InErr" : "NoErr",
                cmd_idx, i,
                waits->objs[i].addr,
                waits->objs[i].val,
                msg);
        }
    }
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_set(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    unsigned int i;
    struct kbase_kcpu_command_cqs_set_info *sets = &cmd->info.cqs_set;

    if (sets->nr_objs == 0) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),      CQS Set, nr_objs == 0",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    if (sets->objs == NULL) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),      CQS Set, objs == NULL\n",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    for (i = 0; i < sets->nr_objs; i++) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(%d),      CQS Set, %llx",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx, i,
            sets->objs[i].addr);
    }
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_wait_operation(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    unsigned int i;
    struct kbase_kcpu_command_cqs_wait_operation_info *waits = &cmd->info.cqs_wait_operation;

    if (waits->nr_objs == 0) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),   CQS WaitOp, nr_objs == 0\n",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    if (waits->objs == NULL) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),   CQS WaitOp, objs == NULL\n",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    for (i = 0; i < waits->nr_objs; i++) {
        char const *msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";
        struct kbase_vmap_struct *mapping;
        void *const cpu_ptr = kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);
        u64 val;

        if (cpu_ptr == NULL) {
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(??val?? > %llu, inherit_err: %s)",
                kctx->tgid, kctx->id,
                idx,
                queue->has_error ? "InErr" : "NoErr",
                cmd_idx, i,
                waits->objs[i].addr,
                waits->objs[i].val,
                msg);
            continue;
        }

        if (waits->objs[i].data_type == BASEP_CQS_DATA_TYPE_U32)
            val = (u64)*((u32 *)cpu_ptr);
        else if (waits->objs[i].data_type == BASEP_CQS_DATA_TYPE_U64)
            val = *((u64 *)cpu_ptr);
        else {
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(invalid data_type (%d), inherit_err: %s)",
                kctx->tgid, kctx->id,
                idx,
                queue->has_error ? "InErr" : "NoErr",
                cmd_idx, i,
                waits->objs[i].addr,
                waits->objs[i].data_type,
                msg);
            kbase_phy_alloc_mapping_put(kctx, mapping);
            continue;
        }
        kbase_phy_alloc_mapping_put(kctx, mapping);

        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(%llu %s %llu, inherit_err: %s)",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx, i,
            waits->objs[i].addr,
            val,
            waits->objs[i].operation == BASEP_CQS_WAIT_OPERATION_LE ?
                "<=" : (waits->objs[i].operation == BASEP_CQS_WAIT_OPERATION_GT ? ">" : "InvalidOp"),
            waits->objs[i].val,
            msg);
    }
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_set_operation(struct kbase_device *kbdev, struct kbase_context *kctx,
    unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
    unsigned int i;
    struct kbase_kcpu_command_cqs_set_operation_info *sets = &cmd->info.cqs_set_operation;

    if (sets->nr_objs == 0) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),    CQS SetOp, nr_objs == 0",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    if (sets->objs == NULL) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(-),    CQS SetOp, objs == NULL",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx);
        return;
    }
    for (i = 0; i < sets->nr_objs; i++) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %4d(%d),    CQS SetOp, %llx(%llu)",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            cmd_idx, i,
            sets->objs[i].addr,
            sets->objs[i].val);
    }
}

/* REFERENCE FROM kbasep_csf_kcpu_debugfs_show() in midgard/scf/mali_kbase_csf_kcpu_debugfs.c */
void mtk_debug_csf_dump_kcpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
    unsigned long idx = 0;

    mtk_log_critical_exception(kbdev, true,
        "[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u",
        MALI_CSF_CSG_DUMP_VERSION);
    mtk_log_critical_exception(kbdev, true,
        "[kcpu_queues] ##### Ctx %d_%d #####",
        kctx->tgid, kctx->id);

    if (!mtk_debug_trylock(&kctx->csf.kcpu_queues.lock)) {
        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] lock csf.kcpu_queues.lock failed!",
            kctx->tgid, kctx->id);
        return;
    }

    mtk_log_critical_exception(kbdev, true,
        "[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno",
        kctx->tgid, kctx->id);
    idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

    while (idx < KBASEP_MAX_KCPU_QUEUES) {
        struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];
        int i = 0;

        if (!queue) {
            idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
            continue;
        }

        if (!mtk_debug_trylock(&queue->lock)) {
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] %9lu( lock held, bypass dump )",
                kctx->tgid, kctx->id, idx);
            idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
            continue;
        }

        mtk_log_critical_exception(kbdev, true,
            "[%d_%d] %9lu(  %s ), %16u, %11u, %7u, %12u, %13llu  %8u",
            kctx->tgid, kctx->id,
            idx,
            queue->has_error ? "InErr" : "NoErr",
            queue->num_pending_cmds,
            queue->enqueue_failed,
            queue->command_started ? 1 : 0,
            queue->start_offset,
            queue->fence_context,
            queue->fence_seqno);

        if (queue->num_pending_cmds)
            mtk_log_critical_exception(kbdev, true,
                "[%d_%d] Queue Idx(err-mode), CMD Idx,    Wait Type, Additional info",
                kctx->tgid, kctx->id);

        for (i = 0; i < queue->num_pending_cmds; i++) {
            struct kbase_kcpu_command *cmd;
            u8 cmd_idx = (u8)(queue->start_offset + i);

            /* The offset to the first command that is being processed or yet to
             * be processed is of u8 type, so the number of commands inside the
             * queue cannot be more than 256. The current implementation expects
             * exactly 256, any other size will require the addition of wrapping
             * logic.
             */
            BUILD_BUG_ON(KBASEP_KCPU_QUEUE_SIZE != 256);

            cmd = &queue->commands[cmd_idx];
            if (cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
                mtk_log_critical_exception(kbdev, true,
                    "[%d_%d] %9lu(  %s ), %4d(-), %12d, (unknown blocking command)",
                    kctx->tgid, kctx->id,
                    idx,
                    queue->has_error ? "InErr" : "NoErr",
                    cmd_idx,
                    cmd->type);
                continue;
            }

            switch (cmd->type) {
            case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
                mtk_debug_csf_dump_kcpu_cmd_fence_signal(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
                mtk_debug_csf_dump_kcpu_cmd_fence_wait(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
                mtk_debug_csf_dump_kcpu_cmd_cqs_wait(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            case BASE_KCPU_COMMAND_TYPE_CQS_SET:
                mtk_debug_csf_dump_kcpu_cmd_cqs_set(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
                mtk_debug_csf_dump_kcpu_cmd_cqs_wait_operation(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
                mtk_debug_csf_dump_kcpu_cmd_cqs_set_operation(kbdev, kctx, idx, queue, cmd_idx, cmd);
                break;
            default:
                mtk_log_critical_exception(kbdev, true,
                    "[%d_%d] %9lu(  %s ), %4d(-), %12d, (other blocking command)",
                    kctx->tgid, kctx->id,
                    idx,
                    queue->has_error ? "InErr" : "NoErr",
                    cmd_idx,
                    cmd->type);
                break;
            }
        }

        mutex_unlock(&queue->lock);
        idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
    }

    mutex_unlock(&kctx->csf.kcpu_queues.lock);
}
