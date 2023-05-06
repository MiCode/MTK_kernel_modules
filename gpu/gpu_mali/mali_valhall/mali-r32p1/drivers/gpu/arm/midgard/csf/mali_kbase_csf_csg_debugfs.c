// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2021 ARM Limited. All rights reserved.
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

#include "mali_kbase_csf_csg_debugfs.h"
#include <mali_kbase.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <csf/mali_kbase_csf_trace_buffer.h>

#include <mali_kbase_sync.h>
#include <csf/mali_kbase_csf_kcpu_debugfs.h>
#include <csf/mali_kbase_csf_cpu_queue_debugfs.h>
#include <csf/mali_kbase_csf.h>

#if IS_ENABLED(CONFIG_DEBUG_FS)
#include "mali_kbase_csf_tl_reader.h"

/**
 * blocked_reason_to_string() - Convert blocking reason id to a string
 *
 * @reason_id: blocked_reason
 *
 * Return: Suitable string
 */
static const char *blocked_reason_to_string(u32 reason_id)
{
	/* possible blocking reasons of a cs */
	static const char *const cs_blocked_reason[] = {
		[CS_STATUS_BLOCKED_REASON_REASON_UNBLOCKED] = "UNBLOCKED",
		[CS_STATUS_BLOCKED_REASON_REASON_WAIT] = "WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_PROGRESS_WAIT] =
			"PROGRESS_WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_SYNC_WAIT] = "SYNC_WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_DEFERRED] = "DEFERRED",
		[CS_STATUS_BLOCKED_REASON_REASON_RESOURCE] = "RESOURCE",
		[CS_STATUS_BLOCKED_REASON_REASON_FLUSH] = "FLUSH"
	};

	if (WARN_ON(reason_id >= ARRAY_SIZE(cs_blocked_reason)))
		return "UNKNOWN_BLOCKED_REASON_ID";

	return cs_blocked_reason[reason_id];
}

static void kbasep_csf_scheduler_dump_active_queue_cs_status_wait(
	struct seq_file *file, u32 wait_status, u32 wait_sync_value,
	u64 wait_sync_live_value, u64 wait_sync_pointer, u32 sb_status,
	u32 blocked_reason)
{
#define WAITING "Waiting"
#define NOT_WAITING "Not waiting"

	seq_printf(file, "SB_MASK: %d\n",
			CS_STATUS_WAIT_SB_MASK_GET(wait_status));
	seq_printf(file, "PROGRESS_WAIT: %s\n",
			CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ?
			WAITING : NOT_WAITING);
	seq_printf(file, "PROTM_PEND: %s\n",
			CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ?
			WAITING : NOT_WAITING);
	seq_printf(file, "SYNC_WAIT: %s\n",
			CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ?
			WAITING : NOT_WAITING);
	seq_printf(file, "WAIT_CONDITION: %s\n",
			CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ?
			"greater than" : "less or equal");
	seq_printf(file, "SYNC_POINTER: 0x%llx\n", wait_sync_pointer);
	seq_printf(file, "SYNC_VALUE: %d\n", wait_sync_value);
	seq_printf(file, "SYNC_LIVE_VALUE: 0x%016llx\n", wait_sync_live_value);
	seq_printf(file, "SB_STATUS: %u\n",
		   CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	seq_printf(file, "BLOCKED_REASON: %s\n",
		   blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(
			   blocked_reason)));
}

static void kbasep_csf_scheduler_dump_active_cs_trace(struct seq_file *file,
			struct kbase_csf_cmd_stream_info const *const stream)
{
	u32 val = kbase_csf_firmware_cs_input_read(stream,
			CS_INSTR_BUFFER_BASE_LO);
	u64 addr = ((u64)kbase_csf_firmware_cs_input_read(stream,
				CS_INSTR_BUFFER_BASE_HI) << 32) | val;
	val = kbase_csf_firmware_cs_input_read(stream,
				CS_INSTR_BUFFER_SIZE);

	seq_printf(file, "CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u\n", addr, val);

