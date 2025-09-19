// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2023 ARM Limited. All rights reserved.
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

#include <linux/delay.h>
#include <linux/workqueue.h>
#include <device/mali_kbase_device.h>
#include <mali_kbase_mem.h>
#include <mali_kbase_reset_gpu.h>
#include <kutf/kutf_helpers_user.h>
#include "../mali_kutf_kernel_defect_test_GPUCORE_37465.h"
#include "mali_kutf_kernel_defect_test_main.h"
#include "backend/gpu/mali_kbase_irq_internal.h"


struct kutf_tiler_heap_in {
	struct kutf_helper_named_val ctx_id;
	struct kutf_helper_named_val gpu_heap_va;
	struct kutf_helper_named_val chunk_size;
	struct kutf_helper_named_val gpu_queue_va;
	struct kutf_helper_named_val vt_start;
	struct kutf_helper_named_val vt_end;
	struct kutf_helper_named_val frag_end;
};

static struct kbase_queue *get_scheduled_queue_ptr(struct kutf_tiler_heap_in *in,
						   struct kbase_context *kctx)
{
	struct kbase_queue *queue = list_first_entry(&kctx->csf.queue_list, typeof(*queue), link);

	if (!queue) {
		pr_warn("queue is null\n");
		return NULL;
	}

	if (queue->base_addr != in->gpu_queue_va.u.val_u64) {
		pr_warn("invalid queue->base_addr, got %llu, expected %llu\n", queue->base_addr,
			in->gpu_queue_va.u.val_u64);
		return NULL;
	}

	if (queue->csi_index == KBASEP_IF_NR_INVALID) {
		pr_warn("Invalid value of queue->csi_index\n");
		return NULL;
	}

	if (!queue->group) {
		pr_warn("queue->group is null\n");
		return NULL;
	}

	if (queue->group->csg_nr == KBASEP_CSG_NR_INVALID) {
		pr_warn("Group-%d not on slot\n", queue->group->handle);
		return NULL;
	}

	return queue;
}

static void set_oom_event_info(struct kutf_tiler_heap_in *in, struct kbase_device *kbdev,
			       int csi_index, int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	u32 cs_req = ((u32 *)stream->input)[CS_REQ / sizeof(u32)] & CS_ACK_TILER_OOM_MASK;
	u32 cs_ack = ((u32 *)stream->output)[CS_ACK / sizeof(u32)] & CS_ACK_TILER_OOM_MASK;

	/* set Out Of Memory event for the Driver */
	((u32 *)stream->output)[CS_ACK / sizeof(u32)] = (cs_ack & ~CS_ACK_TILER_OOM_MASK) |
							(~cs_req & CS_ACK_TILER_OOM_MASK);

	/* set Out Of Memory event parameters for the Driver */
	((u32 *)stream->output)[CS_HEAP_ADDRESS_LO / sizeof(u32)] = in->gpu_heap_va.u.val_u64 &
								    0xFFFFFFFF;
	((u32 *)stream->output)[CS_HEAP_ADDRESS_HI / sizeof(u32)] = in->gpu_heap_va.u.val_u64 >> 32;
	((u32 *)stream->output)[CS_HEAP_VT_START / sizeof(u32)] = in->vt_start.u.val_u64;
	((u32 *)stream->output)[CS_HEAP_VT_END / sizeof(u32)] = in->vt_end.u.val_u64;
	((u32 *)stream->output)[CS_HEAP_FRAG_END / sizeof(u32)] = in->frag_end.u.val_u64;
}

