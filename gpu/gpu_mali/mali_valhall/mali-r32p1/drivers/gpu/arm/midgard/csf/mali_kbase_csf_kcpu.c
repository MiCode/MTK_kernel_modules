// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2018-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include <mali_kbase.h>
#include <tl/mali_kbase_tracepoints.h>
#include <mali_kbase_ctx_sched.h>
#include "device/mali_kbase_device.h"
#include "mali_kbase_csf.h"
#include <linux/export.h>

#if IS_ENABLED(CONFIG_SYNC_FILE)
#include "mali_kbase_fence.h"
#include "mali_kbase_sync.h"

static DEFINE_SPINLOCK(kbase_csf_fence_lock);
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include <mtk_gpufreq.h>
#include <platform/mtk_platform_common.h>
#endif

#ifdef CONFIG_MALI_FENCE_DEBUG
#include <platform/mtk_platform_common/mtk_platform_debug.h>
#define FENCE_WAIT_TIMEOUT_MS 3000
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
#include <mali_kbase_reset_gpu.h>
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
#define COMMAND_TIMEOUT_MS 5000
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

static void kcpu_queue_process(struct kbase_kcpu_command_queue *kcpu_queue,
			bool ignore_waits);

static void kcpu_queue_process_worker(struct work_struct *data);
#ifdef CONFIG_MALI_FENCE_DEBUG
static void kcpu_queue_timeout_worker(struct work_struct *data);
#endif

static int kbase_kcpu_map_import_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_import_info *import_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_va_region *reg;
	struct kbase_mem_phy_alloc *alloc;
	struct page **pages;
	struct tagged_addr *pa;
	long i;
	int ret = 0;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	/* Take the processes mmap lock */
	down_read(kbase_mem_get_process_mmap_lock());
	kbase_gpu_vm_lock(kctx);

	reg = kbase_region_tracker_find_region_enclosing_address(kctx,
					import_info->handle);

	if (kbase_is_region_invalid_or_free(reg) ||
	    !kbase_mem_is_imported(reg->gpu_alloc->type)) {
		ret = -EINVAL;
		goto out;
	}

	if (reg->gpu_alloc->type == KBASE_MEM_TYPE_IMPORTED_USER_BUF) {
		/* Pin the physical pages backing the user buffer while
		 * we are in the process context and holding the mmap lock.
		 * The dma mapping & GPU mapping of the pages would be done
		 * when the MAP_IMPORT operation is executed.
		 *
		 * Though the pages would be pinned, no reference is taken
		 * on the physical pages tracking object. When the last
		 * reference to the tracking object is dropped the pages
		 * would be unpinned if they weren't unpinned before.
		 */
		ret = kbase_jd_user_buf_pin_pages(kctx, reg);
		if (ret)
			goto out;

		alloc = reg->gpu_alloc;
		pa = kbase_get_gpu_phy_pages(reg);
		pages = alloc->imported.user_buf.pages;

		for (i = 0; i < alloc->nents; i++)
			pa[i] = as_tagged(page_to_phys(pages[i]));
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_MAP_IMPORT;
	current_command->info.import.gpu_va = import_info->handle;

out:
	kbase_gpu_vm_unlock(kctx);
	/* Release the processes mmap lock */
	up_read(kbase_mem_get_process_mmap_lock());

	return ret;
}

static int kbase_kcpu_unmap_import_prepare_internal(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_import_info *import_info,
		struct kbase_kcpu_command *current_command,
		enum base_kcpu_command_type type)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_va_region *reg;
	int ret = 0;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	kbase_gpu_vm_lock(kctx);

	reg = kbase_region_tracker_find_region_enclosing_address(kctx,
					import_info->handle);

	if (kbase_is_region_invalid_or_free(reg) ||
	    !kbase_mem_is_imported(reg->gpu_alloc->type)) {
		ret = -EINVAL;
		goto out;
	}

	if (reg->gpu_alloc->type == KBASE_MEM_TYPE_IMPORTED_USER_BUF) {
		/* The pages should have been pinned when MAP_IMPORT
		 * was enqueued previously.
		 */
		if (reg->gpu_alloc->nents !=
		    reg->gpu_alloc->imported.user_buf.nr_pages) {
			ret = -EINVAL;
			goto out;
		}
	}

	current_command->type = type;
	current_command->info.import.gpu_va = import_info->handle;

out:
	kbase_gpu_vm_unlock(kctx);

	return ret;
}

static int kbase_kcpu_unmap_import_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_import_info *import_info,
		struct kbase_kcpu_command *current_command)
{
	return kbase_kcpu_unmap_import_prepare_internal(kcpu_queue,
			import_info, current_command,
			BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT);
}

static int kbase_kcpu_unmap_import_force_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_import_info *import_info,
		struct kbase_kcpu_command *current_command)
{
	return kbase_kcpu_unmap_import_prepare_internal(kcpu_queue,
			import_info, current_command,
			BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT_FORCE);
}

/**
 * kbase_jit_add_to_pending_alloc_list() - Pend JIT allocation
 *
 * @queue: The queue containing this JIT allocation
 * @cmd:   The JIT allocation that is blocking this queue
 */
static void kbase_jit_add_to_pending_alloc_list(
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command *cmd)
{
	struct kbase_context *const kctx = queue->kctx;
	struct list_head *target_list_head =
			&kctx->csf.kcpu_queues.jit_blocked_queues;
	struct kbase_kcpu_command_queue *blocked_queue;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	list_for_each_entry(blocked_queue,
			&kctx->csf.kcpu_queues.jit_blocked_queues,
			jit_blocked) {
		struct kbase_kcpu_command const*const jit_alloc_cmd =
				&blocked_queue->commands[blocked_queue->start_offset];

		WARN_ON(jit_alloc_cmd->type != BASE_KCPU_COMMAND_TYPE_JIT_ALLOC);
		if (cmd->enqueue_ts < jit_alloc_cmd->enqueue_ts) {
			target_list_head = &blocked_queue->jit_blocked;
			break;
		}
	}

	list_add_tail(&queue->jit_blocked, target_list_head);
}

/**
 * kbase_kcpu_jit_allocate_process() - Process JIT allocation
 *
 * @queue: The queue containing this JIT allocation
 * @cmd:   The JIT allocation command
 */
static int kbase_kcpu_jit_allocate_process(
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command *cmd)
{
	struct kbase_context *const kctx = queue->kctx;
	struct kbase_kcpu_command_jit_alloc_info *alloc_info =
			&cmd->info.jit_alloc;
	struct base_jit_alloc_info *info = alloc_info->info;
	struct kbase_vmap_struct mapping;
	struct kbase_va_region *reg;
	u32 count = alloc_info->count;
	u64 *ptr, new_addr;
	u32 i;
	int ret;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (alloc_info->blocked) {
		list_del(&queue->jit_blocked);
		alloc_info->blocked = false;
	}

	if (WARN_ON(!info))
		return -EINVAL;

	/* Check if all JIT IDs are not in use */
	for (i = 0; i < count; i++, info++) {
		/* The JIT ID is still in use so fail the allocation */
		if (kctx->jit_alloc[info->id]) {
			dev_warn(kctx->kbdev->dev, "JIT ID still in use\n");
			return -EINVAL;
		}
	}

	/* Now start the allocation loop */
	for (i = 0, info = alloc_info->info; i < count; i++, info++) {
		/* Create a JIT allocation */
		reg = kbase_jit_allocate(kctx, info, true);
		if (!reg) {
			bool can_block = false;
			struct kbase_kcpu_command const *jit_cmd;

			list_for_each_entry(jit_cmd, &kctx->csf.kcpu_queues.jit_cmds_head, info.jit_alloc.node) {
				if (jit_cmd == cmd)
					break;

				if (jit_cmd->type == BASE_KCPU_COMMAND_TYPE_JIT_FREE) {
					u8 const*const free_ids = jit_cmd->info.jit_free.ids;

					if (free_ids && *free_ids && kctx->jit_alloc[*free_ids]) {
						/*
						 * A JIT free which is active
						 * and submitted before this
						 * command.
						 */
						can_block = true;
						break;
					}
				}
			}

			if (!can_block) {
				/*
				 * No prior JIT_FREE command is active. Roll
				 * back previous allocations and fail.
				 */
				dev_warn_ratelimited(kctx->kbdev->dev, "JIT alloc command failed: %pK\n", cmd);
				ret = -ENOMEM;
				goto fail;
			}

			/* There are pending frees for an active allocation
			 * so we should wait to see whether they free the
			 * memory. Add to the list of atoms for which JIT
			 * allocation is pending.
			 */
			kbase_jit_add_to_pending_alloc_list(queue, cmd);
			alloc_info->blocked = true;

			/* Rollback, the whole set will be re-attempted */
			while (i-- > 0) {
				info--;
				kbase_jit_free(kctx, kctx->jit_alloc[info->id]);
				kctx->jit_alloc[info->id] = NULL;
			}

			return -EAGAIN;
		}

		/* Bind it to the user provided ID. */
		kctx->jit_alloc[info->id] = reg;
	}

	for (i = 0, info = alloc_info->info; i < count; i++, info++) {
		/*
		 * Write the address of the JIT allocation to the user provided
		 * GPU allocation.
		 */
		ptr = kbase_vmap_prot(kctx, info->gpu_alloc_addr, sizeof(*ptr),
				KBASE_REG_CPU_WR, &mapping);
		if (!ptr) {
			ret = -ENOMEM;
			goto fail;
		}

		reg = kctx->jit_alloc[info->id];
		new_addr = reg->start_pfn << PAGE_SHIFT;
		*ptr = new_addr;
		kbase_vunmap(kctx, &mapping);
	}

	return 0;

fail:
	/* Roll back completely */
	for (i = 0, info = alloc_info->info; i < count; i++, info++) {
		/* Free the allocations that were successful.
		 * Mark all the allocations including the failed one and the
		 * other un-attempted allocations in the set, so we know they
		 * are in use.
		 */
		if (kctx->jit_alloc[info->id])
			kbase_jit_free(kctx, kctx->jit_alloc[info->id]);

		kctx->jit_alloc[info->id] = KBASE_RESERVED_REG_JIT_ALLOC;
	}

	return ret;
}