	/* Write offset variable address (pointer) */
	val = kbase_csf_firmware_cs_input_read(stream,
			CS_INSTR_BUFFER_OFFSET_POINTER_LO);
	addr = ((u64)kbase_csf_firmware_cs_input_read(stream,
			CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
	seq_printf(file, "CS_TRACE_BUF_OFFSET_PTR: 0x%16llx\n", addr);

	/* EVENT_SIZE and EVENT_STATEs */
	val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
	seq_printf(file, "TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x\n",
			CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
			CS_INSTR_CONFIG_EVENT_STATE_GET(val));
}

/**
 * kbasep_csf_scheduler_dump_active_queue() - Print GPU command queue
 *                                            debug information
 *
 * @file:  seq_file for printing to
 * @queue: Address of a GPU command queue to examine
 */
static void kbasep_csf_scheduler_dump_active_queue(struct seq_file *file,
		struct kbase_queue *queue)
{
	u32 *addr;
	u64 cs_extract;
	u64 cs_insert;
	u32 cs_active;
	u64 wait_sync_pointer;
	u32 wait_status, wait_sync_value;
	u32 sb_status;
	u32 blocked_reason;
	struct kbase_vmap_struct *mapping;
	u64 *evt;
	u64 wait_sync_live_value;

	if (!queue)
		return;

	if (WARN_ON(queue->csi_index == KBASEP_IF_NR_INVALID ||
		    !queue->group))
		return;

	/* Ring the doorbell to have firmware update CS_EXTRACT */
	kbase_csf_ring_cs_user_doorbell(queue->kctx->kbdev, queue);
	msleep(100);

	addr = (u32 *)queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO/4] | ((u64)addr[CS_INSERT_HI/4] << 32);

	addr = (u32 *)(queue->user_io_addr + PAGE_SIZE);
	cs_extract = addr[CS_EXTRACT_LO/4] | ((u64)addr[CS_EXTRACT_HI/4] << 32);
	cs_active = addr[CS_ACTIVE/4];

#define KBASEP_CSF_DEBUGFS_CS_HEADER_USER_IO \
	"Bind Idx,     Ringbuf addr, Prio,    Insert offset,   Extract offset, Active, Doorbell\n"

	seq_printf(file, KBASEP_CSF_DEBUGFS_CS_HEADER_USER_IO "%8d, %16llx, %4u, %16llx, %16llx, %6u, %8d\n",
			queue->csi_index, queue->base_addr, queue->priority,
			cs_insert, cs_extract, cs_active, queue->doorbell_nr);

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		if (CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait)) {
			wait_status = queue->status_wait;
			wait_sync_value = queue->sync_value;
			wait_sync_pointer = queue->sync_ptr;
			sb_status = queue->sb_status;
			blocked_reason = queue->blocked_reason;

			evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
			if (evt) {
				wait_sync_live_value = evt[0];
				kbase_phy_alloc_mapping_put(queue->kctx, mapping);
			} else {
				wait_sync_live_value = U64_MAX;
			}

			kbasep_csf_scheduler_dump_active_queue_cs_status_wait(
				file, wait_status, wait_sync_value,
				wait_sync_live_value, wait_sync_pointer,
				sb_status, blocked_reason);
		}
	} else {
		struct kbase_device const *const kbdev =
			queue->group->kctx->kbdev;
		struct kbase_csf_cmd_stream_group_info const *const ginfo =
			&kbdev->csf.global_iface.groups[queue->group->csg_nr];
		struct kbase_csf_cmd_stream_info const *const stream =
			&ginfo->streams[queue->csi_index];
		u64 cmd_ptr;
		u32 req_res;

		if (WARN_ON(!stream))
			return;

		cmd_ptr = kbase_csf_firmware_cs_output(stream,
				CS_STATUS_CMD_PTR_LO);
		cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream,
				CS_STATUS_CMD_PTR_HI) << 32;
		req_res = kbase_csf_firmware_cs_output(stream,
					CS_STATUS_REQ_RESOURCE);

		seq_printf(file, "CMD_PTR: 0x%llx\n", cmd_ptr);
		seq_printf(file, "REQ_RESOURCE [COMPUTE]: %d\n",
			CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		seq_printf(file, "REQ_RESOURCE [FRAGMENT]: %d\n",
			CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		seq_printf(file, "REQ_RESOURCE [TILER]: %d\n",
			CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		seq_printf(file, "REQ_RESOURCE [IDVS]: %d\n",
			CS_STATUS_REQ_RESOURCE_IDVS_RESOURCES_GET(req_res));

		wait_status = kbase_csf_firmware_cs_output(stream,
				CS_STATUS_WAIT);
		wait_sync_value = kbase_csf_firmware_cs_output(stream,
					CS_STATUS_WAIT_SYNC_VALUE);
		wait_sync_pointer = kbase_csf_firmware_cs_output(stream,
					CS_STATUS_WAIT_SYNC_POINTER_LO);
		wait_sync_pointer |= (u64)kbase_csf_firmware_cs_output(stream,
					CS_STATUS_WAIT_SYNC_POINTER_HI) << 32;

		sb_status = kbase_csf_firmware_cs_output(stream,
							 CS_STATUS_SCOREBOARDS);
		blocked_reason = kbase_csf_firmware_cs_output(
			stream, CS_STATUS_BLOCKED_REASON);

		evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
		if (evt) {
			wait_sync_live_value = evt[0];
			kbase_phy_alloc_mapping_put(queue->kctx, mapping);
		} else {
			wait_sync_live_value = U64_MAX;
		}

		kbasep_csf_scheduler_dump_active_queue_cs_status_wait(
			file, wait_status, wait_sync_value,
			wait_sync_live_value, wait_sync_pointer, sb_status,
			blocked_reason);
		/* Dealing with cs_trace */
		if (kbase_csf_scheduler_queue_has_trace(queue))
			kbasep_csf_scheduler_dump_active_cs_trace(file, stream);
		else
			seq_puts(file, "NO CS_TRACE\n");
	}

	seq_puts(file, "\n");
}

/* Waiting timeout for STATUS_UPDATE acknowledgment, in milliseconds */
#define CSF_STATUS_UPDATE_TO_MS (100)

static void kbasep_csf_scheduler_dump_active_group(struct seq_file *file,
		struct kbase_queue_group *const group)
{
	if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
		struct kbase_device *const kbdev = group->kctx->kbdev;
		unsigned long flags;
		u32 ep_c, ep_r;
		char exclusive;
		struct kbase_csf_cmd_stream_group_info const *const ginfo =
			&kbdev->csf.global_iface.groups[group->csg_nr];
		long remaining =
			kbase_csf_timeout_in_jiffies(CSF_STATUS_UPDATE_TO_MS);
		u8 slot_priority =
			kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

		kbase_csf_scheduler_spin_lock(kbdev, &flags);
		kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ,
				~kbase_csf_firmware_csg_output(ginfo, CSG_ACK),
				CSG_REQ_STATUS_UPDATE_MASK);
		kbase_csf_scheduler_spin_unlock(kbdev, flags);
		kbase_csf_ring_csg_doorbell(kbdev, group->csg_nr);

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			!((kbase_csf_firmware_csg_input_read(ginfo, CSG_REQ) ^
			   kbase_csf_firmware_csg_output(ginfo, CSG_ACK)) &
			   CSG_REQ_STATUS_UPDATE_MASK), remaining);

		ep_c = kbase_csf_firmware_csg_output(ginfo,
				CSG_STATUS_EP_CURRENT);
		ep_r = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_REQ);

		if (CSG_STATUS_EP_REQ_EXCLUSIVE_COMPUTE_GET(ep_r))
			exclusive = 'C';
		else if (CSG_STATUS_EP_REQ_EXCLUSIVE_FRAGMENT_GET(ep_r))
			exclusive = 'F';
		else
			exclusive = '0';

		if (!remaining) {
			dev_err(kbdev->dev,
				"Timed out for STATUS_UPDATE on group %d on slot %d",
				group->handle, group->csg_nr);

			seq_printf(file, "*** Warn: Timed out for STATUS_UPDATE on slot %d\n",
				group->csg_nr);
			seq_printf(file, "*** The following group-record is likely stale\n");
		}

		seq_puts(file, "GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive\n");
		seq_printf(file, "%7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c\n",
			group->handle,
			group->csg_nr,
			slot_priority,
			group->run_state,
			group->priority,
			CSG_STATUS_EP_CURRENT_COMPUTE_EP_GET(ep_c),
			CSG_STATUS_EP_REQ_COMPUTE_EP_GET(ep_r),
			CSG_STATUS_EP_CURRENT_FRAGMENT_EP_GET(ep_c),
			CSG_STATUS_EP_REQ_FRAGMENT_EP_GET(ep_r),
			CSG_STATUS_EP_CURRENT_TILER_EP_GET(ep_c),
			CSG_STATUS_EP_REQ_TILER_EP_GET(ep_r),
			exclusive);
	} else {
		seq_puts(file, "GroupID, CSG NR, Run State, Priority\n");
		seq_printf(file, "%7d, %6d, %9d, %8d\n",
			group->handle,
			group->csg_nr,
			group->run_state,
			group->priority);
	}

	if (group->run_state != KBASE_CSF_GROUP_TERMINATED) {
		unsigned int i;

		seq_puts(file, "Bound queues:\n");

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			kbasep_csf_scheduler_dump_active_queue(file,
					group->bound_queues[i]);
		}
	}

	seq_puts(file, "\n");
}

