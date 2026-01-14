// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_sync.h>
#include <platform/mtk_platform_common.h>
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg_debugfs.h>
#include <csf/mali_kbase_csf.h>
#endif /* CONFIG_MALI_CSF_SUPPORT */
#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
#include <mali_kbase_reset_gpu.h>
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
#include <platform/mtk_platform_common/mtk_platform_qinspect.h>
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif /* CONFIG_MTK_AEE_FEATURE */

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
/********************************/
/*            define            */
/********************************/
#define TAG "[QINSPECT_RECOVERY]"

#define MTK_QINSPECT_ARRYA_SIZE 256

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#define mtk_qinspect_log(fmt, args...) \
	do { \
		dev_info(g_kctx->kbdev->dev, TAG fmt, ##args); \
		mtk_logbuffer_type_print(g_kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL, TAG fmt "\n", ##args); \
	} while (0)
#else
#define mtk_qinspect_log(fmt, args...) \
	do { \
		dev_info(g_kctx->kbdev->dev, TAG fmt, ##args); \
	} while (0)
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

/********************************/
/*           structure          */
/********************************/
struct mtk_qinspect_wait_queue_node {
	enum mtk_qinspect_queue_type queue_type;
	void *queue;
	struct mtk_qinspect_wait_queue_node *parent;
};

struct mtk_qinspect_cqs_rootlocker {
	struct kbase_context *kctx;
	struct mtk_qinspect_cqs_wait_obj obj;
};

/********************************/
/*       global variable        */
/********************************/
struct kbase_context *g_kctx;

DEFINE_MUTEX(recovery_lock);

struct mtk_qinspect_wait_queue_node g_wait_queue_list[MTK_QINSPECT_ARRYA_SIZE];
unsigned int g_wait_queue_insert = 0;
unsigned int g_wait_queue_extract = 0;

struct mtk_qinspect_cqs_rootlocker g_cqs_rootlocker_list[MTK_QINSPECT_ARRYA_SIZE];
unsigned int g_cqs_rootlocker_idx = 0;

/********************************/
/*           function           */
/********************************/
static int mtk_qinspect_enqueue_wait_queue(enum mtk_qinspect_queue_type queue_type, void *queue) {
	unsigned int i;

	if (!queue) {
		mtk_qinspect_log("queue is null, skip enqueue wait queue");
		return -1;
	}

	if (g_wait_queue_insert == MTK_QINSPECT_ARRYA_SIZE) {
		mtk_qinspect_log("qinspect wait queue is full, skip enqueue wait queue");
		return -2;
	}

	if (g_wait_queue_insert == 0)
		g_wait_queue_list[g_wait_queue_insert].parent = NULL;
	else
		g_wait_queue_list[g_wait_queue_insert].parent =
			&g_wait_queue_list[g_wait_queue_extract];
	g_wait_queue_list[g_wait_queue_insert].queue_type = queue_type;
	g_wait_queue_list[g_wait_queue_insert].queue = queue;
	g_wait_queue_insert++;

	return 0;
}

static void mtk_qinspect_reset_global_list(void) {
	g_wait_queue_insert = 0;
	g_wait_queue_extract = 0;
	g_cqs_rootlocker_idx = 0;
}

static void mtk_qinspect_cqs_unlock_32(uintptr_t evt, u8 operation, u64 val)
{
	switch (operation) {
	case MTK_BASEP_CQS_WAIT_OPERATION_GT:
		*(u32 *)evt = (u32)val+1;
		break;
	case MTK_BASEP_CQS_WAIT_OPERATION_LE:
	case MTK_BASEP_CQS_WAIT_OPERATION_GE:
		*(u32 *)evt = val;
		break;
	default:
		mtk_qinspect_log("cqs unlock 32 with unexpected operation = %u", operation);
		break;
	}
}

static void mtk_qinspect_cqs_unlock_64(uintptr_t evt, u8 operation, u64 val)
{
	switch (operation) {
	case MTK_BASEP_CQS_WAIT_OPERATION_GT:
		*(u64 *)evt = val+1;
		break;
	case MTK_BASEP_CQS_WAIT_OPERATION_LE:
	case MTK_BASEP_CQS_WAIT_OPERATION_GE:
		*(u64 *)evt = val;
		break;
	default:
		mtk_qinspect_log("cqs unlock 64 with unexpected operation = %u", operation);
		break;
	}
}

static int mtk_qinspect_unlock_cqs(struct mtk_qinspect_cqs_rootlocker *cqs_rl)
{
	struct kbase_vmap_struct *mapping;
	uintptr_t evt;

	evt = (uintptr_t)kbase_phy_alloc_mapping_get(
		cqs_rl->kctx, cqs_rl->obj.addr, &mapping);

	if (!evt) {
		mtk_qinspect_log("Sync memory %llx already freed", cqs_rl->obj.addr);
		return -1;
	}

	mtk_qinspect_log("unlock cqs obj, addr=%llx val=%llu threshold=%llu operation=%u",
		cqs_rl->obj.addr, *(u64 *)evt, cqs_rl->obj.val, cqs_rl->obj.operation);

	switch (cqs_rl->obj.data_type) {
	case BASEP_CQS_DATA_TYPE_U32:
		mtk_qinspect_cqs_unlock_32(evt, cqs_rl->obj.operation,	cqs_rl->obj.val);
		break;
	case BASEP_CQS_DATA_TYPE_U64:
		mtk_qinspect_cqs_unlock_64(evt, cqs_rl->obj.operation, cqs_rl->obj.val);
		break;
	default:
		mtk_qinspect_log("unexpected data type = %u", cqs_rl->obj.data_type);
		return -2;
	}

	mtk_qinspect_log("unlock cqs obj, addr=%llx new_val=%llu",
		cqs_rl->obj.addr, *(u64 *)evt);

	kbase_phy_alloc_mapping_put(cqs_rl->kctx, mapping);
	kbase_csf_event_signal_notify_gpu(cqs_rl->kctx);

	return 0;
}

static void mtk_qinspect_unlock_root_locker(void) {
	unsigned int i;

	// unlock cqs root locker
	for (i = 0; i < g_cqs_rootlocker_idx; i++)
		mtk_qinspect_unlock_cqs(&g_cqs_rootlocker_list[i]);

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
	//if (g_cqs_rootlocker_idx)
	//	aee_kernel_warning("GPU_RECOVERY", "\nCRDISPATCH_KEY:GPU_RECOVERY\nfound blocked cqs_wait");
#endif /* CONFIG_MTK_AEE_FEATURE */
}

#if 0
static void mtk_qinspect_save_shared_sb_root_locker(struct mtk_qinspect_shared_sb_wait_it *wait_it) {
	// TBD
	return;
}

static void mtk_qinspect_save_fence_root_locker(struct mtk_qinspect_fence_wait_it *wait_it) {
	// TBD
	return;
}
#endif

static void mtk_qinspect_save_cqs_root_locker(struct mtk_qinspect_cqs_wait_it *wait_it) {
	u64 objs_root_locker_map = wait_it->objs_map_mask ^ wait_it->objs_signaled_map ^
		wait_it->objs_failure_map ^ wait_it->objs_match_map ^ wait_it->objs_deadlock_map;
	unsigned int idx = 0;

	if (!objs_root_locker_map) {
		if (wait_it->objs_failure_map)
			mtk_qinspect_log("objs_failure_map=%llx", wait_it->objs_failure_map);
		return;
	}

	mtk_qinspect_log("objs_root_locker_map=%llx", objs_root_locker_map);
	mtk_qinspect_log("objs_map_mask=%llx", wait_it->objs_map_mask);
	mtk_qinspect_log("objs_signaled_map=%llx", wait_it->objs_signaled_map);
	mtk_qinspect_log("objs_failure_map=%llx", wait_it->objs_failure_map);
	mtk_qinspect_log("objs_match_map=%llx", wait_it->objs_match_map);
	mtk_qinspect_log("objs_deadlock_map=%llx", wait_it->objs_deadlock_map);

	while (objs_root_locker_map) {
		if (objs_root_locker_map & 0x1) {
			int rc;
			struct mtk_qinspect_cqs_wait_obj wait_obj;

			rc = mtk_qinspect_query_internal_cqs_wait_obj(wait_it, idx, &wait_obj);
			if (rc)
				mtk_qinspect_log("query cqs wait obj[%u] fail, skip it", idx);
			else {
				mtk_qinspect_log("find cqs root locker: wait_obj_nr=%u addr=%llx wait_val=%llu wait_cond=%d",
					idx, wait_obj.addr, wait_obj.val, wait_obj.operation);

				if (g_cqs_rootlocker_idx < MTK_QINSPECT_ARRYA_SIZE) {
					g_cqs_rootlocker_list[g_cqs_rootlocker_idx].kctx = wait_it->kctx;
					g_cqs_rootlocker_list[g_cqs_rootlocker_idx].obj.addr = wait_obj.addr;
					g_cqs_rootlocker_list[g_cqs_rootlocker_idx].obj.val = wait_obj.val;
					g_cqs_rootlocker_list[g_cqs_rootlocker_idx].obj.operation = wait_obj.operation;
					g_cqs_rootlocker_idx++;
				} else {
					mtk_qinspect_log("g_cqs_rootlocker_list is full, skip it");
				}
			}
		}

		objs_root_locker_map >>= 1;
		idx++;
	}
}

static bool mtk_qinspect_deadlock_check(enum mtk_qinspect_wait_cmd_type wait_cmd_type, void *wait_on)
{
	struct mtk_qinspect_wait_queue_node *cur_node = &g_wait_queue_list[g_wait_queue_extract];
	void *wait_on_queue;

	// get wait_on queue
	if (wait_cmd_type == MTK_QINSPECT_CQS_WAIT) {
		struct mtk_qinspect_cqs_wait_on *cqs_wait_on = (struct mtk_qinspect_cqs_wait_on *)wait_on;

		switch (cqs_wait_on->queue_type) {
		case QINSPECT_CPU_QUEUE:
			wait_on_queue = cqs_wait_on->cpu_queue_buf;
			break;
		case QINSPECT_KCPU_QUEUE:
			wait_on_queue = cqs_wait_on->kcpu_queue;
			break;
		case QINSPECT_GPU_QUEUE:
			wait_on_queue = cqs_wait_on->gpu_queue;
			break;
		default:
			mtk_qinspect_log("unexpected wait on queue type = %u", cqs_wait_on->queue_type);
			return true;
		}
	} else if (wait_cmd_type == MTK_QINSPECT_FENCE_WAIT) {
		struct mtk_qinspect_fence_wait_on *fence_wait_on = (struct mtk_qinspect_fence_wait_on *)wait_on;
		wait_on_queue = fence_wait_on->kcpu_queue;
	} else if (wait_cmd_type == MTK_QINSPECT_SHARED_SB_WAIT) {
		struct mtk_qinspect_shared_sb_wait_on *shared_sb_wait_on = (struct mtk_qinspect_shared_sb_wait_on *)wait_on;
		wait_on_queue = shared_sb_wait_on->gpu_queue;
	} else {
		mtk_qinspect_log("unexpected wait cmd type = %u", wait_cmd_type);
		return true;
	}

	// trace back parent link and check if wait_it queue and wait_on queue are the same
	while (cur_node) {
		if (cur_node->queue == wait_on_queue) {
			mtk_qinspect_log("find deadlock");
			return true;
		}

		cur_node = cur_node->parent;
	}

	return false;
}

static void mtk_qinspect_query_shared_sb_wait_it(struct mtk_qinspect_shared_sb_wait_it *wait_it)
{
	struct mtk_qinspect_shared_sb_wait_on *wait_on;

	while ((wait_on = mtk_qinspect_query_internal_shared_sb_wait_it(wait_it))) {
		mtk_qinspect_log("   * wait on CSG_%u_CSI_%d",
			wait_on->gpu_queue->group->handle, wait_on->gpu_queue->csi_index);

		if (!mtk_qinspect_deadlock_check(MTK_QINSPECT_SHARED_SB_WAIT, wait_on)) {
			wait_it->wait_on_found=true;
			mtk_qinspect_enqueue_wait_queue(QINSPECT_GPU_QUEUE, wait_on->gpu_queue);
		}
	}
}

static void mtk_qinspect_query_fence_wait_it(struct mtk_qinspect_fence_wait_it *wait_it) {
	struct mtk_qinspect_fence_wait_on *wait_on;

	while ((wait_on = mtk_qinspect_query_internal_fence_wait_it(wait_it))) {
		mtk_qinspect_log("   * wait on Ctx %d_%d, kcpu_queue_%u",
			wait_on->kctx->tgid, wait_on->kctx->id, wait_on->kcpu_queue->id);

		if (!mtk_qinspect_deadlock_check(MTK_QINSPECT_FENCE_WAIT, wait_on)) {
			wait_it->wait_on_found=true;

			// skip cross kctx fence set, handle it in another fence signal timeout
			if (wait_it->kctx == wait_on->kctx)
				mtk_qinspect_enqueue_wait_queue(QINSPECT_KCPU_QUEUE, wait_on->kcpu_queue);
		}
	}
}

static void mtk_qinspect_query_cqs_wait_it(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	struct mtk_qinspect_cqs_wait_on *wait_on;

	while ((wait_on = mtk_qinspect_query_internal_cqs_wait_it(wait_it))) {
		switch (wait_on->queue_type) {
		case QINSPECT_CPU_QUEUE:
			mtk_qinspect_log("   * wait on cpu_queue_%llx",
				wait_on->cpu_queue_buf->queue.queue);
			if (mtk_qinspect_deadlock_check(MTK_QINSPECT_CQS_WAIT, wait_on))
				mtk_qinspect_query_internal_cqs_wait_it_update_deadlock_map(wait_it);
			else
				mtk_qinspect_enqueue_wait_queue(wait_on->queue_type, wait_on->cpu_queue_buf);
			break;
		case QINSPECT_KCPU_QUEUE:
			mtk_qinspect_log("   * wait on kcpu_queue_%u",
				wait_on->kcpu_queue->id);
			if (mtk_qinspect_deadlock_check(MTK_QINSPECT_CQS_WAIT, wait_on))
				mtk_qinspect_query_internal_cqs_wait_it_update_deadlock_map(wait_it);
			else
				mtk_qinspect_enqueue_wait_queue(wait_on->queue_type, wait_on->kcpu_queue);
			break;
		case QINSPECT_GPU_QUEUE:
			mtk_qinspect_log("   * wait on CSG_%u_CSI_%d",
				wait_on->gpu_queue->group->handle, wait_on->gpu_queue->csi_index);
			if (mtk_qinspect_deadlock_check(MTK_QINSPECT_CQS_WAIT, wait_on))
				mtk_qinspect_query_internal_cqs_wait_it_update_deadlock_map(wait_it);
			else
				mtk_qinspect_enqueue_wait_queue(wait_on->queue_type, wait_on->gpu_queue);
			break;
		default:
			break;
		}
	}
}

static void mtk_qinspect_query_top_wait_cpuq(union mtk_qinspect_cpu_command_buf *queue_buf)
{
	struct mtk_qinspect_cpu_command *top_wait_cmd;
	int completed;

	mtk_qinspect_log("=> query top wait on cpu_queue_%llx", queue_buf->queue.queue);

	top_wait_cmd = mtk_qinspect_query_cpuq_internal_top_wait_cmd(queue_buf, &completed);
	if (top_wait_cmd) {
		/* process top_wait_cmd */
		if (top_wait_cmd->work_type == MTK_BASE_CPU_QUEUE_WORK_WAIT ||
			top_wait_cmd->work_type == MTK_BASE_CPU_QUEUE_WORK_WAIT_OP) {
			struct mtk_qinspect_cqs_wait_it wait_it;

			mtk_qinspect_log("   top wait is wait%s",
				top_wait_cmd->work_type == MTK_BASE_CPU_QUEUE_WORK_WAIT_OP ? "_op" : "");

			mtk_qinspect_query_internal_cqs_wait_it_init(QINSPECT_CPU_QUEUE,
				queue_buf, top_wait_cmd, &wait_it);
			mtk_qinspect_query_cqs_wait_it(&wait_it);
			mtk_qinspect_save_cqs_root_locker(&wait_it);
		} else {
			mtk_qinspect_log("   top wait is %d", top_wait_cmd->work_type);
		}
	} else
		mtk_qinspect_log("   top wait is NULL");
}

static void mtk_qinspect_query_top_wait_kcpuq(struct kbase_kcpu_command_queue *queue)
{
	struct kbase_kcpu_command *top_wait_cmd;
	int blocked;

	lockdep_assert_held(&queue->lock);

	mtk_qinspect_log("=> query top wait on kcpu_queue_%u", queue->id);

	top_wait_cmd = mtk_qinspect_query_kcpuq_internal_top_wait_cmd(queue, &blocked);
	if (top_wait_cmd) {
		/* process top_wait_cmd */
		if (top_wait_cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT ||
			top_wait_cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION) {
			struct mtk_qinspect_cqs_wait_it wait_it;

			mtk_qinspect_log("   top wait is cqs_wait%s",
				top_wait_cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION ? "_op" : "");

			mtk_qinspect_query_internal_cqs_wait_it_init(QINSPECT_KCPU_QUEUE,
				queue, top_wait_cmd, &wait_it);
			mtk_qinspect_query_cqs_wait_it(&wait_it);
			mtk_qinspect_save_cqs_root_locker(&wait_it);
		}
#if IS_ENABLED(CONFIG_SYNC_FILE)
		else if (top_wait_cmd->type == BASE_KCPU_COMMAND_TYPE_FENCE_WAIT) {
			struct mtk_qinspect_fence_wait_it wait_it;
			struct mtk_qinspect_fence_wait_on *wait_on;

			mtk_qinspect_log("   top wait is fence_wait");

			mtk_qinspect_query_internal_fence_wait_it_init(QINSPECT_KCPU_QUEUE,
				queue, &(top_wait_cmd->info.fence), &wait_it);
			mtk_qinspect_query_fence_wait_it(&wait_it);
			mtk_qinspect_query_internal_fence_wait_it_done(&wait_it);
			// mtk_qinspect_save_fence_root_locker(&wait_it);
		}
#endif
		else
			mtk_qinspect_log("   top wait is %d", top_wait_cmd->type);
	} else
		mtk_qinspect_log("   top wait is NULL");
}

static void mtk_qinspect_query_top_wait_gpuq(struct kbase_queue *queue)
{
	struct mtk_qinspect_gpu_command gpu_cmd_buf, *top_wait_cmd;
	u32 blocked_reason;

	mtk_qinspect_log("=> query top wait on CSG_%u_CSI_%d", queue->group->handle, queue->csi_index);

	top_wait_cmd = mtk_qinspect_query_gpuq_internal_top_wait_cmd(queue, &blocked_reason, &gpu_cmd_buf);
	if (top_wait_cmd) {
		/* process top_wait_cmd */
		if (top_wait_cmd->type == GPU_COMMAND_TYPE_SYNC_WAIT) {
			struct mtk_qinspect_cqs_wait_it wait_it;

			mtk_qinspect_log("   top wait is sync_wait: %llx",
				top_wait_cmd->gpu_sync_info.cqs_addr);

			mtk_qinspect_query_internal_cqs_wait_it_init(QINSPECT_GPU_QUEUE,
				queue, top_wait_cmd, &wait_it);
			mtk_qinspect_query_cqs_wait_it(&wait_it);
			mtk_qinspect_save_cqs_root_locker(&wait_it);
		} else if (top_wait_cmd->type == GPU_COMMAND_TYPE_SHARED_SB_WAIT) {
			struct mtk_qinspect_shared_sb_wait_it wait_it;
			struct mtk_qinspect_shared_sb_wait_on *wait_on;

			mtk_qinspect_log("   top wait is shared_sb_wait, se = %u",
				top_wait_cmd->gpu_shared_sb_info.shared_entry);

			mtk_qinspect_query_internal_shared_sb_wait_it_init(QINSPECT_GPU_QUEUE,
				queue, top_wait_cmd, &wait_it);
			mtk_qinspect_query_shared_sb_wait_it(&wait_it);
			// mtk_qinspect_save_shared_sb_root_locker(&wait_it);
		} else {
			mtk_qinspect_log("   top wait is %d, blocked_reason = %u",
				top_wait_cmd->type, blocked_reason);
		}
	} else
		mtk_qinspect_log("   top wait is NULL");
}

static void mtk_qinspect_query_top_wait(enum mtk_qinspect_queue_type queue_type, void *queue)
{
	if (queue == NULL) {
		mtk_qinspect_log("query queue is null, skip it");
		return;
	}

	switch (queue_type) {
	case QINSPECT_CPU_QUEUE:
		mtk_qinspect_query_top_wait_cpuq((union mtk_qinspect_cpu_command_buf *)queue);
		break;
	case QINSPECT_KCPU_QUEUE:
		if (!mutex_trylock(&(((struct kbase_kcpu_command_queue *)queue)->lock))) {
			mtk_qinspect_log("get kcpu queue lock fail, skip it");
		} else {
			mtk_qinspect_query_top_wait_kcpuq((struct kbase_kcpu_command_queue *)queue);
			mutex_unlock(&(((struct kbase_kcpu_command_queue *)queue)->lock));
		}
		break;
	case QINSPECT_GPU_QUEUE:
		mtk_qinspect_query_top_wait_gpuq((struct kbase_queue *)queue);
		break;
	default:
		mtk_qinspect_log("unexpected query queue type = %u", queue_type);
		break;
	}
}

static void mtk_qinspect_acquire_lock(struct kbase_context *kctx) {
	mutex_lock(&kctx->csf.kcpu_queues.lock);
	mutex_lock(&kctx->csf.lock);
	kbase_csf_scheduler_lock(kctx->kbdev);
}

static void mtk_qinspect_release_lock(struct kbase_context *kctx) {
	kbase_csf_scheduler_unlock(kctx->kbdev);
	mutex_unlock(&kctx->csf.lock);
	mutex_unlock(&kctx->csf.kcpu_queues.lock);
}

void mtk_qinspect_recovery(struct kbase_context *kctx, enum mtk_qinspect_queue_type queue_type, void *queue) {
	int i = 0;
	g_kctx = kctx;

	lockdep_assert_held(&recovery_lock);

	mtk_qinspect_log("Preparing to recovery");

	/* preprocess: 1. acquired lock 2. prevent gpu reset 3. load cpu queue 4. update groups status */
	mtk_qinspect_log("acquire lock");
	mtk_qinspect_acquire_lock(kctx);

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
	mtk_qinspect_log("prevent gpu reset");
	for (i = 0; i < 10; i++) {
		if (!kbase_reset_gpu_try_prevent(kctx->kbdev))
			break;
	}
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */
	if (i == 10) {
		mtk_qinspect_log("prevent gpu reset fail, skip this run");
		mtk_qinspect_release_lock(kctx);
		return;
	}

	mtk_qinspect_log("load cpu queue");
	if (mtk_qinspect_cpuq_internal_load_cpuq(kctx) < 0) {
		mtk_qinspect_log("load cpu queue fail, skip this run");
		kbase_reset_gpu_allow(kctx->kbdev);
		mtk_qinspect_release_lock(kctx);
		return;
	}

	mtk_qinspect_log("update groups status");
	kbase_csf_debugfs_update_active_groups_status(kctx->kbdev);

	/* find root locker algo start */
	mtk_qinspect_log("recovery start");

	// insert first query queue
	switch (queue_type) {
		case QINSPECT_CPU_QUEUE:
			mtk_qinspect_enqueue_wait_queue(QINSPECT_CPU_QUEUE,
				mtk_qinspect_search_cpuq_internal_by_queue_addr(kctx, (*(u64*)queue)));
			break;
		case QINSPECT_KCPU_QUEUE:
			mtk_qinspect_enqueue_wait_queue(QINSPECT_KCPU_QUEUE, queue);
			break;
		default:
			mtk_qinspect_log("unexpected entry queue type = %u", queue_type);
			break;
	}

	// start query queue
	while (g_wait_queue_insert != g_wait_queue_extract) {
		mtk_qinspect_query_top_wait(
			g_wait_queue_list[g_wait_queue_extract].queue_type,
			g_wait_queue_list[g_wait_queue_extract].queue);

		g_wait_queue_extract++;
	}

	mtk_qinspect_unlock_root_locker();

	mtk_qinspect_reset_global_list();

	/* find root locker algo end */
	mtk_qinspect_log("recovery end");

	/* postprocess: 1. unload cpu queue 2. allow gpu reset 3. release lock */
	mtk_qinspect_log("unload cpu queue");
	mtk_qinspect_cpuq_internal_unload_cpuq(kctx);

	mtk_qinspect_log("allow gpu reset");
	kbase_reset_gpu_allow(kctx->kbdev);

	mtk_qinspect_log("release lock");
	mtk_qinspect_release_lock(kctx);

	mtk_qinspect_log("recovery complete");
}

#endif /* CONFIG_MALI_CSF_SUPPORT */