static int kbase_kcpu_jit_allocate_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_jit_alloc_info *alloc_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	void __user *data = u64_to_user_ptr(alloc_info->info);
	struct base_jit_alloc_info *info;
	u32 count = alloc_info->count;
	int ret = 0;
	u32 i;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (!kbase_mem_allow_alloc(kctx)) {
		dev_dbg(kctx->kbdev->dev,
			"Invalid attempt to allocate JIT memory by %s/%d for ctx %d_%d",
			current->comm, current->pid, kctx->tgid, kctx->id);
		ret = -EINVAL;
		goto out;
	}

	if (!data || count > kcpu_queue->kctx->jit_max_allocations ||
			count > ARRAY_SIZE(kctx->jit_alloc)) {
		ret = -EINVAL;
		goto out;
	}

	info = kmalloc_array(count, sizeof(*info), GFP_KERNEL);
	if (!info) {
		ret = -ENOMEM;
		goto out;
	}

	if (copy_from_user(info, data, sizeof(*info) * count) != 0) {
		ret = -EINVAL;
		goto out_free;
	}

	for (i = 0; i < count; i++) {
		ret = kbasep_jit_alloc_validate(kctx, &info[i]);
		if (ret)
			goto out_free;
	}

	/* Search for duplicate JIT ids */
	for (i = 0; i < (count - 1); i++) {
		u32 j;

		for (j = (i + 1); j < count; j++) {
			if (info[i].id == info[j].id) {
				ret = -EINVAL;
				goto out_free;
			}
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_JIT_ALLOC;
	list_add_tail(&current_command->info.jit_alloc.node,
			&kctx->csf.kcpu_queues.jit_cmds_head);
	current_command->info.jit_alloc.info = info;
	current_command->info.jit_alloc.count = count;
	current_command->info.jit_alloc.blocked = false;

	return 0;
out_free:
	kfree(info);
out:
	return ret;
}

/**
 * kbase_kcpu_jit_allocate_finish() - Finish handling the JIT_ALLOC command
 *
 * @queue: The queue containing this JIT allocation
 * @cmd:  The JIT allocation command
 */
static void kbase_kcpu_jit_allocate_finish(
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command *cmd)
{
	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	/* Remove this command from the jit_cmds_head list */
	list_del(&cmd->info.jit_alloc.node);

	/*
	 * If we get to this point we must have already cleared the blocked
	 * flag, otherwise it'd be a bug.
	 */
	if (WARN_ON(cmd->info.jit_alloc.blocked)) {
		list_del(&queue->jit_blocked);
		cmd->info.jit_alloc.blocked = false;
	}

	kfree(cmd->info.jit_alloc.info);
}

/**
 * kbase_kcpu_jit_retry_pending_allocs() - Retry blocked JIT_ALLOC commands
 *
 * @kctx: The context containing the blocked JIT_ALLOC commands
 */
static void kbase_kcpu_jit_retry_pending_allocs(struct kbase_context *kctx)
{
	struct kbase_kcpu_command_queue *blocked_queue;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	/*
	 * Reschedule all queues blocked by JIT_ALLOC commands.
	 * NOTE: This code traverses the list of blocked queues directly. It
	 * only works as long as the queued works are not executed at the same
	 * time. This precondition is true since we're holding the
	 * kbase_csf_kcpu_queue_context.lock .
	 */
	list_for_each_entry(blocked_queue,
			&kctx->csf.kcpu_queues.jit_blocked_queues, jit_blocked)
		queue_work(kctx->csf.kcpu_queues.wq, &blocked_queue->work);
}

static int kbase_kcpu_jit_free_process(struct kbase_kcpu_command_queue *queue,
				       struct kbase_kcpu_command *const cmd)
{
	struct kbase_kcpu_command_jit_free_info const *const free_info =
		&cmd->info.jit_free;
	u8 const *const ids = free_info->ids;
	u32 const count = free_info->count;
	u32 i;
	int rc = 0;
	struct kbase_context *kctx = queue->kctx;

	if (WARN_ON(!ids))
		return -EINVAL;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	KBASE_TLSTREAM_TL_KBASE_ARRAY_BEGIN_KCPUQUEUE_EXECUTE_JIT_FREE_END(
		queue->kctx->kbdev, queue);

	for (i = 0; i < count; i++) {
		u64 pages_used = 0;
		int item_err = 0;

		if (!kctx->jit_alloc[ids[i]]) {
			dev_warn(kctx->kbdev->dev, "invalid JIT free ID\n");
			rc = -EINVAL;
			item_err = rc;
		} else {
			struct kbase_va_region *const reg = kctx->jit_alloc[ids[i]];

			/*
			 * If the ID is valid but the allocation request failed, still
			 * succeed this command but don't try and free the allocation.
			 */
			if (reg != KBASE_RESERVED_REG_JIT_ALLOC) {
				pages_used = reg->gpu_alloc->nents;
				kbase_jit_free(kctx, reg);
			}

			kctx->jit_alloc[ids[i]] = NULL;
		}

		KBASE_TLSTREAM_TL_KBASE_ARRAY_ITEM_KCPUQUEUE_EXECUTE_JIT_FREE_END(
			queue->kctx->kbdev, queue, item_err, pages_used);
	}

	/* Free the list of ids */
	kfree(ids);

	/*
	 * Remove this command from the jit_cmds_head list and retry pending
	 * allocations.
	 */
	list_del(&cmd->info.jit_free.node);
	kbase_kcpu_jit_retry_pending_allocs(kctx);

	return rc;
}

static int kbase_kcpu_jit_free_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_jit_free_info *free_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	void __user *data = u64_to_user_ptr(free_info->ids);
	u8 *ids;
	u32 count = free_info->count;
	int ret;
	u32 i;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	/* Sanity checks */
	if (!count || count > ARRAY_SIZE(kctx->jit_alloc)) {
		ret = -EINVAL;
		goto out;
	}

	/* Copy the information for safe access and future storage */
	ids = kmalloc_array(count, sizeof(*ids), GFP_KERNEL);
	if (!ids) {
		ret = -ENOMEM;
		goto out;
	}

	if (!data) {
		ret = -EINVAL;
		goto out_free;
	}

	if (copy_from_user(ids, data, sizeof(*ids) * count)) {
		ret = -EINVAL;
		goto out_free;
	}

	for (i = 0; i < count; i++) {
		/* Fail the command if ID sent is zero */
		if (!ids[i]) {
			ret = -EINVAL;
			goto out_free;
		}
	}

	/* Search for duplicate JIT ids */
	for (i = 0; i < (count - 1); i++) {
		u32 j;

		for (j = (i + 1); j < count; j++) {
			if (ids[i] == ids[j]) {
				ret = -EINVAL;
				goto out_free;
			}
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_JIT_FREE;
	list_add_tail(&current_command->info.jit_free.node,
			&kctx->csf.kcpu_queues.jit_cmds_head);
	current_command->info.jit_free.ids = ids;
	current_command->info.jit_free.count = count;

	return 0;
out_free:
	kfree(ids);
out:
	return ret;
}

static int kbase_csf_queue_group_suspend_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_group_suspend_info *suspend_buf,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_suspend_copy_buffer *sus_buf = NULL;
	const u32 csg_suspend_buf_size =
		kctx->kbdev->csf.global_iface.groups[0].suspend_size;
	u64 addr = suspend_buf->buffer;
	u64 page_addr = addr & PAGE_MASK;
	u64 end_addr = addr + csg_suspend_buf_size - 1;
	u64 last_page_addr = end_addr & PAGE_MASK;
	int nr_pages = (last_page_addr - page_addr) / PAGE_SIZE + 1;
	int pinned_pages = 0, ret = 0;
	struct kbase_va_region *reg;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (suspend_buf->size < csg_suspend_buf_size)
		return -EINVAL;

	ret = kbase_csf_queue_group_handle_is_valid(kctx,
			suspend_buf->group_handle);
	if (ret)
		return ret;

	sus_buf = kzalloc(sizeof(*sus_buf), GFP_KERNEL);
	if (!sus_buf)
		return -ENOMEM;

	sus_buf->size = csg_suspend_buf_size;
	sus_buf->nr_pages = nr_pages;
	sus_buf->offset = addr & ~PAGE_MASK;

	sus_buf->pages = kcalloc(nr_pages, sizeof(struct page *), GFP_KERNEL);
	if (!sus_buf->pages) {
		ret = -ENOMEM;
		goto out_clean_sus_buf;
	}

	/* Check if the page_addr is a valid GPU VA from SAME_VA zone,
	 * otherwise consider it is a CPU VA corresponding to the Host
	 * memory allocated by userspace.
	 */
	kbase_gpu_vm_lock(kctx);
	reg = kbase_region_tracker_find_region_enclosing_address(kctx,
							page_addr);

	if (kbase_is_region_invalid_or_free(reg)) {
		kbase_gpu_vm_unlock(kctx);
		pinned_pages = get_user_pages_fast(page_addr, nr_pages, 1,
							sus_buf->pages);
		kbase_gpu_vm_lock(kctx);

		if (pinned_pages != nr_pages) {
			int i;

			/* Unpin those partially pinned pages */
			for (i = 0; i < pinned_pages; i++)
				kbase_unpin_user_buf_page(sus_buf->pages[i]);

			/* Set the error return value */
			ret = (pinned_pages < 0) ? pinned_pages : -EINVAL;
			goto out_clean_pages;
		}
	} else {
		struct tagged_addr *page_array;
		u64 start, end, i;

		if (((reg->flags & KBASE_REG_ZONE_MASK) != KBASE_REG_ZONE_SAME_VA) ||
		    (kbase_reg_current_backed_size(reg) < nr_pages) ||
		    !(reg->flags & KBASE_REG_CPU_WR) ||
		    (reg->gpu_alloc->type != KBASE_MEM_TYPE_NATIVE) ||
		    (kbase_is_region_shrinkable(reg)) ||
		    (kbase_va_region_is_no_user_free(kctx, reg))) {
			ret = -EINVAL;
			goto out_clean_pages;
		}

		start = PFN_DOWN(page_addr) - reg->start_pfn;
		end = start + nr_pages;

		if (end > reg->nr_pages) {
			ret = -EINVAL;
			goto out_clean_pages;
		}

		sus_buf->cpu_alloc = kbase_mem_phy_alloc_get(reg->cpu_alloc);
		kbase_mem_phy_alloc_kernel_mapped(reg->cpu_alloc);
		page_array = kbase_get_cpu_phy_pages(reg);
		page_array += start;

		for (i = 0; i < nr_pages; i++, page_array++)
			sus_buf->pages[i] = as_page(*page_array);
	}

	kbase_gpu_vm_unlock(kctx);
	current_command->type = BASE_KCPU_COMMAND_TYPE_GROUP_SUSPEND;
	current_command->info.suspend_buf_copy.sus_buf = sus_buf;
	current_command->info.suspend_buf_copy.group_handle =
				suspend_buf->group_handle;
	return ret;

out_clean_pages:
	kbase_gpu_vm_unlock(kctx);
	kfree(sus_buf->pages);
out_clean_sus_buf:
	kfree(sus_buf);

	return ret;
}

static int kbase_csf_queue_group_suspend_process(struct kbase_context *kctx,
		struct kbase_suspend_copy_buffer *sus_buf,
		u8 group_handle)
{
	return kbase_csf_queue_group_suspend(kctx, sus_buf, group_handle);
}

static enum kbase_csf_event_callback_action event_cqs_callback(void *param)
{
	struct kbase_kcpu_command_queue *kcpu_queue =
		(struct kbase_kcpu_command_queue *)param;
	struct kbase_context *const kctx = kcpu_queue->kctx;

	queue_work(kctx->csf.kcpu_queues.wq, &kcpu_queue->work);

	return KBASE_CSF_EVENT_CALLBACK_KEEP;
}

static void cleanup_cqs_wait(struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_wait_info *cqs_wait)
{
	WARN_ON(!cqs_wait->nr_objs);
	WARN_ON(!cqs_wait->objs);
	WARN_ON(!cqs_wait->signaled);
	WARN_ON(!queue->cqs_wait_count);

	if (--queue->cqs_wait_count == 0) {
		kbase_csf_event_wait_remove(queue->kctx,
				event_cqs_callback, queue);
	}

	kfree(cqs_wait->signaled);
	kfree(cqs_wait->objs);
	cqs_wait->signaled = NULL;
	cqs_wait->objs = NULL;
}

static int kbase_kcpu_cqs_wait_process(struct kbase_device *kbdev,
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_wait_info *cqs_wait)
{
	u32 i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!cqs_wait->objs))
		return -EINVAL;

	/* Skip the CQS waits that have already been signaled when processing */
	for (i = find_first_zero_bit(cqs_wait->signaled, cqs_wait->nr_objs); i < cqs_wait->nr_objs; i++) {
		if (!test_bit(i, cqs_wait->signaled)) {
			struct kbase_vmap_struct *mapping;
			bool sig_set;
			u32 *evt = (u32 *)kbase_phy_alloc_mapping_get(queue->kctx,
						cqs_wait->objs[i].addr, &mapping);

			if (!queue->command_started) {
				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_CQS_WAIT_START(
					kbdev, queue);
				queue->command_started = true;
				KBASE_KTRACE_ADD_CSF_KCPU(kbdev, CQS_WAIT_START,
						   queue, cqs_wait->nr_objs, 0);
			}

			if (!evt) {
				dev_warn(kbdev->dev,
					"Sync memory %llx already freed", cqs_wait->objs[i].addr);
				queue->has_error = true;
				return -EINVAL;
			}

			sig_set =
				evt[BASEP_EVENT32_VAL_OFFSET / sizeof(u32)] > cqs_wait->objs[i].val;
			if (sig_set) {
				bool error = false;

				bitmap_set(cqs_wait->signaled, i, 1);
				if ((cqs_wait->inherit_err_flags & (1U << i)) &&
				    evt[BASEP_EVENT32_ERR_OFFSET / sizeof(u32)] > 0) {
					queue->has_error = true;
					error = true;
				}

				KBASE_KTRACE_ADD_CSF_KCPU(kbdev, CQS_WAIT_END,
						queue, cqs_wait->objs[i].addr,
						error);

				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_CQS_WAIT_END(
					kbdev, queue, evt[BASEP_EVENT32_ERR_OFFSET / sizeof(u32)]);
				queue->command_started = false;
			}

			kbase_phy_alloc_mapping_put(queue->kctx, mapping);

			if (!sig_set)
				break;
		}
	}

	/* For the queue to progress further, all cqs objects should get
	 * signaled.
	 */
	return bitmap_full(cqs_wait->signaled, cqs_wait->nr_objs);
}