/**
 * kbasep_csf_queue_group_debugfs_show() - Print per-context GPU command queue
 *					   group debug information
 *
 * @file: The seq_file for printing to
 * @data: The debugfs dentry private data, a pointer to kbase context
 *
 * Return: Negative error code or 0 on success.
 */
static int kbasep_csf_queue_group_debugfs_show(struct seq_file *file,
		void *data)
{
	u32 gr;
	struct kbase_context *const kctx = file->private;
	struct kbase_device *const kbdev = kctx->kbdev;

	if (WARN_ON(!kctx))
		return -EINVAL;

	seq_printf(file, "MALI_CSF_CSG_DEBUGFS_VERSION: v%u\n",
			MALI_CSF_CSG_DEBUGFS_VERSION);

	mutex_lock(&kctx->csf.lock);
	kbase_csf_scheduler_lock(kbdev);
	for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
		struct kbase_queue_group *const group =
			kctx->csf.queue_groups[gr];

		if (group)
			kbasep_csf_scheduler_dump_active_group(file, group);
	}
	kbase_csf_scheduler_unlock(kbdev);
	mutex_unlock(&kctx->csf.lock);

	return 0;
}

/**
 * kbasep_csf_scheduler_dump_active_groups() - Print debug info for active
 *                                             GPU command queue groups
 *
 * @file: The seq_file for printing to
 * @data: The debugfs dentry private data, a pointer to kbase_device
 *
 * Return: Negative error code or 0 on success.
 */