static void send_interrupt(struct kbase_device *kbdev, int csi_index, int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	u32 host_irq_bit = (u32)1 << csg_nr;

	/* Interrupt setup.
	 *
	 * Indicate the interrupt pertains to which CSI within the CSG.
	 */
	u32 csg_irqmask = (u32)1 << csi_index;
	/*
	 * CSG_IRQ_REQ should not be confused with CSG_REQ - for interrupts,
	 * firmware is the source, meaning that it requests the interrupt to be
	 * handled by writing to its output page. This is different from host
	 * requesting something from the firmware - in which case, host would
	 * write to the FW's input page. In this case, we pretend to be the
	 * firmware, so our request lives in the output page.
	 */
	u32 csg_irqreq = ((u32 *)group->output)[CSG_IRQ_REQ / sizeof(u32)];
	u32 csg_irqack = ((u32 *)group->input)[CSG_IRQ_ACK / sizeof(u32)];

	csg_irqreq = (csg_irqreq & ~csg_irqmask) | (~csg_irqack & csg_irqmask);
	((u32 *)group->output)[CSG_IRQ_REQ / sizeof(u32)] = csg_irqreq;

	/* Generate Interrupt.
	 *
	 * Indicate for which CSG interrupt is being generated.
	 */
	kbase_reg_write32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT), host_irq_bit);

	/* Wait until the interrupt has been handled */
	while (kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)) & host_irq_bit)
		msleep(20);
}

static void wait_for_oom_event_handling(struct kbase_queue *queue)
{
	/* First wait for Out Of Memory event interrupt to get handled */
	kbase_synchronize_irqs(queue->kctx->kbdev);

	/* Now wait for Out Of Memory event work item to finish */
	flush_work(&queue->oom_event_work);
}

/**
 * mali_kutf_GPUCORE37465_test_function - Main test function (kernel-side)
 *
 * @context: pointer to the @p struct kutf_context object
 *
 * This is the kernel counterpart for the associated userspace test.
 * Refer to GPUCORE37465_test_func in ../test_runner/GPUCORE-37465.c
 * for a more precise overview of the control flow.
 *
 * After retrieving the user-allocated data, this side of the test
 * does a few things on behalf of userspace:
 *   - Prepare Tiler Out Of Memory IRQ info
 *   - Store GPU command queue reference counter
 *   - Fake kernel backend enter into GPU-reset state
 *   - Trigger Tiler Out Of Memory IRQ
 *   - Wait for Tiler Out Of Memory IRQ to be served
 *   - Make kernel backend out of GPU-reset state'
 *   - Check if GPU command queue reference counter is same as before Tiler Out Of Memory IRQ
 *   - If reference counter is different, it means that Tiler Out Of Memory does not release
 *     queue properly and will cause warning and memory leak
 * Both synchronization (at each step) and data exchange are done via the KUTF
 * data mechanism.
 */