static inline bool kbase_kcpu_cqs_is_data_type_valid(u8 data_type)
{
	return data_type == BASEP_CQS_DATA_TYPE_U32 || data_type == BASEP_CQS_DATA_TYPE_U64;
}

static inline bool kbase_kcpu_cqs_is_aligned(u64 addr, u8 data_type)
{
	BUILD_BUG_ON(BASEP_EVENT32_ALIGN_BYTES != BASEP_EVENT32_SIZE_BYTES);
	BUILD_BUG_ON(BASEP_EVENT64_ALIGN_BYTES != BASEP_EVENT64_SIZE_BYTES);
	WARN_ON(!kbase_kcpu_cqs_is_data_type_valid(data_type));

	switch (data_type) {
	default:
		return false;
	case BASEP_CQS_DATA_TYPE_U32:
		return (addr & (BASEP_EVENT32_ALIGN_BYTES - 1)) == 0;
	case BASEP_CQS_DATA_TYPE_U64:
		return (addr & (BASEP_EVENT64_ALIGN_BYTES - 1)) == 0;
	}
}

static int kbase_kcpu_cqs_wait_prepare(struct kbase_kcpu_command_queue *queue,
		struct base_kcpu_command_cqs_wait_info *cqs_wait_info,
		struct kbase_kcpu_command *current_command)
{
	struct base_cqs_wait_info *objs;
	unsigned int nr_objs = cqs_wait_info->nr_objs;
	unsigned int i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (nr_objs > BASEP_KCPU_CQS_MAX_NUM_OBJS)
		return -EINVAL;

	if (!nr_objs)
		return -EINVAL;

	objs = kcalloc(nr_objs, sizeof(*objs), GFP_KERNEL);
	if (!objs)
		return -ENOMEM;

	if (copy_from_user(objs, u64_to_user_ptr(cqs_wait_info->objs),
			nr_objs * sizeof(*objs))) {
		kfree(objs);
		return -ENOMEM;
	}

	/* Check the CQS objects as early as possible. By checking their alignment
	 * (required alignment equals to size for Sync32 and Sync64 objects), we can
	 * prevent overrunning the supplied event page.
	 */
	for (i = 0; i < nr_objs; i++) {
		if (!kbase_kcpu_cqs_is_aligned(objs[i].addr, BASEP_CQS_DATA_TYPE_U32)) {
			kfree(objs);
			return -EINVAL;
		}
	}

	if (++queue->cqs_wait_count == 1) {
		if (kbase_csf_event_wait_add(queue->kctx,
				event_cqs_callback, queue)) {
			kfree(objs);
			queue->cqs_wait_count--;
			return -ENOMEM;
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_CQS_WAIT;
	current_command->info.cqs_wait.nr_objs = nr_objs;
	current_command->info.cqs_wait.objs = objs;
	current_command->info.cqs_wait.inherit_err_flags =
					cqs_wait_info->inherit_err_flags;

	current_command->info.cqs_wait.signaled = kcalloc(BITS_TO_LONGS(nr_objs),
		sizeof(*current_command->info.cqs_wait.signaled), GFP_KERNEL);
	if (!current_command->info.cqs_wait.signaled) {
		if (--queue->cqs_wait_count == 0) {
			kbase_csf_event_wait_remove(queue->kctx,
				event_cqs_callback, queue);
		}

		kfree(objs);
		return -ENOMEM;
	}

	return 0;
}

static void kbase_kcpu_cqs_set_process(struct kbase_device *kbdev,
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_set_info *cqs_set)
{
	unsigned int i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!cqs_set->objs))
		return;

	for (i = 0; i < cqs_set->nr_objs; i++) {
		struct kbase_vmap_struct *mapping;
		u32 *evt;

		evt = (u32 *)kbase_phy_alloc_mapping_get(
			queue->kctx, cqs_set->objs[i].addr, &mapping);

		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_CQS_SET(kbdev, queue,
								  evt ? 0 : 1);

		if (!evt) {
			dev_warn(kbdev->dev,
				"Sync memory %llx already freed", cqs_set->objs[i].addr);
			queue->has_error = true;
		} else {
			evt[BASEP_EVENT32_ERR_OFFSET / sizeof(u32)] = queue->has_error;
			/* Set to signaled */
			evt[BASEP_EVENT32_VAL_OFFSET / sizeof(u32)]++;
			kbase_phy_alloc_mapping_put(queue->kctx, mapping);

			KBASE_KTRACE_ADD_CSF_KCPU(kbdev, CQS_SET, queue, cqs_set->objs[i].addr,
						  evt[BASEP_EVENT32_ERR_OFFSET / sizeof(u32)]);
		}
	}

	kbase_csf_event_signal_notify_gpu(queue->kctx);

	kfree(cqs_set->objs);
	cqs_set->objs = NULL;
}

static int kbase_kcpu_cqs_set_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_cqs_set_info *cqs_set_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct base_cqs_set *objs;
	unsigned int nr_objs = cqs_set_info->nr_objs;
	unsigned int i;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (nr_objs > BASEP_KCPU_CQS_MAX_NUM_OBJS)
		return -EINVAL;

	if (!nr_objs)
		return -EINVAL;

	objs = kcalloc(nr_objs, sizeof(*objs), GFP_KERNEL);
	if (!objs)
		return -ENOMEM;

	if (copy_from_user(objs, u64_to_user_ptr(cqs_set_info->objs),
			nr_objs * sizeof(*objs))) {
		kfree(objs);
		return -ENOMEM;
	}

	/* Check the CQS objects as early as possible. By checking their alignment
	 * (required alignment equals to size for Sync32 and Sync64 objects), we can
	 * prevent overrunning the supplied event page.
	 */
	for (i = 0; i < nr_objs; i++) {
		if (!kbase_kcpu_cqs_is_aligned(objs[i].addr, BASEP_CQS_DATA_TYPE_U32)) {
			kfree(objs);
			return -EINVAL;
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_CQS_SET;
	current_command->info.cqs_set.nr_objs = nr_objs;
	current_command->info.cqs_set.objs = objs;

	return 0;
}

static void cleanup_cqs_wait_operation(struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_wait_operation_info *cqs_wait_operation)
{
	WARN_ON(!cqs_wait_operation->nr_objs);
	WARN_ON(!cqs_wait_operation->objs);
	WARN_ON(!cqs_wait_operation->signaled);
	WARN_ON(!queue->cqs_wait_count);

	if (--queue->cqs_wait_count == 0) {
		kbase_csf_event_wait_remove(queue->kctx,
				event_cqs_callback, queue);
	}

	kfree(cqs_wait_operation->signaled);
	kfree(cqs_wait_operation->objs);
	cqs_wait_operation->signaled = NULL;
	cqs_wait_operation->objs = NULL;
}

static int kbase_kcpu_cqs_wait_operation_process(struct kbase_device *kbdev,
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_wait_operation_info *cqs_wait_operation)
{
	u32 i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!cqs_wait_operation->objs))
		return -EINVAL;

	/* Skip the CQS waits that have already been signaled when processing */
	for (i = find_first_zero_bit(cqs_wait_operation->signaled, cqs_wait_operation->nr_objs); i < cqs_wait_operation->nr_objs; i++) {
		if (!test_bit(i, cqs_wait_operation->signaled)) {
			struct kbase_vmap_struct *mapping;
			bool sig_set;
			uintptr_t evt = (uintptr_t)kbase_phy_alloc_mapping_get(
				queue->kctx, cqs_wait_operation->objs[i].addr, &mapping);
			u64 val = 0;

			/* GPUCORE-28172 RDT to review */
			if (!queue->command_started)
				queue->command_started = true;

			if (!evt) {
				dev_warn(kbdev->dev,
					"Sync memory %llx already freed", cqs_wait_operation->objs[i].addr);
				queue->has_error = true;
				return -EINVAL;
			}

			switch (cqs_wait_operation->objs[i].data_type) {
			default:
				WARN_ON(!kbase_kcpu_cqs_is_data_type_valid(
					cqs_wait_operation->objs[i].data_type));
				kbase_phy_alloc_mapping_put(queue->kctx, mapping);
				queue->has_error = true;
				return -EINVAL;
			case BASEP_CQS_DATA_TYPE_U32:
				val = *(u32 *)evt;
				evt += BASEP_EVENT32_ERR_OFFSET - BASEP_EVENT32_VAL_OFFSET;
				break;
			case BASEP_CQS_DATA_TYPE_U64:
				val = *(u64 *)evt;
				evt += BASEP_EVENT64_ERR_OFFSET - BASEP_EVENT64_VAL_OFFSET;
				break;
			}

			switch (cqs_wait_operation->objs[i].operation) {
			case BASEP_CQS_WAIT_OPERATION_LE:
				sig_set = val <= cqs_wait_operation->objs[i].val;
				break;
			case BASEP_CQS_WAIT_OPERATION_GT:
				sig_set = val > cqs_wait_operation->objs[i].val;
				break;
			default:
				dev_warn(kbdev->dev,
					"Unsupported CQS wait operation %d", cqs_wait_operation->objs[i].operation);

				kbase_phy_alloc_mapping_put(queue->kctx, mapping);
				queue->has_error = true;

				return -EINVAL;
			}

			if (sig_set) {
				bitmap_set(cqs_wait_operation->signaled, i, 1);
				if ((cqs_wait_operation->inherit_err_flags & (1U << i)) &&
				    *(u32 *)evt > 0) {
					queue->has_error = true;
				}

				/* GPUCORE-28172 RDT to review */

				queue->command_started = false;
			}

			kbase_phy_alloc_mapping_put(queue->kctx, mapping);

			if (!sig_set)
				break;
		}
	}

	/* For the queue to progress further, all cqs objects should get
	 * signaled.
	 */
	return bitmap_full(cqs_wait_operation->signaled, cqs_wait_operation->nr_objs);
}