static int kbasep_csf_scheduler_dump_active_groups(struct seq_file *file,
		void *data)
{
	u32 csg_nr;
	struct kbase_device *kbdev = file->private;
	u32 num_groups = kbdev->csf.global_iface.group_num;

	seq_printf(file, "MALI_CSF_CSG_DEBUGFS_VERSION: v%u\n",
			MALI_CSF_CSG_DEBUGFS_VERSION);

	kbase_csf_scheduler_lock(kbdev);
	for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
		struct kbase_queue_group *const group =
			kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

		if (!group)
			continue;

		seq_printf(file, "\nCtx %d_%d\n", group->kctx->tgid,
				group->kctx->id);

		kbasep_csf_scheduler_dump_active_group(file, group);
	}
	kbase_csf_scheduler_unlock(kbdev);

	return 0;
}

static int kbasep_csf_queue_group_debugfs_open(struct inode *in,
		struct file *file)
{
	return single_open(file, kbasep_csf_queue_group_debugfs_show,
			in->i_private);
}

static int kbasep_csf_active_queue_groups_debugfs_open(struct inode *in,
		struct file *file)
{
	return single_open(file, kbasep_csf_scheduler_dump_active_groups,
			in->i_private);
}

static const struct file_operations kbasep_csf_queue_group_debugfs_fops = {
	.open = kbasep_csf_queue_group_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void kbase_csf_queue_group_debugfs_init(struct kbase_context *kctx)
{
	struct dentry *file;
#if (KERNEL_VERSION(4, 7, 0) <= LINUX_VERSION_CODE)
	const mode_t mode = 0444;
#else
	const mode_t mode = 0400;
#endif

	if (WARN_ON(!kctx || IS_ERR_OR_NULL(kctx->kctx_dentry)))
		return;

	file = debugfs_create_file("groups", mode,
		kctx->kctx_dentry, kctx, &kbasep_csf_queue_group_debugfs_fops);

	if (IS_ERR_OR_NULL(file)) {
		dev_warn(kctx->kbdev->dev,
		    "Unable to create per context queue groups debugfs entry");
	}
}

static const struct file_operations
	kbasep_csf_active_queue_groups_debugfs_fops = {
	.open = kbasep_csf_active_queue_groups_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int kbasep_csf_debugfs_scheduling_timer_enabled_get(
		void *data, u64 *val)
{
	struct kbase_device *const kbdev = data;

	*val = kbase_csf_scheduler_timer_is_enabled(kbdev);

	return 0;
}

static int kbasep_csf_debugfs_scheduling_timer_enabled_set(
		void *data, u64 val)
{
	struct kbase_device *const kbdev = data;

	kbase_csf_scheduler_timer_set_enabled(kbdev, val != 0);

	return 0;
}

static int kbasep_csf_debugfs_scheduling_timer_kick_set(
		void *data, u64 val)
{
	struct kbase_device *const kbdev = data;

	kbase_csf_scheduler_kick(kbdev);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(kbasep_csf_debugfs_scheduling_timer_enabled_fops,
		&kbasep_csf_debugfs_scheduling_timer_enabled_get,
		&kbasep_csf_debugfs_scheduling_timer_enabled_set,
		"%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(kbasep_csf_debugfs_scheduling_timer_kick_fops,
		NULL,
		&kbasep_csf_debugfs_scheduling_timer_kick_set,
		"%llu\n");

/**
 * kbase_csf_debugfs_scheduler_suspend_get() - get if the scheduler is suspended.
 *
 * @data: The debugfs dentry private data, a pointer to kbase_device
 * @val: The debugfs output value, boolean: 1 suspended, 0 otherwise
 *
 * Return: 0
 */
static int kbase_csf_debugfs_scheduler_suspend_get(
		void *data, u64 *val)
{
	struct kbase_device *kbdev = data;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;

	kbase_csf_scheduler_lock(kbdev);
	*val = (scheduler->state == SCHED_SUSPENDED);
	kbase_csf_scheduler_unlock(kbdev);

	return 0;
}

/**
 * kbase_csf_debugfs_scheduler_suspend_set() - set the scheduler to suspended.
 *
 * @data: The debugfs dentry private data, a pointer to kbase_device
 * @val: The debugfs input value, boolean: 1 suspend, 0 otherwise
 *
 * Return: Negative value if already in requested state, 0 otherwise.
 */
static int kbase_csf_debugfs_scheduler_suspend_set(
		void *data, u64 val)
{
	struct kbase_device *kbdev = data;
	struct kbase_csf_scheduler *scheduler = &kbdev->csf.scheduler;
	enum kbase_csf_scheduler_state state;

	kbase_csf_scheduler_lock(kbdev);
	state = scheduler->state;
	kbase_csf_scheduler_unlock(kbdev);

	if (val && (state != SCHED_SUSPENDED))
		kbase_csf_scheduler_pm_suspend(kbdev);
	else if (!val && (state == SCHED_SUSPENDED))
		kbase_csf_scheduler_pm_resume(kbdev);
	else
		return -1;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(kbasep_csf_debugfs_scheduler_suspend_fops,
		&kbase_csf_debugfs_scheduler_suspend_get,
		&kbase_csf_debugfs_scheduler_suspend_set,
		"%llu\n");

void kbase_csf_debugfs_init(struct kbase_device *kbdev)
{
	debugfs_create_file("active_groups", 0444,
		kbdev->mali_debugfs_directory, kbdev,
		&kbasep_csf_active_queue_groups_debugfs_fops);

	debugfs_create_file("scheduling_timer_enabled", 0644,
			kbdev->mali_debugfs_directory, kbdev,
			&kbasep_csf_debugfs_scheduling_timer_enabled_fops);
	debugfs_create_file("scheduling_timer_kick", 0200,
			kbdev->mali_debugfs_directory, kbdev,
			&kbasep_csf_debugfs_scheduling_timer_kick_fops);
	debugfs_create_file("scheduler_suspend", 0644,
			kbdev->mali_debugfs_directory, kbdev,
			&kbasep_csf_debugfs_scheduler_suspend_fops);

	kbase_csf_tl_reader_debugfs_init(kbdev);
	kbase_csf_firmware_trace_buffer_debugfs_init(kbdev);
}

#else
/*
 * Stub functions for when debugfs is disabled
 */
void kbase_csf_queue_group_debugfs_init(struct kbase_context *kctx)
{
}

void kbase_csf_debugfs_init(struct kbase_device *kbdev)
{
}

#endif /* CONFIG_DEBUG_FS */

static void kbasep_csf_scheduler_dump_active_queue_cs_status_wait_2(
	struct kbase_device *kbdev, pid_t tgid, u32 id,
	u32 wait_status, u32 wait_sync_value, u64 wait_sync_live_value,
	u64 wait_sync_pointer, u32 sb_status, u32 blocked_reason)
{
#define WAITING "Waiting"
#define NOT_WAITING "Not waiting"

	dev_info(kbdev->dev,
	         "[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
	         tgid,
	         id,
	         CS_STATUS_WAIT_SB_MASK_GET(wait_status),
	         CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
	         CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
	dev_info(kbdev->dev,
	         "[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
	         tgid,
	         id,
	         CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
	         CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ? "greater than" : "less or equal",
	         wait_sync_pointer);
	dev_info(kbdev->dev,
	         "[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
	         tgid,
	         id,
	         wait_sync_value,
	         wait_sync_live_value,
	         CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	dev_info(kbdev->dev,
	         "[%d_%d] BLOCKED_REASON: %s",
	         tgid,
	         id,
	         blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));
}

static void kbasep_csf_scheduler_dump_active_queue_2(pid_t tgid, u32 id, struct kbase_queue *queue)
{
	u32 *addr;
	u64 cs_extract;
	u64 cs_insert;
	u32 cs_active;
	u64 wait_sync_pointer;
	u32 wait_status, wait_sync_value;
	u32 sb_status;
	u32 blocked_reason;
	struct kbase_vmap_struct *mapping;
	u64 *evt;
	u64 wait_sync_live_value;

	if (!queue)
		return;

	if (queue->csi_index == KBASEP_IF_NR_INVALID || !queue->group)
		return;

	if (!queue->user_io_addr) {
		dev_info(queue->kctx->kbdev->dev,
			 "[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d",
			 tgid,
			 id,
			 queue->csi_index,
			 queue->base_addr,
			 queue->priority,
			 queue->doorbell_nr);
		return;
	}

	/* Ring the doorbell to have firmware update CS_EXTRACT */
	kbase_csf_ring_cs_user_doorbell(queue->kctx->kbdev, queue);
	msleep(100);

	addr = (u32 *)queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO/4] | ((u64)addr[CS_INSERT_HI/4] << 32);

	addr = (u32 *)(queue->user_io_addr + PAGE_SIZE);
	cs_extract = addr[CS_EXTRACT_LO/4] | ((u64)addr[CS_EXTRACT_HI/4] << 32);
	cs_active = addr[CS_ACTIVE/4];

	dev_info(queue->kctx->kbdev->dev,
			"[%d_%d] Bind Idx,     Ringbuf addr, Prio,    Insert offset,   Extract offset, Active, Doorbell",
			tgid,
			id);
	dev_info(queue->kctx->kbdev->dev,
			 "[%d_%d] %8d, %16llx, %4u, %16llx, %16llx, %6u, %8d",
			 tgid,
			 id,
			 queue->csi_index,
			 queue->base_addr,
			 queue->priority,
			 cs_insert,
			 cs_extract,
			 cs_active,
			 queue->doorbell_nr);


	/* if have command didn't complete print the last command's before and after 4 commands */
	if (cs_insert != cs_extract) {
		size_t size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;
		const unsigned int instruction_size = sizeof(u64);
		u64 start, stop, aligned_cs_extract;

		dev_info(queue->kctx->kbdev->dev,"Dumping instructions around the last Extract offset");

		aligned_cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

		/* Go 16 instructions back */
		if (aligned_cs_extract > (16 * instruction_size))
			start = aligned_cs_extract - (16 * instruction_size);
		else
			start = 0;

		/* Print upto 32 instructions */
		stop = start + (32 * instruction_size);
		if (stop > cs_insert)
			stop = cs_insert;

		dev_info(queue->kctx->kbdev->dev,"Instructions from Extract offset %llx\n",  start);

		while (start != stop) {
			u64 page_off = (start & size_mask) >> PAGE_SHIFT;
			u64 offset = (start & size_mask) & ~PAGE_MASK;
			struct page *page =
				as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			u64 *ringbuffer = kmap_atomic(page);
			u64 *ptr = &ringbuffer[offset/8];

			dev_info(queue->kctx->kbdev->dev,"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);

			kunmap_atomic(ringbuffer);
			start += (8 * instruction_size);
		}
	}

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		if (CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait)) {
			wait_status = queue->status_wait;
			wait_sync_value = queue->sync_value;
			wait_sync_pointer = queue->sync_ptr;
			sb_status = queue->sb_status;
			blocked_reason = queue->blocked_reason;

			evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
			if (evt) {
				wait_sync_live_value = evt[0];
				kbase_phy_alloc_mapping_put(queue->kctx, mapping);
			} else {
				wait_sync_live_value = U64_MAX;
			}

			kbasep_csf_scheduler_dump_active_queue_cs_status_wait_2(
				queue->kctx->kbdev, tgid, id,
				wait_status, wait_sync_value,
				wait_sync_live_value, wait_sync_pointer,
				sb_status, blocked_reason);
		}
	} else {
		struct kbase_device const *const kbdev = queue->group->kctx->kbdev;
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[queue->group->csg_nr];
		struct kbase_csf_cmd_stream_info const *const stream = &ginfo->streams[queue->csi_index];
		u64 cmd_ptr;
		u32 req_res;

		if (!stream)
			return;

		cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_LO);
		cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_HI) << 32;
		req_res = kbase_csf_firmware_cs_output(stream, CS_STATUS_REQ_RESOURCE);

		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] CMD_PTR: 0x%llx",
		         tgid,
		         id,
		         cmd_ptr);
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [COMPUTE]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [FRAGMENT]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [TILER]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [IDVS]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_IDVS_RESOURCES_GET(req_res));

		wait_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT);
		wait_sync_value = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_VALUE);
		wait_sync_pointer = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_LO);
		wait_sync_pointer |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_HI) << 32;

		sb_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_SCOREBOARDS);
		blocked_reason = kbase_csf_firmware_cs_output( stream, CS_STATUS_BLOCKED_REASON);

		evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
		if (evt) {
			wait_sync_live_value = evt[0];
			kbase_phy_alloc_mapping_put(queue->kctx, mapping);
		} else {
			wait_sync_live_value = U64_MAX;
		}

		kbasep_csf_scheduler_dump_active_queue_cs_status_wait_2(
			queue->kctx->kbdev, tgid, id,
			wait_status, wait_sync_value,
			wait_sync_live_value, wait_sync_pointer, sb_status,
			blocked_reason);
		/* Dealing with cs_trace */
		if (kbase_csf_scheduler_queue_has_trace(queue)) {
			u32 val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_LO);
			u64 addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_HI) << 32) | val;
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_SIZE);

			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u",
					 tgid,
					 id,
					 addr,
					 val);

			/* Write offset variable address (pointer) */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
			addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
					 tgid,
					 id,
					 addr);

			/* EVENT_SIZE and EVENT_STATEs */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
					 tgid,
					 id,
					 CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
					 CS_INSTR_CONFIG_EVENT_STATE_GET(val));
		} else {
			dev_info(queue->kctx->kbdev->dev,
			         "[%d_%d] NO CS_TRACE",
					 tgid,
					 id);
		}
	}
}

