// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2023 ARM Limited. All rights reserved.
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

#include <linux/fdtable.h>
#include <linux/module.h>

#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <linux/version.h>
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/task.h>
#else
#include <linux/sched.h>
#endif
#include "mali_kbase.h"
#include "backend/gpu/mali_kbase_irq_internal.h"

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers.h>
#include <kutf/kutf_helpers_user.h>

#include "../mali_kutf_csf_tiler_heap_unit_test.h"

/* KUTF test application pointer for this test */
static struct kutf_application *tiler_heap_app;

/**
 * struct kutf_tiler_heap_fixture_data - Fixture data for the test functions.
 * @kbdev:	kbase device for the GPU.
 */
struct kutf_tiler_heap_fixture_data {
	struct kbase_device *kbdev;
};

/**
 * struct kutf_tiler_heap_in - Input values copied from user space, used by the
 *                             test functions.
 * @ctx_id:           Global ID of the Base context created by the test runner.
 * @gpu_heap_va:      GPU virtual address of the Tiler Heap context created by
 *                    the test runner.
 * @chunk_size:       The size of each chunk, in bytes.
 * @gpu_queue_va:     GPU virtual address of the GPU command queue created by
 *                    the test runner.
 * @vt_start:         Number of vertex/tiling operations that have started on
 *                    the Heap.
 * @vt_end:           Number of vertex/tiling operations that have completed on
 *                    the Heap.
 * @frag_end:         Number of fragment operations that have completed on the
 *                    Heap.
 */
struct kutf_tiler_heap_in {
	struct kutf_helper_named_val ctx_id;
	struct kutf_helper_named_val gpu_heap_va;
	struct kutf_helper_named_val chunk_size;
	struct kutf_helper_named_val gpu_queue_va;
	struct kutf_helper_named_val vt_start;
	struct kutf_helper_named_val vt_end;
	struct kutf_helper_named_val frag_end;
};

static DEFINE_MUTEX(kutf_tiler_heap_mutex);

/**
 * mali_kutf_tiler_heap_unit_create_fixture() - Creates the fixture data
 *                          required for all the tests in the tiler_heap suite.
 * @context:	KUTF context.
 *
 * Return: Fixture data created on success or NULL on failure
 */
static void *mali_kutf_tiler_heap_unit_create_fixture(struct kutf_context *context)
{
	struct kutf_tiler_heap_fixture_data *data;
	struct kbase_device *kbdev = NULL;

	pr_debug("Creating fixture\n");
	data = kutf_mempool_alloc(&context->fixture_pool,
				  sizeof(struct kutf_tiler_heap_fixture_data));
	if (!data)
		return NULL;

	*data = (const struct kutf_tiler_heap_fixture_data){ NULL };

	/* Acquire the kbase device */
	pr_debug("Finding device\n");
	kbdev = kbase_find_device(-1);
	if (kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	kbase_pm_context_active(kbdev);
	data->kbdev = kbdev;

	pr_debug("Created fixture\n");

	return data;
}

/**
 * mali_kutf_tiler_heap_unit_remove_fixture() - Destroy fixture data previously
 * created by mali_kutf_tiler_heap_unit_create_fixture.
 *
 * @context:             KUTF context.
 */
static void mali_kutf_tiler_heap_unit_remove_fixture(struct kutf_context *context)
{
	struct kutf_tiler_heap_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;

	pr_debug("Destroying fixture\n");
	kbase_pm_context_idle(kbdev);
	kbase_release_device(kbdev);
	pr_debug("Destroyed fixture\n");
}

/**
 * kutf_tiler_heap_in_new() - Receive all variable values from user-side
 * @context: KUTF context.
 * @values:  Where to store a pointer to a set of input values copied
 *           from user-side, if successful.
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file or memory allocation failure, positive value indicates an
 *         enum kutf_helper_err value for correct reception of data but
 *         invalid parsing.
 *
 * The caller need not free the set of input values allocated by this
 * function because they are allocated from a memory pool with the same
 * lifetime as the test fixture.
 */
static int kutf_tiler_heap_in_new(struct kutf_context *const context,
				  struct kutf_tiler_heap_in **const values)
{
	int err = 0;
	struct kutf_tiler_heap_in *const in =
		kutf_mempool_alloc(&context->fixture_pool, sizeof(*in));

	if (!in) {
		kutf_test_fail(context, "Out of memory");
		err = -ENOMEM;
	}

	/* Receive Global ID of the Base context */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->ctx_id, context, BASE_CTX_ID,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive GPU VA of the Heap context */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->gpu_heap_va, context, GPU_HEAP_VA,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive size of the Heap chunk */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->chunk_size, context, HEAP_CHUNK_SIZE,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive GPU VA of the GPU queue */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->gpu_queue_va, context, GPU_QUEUE_VA,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive number of vertex/tiling operations which have started on
	 * the Heap
	 */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->vt_start, context, HEAP_VT_START,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive number of vertex/tiling operations which have completed on
	 * the Heap
	 */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->vt_end, context, HEAP_VT_END,
						    KUTF_HELPER_VALTYPE_U64);
	}

	/* Receive number of fragment operations which have completed on
	 * the Heap
	 */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->frag_end, context, HEAP_FRAG_END,
						    KUTF_HELPER_VALTYPE_U64);
	}

	if (!err) {
		if (values)
			*values = in;
		else
			err = -EINVAL;
	}

	return err;
}