static int kbase_kcpu_cqs_wait_operation_prepare(struct kbase_kcpu_command_queue *queue,
		struct base_kcpu_command_cqs_wait_operation_info *cqs_wait_operation_info,
		struct kbase_kcpu_command *current_command)
{
	struct base_cqs_wait_operation_info *objs;
	unsigned int nr_objs = cqs_wait_operation_info->nr_objs;
	unsigned int i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (nr_objs > BASEP_KCPU_CQS_MAX_NUM_OBJS)
		return -EINVAL;

	if (!nr_objs)
		return -EINVAL;

	objs = kcalloc(nr_objs, sizeof(*objs), GFP_KERNEL);
	if (!objs)
		return -ENOMEM;

	if (copy_from_user(objs, u64_to_user_ptr(cqs_wait_operation_info->objs),
			nr_objs * sizeof(*objs))) {
		kfree(objs);
		return -ENOMEM;
	}

	/* Check the CQS objects as early as possible. By checking their alignment
	 * (required alignment equals to size for Sync32 and Sync64 objects), we can
	 * prevent overrunning the supplied event page.
	 */
	for (i = 0; i < nr_objs; i++) {
		if (!kbase_kcpu_cqs_is_data_type_valid(objs[i].data_type) ||
		    !kbase_kcpu_cqs_is_aligned(objs[i].addr, objs[i].data_type)) {
			kfree(objs);
			return -EINVAL;
		}
	}

	if (++queue->cqs_wait_count == 1) {
		if (kbase_csf_event_wait_add(queue->kctx,
				event_cqs_callback, queue)) {
			kfree(objs);
			queue->cqs_wait_count--;
			return -ENOMEM;
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION;
	current_command->info.cqs_wait_operation.nr_objs = nr_objs;
	current_command->info.cqs_wait_operation.objs = objs;
	current_command->info.cqs_wait_operation.inherit_err_flags =
					cqs_wait_operation_info->inherit_err_flags;

	current_command->info.cqs_wait_operation.signaled = kcalloc(BITS_TO_LONGS(nr_objs),
		sizeof(*current_command->info.cqs_wait_operation.signaled), GFP_KERNEL);
	if (!current_command->info.cqs_wait_operation.signaled) {
		if (--queue->cqs_wait_count == 0) {
			kbase_csf_event_wait_remove(queue->kctx,
				event_cqs_callback, queue);
		}

		kfree(objs);
		return -ENOMEM;
	}

	return 0;
}

static void kbasep_kcpu_cqs_do_set_operation_32(struct kbase_kcpu_command_queue *queue,
						uintptr_t evt, u8 operation, u64 val)
{
	struct kbase_device *kbdev = queue->kctx->kbdev;

	switch (operation) {
	case BASEP_CQS_SET_OPERATION_ADD:
		*(u32 *)evt += (u32)val;
		break;
	case BASEP_CQS_SET_OPERATION_SET:
		*(u32 *)evt = val;
		break;
	default:
		dev_dbg(kbdev->dev, "Unsupported CQS set operation %d", operation);
		queue->has_error = true;
		break;
	}
}

static void kbasep_kcpu_cqs_do_set_operation_64(struct kbase_kcpu_command_queue *queue,
						uintptr_t evt, u8 operation, u64 val)
{
	struct kbase_device *kbdev = queue->kctx->kbdev;

	switch (operation) {
	case BASEP_CQS_SET_OPERATION_ADD:
		*(u64 *)evt += val;
		break;
	case BASEP_CQS_SET_OPERATION_SET:
		*(u64 *)evt = val;
		break;
	default:
		dev_dbg(kbdev->dev, "Unsupported CQS set operation %d", operation);
		queue->has_error = true;
		break;
	}
}

static void kbase_kcpu_cqs_set_operation_process(
		struct kbase_device *kbdev,
		struct kbase_kcpu_command_queue *queue,
		struct kbase_kcpu_command_cqs_set_operation_info *cqs_set_operation)
{
	unsigned int i;

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!cqs_set_operation->objs))
		return;

	for (i = 0; i < cqs_set_operation->nr_objs; i++) {
		struct kbase_vmap_struct *mapping;
		uintptr_t evt;

		evt = (uintptr_t)kbase_phy_alloc_mapping_get(
			queue->kctx, cqs_set_operation->objs[i].addr, &mapping);

		/* GPUCORE-28172 RDT to review */

		if (!evt) {
			dev_warn(kbdev->dev,
				"Sync memory %llx already freed", cqs_set_operation->objs[i].addr);
			queue->has_error = true;
		} else {
			struct base_cqs_set_operation_info *obj = &cqs_set_operation->objs[i];

			switch (obj->data_type) {
			default:
				WARN_ON(!kbase_kcpu_cqs_is_data_type_valid(obj->data_type));
				queue->has_error = true;
				goto skip_err_propagation;
			case BASEP_CQS_DATA_TYPE_U32:
				kbasep_kcpu_cqs_do_set_operation_32(queue, evt, obj->operation,
								    obj->val);
				evt += BASEP_EVENT32_ERR_OFFSET - BASEP_EVENT32_VAL_OFFSET;
				break;
			case BASEP_CQS_DATA_TYPE_U64:
				kbasep_kcpu_cqs_do_set_operation_64(queue, evt, obj->operation,
								    obj->val);
				evt += BASEP_EVENT64_ERR_OFFSET - BASEP_EVENT64_VAL_OFFSET;
				break;
			}

			/* GPUCORE-28172 RDT to review */

			/* Always propagate errors */
			*(u32 *)evt = queue->has_error;

skip_err_propagation:
			kbase_phy_alloc_mapping_put(queue->kctx, mapping);
		}
	}

	kbase_csf_event_signal_notify_gpu(queue->kctx);

	kfree(cqs_set_operation->objs);
	cqs_set_operation->objs = NULL;
}

static int kbase_kcpu_cqs_set_operation_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_cqs_set_operation_info *cqs_set_operation_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct base_cqs_set_operation_info *objs;
	unsigned int nr_objs = cqs_set_operation_info->nr_objs;
	unsigned int i;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (nr_objs > BASEP_KCPU_CQS_MAX_NUM_OBJS)
		return -EINVAL;

	if (!nr_objs)
		return -EINVAL;

	objs = kcalloc(nr_objs, sizeof(*objs), GFP_KERNEL);
	if (!objs)
		return -ENOMEM;

	if (copy_from_user(objs, u64_to_user_ptr(cqs_set_operation_info->objs),
			nr_objs * sizeof(*objs))) {
		kfree(objs);
		return -ENOMEM;
	}

	/* Check the CQS objects as early as possible. By checking their alignment
	 * (required alignment equals to size for Sync32 and Sync64 objects), we can
	 * prevent overrunning the supplied event page.
	 */
	for (i = 0; i < nr_objs; i++) {
		if (!kbase_kcpu_cqs_is_data_type_valid(objs[i].data_type) ||
		    !kbase_kcpu_cqs_is_aligned(objs[i].addr, objs[i].data_type)) {
			kfree(objs);
			return -EINVAL;
		}
	}

	current_command->type = BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION;
	current_command->info.cqs_set_operation.nr_objs = nr_objs;
	current_command->info.cqs_set_operation.objs = objs;

	return 0;
}

#if IS_ENABLED(CONFIG_SYNC_FILE)
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
static void kbase_csf_fence_wait_callback(struct fence *fence,
			struct fence_cb *cb)
#else
static void kbase_csf_fence_wait_callback(struct dma_fence *fence,
			struct dma_fence_cb *cb)
#endif
{
	struct kbase_kcpu_command_fence_info *fence_info = container_of(cb,
			struct kbase_kcpu_command_fence_info, fence_cb);
	struct kbase_kcpu_command_queue *kcpu_queue = fence_info->kcpu_queue;
	struct kbase_context *const kctx = kcpu_queue->kctx;

#ifdef CONFIG_MALI_FENCE_DEBUG
	/* Fence gets signaled. Deactivate the timer for fence-wait timeout */
	del_timer(&kcpu_queue->fence_timeout);
#endif
	KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev, FENCE_WAIT_END, kcpu_queue,
				  fence->context, fence->seqno);

	/* Resume kcpu command queue processing. */
	queue_work(kctx->csf.kcpu_queues.wq, &kcpu_queue->work);
}

static void kbase_kcpu_fence_wait_cancel(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct kbase_kcpu_command_fence_info *fence_info)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!fence_info->fence))
		return;

	if (kcpu_queue->fence_wait_processed) {
		bool removed = dma_fence_remove_callback(fence_info->fence,
				&fence_info->fence_cb);

#ifdef CONFIG_MALI_FENCE_DEBUG
		/* Fence-wait cancelled or fence signaled. In the latter case
		 * the timer would already have been deactivated inside
		 * kbase_csf_fence_wait_callback().
		 */
		del_timer_sync(&kcpu_queue->fence_timeout);
#endif
		if (removed)
			KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev, FENCE_WAIT_END,
					kcpu_queue, fence_info->fence->context,
					fence_info->fence->seqno);
	}

	/* Release the reference which is kept by the kcpu_queue */
	kbase_fence_put(fence_info->fence);
	kcpu_queue->fence_wait_processed = false;

	fence_info->fence = NULL;
}

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
static void pending_cmds_timer_callback(struct timer_list *timer)
{
	struct kbase_kcpu_command_queue *kcpu_queue =
		container_of(timer, struct kbase_kcpu_command_queue, pending_cmds_timer);
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_kcpu_command *cmd = &kcpu_queue->commands[kcpu_queue->start_offset];
	dev_info(kctx->kbdev->dev,
		 "KCPU queue has stuck in %ums! ctx=%d_%d queue_idx=%u cmd_type=%u start_offset=%u",
		 COMMAND_TIMEOUT_MS, kctx->tgid, kctx->id, kcpu_queue->id, cmd->type, kcpu_queue->start_offset);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	ged_log_buf_print2(
		kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
		"KCPU queue has stuck in %ums! ctx=%d_%d queue_idx=%u cmd_type=%u start_offset=%u\n",
		COMMAND_TIMEOUT_MS, kctx->tgid, kctx->id, kcpu_queue->id, cmd->type, kcpu_queue->start_offset);
#endif
	queue_work(kcpu_queue->cmds_timeout_wq, &kcpu_queue->cmds_timeout_work);
}
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

#ifdef CONFIG_MALI_FENCE_DEBUG
/**
 * fence_timeout_callback() - Timeout callback function for fence-wait
 *
 * @timer: Timer struct
 *
 * Context and seqno of the timed-out fence will be displayed in dmesg.
 * If the fence has been signalled a work will be enqueued to process
 * the fence-wait without displaying debugging information.
 */