/* Waiting timeout for STATUS_UPDATE acknowledgment, in milliseconds */
#define CSF_STATUS_UPDATE_TO_MS (100)

static void kbasep_csf_scheduler_dump_active_group_2(struct kbase_queue_group *const group)
{
	if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
		struct kbase_device *const kbdev = group->kctx->kbdev;
		unsigned long flags;
		u32 ep_c, ep_r;
		char exclusive;
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[group->csg_nr];
		long remaining = kbase_csf_timeout_in_jiffies(CSF_STATUS_UPDATE_TO_MS);
		u8 slot_priority = kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

		kbase_csf_scheduler_spin_lock(kbdev, &flags);
		kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ,
				~kbase_csf_firmware_csg_output(ginfo, CSG_ACK),
				CSG_REQ_STATUS_UPDATE_MASK);
		kbase_csf_scheduler_spin_unlock(kbdev, flags);
		kbase_csf_ring_csg_doorbell(kbdev, group->csg_nr);

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			!((kbase_csf_firmware_csg_input_read(ginfo, CSG_REQ) ^
			   kbase_csf_firmware_csg_output(ginfo, CSG_ACK)) &
			   CSG_REQ_STATUS_UPDATE_MASK), remaining);

		ep_c = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_CURRENT);
		ep_r = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_REQ);

		if (CSG_STATUS_EP_REQ_EXCLUSIVE_COMPUTE_GET(ep_r))
			exclusive = 'C';
		else if (CSG_STATUS_EP_REQ_EXCLUSIVE_FRAGMENT_GET(ep_r))
			exclusive = 'F';
		else
			exclusive = '0';

		if (!remaining) {
			dev_info(kbdev->dev,
			         "[%d_%d] Timed out for STATUS_UPDATE on group %d on slot %d",
			         group->kctx->tgid,
			         group->kctx->id,
			         group->handle,
			         group->csg_nr);
		}

		dev_info(kbdev->dev,
		        "[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive",
		        group->kctx->tgid,
		        group->kctx->id);
		dev_info(kbdev->dev,
		        "[%d_%d] %7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c",
		        group->kctx->tgid,
		        group->kctx->id,
		        group->handle,
		        group->csg_nr,
		        slot_priority,
		        group->run_state,
		        group->priority,
		        CSG_STATUS_EP_CURRENT_COMPUTE_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_COMPUTE_EP_GET(ep_r),
		        CSG_STATUS_EP_CURRENT_FRAGMENT_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_FRAGMENT_EP_GET(ep_r),
		        CSG_STATUS_EP_CURRENT_TILER_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_TILER_EP_GET(ep_r),
		        exclusive);
	} else {
		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] GroupID, CSG NR, Run State, Priority",
		        group->kctx->tgid,
		        group->kctx->id);
		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] %7d, %6d, %9d, %8d",
		        group->kctx->tgid,
		        group->kctx->id,
		        group->handle,
		        group->csg_nr,
		        group->run_state,
		        group->priority);
	}

	if (group->run_state != KBASE_CSF_GROUP_TERMINATED) {
		unsigned int i;

		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] Bound queues:",
		        group->kctx->tgid,
		        group->kctx->id);

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			kbasep_csf_scheduler_dump_active_queue_2(
				group->kctx->tgid,
				group->kctx->id,
				group->bound_queues[i]);
		}
	}
}

