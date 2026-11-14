// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_ctx_sched.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include "mtk_platform_logbuffer.h"
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

void mtk_debug_dump_as_status_nolock(struct kbase_device *kbdev)
{
	int as;
	struct task_struct *task;
	struct pid *pid_struct;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	dev_info(kbdev->dev, "AS status dump start");

	for (as = 0; as < kbdev->nr_hw_address_spaces; as++) {
		unsigned long flags;
		struct kbase_context *kctx;
		struct kbase_fault fault;

		if (as == MCU_AS_NR)
			continue;

		kctx = kbase_ctx_sched_as_to_ctx_nolock(kbdev, as);

		if (kctx)
			kbase_ctx_sched_retain_ctx_refcount(kctx);
		else
			continue;

		pid_struct = find_get_pid(kctx->tgid);
		if (pid_struct != NULL) {
			rcu_read_lock();
			task = pid_task(pid_struct, PIDTYPE_PID);
			if (task && task->group_leader) {
				dev_info(kbdev->dev, "AS[%d], kctx %d_%d, process_name: %s", as, kctx->tgid, kctx->id, task->group_leader->comm);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev,
					MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "AS[%d], kctx %d_%d, process_name: %s\n",
					as, kctx->tgid, kctx->id, task->group_leader->comm);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
			} else {
				dev_info(kbdev->dev, "AS[%d], kctx %d_%d, process_name: %s", as, kctx->tgid, kctx->id, "NULL");
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev,
					MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "AS[%d], kctx %d_%d, process_name: %s\n",
					as, kctx->tgid, kctx->id, "NULL");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
			}
			rcu_read_unlock();
			put_pid(pid_struct);
		} else {
			dev_info(kbdev->dev, "AS[%d], kctx %d_%d, process_name: %s", as, kctx->tgid, kctx->id, "NULL");
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(kbdev,
				MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "AS[%d], kctx %d_%d, process_name: %s\n",
				as, kctx->tgid, kctx->id, "NULL");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}

		kbase_ctx_sched_release_ctx(kctx);
	}
	dev_info(kbdev->dev, "AS status dump end");
}

void mtk_debug_dump_as_status(struct kbase_device *kbdev)
{
	unsigned long flags;
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	mtk_debug_dump_as_status_nolock(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}