static void fence_timeout_callback(struct timer_list *timer)
{
	struct kbase_kcpu_command_queue *kcpu_queue =
		container_of(timer, struct kbase_kcpu_command_queue, fence_timeout);
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_kcpu_command *cmd = &kcpu_queue->commands[kcpu_queue->start_offset];
	struct kbase_kcpu_command_fence_info *fence_info;
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	struct fence *fence;
#else
	struct dma_fence *fence;
#endif
	struct kbase_sync_fence_info info;

	if (cmd->type != BASE_KCPU_COMMAND_TYPE_FENCE_WAIT) {
		dev_info(kctx->kbdev->dev,
			"%s: Unexpected command type %d in ctx:%d_%d kcpu queue:%u", __func__,
			cmd->type, kctx->tgid, kctx->id, kcpu_queue->id);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"%s: Unexpected command type %d in ctx:%d_%d kcpu queue:%u\n",
			__func__, cmd->type, kctx->tgid, kctx->id, kcpu_queue->id);
#endif
		return;
	}

	fence_info = &cmd->info.fence;

	fence = kbase_fence_get(fence_info);
	if (!fence) {
		dev_info(kctx->kbdev->dev, "no fence found in ctx:%d_%d kcpu queue:%u", kctx->tgid,
			kctx->id, kcpu_queue->id);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"no fence found in ctx:%d_%d kcpu queue:%u\n",
			kctx->tgid, kctx->id, kcpu_queue->id);
#endif
		return;
	}

	kbase_sync_fence_info_get(fence, &info);

	if (info.status == 1) {
		queue_work(kctx->csf.kcpu_queues.wq, &kcpu_queue->work);
	} else if (info.status == 0) {
		dev_info(kctx->kbdev->dev, "fence has not yet signalled in %ums",
			 FENCE_WAIT_TIMEOUT_MS);
		dev_info(kctx->kbdev->dev,
			 "ctx:%d_%d kcpu queue:%u still waiting for fence[%px] context#seqno:%s (driver=%s, timeline=%s)",
			 kctx->tgid, kctx->id, kcpu_queue->id, fence, info.name,
			 fence->ops->get_driver_name(fence), fence->ops->get_timeline_name(fence));
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"fence has not yet signalled in %ums\n",
			FENCE_WAIT_TIMEOUT_MS);
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"ctx:%d_%d kcpu queue:%u still waiting for fence[%px] context#seqno:%s (driver=%s, timeline=%s)\n",
			kctx->tgid, kctx->id, kcpu_queue->id, fence, info.name,
			fence->ops->get_driver_name(fence), fence->ops->get_timeline_name(fence));
#endif
		queue_work(kctx->csf.kcpu_queues.timeout_wq, &kcpu_queue->timeout_work);
	} else {
		dev_info(kctx->kbdev->dev, "fence has got error");
		dev_info(kctx->kbdev->dev,
			 "ctx:%d_%d kcpu queue:%u faulty fence[%px] context#seqno:%s error(%d)",
			 kctx->tgid, kctx->id, kcpu_queue->id, fence, info.name, info.status);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"fence has got error\n");
		ged_log_buf_print2(
			kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"ctx:%d_%d kcpu queue:%u faulty fence[%px] context#seqno:%s error(%d)\n",
			kctx->tgid, kctx->id, kcpu_queue->id, fence, info.name, info.status);
#endif
	}

	kbase_fence_put(fence);
}

/**
 * fence_timeout_start() - Start a timer to check fence-wait timeout
 *
 * @cmd: KCPU command queue
 *
 * Activate a timer to check whether a fence-wait command in the queue
 * gets completed  within FENCE_WAIT_TIMEOUT_MS
 */
static void fence_timeout_start(struct kbase_kcpu_command_queue *cmd)
{
	mod_timer(&cmd->fence_timeout, jiffies + msecs_to_jiffies(FENCE_WAIT_TIMEOUT_MS));
}
#endif

/**
 * kbase_kcpu_fence_wait_process() - Process the kcpu fence wait command
 *
 * @kcpu_queue: The queue containing the fence wait command
 * @fence_info: Reference to a fence for which the command is waiting
 *
 * Return: 0 if fence wait is blocked, 1 if it is unblocked, negative error if
 *         an error has occurred and fence should no longer be waited on.
 */
static int kbase_kcpu_fence_wait_process(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct kbase_kcpu_command_fence_info *fence_info)
{
	int fence_status = 0;
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	struct fence *fence;
#else
	struct dma_fence *fence;
#endif
	struct kbase_context *const kctx = kcpu_queue->kctx;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (WARN_ON(!fence_info->fence))
		return -EINVAL;

	fence = fence_info->fence;

	if (kcpu_queue->fence_wait_processed) {
		fence_status = dma_fence_get_status(fence);
	} else {
		int cb_err = dma_fence_add_callback(fence,
			&fence_info->fence_cb,
			kbase_csf_fence_wait_callback);

		KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev,
					  FENCE_WAIT_START, kcpu_queue,
					  fence->context, fence->seqno);
		fence_status = cb_err;
		if (cb_err == 0) {
			kcpu_queue->fence_wait_processed = true;
#ifdef CONFIG_MALI_FENCE_DEBUG
			fence_timeout_start(kcpu_queue);
#endif
		} else if (cb_err == -ENOENT) {
			fence_status = dma_fence_get_status(fence);
			if (!fence_status) {
				struct kbase_sync_fence_info info;

				kbase_sync_fence_info_get(fence, &info);
				dev_info(kctx->kbdev->dev,
					 "Unexpected status for fence %s of ctx:%d_%d kcpu queue:%u",
					 info.name, kctx->tgid, kctx->id, kcpu_queue->id);
			}
		}
	}

	/*
	 * At this point fence status can contain 3 types of values:
	 * - Value 0 to represent that fence in question is not signalled yet
	 * - Value 1 to represent that fence in question is signalled without
	 *   errors
	 * - Negative error code to represent that some error has occurred such
	 *   that waiting on it is no longer valid.
	 */

	if (fence_status)
		kbase_kcpu_fence_wait_cancel(kcpu_queue, fence_info);

	return fence_status;
}

static int kbase_kcpu_fence_wait_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_fence_info *fence_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	struct fence *fence_in;
#else
	struct dma_fence *fence_in;
#endif
	struct base_fence fence;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (copy_from_user(&fence, u64_to_user_ptr(fence_info->fence),
			sizeof(fence)))
		return -ENOMEM;

	fence_in = sync_file_get_fence(fence.basep.fd);

	if (!fence_in)
		return -ENOENT;

	current_command->type = BASE_KCPU_COMMAND_TYPE_FENCE_WAIT;
	current_command->info.fence.fence = fence_in;
	current_command->info.fence.kcpu_queue = kcpu_queue;

	return 0;
}

static int kbase_kcpu_fence_signal_process(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct kbase_kcpu_command_fence_info *fence_info)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
	int ret;

	if (WARN_ON(!fence_info->fence))
		return -EINVAL;

	ret = dma_fence_signal(fence_info->fence);

	if (unlikely(ret < 0)) {
		dev_warn(kctx->kbdev->dev,
			"fence_signal() failed with %d\n", ret);
	}

	KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev, FENCE_SIGNAL, kcpu_queue,
				  fence_info->fence->context,
				  fence_info->fence->seqno);

	dma_fence_put(fence_info->fence);
	fence_info->fence = NULL;

	return ret;
}

static int kbase_kcpu_fence_signal_prepare(
		struct kbase_kcpu_command_queue *kcpu_queue,
		struct base_kcpu_command_fence_info *fence_info,
		struct kbase_kcpu_command *current_command)
{
	struct kbase_context *const kctx = kcpu_queue->kctx;
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	struct fence *fence_out;
#else
	struct dma_fence *fence_out;
#endif
	struct base_fence fence;
	struct sync_file *sync_file;
	int ret = 0;
	int fd;

	lockdep_assert_held(&kctx->csf.kcpu_queues.lock);

	if (copy_from_user(&fence, u64_to_user_ptr(fence_info->fence),
			sizeof(fence)))
		return -EFAULT;

	fence_out = kzalloc(sizeof(*fence_out), GFP_KERNEL);
	if (!fence_out)
		return -ENOMEM;

	dma_fence_init(fence_out,
		       &kbase_fence_ops,
		       &kbase_csf_fence_lock,
		       kcpu_queue->fence_context,
		       ++kcpu_queue->fence_seqno);

#if (KERNEL_VERSION(4, 9, 67) >= LINUX_VERSION_CODE)
	/* Take an extra reference to the fence on behalf of the sync file.
	 * This is only needded on older kernels where sync_file_create()
	 * does not take its own reference. This was changed in v4.9.68
	 * where sync_file_create() now takes its own reference.
	 */
	dma_fence_get(fence_out);
#endif

	/* create a sync_file fd representing the fence */
	sync_file = sync_file_create(fence_out);
	if (!sync_file) {
#if (KERNEL_VERSION(4, 9, 67) >= LINUX_VERSION_CODE)
		dma_fence_put(fence_out);
#endif
		ret = -ENOMEM;
		goto file_create_fail;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		ret = fd;
		goto fd_flags_fail;
	}

	fence.basep.fd = fd;

	current_command->type = BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL;
	current_command->info.fence.fence = fence_out;

	if (copy_to_user(u64_to_user_ptr(fence_info->fence), &fence,
			sizeof(fence))) {
		ret = -EFAULT;
		goto fd_flags_fail;
	}

	/* 'sync_file' pointer can't be safely dereferenced once 'fd' is
	 * installed, so the install step needs to be done at the last
	 * before returning success.
	 */
	fd_install(fd, sync_file->file);
	return 0;

fd_flags_fail:
	fput(sync_file->file);
file_create_fail:
	dma_fence_put(fence_out);

	return ret;
}
#endif /* CONFIG_SYNC_FILE */

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
static void kcpu_queue_cmds_timeout_worker(struct work_struct *data)
{
	struct kbase_kcpu_command_queue *kcpu_queue = container_of(data,
				struct kbase_kcpu_command_queue, cmds_timeout_work);
	struct kbase_context *const kctx = kcpu_queue->kctx;
	struct kbase_kcpu_command *cmd = &kcpu_queue->commands[kcpu_queue->start_offset];
	dev_info(kctx->kbdev->dev,
		 "KCPU queue command timeouts(%d ms)! ctx=%d_%d queue_idx=%u cmd_type=%u start_offset=%u",
		 COMMAND_TIMEOUT_MS, kctx->tgid, kctx->id, kcpu_queue->id, cmd->type, kcpu_queue->start_offset);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	ged_log_buf_print2(
		kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
		"KCPU queue command timeouts(%d ms)! ctx=%d_%d queue_idx=%u cmd_type=%u start_offset=%u\n",
		COMMAND_TIMEOUT_MS, kctx->tgid, kctx->id, kcpu_queue->id, cmd->type, kcpu_queue->start_offset);

	mtk_debug_csf_dump_groups_and_queues(kctx->kbdev, kctx->tgid);

	if (!mtk_common_gpufreq_bringup()) {
		if (kctx->kbdev->pm.backend.gpu_powered)
			gpufreq_dump_infra_status();
		mtk_common_debug_dump();
	}
#endif /* CONFIG_MALI_MTK_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
	if (COMMAND_TIMEOUT_MS > 3000) {
		spin_lock(&kctx->kbdev->reset_force_change);
		kctx->kbdev->reset_force_evict_group_work = true;
		spin_unlock(&kctx->kbdev->reset_force_change);
		if (kbase_prepare_to_reset_gpu(kctx->kbdev, RESET_FLAGS_NONE)) {
			dev_info(kctx->kbdev->dev, "KCPU queue command timeouts(%d ms)! Trigger GPU reset", COMMAND_TIMEOUT_MS);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
			ged_log_buf_print2(
				kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
				"KCPU queue command timeouts(%d ms)! Trigger GPU reset\n",
				COMMAND_TIMEOUT_MS);
#endif
			kbase_reset_gpu(kctx->kbdev);
		} else {
			dev_info(kctx->kbdev->dev, "KCPU queue command timeouts(%d ms)! Other threads are already resetting the GPU", COMMAND_TIMEOUT_MS);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
			ged_log_buf_print2(
				kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
				"KCPU queue command timeouts(%d ms)! Other threads are already resetting the GPU\n",
				COMMAND_TIMEOUT_MS);
#endif
		}
	}
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */
}
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