void kbase_csf_local_fence_wait_dump(struct kbase_context *kctx, unsigned int pid,
                                     unsigned int flags, unsigned long time_in_microseconds)
{
	struct kbase_device *kbdev = kctx->kbdev;

	lockdep_off();

	mutex_lock(&kbdev->kctx_list_lock);  // By Akash
	{
		struct kbase_context *kctx;

		list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
			if (kctx->tgid == pid) {
				mutex_lock(&kctx->csf.lock);
				kbase_csf_scheduler_lock(kbdev);
				// cat /sys/kernel/debug/mali0/active_groups
				// Print debug info for active GPU command queue groups
				{
				u32 csg_nr;
				u32 num_groups = kbdev->csf.global_iface.group_num;

				dev_info(kbdev->dev, "[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u\n", MALI_CSF_CSG_DEBUGFS_VERSION);

				for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
					struct kbase_queue_group *const group = kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

					if (!group)
						continue;

					dev_info(kbdev->dev, "[active_groups] ##### Ctx %d_%d #####", group->kctx->tgid, group->kctx->id);

					kbasep_csf_scheduler_dump_active_group_2(group);
				}
				//kbase_csf_scheduler_unlock(kbdev); By Akash
				}

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/groups
				// Print per-context GPU command queue group debug information
				{
				u32 gr;

				for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
					struct kbase_queue_group *const group = kctx->csf.queue_groups[gr];

					if (!group)
						continue;

					dev_info(kbdev->dev, "[groups] ##### Ctx %d_%d #####", group->kctx->tgid, group->kctx->id);

					if (group)
						kbasep_csf_scheduler_dump_active_group_2(group);
				}
				}
				//kbase_csf_scheduler_unlock(kbdev);

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/kcpu_queues
				// Print per-context KCPU queues debug information
				{
				unsigned long idx;

				dev_info(kbdev->dev, "[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u\n", MALI_CSF_CSG_DEBUGFS_VERSION);
				dev_info(kbdev->dev, "[kcpu_queues] ##### Ctx %d_%d #####", kctx->tgid, kctx->id);
				dev_info(kbdev->dev,
				         "[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Fence context  &  seqno",
				         kctx->tgid,
				         kctx->id);

				mutex_lock(&kctx->csf.kcpu_queues.lock);

				idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

				while (idx < KBASEP_MAX_KCPU_QUEUES) {
					struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];

					if (!queue) {
						idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
						continue;
					}

					dev_info(kbdev->dev,
					         "[%d_%d] %9lu( %s ), %16u, %11u, %7u, %13llu  %8u",
							kctx->tgid,
							kctx->id,
							idx,
							queue->has_error ? "InErr" : "NoErr",
							queue->num_pending_cmds,
							queue->enqueue_failed,
							queue->command_started ? 1 : 0,
							queue->fence_context,
							queue->fence_seqno);

					if (queue->command_started) {
						int i;
						for (i = 0; i < queue->num_pending_cmds; i++) {
						struct kbase_kcpu_command *cmd = &queue->commands[queue->start_offset + i];

						if (cmd->type < 0 || cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu,         %d, (unknown blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         cmd->type);
							continue;
						}

						switch (cmd->type) {
#if IS_ENABLED(CONFIG_SYNC_FILE)
						case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
						{
							struct kbase_sync_fence_info info;

							kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							dev_info(kbdev->dev,
										 "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
										 kctx->tgid,
										 kctx->id);
							dev_info(kbdev->dev,
										 "[%d_%d] %9lu( %s ),     Fence Signal, %pK %s %s",
										 kctx->tgid,
										 kctx->id,
										 idx,
										 queue->has_error ? "InErr" : "NoErr",
										 info.fence,
										 info.name,
										 kbase_sync_status_string(info.status));
							break;
						}
						case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
						{
							struct kbase_sync_fence_info info;

							kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu( %s ),     Fence Wait, %pK %s %s",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         info.fence,
							         info.name,
							         kbase_sync_status_string(info.status));
							break;
						}
#endif
						case BASE_KCPU_COMMAND_TYPE_CQS_SET:
						{
							unsigned int i;
							struct kbase_kcpu_command_cqs_set_info *sets = &cmd->info.cqs_set;

							for (i = 0; i < sets->nr_objs; i++) {
								dev_info(kbdev->dev,
								         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								        "[%d_%d] %9lu,       CQS Set %llx",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         sets->objs[i].addr);
							}
							break;
						}
						case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
						{
							unsigned int i;
							struct kbase_kcpu_command_cqs_wait_info *waits = &cmd->info.cqs_wait;

							for (i = 0; i < waits->nr_objs; i++) {
								struct kbase_vmap_struct *mapping;
								u32 val;
								char const *msg;
								u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);

								if (!cpu_ptr)
									break;

								val = *cpu_ptr;
								kbase_phy_alloc_mapping_put(kctx, mapping);

								msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";

								dev_info(kbdev->dev,
								         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								         "[%d_%d] %9lu( %s ),       CQS Wait, %llx(%u > %u, inherit_err: %s)",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         queue->has_error ? "InErr" : "NoErr",
								         waits->objs[i].addr,
								         val,
								         waits->objs[i].val,
								         msg);
							}
							break;
						}
						default:
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu,         %d, (other blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         cmd->type);
							break;
						}
						}
					}

					idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
				}

				mutex_unlock(&kctx->csf.kcpu_queues.lock);
				}

				// dump firmware trace buffer