static enum oom_event_action get_oom_event_action(struct kbase_device *kbdev,
						  struct kbase_context *kctx, int csi_index,
						  int csg_nr, u32 chunk_size, u8 csi_handlers)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	struct kbase_va_region *region;
	u64 first_chunk_ptr, last_chunk_ptr;
	u64 chunk_va;
	u32 csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;
	u32 cs_state = ((u32 *)stream->input)[CS_REQ / 4] & CS_REQ_STATE_MASK;
	bool valid_region;

	if (csg_state == CSG_REQ_STATE_TERMINATE) {
		if (WARN_ON(cs_state != CS_REQ_STATE_STOP))
			return OOM_EVENT_INCONSISTENT_CS_STATE;
		else
			return OOM_EVENT_CSG_TERMINATED;
	}

	first_chunk_ptr = ((u32 *)stream->input)[CS_TILER_HEAP_START_LO / 4] |
			  ((u64)((u32 *)stream->input)[CS_TILER_HEAP_START_HI / 4] << 32);

	last_chunk_ptr = ((u32 *)stream->input)[CS_TILER_HEAP_END_LO / 4] |
			 ((u64)((u32 *)stream->input)[CS_TILER_HEAP_END_HI / 4] << 32);

	/* Expect only a single chunk */
	if (WARN_ON(first_chunk_ptr != last_chunk_ptr))
		return OOM_EVENT_MULTIPLE_NEW_CHUNKS;

	if (!first_chunk_ptr) {
		const u32 vt_end = ((u32 *)stream->output)[CS_HEAP_VT_END / 4];
		const u32 frag_end = ((u32 *)stream->output)[CS_HEAP_FRAG_END / 4];

		if ((csi_handlers & BASE_CSF_TILER_OOM_EXCEPTION_FLAG) && (vt_end == frag_end))
			return OOM_EVENT_INCREMENTAL_RENDER;
		else
			return OOM_EVENT_NULL_CHUNK;
	}

	chunk_va = first_chunk_ptr & PAGE_MASK;

	/* Check if the chunk address is valid */
	kbase_gpu_vm_lock(kctx);
	region = kbase_region_tracker_find_region_enclosing_address(kctx, chunk_va);
	valid_region = !kbase_is_region_invalid_or_free(region);
	kbase_gpu_vm_unlock(kctx);

	if (WARN_ON(!valid_region))
		return OOM_EVENT_INVALID_CHUNK;

	if ((chunk_size >> PAGE_SHIFT) != (first_chunk_ptr & ~PAGE_MASK))
		return OOM_EVENT_INVALID_CHUNK;

	return OOM_EVENT_NEW_CHUNK_ALLOCATED;
}