#ifdef CONFIG_MALI_FENCE_DEBUG
static void kcpu_queue_timeout_worker(struct work_struct *data)
{
	struct kbase_kcpu_command_queue *kcpu_queue = container_of(data,
				struct kbase_kcpu_command_queue, timeout_work);
	struct kbase_context *const kctx = kcpu_queue->kctx;

	dev_info(kctx->kbdev->dev,
	         "%s: mali fence timeouts(%d ms)! kcpu_queue=%u pid=%d",
	         __func__,
	         FENCE_WAIT_TIMEOUT_MS,
	         kcpu_queue->id,
	         kctx->tgid);

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	ged_log_buf_print2(
		kctx->kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
		"%s: mali fence timeouts(%d ms)! kcpu_queue=%u pid=%d\n",
		__func__, FENCE_WAIT_TIMEOUT_MS, kcpu_queue->id, kctx->tgid);

#if IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
	mtk_debug_csf_dump_groups_and_queues(kctx->kbdev, kctx->tgid);
#endif /* CONFIG_MALI_MTK_FENCE_DEBUG */
	if (!mtk_common_gpufreq_bringup()) {
		if (kctx->kbdev->pm.backend.gpu_powered)
			gpufreq_dump_infra_status();
		mtk_common_debug_dump();
	}
#endif /* CONFIG_MALI_MTK_DEBUG */
}
#endif /* CONFIG_MALI_FENCE_DEBUG */

static void kcpu_queue_process_worker(struct work_struct *data)
{
	struct kbase_kcpu_command_queue *queue = container_of(data,
				struct kbase_kcpu_command_queue, work);

	mutex_lock(&queue->kctx->csf.kcpu_queues.lock);

	kcpu_queue_process(queue, false);

	mutex_unlock(&queue->kctx->csf.kcpu_queues.lock);
}

static int delete_queue(struct kbase_context *kctx, u32 id)
{
	int err = 0;

	mutex_lock(&kctx->csf.kcpu_queues.lock);

	if ((id < KBASEP_MAX_KCPU_QUEUES) && kctx->csf.kcpu_queues.array[id]) {
		struct kbase_kcpu_command_queue *queue =
					kctx->csf.kcpu_queues.array[id];

		KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev, KCPU_QUEUE_DESTROY,
			queue, queue->num_pending_cmds, queue->cqs_wait_count);

		/* Drain the remaining work for this queue first and go past
		 * all the waits.
		 */
		kcpu_queue_process(queue, true);

		/* All commands should have been processed */
		WARN_ON(queue->num_pending_cmds);

		/* All CQS wait commands should have been cleaned up */
		WARN_ON(queue->cqs_wait_count);

		kctx->csf.kcpu_queues.array[id] = NULL;
		bitmap_clear(kctx->csf.kcpu_queues.in_use, id, 1);

		/* Fire the tracepoint with the mutex held to enforce correct
		 * ordering with the summary stream.
		 */
		KBASE_TLSTREAM_TL_KBASE_DEL_KCPUQUEUE(kctx->kbdev, queue);

		mutex_unlock(&kctx->csf.kcpu_queues.lock);

		cancel_work_sync(&queue->work);
#ifdef CONFIG_MALI_FENCE_DEBUG
		cancel_work_sync(&queue->timeout_work);
#endif /* CONFIG_MALI_FENCE_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
		cancel_work_sync(&queue->cmds_timeout_work);
		destroy_workqueue(queue->cmds_timeout_wq);
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

		kfree(queue);
	} else {
		dev_warn(kctx->kbdev->dev,
			"Attempt to delete a non-existent KCPU queue\n");
		mutex_unlock(&kctx->csf.kcpu_queues.lock);
		err = -EINVAL;
	}
	return err;
}

static void KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_ALLOC_INFO(
	struct kbase_device *kbdev,
	const struct kbase_kcpu_command_queue *queue,
	const struct kbase_kcpu_command_jit_alloc_info *jit_alloc,
	int alloc_status)
{
	u8 i;

	KBASE_TLSTREAM_TL_KBASE_ARRAY_BEGIN_KCPUQUEUE_EXECUTE_JIT_ALLOC_END(
		kbdev, queue);
	for (i = 0; i < jit_alloc->count; i++) {
		const u8 id = jit_alloc->info[i].id;
		const struct kbase_va_region *reg = queue->kctx->jit_alloc[id];
		u64 gpu_alloc_addr = 0;
		u64 mmu_flags = 0;

		if ((alloc_status == 0) && !WARN_ON(!reg) &&
		    !WARN_ON(reg == KBASE_RESERVED_REG_JIT_ALLOC)) {
#ifdef CONFIG_MALI_VECTOR_DUMP
			struct tagged_addr phy = {0};
#endif /* CONFIG_MALI_VECTOR_DUMP */

			gpu_alloc_addr = reg->start_pfn << PAGE_SHIFT;
#ifdef CONFIG_MALI_VECTOR_DUMP
			mmu_flags = kbase_mmu_create_ate(kbdev,
				phy, reg->flags,
				MIDGARD_MMU_BOTTOMLEVEL,
				queue->kctx->jit_group_id);
#endif /* CONFIG_MALI_VECTOR_DUMP */
		}
		KBASE_TLSTREAM_TL_KBASE_ARRAY_ITEM_KCPUQUEUE_EXECUTE_JIT_ALLOC_END(
			kbdev, queue, alloc_status, gpu_alloc_addr, mmu_flags);
	}
}

static void KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_ALLOC_END(
	struct kbase_device *kbdev,
	const struct kbase_kcpu_command_queue *queue)
{
	KBASE_TLSTREAM_TL_KBASE_ARRAY_END_KCPUQUEUE_EXECUTE_JIT_ALLOC_END(
		kbdev, queue);
}

static void KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_FREE_END(
	struct kbase_device *kbdev,
	const struct kbase_kcpu_command_queue *queue)
{
	KBASE_TLSTREAM_TL_KBASE_ARRAY_END_KCPUQUEUE_EXECUTE_JIT_FREE_END(
		kbdev, queue);
}