//				kbase_csf_dump_firmware_trace_buffer(kbdev);
				// dump ktrace log
//				KBASE_KTRACE_DUMP(kbdev);

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/cpu_queue
				// Print per-context CPU queues debug information
				{
				if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
						BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
					dev_info(kbdev->dev, "Dump request already started! (try again)\n");
					kbase_csf_scheduler_unlock(kbdev);
					mutex_unlock(&kctx->csf.lock);
					mutex_unlock(&kbdev->kctx_list_lock);
					return;
				}

				atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
				init_completion(&kctx->csf.cpu_queue.dump_cmp);
				kbase_event_wakeup(kctx);
				//mutex_unlock(&kctx->csf.lock);

				kbase_csf_scheduler_unlock(kbdev);
				mutex_unlock(&kctx->csf.lock);

				dev_info(kbdev->dev, "[cpu_queue] CPU Queues table (version:v%u):\n", MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
				dev_info(kbdev->dev, "[cpu_queue] ##### Ctx %d_%d #####", kctx->tgid, kctx->id);

				wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp,
						msecs_to_jiffies(3000));

				mutex_lock(&kctx->csf.lock);
				kbase_csf_scheduler_lock(kbdev);

				if (kctx->csf.cpu_queue.buffer) {
					WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
							    BASE_CSF_CPU_QUEUE_DUMP_PENDING);

					dev_info(kbdev->dev, "%s\n", kctx->csf.cpu_queue.buffer);

					kfree(kctx->csf.cpu_queue.buffer);
					kctx->csf.cpu_queue.buffer = NULL;
					kctx->csf.cpu_queue.buffer_size = 0;
				}
				else
					dev_info(kbdev->dev, "Dump error! (time out)\n");

				atomic_set(&kctx->csf.cpu_queue.dump_req_status,
					BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
				}

				kbase_csf_scheduler_unlock(kbdev);
				mutex_unlock(&kctx->csf.lock);
			}
		}
	}

	mutex_unlock(&kbdev->kctx_list_lock); //By Akash
}