static void send_oom_event_action(struct kutf_context *context, enum oom_event_action action)
{
	WARN_ON(kutf_helper_send_named_u64(context, OOM_EVENT_ACTION, action));
}

static void wait_for_oom_event_handling(struct kbase_queue *queue)
{
	/* First wait for OoM event interrupt to get handled */
	kbase_synchronize_irqs(queue->kctx->kbdev);

	/* Now wait for OoM event work item to finish */
	flush_work(&queue->oom_event_work);
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
	u32 csg_irqreq = ((u32 *)group->output)[CSG_IRQ_REQ / 4];
	u32 csg_irqack = ((u32 *)group->input)[CSG_IRQ_ACK / 4];

	csg_irqreq = (csg_irqreq & ~csg_irqmask) | (~csg_irqack & csg_irqmask);
	((u32 *)group->output)[CSG_IRQ_REQ / 4] = csg_irqreq;

	/* Generate Interrupt.
	 *
	 * Indicate for which CSG interrupt is being generated.
	 */
	kbase_reg_write32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT), host_irq_bit);

	/* Wait until the interrupt has been handled */
	while (kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)) & host_irq_bit) {
		/* Sleep for few ms & then check again */
		msleep(20);
	}
}

static void set_oom_event_info(struct kutf_tiler_heap_in *in, struct kbase_device *kbdev,
			       int csi_index, int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	u32 cs_req = ((u32 *)stream->input)[CS_REQ / 4] & CS_ACK_TILER_OOM_MASK;
	u32 cs_ack = ((u32 *)stream->output)[CS_ACK / 4] & CS_ACK_TILER_OOM_MASK;

	/* set OoM event for the Driver */
	((u32 *)stream->output)[CS_ACK / 4] = (cs_ack & ~CS_ACK_TILER_OOM_MASK) |
					      (~cs_req & CS_ACK_TILER_OOM_MASK);

	/* set OoM event parameters for the Driver */
	((u32 *)stream->output)[CS_HEAP_ADDRESS_LO / 4] = in->gpu_heap_va.u.val_u64 & 0xFFFFFFFF;
	((u32 *)stream->output)[CS_HEAP_ADDRESS_HI / 4] = in->gpu_heap_va.u.val_u64 >> 32;
	((u32 *)stream->output)[CS_HEAP_VT_START / 4] = in->vt_start.u.val_u64;
	((u32 *)stream->output)[CS_HEAP_VT_END / 4] = in->vt_end.u.val_u64;
	((u32 *)stream->output)[CS_HEAP_FRAG_END / 4] = in->frag_end.u.val_u64;
}

static struct kbase_queue *get_scheduled_queue_ptr(struct kutf_tiler_heap_in *in,
						   struct kbase_context *kctx)
{
	struct kbase_queue *queue = list_first_entry(&kctx->csf.queue_list, typeof(*queue), link);

	if (!queue || (queue->base_addr != in->gpu_queue_va.u.val_u64))
		return NULL;

	if (queue->csi_index == KBASEP_IF_NR_INVALID)
		return NULL;

	if (!queue->group)
		return NULL;

	if (queue->group->csg_nr == KBASEP_CSG_NR_INVALID) {
		pr_warn("Group-%d not on slot\n", queue->group->handle);
		return NULL;
	}

	return queue;
}

static struct kbase_context *get_kbase_ctx_ptr(struct kbase_device *kbdev,
					       struct kutf_tiler_heap_in *in)
{
	struct kbase_context *target_kctx = NULL;
	struct kbase_context *kctx;

	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		if (kctx->id == in->ctx_id.u.val_u64) {
			target_kctx = kctx;
			break;
		}
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	return target_kctx;
}