static void kcpu_queue_process(struct kbase_kcpu_command_queue *queue,
			bool ignore_waits)
{
	struct kbase_device *kbdev = queue->kctx->kbdev;
	bool process_next = true;
	size_t i;
#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
	u16 num_pending_fence_cmds = 0;
	bool record_the_first_pending_cmd_done = false;
	u64 pending_cmd_cur_offset;
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

	lockdep_assert_held(&queue->kctx->csf.kcpu_queues.lock);

	for (i = 0; i != queue->num_pending_cmds; ++i) {
		struct kbase_kcpu_command *cmd =
			&queue->commands[(u8)(queue->start_offset + i)];
		int status;

		switch (cmd->type) {
		case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
			if (!queue->command_started) {
				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_FENCE_WAIT_START(
					kbdev, queue);
				queue->command_started = true;
			}

			status = 0;
#if IS_ENABLED(CONFIG_SYNC_FILE)
			if (ignore_waits) {
				kbase_kcpu_fence_wait_cancel(queue,
					&cmd->info.fence);
			} else {
				status = kbase_kcpu_fence_wait_process(queue,
					&cmd->info.fence);

				if (status == 0)
					process_next = false;
				else if (status < 0)
					queue->has_error = true;
			}
#else
			dev_warn(kbdev->dev,
				"unexpected fence wait command found\n");

			status = -EINVAL;
			queue->has_error = true;
#endif

			if (process_next) {
				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_FENCE_WAIT_END(
					kbdev, queue, status < 0 ? status : 0);
				queue->command_started = false;
			}
			break;
		case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_FENCE_SIGNAL_START(
				kbdev, queue);

			status = 0;

#if IS_ENABLED(CONFIG_SYNC_FILE)
			status = kbase_kcpu_fence_signal_process(
				queue, &cmd->info.fence);

			if (status < 0)
				queue->has_error = true;
#else
			dev_warn(kbdev->dev,
				"unexpected fence signal command found\n");

			status = -EINVAL;
			queue->has_error = true;
#endif

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_FENCE_SIGNAL_END(
				kbdev, queue, status);
			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
			status = kbase_kcpu_cqs_wait_process(kbdev, queue,
						&cmd->info.cqs_wait);

			if (!status && !ignore_waits) {
				process_next = false;
			} else {
				/* Either all CQS objects were signaled or
				 * there was an error or the queue itself is
				 * being deleted.
				 * In all cases can move to the next command.
				 * TBD: handle the error
				 */
				cleanup_cqs_wait(queue,	&cmd->info.cqs_wait);
			}

			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_SET:
			kbase_kcpu_cqs_set_process(kbdev, queue,
				&cmd->info.cqs_set);

			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
			status = kbase_kcpu_cqs_wait_operation_process(kbdev, queue,
						&cmd->info.cqs_wait_operation);

			if (!status && !ignore_waits) {
				process_next = false;
			} else {
				/* Either all CQS objects were signaled or
				 * there was an error or the queue itself is
				 * being deleted.
				 * In all cases can move to the next command.
				 * TBD: handle the error
				 */
				cleanup_cqs_wait_operation(queue,	&cmd->info.cqs_wait_operation);
			}

			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
			kbase_kcpu_cqs_set_operation_process(kbdev, queue,
				&cmd->info.cqs_set_operation);

			break;
		case BASE_KCPU_COMMAND_TYPE_ERROR_BARRIER:
			/* Clear the queue's error state */
			queue->has_error = false;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_ERROR_BARRIER(
				kbdev, queue);
			break;
		case BASE_KCPU_COMMAND_TYPE_MAP_IMPORT: {
			struct kbase_ctx_ext_res_meta *meta = NULL;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_MAP_IMPORT_START(
				kbdev, queue);

			kbase_gpu_vm_lock(queue->kctx);
			meta = kbase_sticky_resource_acquire(
				queue->kctx, cmd->info.import.gpu_va);
			kbase_gpu_vm_unlock(queue->kctx);

			if (meta == NULL) {
				queue->has_error = true;
				dev_warn(kbdev->dev,
						"failed to map an external resource\n");
			}

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_MAP_IMPORT_END(
				kbdev, queue, meta ? 0 : 1);
			break;
		}
		case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT: {
			bool ret;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_UNMAP_IMPORT_START(
				kbdev, queue);

			kbase_gpu_vm_lock(queue->kctx);
			ret = kbase_sticky_resource_release(
				queue->kctx, NULL, cmd->info.import.gpu_va);
			kbase_gpu_vm_unlock(queue->kctx);

			if (!ret) {
				queue->has_error = true;
				dev_warn(kbdev->dev,
						"failed to release the reference. resource not found\n");
			}

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_UNMAP_IMPORT_END(
				kbdev, queue, ret ? 0 : 1);
			break;
		}
		case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT_FORCE: {
			bool ret;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_UNMAP_IMPORT_FORCE_START(
					kbdev, queue);

			kbase_gpu_vm_lock(queue->kctx);
			ret = kbase_sticky_resource_release_force(
				queue->kctx, NULL, cmd->info.import.gpu_va);
			kbase_gpu_vm_unlock(queue->kctx);

			if (!ret) {
				queue->has_error = true;
				dev_warn(kbdev->dev,
						"failed to release the reference. resource not found\n");
			}

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_UNMAP_IMPORT_FORCE_END(
				kbdev, queue, ret ? 0 : 1);
			break;
		}
		case BASE_KCPU_COMMAND_TYPE_JIT_ALLOC:
		{
			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_ALLOC_START(
				kbdev, queue);

			status = kbase_kcpu_jit_allocate_process(queue, cmd);
			if (status == -EAGAIN) {
				process_next = false;
			} else {
				if (status != 0)
					queue->has_error = true;

				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_ALLOC_INFO(
					kbdev, queue, &cmd->info.jit_alloc,
					status);

				kbase_kcpu_jit_allocate_finish(queue, cmd);
				KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_ALLOC_END(
						kbdev, queue);
			}
			break;
		}
		case BASE_KCPU_COMMAND_TYPE_JIT_FREE:
			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_FREE_START(
				kbdev, queue);

			status = kbase_kcpu_jit_free_process(queue, cmd);
			if (status)
				queue->has_error = true;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_JIT_FREE_END(
				kbdev, queue);
			break;
		case BASE_KCPU_COMMAND_TYPE_GROUP_SUSPEND: {
			struct kbase_suspend_copy_buffer *sus_buf =
					cmd->info.suspend_buf_copy.sus_buf;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_GROUP_SUSPEND_START(
				kbdev, queue);

			status = kbase_csf_queue_group_suspend_process(
					queue->kctx, sus_buf,
					cmd->info.suspend_buf_copy.group_handle);
			if (status)
				queue->has_error = true;

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_EXECUTE_GROUP_SUSPEND_END(
				kbdev, queue, status);

			if (!sus_buf->cpu_alloc) {
				int i;

				for (i = 0; i < sus_buf->nr_pages; i++)
					put_page(sus_buf->pages[i]);
			} else {
				kbase_mem_phy_alloc_kernel_unmapped(
					sus_buf->cpu_alloc);
				kbase_mem_phy_alloc_put(sus_buf->cpu_alloc);
			}

			kfree(sus_buf->pages);
			kfree(sus_buf);
			break;
		}
#if MALI_UNIT_TEST
		case BASE_KCPU_COMMAND_TYPE_SAMPLE_TIME: {
			u64 time = ktime_get_raw_ns();
			void *target_page = kmap(*cmd->info.sample_time.page);

			if (target_page) {
				memcpy(target_page +
					       cmd->info.sample_time.page_offset,
				       &time, sizeof(time));
				kunmap(*cmd->info.sample_time.page);
			} else {
				dev_warn(kbdev->dev,
					 "Could not kmap target page\n");
				queue->has_error = true;
			}
			put_page(*cmd->info.sample_time.page);
			kfree(cmd->info.sample_time.page);
			break;
		}
#endif /* MALI_UNIT_TEST */
		default:
			dev_warn(kbdev->dev,
				"Unrecognized command type\n");
			break;
		} /* switch */

		/*TBD: error handling */

		if (!process_next)
			break;
	}

	if (i > 0) {
		queue->start_offset += i;
		queue->num_pending_cmds -= i;

		/* If an attempt to enqueue commands failed then we must raise
		 * an event in case the client wants to retry now that there is
		 * free space in the buffer.
		 */
		if (queue->enqueue_failed) {
			queue->enqueue_failed = false;
			kbase_csf_event_signal_cpu_only(queue->kctx);
		}
	}

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
	for (i = 0; i != queue->num_pending_cmds; ++i) {
		struct kbase_kcpu_command *cmd =
			&queue->commands[(u8)(queue->start_offset + i)];
		switch (cmd->type) {
		case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
		case BASE_KCPU_COMMAND_TYPE_JIT_ALLOC:
			if (!record_the_first_pending_cmd_done) {
				pending_cmd_cur_offset = cmd->enqueue_ts;
				record_the_first_pending_cmd_done = true;
			}
			num_pending_fence_cmds++;
			break;
		default:
			break;
		} /* switch */
	}
	if (num_pending_fence_cmds) {
		if (!queue->pending_cmds_timer_active) {
			/* has pending cmds, timer not acitve -> start timer */
			queue->pending_cmds_timer_active = true;
			queue->pending_cmd_prev_offset = pending_cmd_cur_offset;
			mod_timer(&queue->pending_cmds_timer, jiffies + msecs_to_jiffies(COMMAND_TIMEOUT_MS));
		} else if (queue->pending_cmd_prev_offset != pending_cmd_cur_offset) {
			/* has pending cmds, timer active,
			   pending offeset is different with previous -> modify timer */
			queue->pending_cmd_prev_offset = pending_cmd_cur_offset;
			mod_timer(&queue->pending_cmds_timer, jiffies + msecs_to_jiffies(COMMAND_TIMEOUT_MS));
		}
	} else if (queue->pending_cmds_timer_active) {
		/* no pending cmds -> del timer */
		del_timer_sync(&queue->pending_cmds_timer);
		queue->pending_cmds_timer_active = false;
	}
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */
}

static size_t kcpu_queue_get_space(struct kbase_kcpu_command_queue *queue)
{
	return KBASEP_KCPU_QUEUE_SIZE - queue->num_pending_cmds;
}