static void mali_kutf_GPUCORE37465_test_function(struct kutf_context *context)
{
	int err = 0;

	struct kutf_kernel_defect_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kutf_tiler_heap_in in;
	struct kbase_context *kctx = NULL;
	struct kutf_helper_named_val val;
	struct kbase_queue *queue = NULL;
	unsigned long reset_state = 0;
	int csi_index;
	int csg_nr;
	unsigned long queue_refcnt_before, queue_refcnt_after;

	if (data->kbdev == NULL) {
		kutf_test_fail(context, "Unexpected NULL kbdev");
		return;
	}

	err = kutf_helper_receive_check_val(&val, context, GPUCORE37465_USERSPACE_READY,
					    KUTF_HELPER_VALTYPE_U64);
	if (err != 0) {
		kutf_test_fail(context, "Failed to communicate with userspace");
		return;
	} else if (val.u.val_u64 == GPUCORE37465_NOK) {
		/* Userspace failed to allocate resources & will signal the test failure */
		return;
	}

	/* Receive GPU VA of the Heap context */
	err = kutf_helper_receive_check_val(&in.gpu_heap_va, context,
					    GPUCORE37465_USERSPACE_GPU_HEAP_VA,
					    KUTF_HELPER_VALTYPE_U64);

	/* Receive size of the Heap chunk */
	if (!err)
		err = kutf_helper_receive_check_val(&in.chunk_size, context,
						    GPUCORE37465_USERSPACE_HEAP_CHUNK_SIZE,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive GPU VA of the GPU queue */
	if (!err)
		err = kutf_helper_receive_check_val(&in.gpu_queue_va, context,
						    GPUCORE37465_USERSPACE_GPU_QUEUE_VA,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive number of vertex/tiling operations which have started on
	 * the Heap
	 */
	if (!err)
		err = kutf_helper_receive_check_val(&in.vt_start, context,
						    GPUCORE37465_USERSPACE_HEAP_VT_START,
						    KUTF_HELPER_VALTYPE_U64);

	/* Receive number of vertex/tiling operations which have completed on
	 * the Heap
	 */
	if (!err)
		err = kutf_helper_receive_check_val(&in.vt_end, context,
						    GPUCORE37465_USERSPACE_HEAP_VT_END,
						    KUTF_HELPER_VALTYPE_U64);
	/* Receive number of fragment operations which have completed on
	 * the Heap
	 */
	if (!err)
		err = kutf_helper_receive_check_val(&in.frag_end, context,
						    GPUCORE37465_USERSPACE_HEAP_FRAG_END,
						    KUTF_HELPER_VALTYPE_U64);

	/* Get the ctx it for the context userspace created */
	if (!err)
		err = kutf_helper_receive_check_val(&in.ctx_id, context,
						    GPUCORE37465_USERSPACE_CTX_ID,
						    KUTF_HELPER_VALTYPE_U64);

	if (err != 0) {
		kutf_test_fail(context, "Failed to communicate with userspace");
		notify_user_val(context, GPUCORE37465_KERNEL_READY, GPUCORE37465_NOK);
		return;
	}

	kctx = get_kbase_ctx_ptr_from_id(kbdev, in.ctx_id.u.val_u64);
	if (kctx == NULL) {
		kutf_test_fail(context, "Userspace sent invalid ctx id");
		notify_user_val(context, GPUCORE37465_KERNEL_READY, GPUCORE37465_NOK);
		return;
	}

	queue = get_scheduled_queue_ptr(&in, kctx);
	if (!queue) {
		notify_user_val(context, GPUCORE37465_KERNEL_READY, GPUCORE37465_NOK);
		return;
	}

	/* Record the CSI index and CSG number to prevent the info gets lost due to
	 * Driver terminating the CSG. This info is required later to inspect
	 * the respective output page of CS & CSG to determine the action taken
	 * for the Out Of Memory event.
	 */
	csi_index = queue->csi_index;
	csg_nr = queue->group->csg_nr;

	/* Past this point we assume KUTF kernel<>user data exchange never fails */
	notify_user_val(context, GPUCORE37465_KERNEL_READY, GPUCORE37465_OK);

	queue_refcnt_before = kbase_refcount_read(&queue->refcount);

	set_oom_event_info(&in, kbdev, csi_index, csg_nr);

	/* Down write reset semaphore and set reset state to KBASE_CSF_RESET_GPU_HAPPENING
	 * simulate reset in progress
	 */
	down_write(&kbdev->csf.reset.sem);
	reset_state = (unsigned long)atomic_read(&kbdev->csf.reset.state);
	atomic_set(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_HAPPENING);
	/* Trigger Tiler Out Of Memory IRQ */
	send_interrupt(kbdev, csi_index, csg_nr);
	up_write(&kbdev->csf.reset.sem);
	wait_for_oom_event_handling(queue);
	down_write(&kbdev->csf.reset.sem);
	/* restore reset state and up_write reset semaphore */
	atomic_set(&kbdev->csf.reset.state, reset_state);
	up_write(&kbdev->csf.reset.sem);

	queue_refcnt_after = kbase_refcount_read(&queue->refcount);
	if (queue_refcnt_after == queue_refcnt_before) {
		notify_user_val(context, GPUCORE37465_KERNEL_TEST_DONE, GPUCORE37465_OK);
	} else {
		kutf_test_fail(context, "Tiler Out Of Memory event does not release queue");
		notify_user_val(context, GPUCORE37465_KERNEL_TEST_DONE, GPUCORE37465_NOK);
	}
	/* Userspace has had an opportunity to perform tiler heap term */
	wait_user_val(context, GPUCORE37465_USERSPACE_TERM_DONE);
	/* Sync on exit */
	wait_user_val(context, GPUCORE37465_USERSPACE_DO_CLEANUP);
	notify_user_val(context, GPUCORE37465_KERNEL_CLEANUP_DONE, GPUCORE37465_OK);
}


/**
 * mali_kutf_kernel_defect_GPUCORE_37465 - Entry point for the test
 * @context: KUTF context
 *
 */
void mali_kutf_kernel_defect_GPUCORE_37465(struct kutf_context *context)
{
	mali_kutf_GPUCORE37465_test_function(context);
}