/**
 * mali_kutf_tiler_heap_oom_event_test() - Tiler heap OOM test
 * @context:	KUTF context
 *
 * Test the handling of OoM event by the Driver.
 */
static void mali_kutf_tiler_heap_oom_event_test(struct kutf_context *context)
{
	struct kutf_tiler_heap_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kutf_tiler_heap_in *in;
	struct kbase_queue *queue;
	struct kbase_context *kctx;
	enum oom_event_action action;
	int csi_index;
	int csg_nr;
	u8 csi_handlers;
	int err;

	mutex_lock(&kutf_tiler_heap_mutex);

	pr_debug("Test start\n");

	err = kutf_tiler_heap_in_new(context, &in);
	if (err) {
		mutex_unlock(&kutf_tiler_heap_mutex);
		return;
	}

	kctx = get_kbase_ctx_ptr(kbdev, in);
	if (!kctx) {
		mutex_unlock(&kutf_tiler_heap_mutex);
		return;
	}

	queue = get_scheduled_queue_ptr(in, kctx);
	if (!queue) {
		mutex_unlock(&kutf_tiler_heap_mutex);
		return;
	}

	/* Record the CSI index and CSG number lest the info gets lost due to
	 * Driver terminating the CSG. This info is required later to inspect
	 * the respective output page of CS & CSG to determine the action taken
	 * for the OoM event.
	 */
	csi_index = queue->csi_index;
	csg_nr = queue->group->csg_nr;
	csi_handlers = queue->group->csi_handlers;

	set_oom_event_info(in, kbdev, csi_index, csg_nr);

	send_interrupt(kbdev, csi_index, csg_nr);

	wait_for_oom_event_handling(queue);

	action = get_oom_event_action(kbdev, kctx, csi_index, csg_nr, (u32)in->chunk_size.u.val_u64,
				      csi_handlers);

	send_oom_event_action(context, action);

	mutex_unlock(&kutf_tiler_heap_mutex);
}

static const struct suite_list {
	const char *name;
} suite_list[] = {
	{ CSF_TILER_HEAP_SUITE_NAME },
};

/**
 * mali_kutf_tiler_heap_unit_test_main_init() - Entry point for test mdoule.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_tiler_heap_unit_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;
	union kutf_callback_data suite_data = { NULL };
	int i;

	pr_debug("Creating app\n");
	tiler_heap_app = kutf_create_application(CSF_TILER_HEAP_APP_NAME);

	if (!tiler_heap_app) {
		pr_warn("Creation of app " CSF_TILER_HEAP_APP_NAME " failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(suite_list); i++) {
		pr_debug("Create suite %s\n", suite_list[i].name);
		suite = kutf_create_suite_with_filters_and_data(
			tiler_heap_app, suite_list[i].name, CSF_TILER_HEAP_SUITE_FIXTURES,
			mali_kutf_tiler_heap_unit_create_fixture,
			mali_kutf_tiler_heap_unit_remove_fixture, KUTF_F_TEST_GENERIC, suite_data);

		if (!suite) {
			pr_warn("Creation of suite %s for app " CSF_TILER_HEAP_APP_NAME
				" failed!\n",
				suite_list[i].name);
			kutf_destroy_application(tiler_heap_app);
			return -ENOMEM;
		}

		filters = suite->suite_default_flags;
		kutf_add_test_with_filters(suite, 0x0, UNIT_TEST_0,
					   mali_kutf_tiler_heap_oom_event_test, filters);
		kutf_add_test_with_filters(suite, 0x1, UNIT_TEST_1,
					   mali_kutf_tiler_heap_oom_event_test, filters);
	}

	pr_debug("Init complete\n");
	return 0;
}

/**
 * mali_kutf_tiler_heap_unit_test_main_exit() - Module exit point for this test.
 */
static void __exit mali_kutf_tiler_heap_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(tiler_heap_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_tiler_heap_unit_test_main_init);
module_exit(mali_kutf_tiler_heap_unit_test_main_exit);

MODULE_LICENSE("GPL");