static void KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_COMMAND(
	const struct kbase_kcpu_command_queue *queue,
	const struct kbase_kcpu_command *cmd)
{
	struct kbase_device *kbdev = queue->kctx->kbdev;

	switch (cmd->type) {
	case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_FENCE_WAIT(
			kbdev, queue, cmd->info.fence.fence);
		break;
	case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_FENCE_SIGNAL(
			kbdev, queue, cmd->info.fence.fence);
		break;
	case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
	{
		const struct base_cqs_wait_info *waits =
			cmd->info.cqs_wait.objs;
		u32 inherit_err_flags = cmd->info.cqs_wait.inherit_err_flags;
		unsigned int i;

		for (i = 0; i < cmd->info.cqs_wait.nr_objs; i++) {
			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_CQS_WAIT(
				kbdev, queue, waits[i].addr, waits[i].val,
				(inherit_err_flags & ((u32)1 << i)) ? 1 : 0);
		}
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_CQS_SET:
	{
		const struct base_cqs_set *sets = cmd->info.cqs_set.objs;
		unsigned int i;

		for (i = 0; i < cmd->info.cqs_set.nr_objs; i++) {
			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_CQS_SET(
				kbdev, queue, sets[i].addr);
		}
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
	{
		/* GPUCORE-28172 RDT to review */
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
	{
		/* GPUCORE-28172 RDT to review */
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_ERROR_BARRIER:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_ERROR_BARRIER(kbdev,
									queue);
		break;
	case BASE_KCPU_COMMAND_TYPE_MAP_IMPORT:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_MAP_IMPORT(
			kbdev, queue, cmd->info.import.gpu_va);
		break;
	case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_UNMAP_IMPORT(
			kbdev, queue, cmd->info.import.gpu_va);
		break;
	case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT_FORCE:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_UNMAP_IMPORT_FORCE(
			kbdev, queue, cmd->info.import.gpu_va);
		break;
	case BASE_KCPU_COMMAND_TYPE_JIT_ALLOC:
	{
		u8 i;

		KBASE_TLSTREAM_TL_KBASE_ARRAY_BEGIN_KCPUQUEUE_ENQUEUE_JIT_ALLOC(
			kbdev, queue);
		for (i = 0; i < cmd->info.jit_alloc.count; i++) {
			const struct base_jit_alloc_info *info =
				&cmd->info.jit_alloc.info[i];

			KBASE_TLSTREAM_TL_KBASE_ARRAY_ITEM_KCPUQUEUE_ENQUEUE_JIT_ALLOC(
				kbdev, queue, info->gpu_alloc_addr,
				info->va_pages, info->commit_pages,
				info->extension, info->id, info->bin_id,
				info->max_allocations, info->flags,
				info->usage_id);
		}
		KBASE_TLSTREAM_TL_KBASE_ARRAY_END_KCPUQUEUE_ENQUEUE_JIT_ALLOC(
			kbdev, queue);
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_JIT_FREE:
	{
		u8 i;

		KBASE_TLSTREAM_TL_KBASE_ARRAY_BEGIN_KCPUQUEUE_ENQUEUE_JIT_FREE(
			kbdev, queue);
		for (i = 0; i < cmd->info.jit_free.count; i++) {
			KBASE_TLSTREAM_TL_KBASE_ARRAY_ITEM_KCPUQUEUE_ENQUEUE_JIT_FREE(
				kbdev, queue, cmd->info.jit_free.ids[i]);
		}
		KBASE_TLSTREAM_TL_KBASE_ARRAY_END_KCPUQUEUE_ENQUEUE_JIT_FREE(
			kbdev, queue);
		break;
	}
	case BASE_KCPU_COMMAND_TYPE_GROUP_SUSPEND:
		KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_GROUP_SUSPEND(
			kbdev, queue, cmd->info.suspend_buf_copy.sus_buf,
			cmd->info.suspend_buf_copy.group_handle);
		break;
#if MALI_UNIT_TEST
	case BASE_KCPU_COMMAND_TYPE_SAMPLE_TIME:
		/*
		 * This is test-only KCPU command, no need to have a timeline
		 * entry
		 */
		break;
#endif /* MALI_UNIT_TEST */
	case BASE_KCPU_COMMAND_TYPE_COUNT:
		break;
	}
}

int kbase_csf_kcpu_queue_enqueue(struct kbase_context *kctx,
			struct kbase_ioctl_kcpu_queue_enqueue *enq)
{
	struct kbase_kcpu_command_queue *queue = NULL;
	void __user *user_cmds = u64_to_user_ptr(enq->addr);
	int ret = 0;
	u32 i;

	/* The offset to the first command that is being processed or yet to
	 * be processed is of u8 type, so the number of commands inside the
	 * queue cannot be more than 256.
	 */
	BUILD_BUG_ON(KBASEP_KCPU_QUEUE_SIZE > 256);

	/* Whilst the backend interface allows enqueueing multiple commands in
	 * a single operation, the Base interface does not expose any mechanism
	 * to do so. And also right now the handling is missing for the case
	 * where multiple commands are submitted and the enqueue of one of the
	 * command in the set fails after successfully enqueuing other commands
	 * in the set.
	 */
	if (enq->nr_commands != 1) {
		dev_err(kctx->kbdev->dev,
			"More than one commands enqueued\n");
		return -EINVAL;
	}

	mutex_lock(&kctx->csf.kcpu_queues.lock);

	if (!kctx->csf.kcpu_queues.array[enq->id]) {
		ret = -EINVAL;
		goto out;
	}

	queue = kctx->csf.kcpu_queues.array[enq->id];

	if (kcpu_queue_get_space(queue) < enq->nr_commands) {
		ret = -EBUSY;
		queue->enqueue_failed = true;
		goto out;
	}

	/* Copy all command's info to the command buffer.
	 * Note: it would be more efficient to process all commands in-line
	 * until we encounter an unresolved CQS_ / FENCE_WAIT, however, the
	 * interface allows multiple commands to be enqueued so we must account
	 * for the possibility to roll back.
	 */

	for (i = 0; (i != enq->nr_commands) && !ret; ++i, ++kctx->csf.kcpu_queues.num_cmds) {
		struct kbase_kcpu_command *kcpu_cmd =
			&queue->commands[(u8)(queue->start_offset + queue->num_pending_cmds + i)];
		struct base_kcpu_command command;
		unsigned int j;

		if (copy_from_user(&command, user_cmds, sizeof(command))) {
			ret = -EFAULT;
			goto out;
		}

		user_cmds = (void __user *)((uintptr_t)user_cmds +
				sizeof(struct base_kcpu_command));

		for (j = 0; j < sizeof(command.padding); j++) {
			if (command.padding[j] != 0) {
				dev_vdbg(kctx->kbdev->dev,
					"base_kcpu_command padding not 0\n");
				ret = -EINVAL;
				goto out;
			}
		}

		kcpu_cmd->enqueue_ts = kctx->csf.kcpu_queues.num_cmds;
		switch (command.type) {
		case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
#if IS_ENABLED(CONFIG_SYNC_FILE)
			ret = kbase_kcpu_fence_wait_prepare(queue,
						&command.info.fence, kcpu_cmd);
#else
			ret = -EINVAL;
			dev_warn(kctx->kbdev->dev, "fence wait command unsupported\n");
#endif
			break;
		case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
#if IS_ENABLED(CONFIG_SYNC_FILE)
			ret = kbase_kcpu_fence_signal_prepare(queue,
						&command.info.fence, kcpu_cmd);
#else
			ret = -EINVAL;
			dev_warn(kctx->kbdev->dev, "fence signal command unsupported\n");
#endif
			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
			ret = kbase_kcpu_cqs_wait_prepare(queue,
					&command.info.cqs_wait, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_SET:
			ret = kbase_kcpu_cqs_set_prepare(queue,
					&command.info.cqs_set, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
			ret = kbase_kcpu_cqs_wait_operation_prepare(queue,
					&command.info.cqs_wait_operation, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
			ret = kbase_kcpu_cqs_set_operation_prepare(queue,
					&command.info.cqs_set_operation, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_ERROR_BARRIER:
			kcpu_cmd->type = BASE_KCPU_COMMAND_TYPE_ERROR_BARRIER;
			ret = 0;
			break;
		case BASE_KCPU_COMMAND_TYPE_MAP_IMPORT:
			ret = kbase_kcpu_map_import_prepare(queue,
					&command.info.import, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT:
			ret = kbase_kcpu_unmap_import_prepare(queue,
					&command.info.import, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_UNMAP_IMPORT_FORCE:
			ret = kbase_kcpu_unmap_import_force_prepare(queue,
					&command.info.import, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_JIT_ALLOC:
			ret = kbase_kcpu_jit_allocate_prepare(queue,
					&command.info.jit_alloc, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_JIT_FREE:
			ret = kbase_kcpu_jit_free_prepare(queue,
					&command.info.jit_free, kcpu_cmd);
			break;
		case BASE_KCPU_COMMAND_TYPE_GROUP_SUSPEND:
			ret = kbase_csf_queue_group_suspend_prepare(queue,
					&command.info.suspend_buf_copy,
					kcpu_cmd);
			break;
#if MALI_UNIT_TEST
		case BASE_KCPU_COMMAND_TYPE_SAMPLE_TIME: {
			int const page_cnt = 1;

			kcpu_cmd->type = BASE_KCPU_COMMAND_TYPE_SAMPLE_TIME;
			kcpu_cmd->info.sample_time.page_addr =
				command.info.sample_time.time & PAGE_MASK;
			kcpu_cmd->info.sample_time.page_offset =
				command.info.sample_time.time & ~PAGE_MASK;
			kcpu_cmd->info.sample_time.page = kcalloc(
				page_cnt, sizeof(struct page *), GFP_KERNEL);
			if (!kcpu_cmd->info.sample_time.page) {
				ret = -ENOMEM;
			} else {
				int pinned_pages = get_user_pages_fast(
					kcpu_cmd->info.sample_time.page_addr,
					page_cnt, 1,
					kcpu_cmd->info.sample_time.page);

				if (pinned_pages < 0) {
					ret = pinned_pages;
					kfree(kcpu_cmd->info.sample_time.page);
				} else if (pinned_pages != page_cnt) {
					ret = -EINVAL;
					kfree(kcpu_cmd->info.sample_time.page);
				}
			}

			break;
		}
#endif /* MALI_UNIT_TEST */
		default:
			dev_warn(queue->kctx->kbdev->dev,
				"Unknown command type %u\n", command.type);
			ret = -EINVAL;
			break;
		}
	}

	if (!ret) {
		/* We only instrument the enqueues after all commands have been
		 * successfully enqueued, as if we do them during the enqueue
		 * and there is an error, we won't be able to roll them back
		 * like is done for the command enqueues themselves.
		 */
		for (i = 0; i != enq->nr_commands; ++i) {
			u8 cmd_idx = (u8)(queue->start_offset + queue->num_pending_cmds + i);

			KBASE_TLSTREAM_TL_KBASE_KCPUQUEUE_ENQUEUE_COMMAND(
				queue, &queue->commands[cmd_idx]);
		}

		queue->num_pending_cmds += enq->nr_commands;
		kcpu_queue_process(queue, false);
	} else {
		/* Roll back the number of enqueued commands */
		kctx->csf.kcpu_queues.num_cmds -= i;
	}

out:
	mutex_unlock(&kctx->csf.kcpu_queues.lock);

	return ret;
}

int kbase_csf_kcpu_queue_context_init(struct kbase_context *kctx)
{
	int idx;

	bitmap_zero(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

	for (idx = 0; idx < KBASEP_MAX_KCPU_QUEUES; ++idx)
		kctx->csf.kcpu_queues.array[idx] = NULL;

	kctx->csf.kcpu_queues.wq = alloc_workqueue("mali_kbase_csf_kcpu",
					WQ_UNBOUND | WQ_HIGHPRI, 0);
	if (!kctx->csf.kcpu_queues.wq)
		return -ENOMEM;

#ifdef CONFIG_MALI_FENCE_DEBUG
	kctx->csf.kcpu_queues.timeout_wq = alloc_workqueue("mali_kbase_csf_kcpu_timeout",
					WQ_UNBOUND | WQ_HIGHPRI, 0);
	if (!kctx->csf.kcpu_queues.timeout_wq)
		return -ENOMEM;
#endif /* CONFIG_MALI_FENCE_DEBUG */

	mutex_init(&kctx->csf.kcpu_queues.lock);

	kctx->csf.kcpu_queues.num_cmds = 0;

	return 0;
}

void kbase_csf_kcpu_queue_context_term(struct kbase_context *kctx)
{
	while (!bitmap_empty(kctx->csf.kcpu_queues.in_use,
			KBASEP_MAX_KCPU_QUEUES)) {
		int id = find_first_bit(kctx->csf.kcpu_queues.in_use,
				KBASEP_MAX_KCPU_QUEUES);

		if (WARN_ON(!kctx->csf.kcpu_queues.array[id]))
			clear_bit(id, kctx->csf.kcpu_queues.in_use);
		else
			(void)delete_queue(kctx, id);
	}

#ifdef CONFIG_MALI_FENCE_DEBUG
	destroy_workqueue(kctx->csf.kcpu_queues.timeout_wq);
#endif /* CONFIG_MALI_FENCE_DEBUG */
	destroy_workqueue(kctx->csf.kcpu_queues.wq);
	mutex_destroy(&kctx->csf.kcpu_queues.lock);
}

int kbase_csf_kcpu_queue_delete(struct kbase_context *kctx,
			struct kbase_ioctl_kcpu_queue_delete *del)
{
	return delete_queue(kctx, (u32)del->id);
}

int kbase_csf_kcpu_queue_new(struct kbase_context *kctx,
			struct kbase_ioctl_kcpu_queue_new *newq)
{
	struct kbase_kcpu_command_queue *queue;
	int idx;
	int ret = 0;

	/* The queue id is of u8 type and we use the index of the kcpu_queues
	 * array as an id, so the number of elements in the array can't be
	 * more than 256.
	 */
	BUILD_BUG_ON(KBASEP_MAX_KCPU_QUEUES > 256);

	mutex_lock(&kctx->csf.kcpu_queues.lock);

	idx = find_first_zero_bit(kctx->csf.kcpu_queues.in_use,
			KBASEP_MAX_KCPU_QUEUES);
	if (idx >= (int)KBASEP_MAX_KCPU_QUEUES) {
		ret = -ENOMEM;
		goto out;
	}

	if (WARN_ON(kctx->csf.kcpu_queues.array[idx])) {
		ret = -EINVAL;
		goto out;
	}

	queue = kzalloc(sizeof(*queue), GFP_KERNEL);

	if (!queue) {
		ret = -ENOMEM;
		goto out;
	}

	bitmap_set(kctx->csf.kcpu_queues.in_use, idx, 1);
	kctx->csf.kcpu_queues.array[idx] = queue;
	queue->kctx = kctx;
	queue->start_offset = 0;
	queue->num_pending_cmds = 0;
#if IS_ENABLED(CONFIG_SYNC_FILE)
	queue->fence_context = dma_fence_context_alloc(1);
	queue->fence_seqno = 0;
	queue->fence_wait_processed = false;
#endif
	queue->enqueue_failed = false;
	queue->command_started = false;
	INIT_LIST_HEAD(&queue->jit_blocked);
	queue->has_error = false;
	INIT_WORK(&queue->work, kcpu_queue_process_worker);
#ifdef CONFIG_MALI_FENCE_DEBUG
	INIT_WORK(&queue->timeout_work, kcpu_queue_timeout_worker);
#endif /* CONFIG_MALI_FENCE_DEBUG */
	queue->id = idx;

	newq->id = idx;

	/* Fire the tracepoint with the mutex held to enforce correct ordering
	 * with the summary stream.
	 */
	KBASE_TLSTREAM_TL_KBASE_NEW_KCPUQUEUE(
		kctx->kbdev, queue, kctx->id, queue->num_pending_cmds);

	KBASE_KTRACE_ADD_CSF_KCPU(kctx->kbdev, KCPU_QUEUE_NEW, queue,
		queue->fence_context, 0);
#if IS_ENABLED(CONFIG_MALI_FENCE_DEBUG)
	kbase_timer_setup(&queue->fence_timeout, fence_timeout_callback);
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_KCPU_DEBUG)
	queue->cmds_timeout_wq = alloc_workqueue("mali_kbase_csf_kcpu_cmds_timeout_wq_%i", WQ_UNBOUND | WQ_HIGHPRI, 0, idx);
	if (queue->cmds_timeout_wq == NULL) {
		kfree(queue);
		ret = -ENOMEM;
		goto out;
	}
	INIT_WORK(&queue->cmds_timeout_work, kcpu_queue_cmds_timeout_worker);
	kbase_timer_setup(&queue->pending_cmds_timer, pending_cmds_timer_callback);
#endif /* CONFIG_MALI_MTK_KCPU_DEBUG */

out:
	mutex_unlock(&kctx->csf.kcpu_queues.lock);

	return ret;
}
