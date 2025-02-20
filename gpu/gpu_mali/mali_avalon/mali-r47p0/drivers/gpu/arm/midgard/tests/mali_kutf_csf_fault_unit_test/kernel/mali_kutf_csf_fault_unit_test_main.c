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
#include "mali_kbase_reset_gpu.h"

#include <kutf/kutf_suite.h>
#include <kutf/kutf_utils.h>
#include <kutf/kutf_helpers.h>
#include <kutf/kutf_helpers_user.h>

#include "../mali_kutf_csf_fault_unit_test.h"

/* KUTF test application pointer for this test */
static struct kutf_application *fault_app;

/**
 * struct kutf_fault_fixture_data - Fixture data for the test functions.
 * @kbdev:	kbase device for the GPU.
 */
struct kutf_fault_fixture_data {
	struct kbase_device *kbdev;
};

/**
 * struct kutf_fault_in - Fault values copied from user space, used by
 *                               the test functions.
 * @exception_type:      Specifies type of CS fault to provoke on kernel side.
 * @exception_data:      Simulated exception data the firmware sends when triggering a
 *                       fault
 * @info_exception_data: Simulated info-exception data the firmware sends when triggering a
 *                       fault
 * @trace_id0:           Simulated trace_id0 field the firmware sends when triggering a fault
 * @trace_id1:           Simulated trace_id1 field the firmware sends when triggering a fault
 * @trace_task:          Simulated trace_task field the firmware sends when triggering a fault
 */
struct kutf_fault_in {
	struct kutf_helper_named_val exception_type;
	struct kutf_helper_named_val exception_data;
	struct kutf_helper_named_val info_exception_data;

	struct kutf_helper_named_val trace_id0;
	struct kutf_helper_named_val trace_id1;
	struct kutf_helper_named_val trace_task;
};

/**
 * struct kutf_user_data_in - Input values copied from user space, used by the
 *                         test functions.
 * @ctx_id:                    Global ID of Base context created by test runner.
 * @gpu_queue_va:              GPU virtual address of the GPU command queue
 *                             created by the test runner.
 * @fault:                     Values comprising a CS fault to
 *                             simulate on kernel side.
 */
struct kutf_user_data_in {
	struct kutf_helper_named_val ctx_id;
	struct kutf_helper_named_val gpu_queue_va;
	struct kutf_fault_in fault;
};

/**
 * mali_kutf_fault_unit_create_fixture() - Creates the fixture data
 *                          required for all the tests in the fault suite.
 * @context:	KUTF context.
 *
 * Return: Fixture data created on success or NULL on failure
 */
static void *mali_kutf_fault_unit_create_fixture(struct kutf_context *context)
{
	struct kutf_fault_fixture_data *data;
	struct kbase_device *kbdev = NULL;

	pr_debug("Creating fixture\n");
	data = kutf_mempool_alloc(&context->fixture_pool, sizeof(*data));
	if (!data)
		return NULL;

	*data = (const struct kutf_fault_fixture_data){ NULL };

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
 * mali_kutf_fault_unit_remove_fixture() - Destroy fixture data previously
 * created by mali_kutf_fault_unit_create_fixture().
 *
 * @context:             KUTF context.
 */
static void mali_kutf_fault_unit_remove_fixture(struct kutf_context *context)
{
	struct kutf_fault_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;

	pr_debug("Destroying fixture\n");
	kbase_pm_context_idle(kbdev);
	kbase_release_device(kbdev);
	pr_debug("Destroyed fixture\n");
}

/**
 * kutf_fault_in_new() - Receive fixture values from user-side
 * @context: KUTF context.
 * @fault:   Where to store a set of input values copied
 *           from a user-side test fixture, if successful.
 * @include_trace_info: Indicates whether trace info is expected.
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file, positive value indicates an enum kutf_helper_err value for
 *         correct reception of data but invalid parsing.
 */
static int kutf_fault_in_new(struct kutf_context *const context, struct kutf_fault_in *const fault,
			     bool include_trace_info)
{
	int err = 0;

	if (WARN_ON(!fault) || WARN_ON(!context))
		err = -EINVAL;

	if (!err) {
		err = kutf_helper_receive_check_val(&fault->exception_type, context,
						    FAULT_EXCEPTION_TYPE, KUTF_HELPER_VALTYPE_U64);
	}

	if (!err) {
		err = kutf_helper_receive_check_val(&fault->exception_data, context,
						    FAULT_EXCEPTION_DATA, KUTF_HELPER_VALTYPE_U64);
	}

	if (!err) {
		err = kutf_helper_receive_check_val(&fault->info_exception_data, context,
						    FAULT_INFO_EXCEPTION_DATA,
						    KUTF_HELPER_VALTYPE_U64);
	}

	if (include_trace_info) {
		if (!err) {
			err = kutf_helper_receive_check_val(&fault->trace_id0, context,
							    FAULT_TRACE_ID0,
							    KUTF_HELPER_VALTYPE_U64);
		}

		if (!err) {
			err = kutf_helper_receive_check_val(&fault->trace_id1, context,
							    FAULT_TRACE_ID1,
							    KUTF_HELPER_VALTYPE_U64);
		}

		if (!err) {
			err = kutf_helper_receive_check_val(&fault->trace_task, context,
							    FAULT_TRACE_TASK,
							    KUTF_HELPER_VALTYPE_U64);
		}
	}

	return err;
}

/**
 * kutf_user_data_in_new() - Receive all variable values from user-side
 * @context:               KUTF context.
 * @in:                    Where to store a pointer to a set of input values copied
 *                         from user-side, if successful.
 * @include_trace_info:    Indicates whether trace info should be read
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file, positive value indicates an enum kutf_helper_err value for
 *         correct reception of data but invalid parsing.
 */
static int kutf_user_data_in_new(struct kutf_context *const context,
				 struct kutf_user_data_in *const in, bool include_trace_info)
{
	int err = 0;

	if (WARN_ON(!in) || WARN_ON(!context))
		err = -EINVAL;

	/* Receive Global ID of the Base context */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->ctx_id, context, BASE_CTX_ID,
						    KUTF_HELPER_VALTYPE_U64);
	}

	if (!err) {
		err = kutf_helper_receive_check_val(&in->gpu_queue_va, context, GPU_QUEUE_VA,
						    KUTF_HELPER_VALTYPE_U64);
	}

	if (!err)
		err = kutf_fault_in_new(context, &in->fault, include_trace_info);

	return err;
}

static enum fault_event_action get_fault_event_action(struct kbase_device *kbdev, int csi_index,
						      int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	const u32 csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;
	const u32 cs_req = ((u32 *)stream->input)[CS_REQ / 4];
	const u32 cs_ack = ((u32 *)stream->input)[CS_ACK / 4];

	const u32 cs_state = cs_req & CS_REQ_STATE_MASK;
	const u32 cs_req_fault = cs_req & CS_REQ_FAULT_MASK;
	const u32 cs_ack_fault = cs_ack & CS_REQ_FAULT_MASK;

	if (csg_state == CSG_REQ_STATE_TERMINATE) {
		if (WARN_ON(cs_state != CS_REQ_STATE_STOP))
			return FAULT_EVENT_INCONSISTENT_CS_STATE;
		else
			return FAULT_EVENT_CSG_TERMINATED;
	} else {
		if (cs_req_fault == cs_ack_fault)
			return FAULT_EVENT_ERROR_RECOVERY_REQUESTED;
		else
			return FAULT_EVENT_NO_ACTION;
	}
}

static void send_fault_event_action(struct kutf_context *context, enum fault_event_action action)
{
	WARN_ON(kutf_helper_send_named_u64(context, FAULT_EVENT_ACTION, action));
}

static void wait_for_fault_event_handling(struct kbase_queue *queue)
{
	/* First wait for fault event interrupt to get handled */
	kbase_synchronize_irqs(queue->kctx->kbdev);
}

static void wait_for_fw_fatal_event_irq_handling(struct kbase_context *kctx)
{
	/* First wait for fw error fatal event interrupt to get handled */
	kbase_synchronize_irqs(kctx->kbdev);
}

static void wait_for_fw_fatal_event_work_handling(struct kbase_context *kctx)
{
	/* Now wait for fw error fatal event work item to finish */
	flush_work(&kctx->kbdev->csf.fw_error_work);
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

static void set_fault_event_info(struct kutf_user_data_in *in, struct kbase_device *kbdev,
				 int csi_index, int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	u32 cs_req = ((u32 *)stream->input)[CS_REQ / 4];
	u32 cs_ack = ((u32 *)stream->output)[CS_ACK / 4];
	u64 temp_cs_fault_info =
		CS_FAULT_INFO_EXCEPTION_DATA_SET(0, in->fault.info_exception_data.u.val_u64);

	((u32 *)stream->output)[CS_ACK / 4] = (cs_ack & ~CS_ACK_FAULT_MASK) |
					      (~cs_req & CS_REQ_FAULT_MASK);

	((u32 *)stream->output)[CS_FAULT / 4] =
		(in->fault.exception_type.u.val_u64 & CS_FAULT_EXCEPTION_TYPE_MASK) |
		((in->fault.exception_data.u.val_u64 << CS_FAULT_EXCEPTION_DATA_SHIFT)
			 & CS_FAULT_EXCEPTION_DATA_MASK);

	((u32 *)stream->output)[CS_FAULT_INFO_LO / 4] = temp_cs_fault_info & U32_MAX;
	((u32 *)stream->output)[CS_FAULT_INFO_HI / 4] = (temp_cs_fault_info >> 32) & U32_MAX;
}


/**
 * set_fatal_event_info() - Set CS fatal information on CS
 *                          interface.
 * @in:         Input values from user space
 * @kbdev:      Pointer to kbase device
 * @csi_index:  ID of CSI
 * @csg_nr:     Number of CSG interface
 */
static void set_fatal_event_info(struct kutf_user_data_in *in, struct kbase_device *kbdev,
				 int csi_index, int csg_nr)
{
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	u32 cs_req = ((u32 *)stream->input)[CS_REQ / 4];
	u32 cs_ack = ((u32 *)stream->output)[CS_ACK / 4];
	u64 temp_cs_fault_info =
		CS_FATAL_INFO_EXCEPTION_DATA_SET(0, in->fault.info_exception_data.u.val_u64);

	((u32 *)stream->output)[CS_ACK / 4] = (cs_ack & ~CS_ACK_FATAL_MASK) |
					      (~cs_req & CS_REQ_FATAL_MASK);

	((u32 *)stream->output)[CS_FATAL / 4] =
		(in->fault.exception_type.u.val_u64 & CS_FATAL_EXCEPTION_TYPE_MASK) |
		((in->fault.exception_data.u.val_u64 << CS_FATAL_EXCEPTION_DATA_SHIFT)
			 & CS_FATAL_EXCEPTION_DATA_MASK);

	((u32 *)stream->output)[CS_FATAL_INFO_LO / 4] = temp_cs_fault_info & U32_MAX;
	((u32 *)stream->output)[CS_FATAL_INFO_HI / 4] = (temp_cs_fault_info >> 32) & U32_MAX;
}


/**
 * get_scheduled_queue_ptr() - Find a queue in the context
 *                             using user-provided value.
 * @in:         Input values from user space
 * @kctx:       Kbase context
 *
 * Return: Pointer of the queue if found. Otherwise NULL.
 */
static struct kbase_queue *get_scheduled_queue_ptr(struct kutf_user_data_in *in,
						   struct kbase_context *kctx)
{
	struct list_head *entry = NULL, *tmp = NULL;
	struct kbase_queue *queue = NULL;

	mutex_lock(&kctx->csf.lock);
	list_for_each_safe(entry, tmp, &kctx->csf.queue_list) {
		queue = list_entry(entry, struct kbase_queue, link);
		if (queue->base_addr == in->gpu_queue_va.u.val_u64)
			break;
	}
	mutex_unlock(&kctx->csf.lock);

	if (!queue)
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
					       struct kutf_user_data_in *in)
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

static void suspend_group(struct kutf_context *context, struct kbase_queue_group *group)
{
	struct kbase_context *kctx = group->kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	struct kbase_suspend_copy_buffer sus_buf;
	int err;

	err = kbase_reset_gpu_prevent_and_wait(kbdev);
	if (err) {
		kutf_test_fail(context, "Unsuccessful GPU reset detected when suspending group");
		return;
	}
	mutex_lock(&kctx->csf.lock);

	/* Just want the group to be suspended */
	memset(&sus_buf, 0, sizeof(sus_buf));
	kbase_csf_scheduler_group_copy_suspend_buf(group, &sus_buf);

	mutex_unlock(&kctx->csf.lock);
	kbase_reset_gpu_allow(kbdev);
}

static bool check_all_groups_in_ctx_dead(struct kbase_context *const kctx)
{
	unsigned int g;
	bool result = true;

	mutex_lock(&kctx->csf.lock);

	for (g = 0; g < ARRAY_SIZE(kctx->csf.queue_groups); ++g) {
		struct kbase_queue_group *const group = kctx->csf.queue_groups[g];

		if (group && group->run_state != KBASE_CSF_GROUP_TERMINATED) {
			result = false;
			break;
		}
	}

	mutex_unlock(&kctx->csf.lock);
	return result;
}

static void check_all_groups_in_ctx_live(struct kutf_context *context,
					 struct kbase_context *const kctx)
{
	unsigned int g;

	mutex_lock(&kctx->csf.lock);

	for (g = 0; g < ARRAY_SIZE(kctx->csf.queue_groups); ++g) {
		struct kbase_queue_group *const group = kctx->csf.queue_groups[g];

		if (group && group->run_state == KBASE_CSF_GROUP_TERMINATED)
			kutf_test_fail(context, "Wrongly terminated a queue group");
	}

	mutex_unlock(&kctx->csf.lock);
}

static bool check_all_groups_dead(struct kbase_device *const kbdev)
{
	struct kbase_context *kctx;
	bool result = true;

	mutex_lock(&kbdev->kctx_list_lock);

	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		if (!check_all_groups_in_ctx_dead(kctx)) {
			result = false;
			break;
		}
	}

	mutex_unlock(&kbdev->kctx_list_lock);
	return result;
}

static void check_all_groups_live(struct kutf_context *context, struct kbase_device *const kbdev)
{
	struct kbase_context *kctx;

	mutex_lock(&kbdev->kctx_list_lock);

	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link)
		check_all_groups_in_ctx_live(context, kctx);

	mutex_unlock(&kbdev->kctx_list_lock);
}

static void check_all_groups_in_most_ctx_live(struct kutf_context *context,
					      struct kbase_device *const kbdev,
					      struct kbase_context *const except)
{
	struct kbase_context *kctx;

	mutex_lock(&kbdev->kctx_list_lock);

	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		if (except != kctx)
			check_all_groups_in_ctx_live(context, kctx);
	}

	mutex_unlock(&kbdev->kctx_list_lock);
}

static void check_dump_on_fault(struct kutf_context *context, struct kbase_context *kctx,
				struct kutf_user_data_in *in, int csi_index, int csg_nr)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct kbase_device *const kbdev = kctx->kbdev;
	u32 exception_type = in->fault.exception_type.u.val_u64;
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	struct kbase_csf_cmd_stream_info *stream = &group->streams[csi_index];
	u32 cs_req = ((u32 *)stream->input)[CS_REQ / 4];
	u32 cs_ack = ((u32 *)stream->output)[CS_ACK / 4];

	if ((exception_type != CS_FAULT_EXCEPTION_TYPE_CS_RESOURCE_TERMINATED) &&
	    (exception_type != CS_FAULT_EXCEPTION_TYPE_CS_INHERIT_FAULT)) {
		u64 dummy = 0;

		if (kbdev->csf.dof.error_code != DF_CS_FAULT)
			kutf_test_fail(context, "Error code not set as DF_CS_FAULT");
		if (kbdev->csf.dof.kctx_tgid != kctx->tgid)
			kutf_test_fail(context, "Unexpected kctx tgid value");
		if ((cs_req & CS_REQ_FAULT_MASK) == (cs_ack & CS_REQ_FAULT_MASK))
			kutf_test_fail(context, "Same value for fault bit in cs_req & cs_ack");

		/* Wait here long enough for the bottom half for CS_FAULT event to
		 * get scheduled and get blocked. Until Kbase is told that dumping
		 * is complete, the bottom half shall remain blocked and tha fault
		 * should not get acknowledged.
		 * Only after DOF_CHECK_DONE signal is sent to the Userspace it would
		 * unblock Kbase by indicating that dumping is complete.
		 */
		msleep(100);

		cs_req = ((u32 *)stream->input)[CS_REQ / 4];
		cs_ack = ((u32 *)stream->output)[CS_ACK / 4];
		if (kbdev->csf.dof.error_code != DF_CS_FAULT)
			kutf_test_fail(context, "Error code not set as DF_CS_FAULT");
		if (kbdev->csf.dof.kctx_tgid != kctx->tgid)
			kutf_test_fail(context, "Unexpected kctx tgid value");
		if ((cs_req & CS_REQ_FAULT_MASK) == (cs_ack & CS_REQ_FAULT_MASK))
			kutf_test_fail(context, "Same value for fault bit in cs_req & cs_ack");

		/* Signal to the Userspace to unblock Kbase */
		kutf_helper_send_named_u64(context, DOF_CHECK_DONE, dummy);

		kbase_debug_csf_fault_wait_completion(kbdev);

		if (kbdev->csf.dof.error_code != DF_NO_ERROR)
			kutf_test_fail(context, "Error code is not NO_ERROR after dump completion");
		if (kbdev->csf.dof.kctx_tgid)
			kutf_test_fail(context, "Unexpected kctx tgid value after dump completion");
	} else {
		if (kbdev->csf.dof.error_code != DF_NO_ERROR)
			kutf_test_fail(context, "Error code is not NO_ERROR");
		if (kbdev->csf.dof.kctx_tgid)
			kutf_test_fail(context, "Unexpected kctx tgid value");
		if ((cs_req & CS_REQ_FAULT_MASK) != (cs_ack & CS_REQ_FAULT_MASK))
			kutf_test_fail(context, "Different value for fault bit in cs_req & cs_ack");
	}
#endif /* CONFIG_DEBUG_FS */
}

/**
 * fault_event_test_common() - Common function to test for CS fault
 *                             handling with or without suspending group
 * @context:             KUTF context
 * @test_mode:           Mode of the CS fault test to indicate additional steps to be taken
 *                       apart from generating the CS fault event.
 * @include_trace_info:  Indicates whether trace info should be read
 */
static void fault_event_test_common(struct kutf_context *context, enum fault_test_mode test_mode,
				    bool include_trace_info)
{
	struct kutf_fault_fixture_data *const data = context->fixture;
	struct kbase_device *const kbdev = data->kbdev;
	struct kbase_context *kctx;
	struct kutf_user_data_in in;
	struct kbase_queue *queue = NULL;
	enum fault_event_action action = FAULT_EVENT_INCONSISTENT_CS_STATE;
	int err;

	CSTD_UNUSED(include_trace_info);

	pr_debug("Test start\n");

	err = kutf_user_data_in_new(context, &in, include_trace_info);
	if (!err) {
		kctx = get_kbase_ctx_ptr(kbdev, &in);

		if (kctx)
			queue = get_scheduled_queue_ptr(&in, kctx);
	}

	if (queue) {
		/* Record the CSI index and CSG number lest the info gets lost due to
		 * Driver terminating the CSG. This info is required later to inspect
		 * the respective output page of CS & CSG to determine the action taken
		 * for the fault event.
		 */
		int const csi_index = queue->csi_index;
		int const csg_nr = queue->group->csg_nr;
		struct kbase_queue_group *const group = queue->group;

		set_fault_event_info(&in, kbdev, csi_index, csg_nr);

		send_interrupt(kbdev, csi_index, csg_nr);

		if (test_mode == FAULT_TEST_SUSPEND)
			suspend_group(context, group);

		wait_for_fault_event_handling(queue);

		if (test_mode == FAULT_TEST_DOF)
			check_dump_on_fault(context, kctx, &in, csi_index, csg_nr);

		/* When group suspend is also involved, the group could go off slot
		 * before the interrupt for fault event is manually generated.
		 */
		if (test_mode != FAULT_TEST_SUSPEND)
			action = get_fault_event_action(kbdev, csi_index, csg_nr);

		/* No queue groups should have been terminated */
		check_all_groups_live(context, kbdev);
	}

	send_fault_event_action(context, action);
	pr_debug("Test end\n");
}

/**
 * mali_kutf_fault_event_test() - Test for CS fault handling
 *
 * @context:   KUTF context
 */
static void mali_kutf_fault_event_test(struct kutf_context *context)
{
	fault_event_test_common(context, FAULT_TEST_DEFAULT, false);
}

/**
 * mali_kutf_trace_info_fault_event_test() - Test for Trace Info fault handling
 *
 * @context:   KUTF context
 */
static void mali_kutf_trace_info_fault_event_test(struct kutf_context *context)
{
	struct kutf_fault_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kbase_gpu_id_props *gpu_id = &kbdev->gpu_props.gpu_id;
	bool skip_trace_info_test = true;

	if ((gpu_id->arch_major > 14) || ((gpu_id->arch_major == 14) && (gpu_id->arch_rev >= 4)))
		skip_trace_info_test = false;

	kutf_helper_send_named_u64(context, SKIP_TRACE_INFO_TEST, skip_trace_info_test);

	if (!skip_trace_info_test)
		fault_event_test_common(context, FAULT_TEST_DEFAULT, true);
	else
		pr_info("Skipping trace_info test\n");
}

/**
 * mali_kutf_fault_event_with_suspend_test() - Test for CS fault
 *                                             handling with suspending group
 * @context:   KUTF context
 *
 * This test tries to verify the handling of CSG suspend operation when a CS
 * fault event is in flight.
 * It just enqueues a forceful group suspend operation immediately after
 * raising the fake interrupt for the CS fault event.
 */
static void mali_kutf_fault_event_with_suspend_test(struct kutf_context *context)
{
	fault_event_test_common(context, FAULT_TEST_SUSPEND, false);
}

static void mali_kutf_fault_event_with_dof_test(struct kutf_context *context)
{
	fault_event_test_common(context, FAULT_TEST_DOF, false);
}

/**
 * mali_kutf_gpu_fault_event_test() - Exercise GPU fault notification.
 *
 * @context:   KUTF context
 *
 * This test triggers GPU fault worker using fault information injected from
 * user space.
 */
static void mali_kutf_gpu_fault_event_test(struct kutf_context *context)
{
	struct kutf_fault_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kutf_fault_in in;

	pr_debug("Test start\n");

	if (!kutf_fault_in_new(context, &in, false)) {
		struct kbase_context *kctx;

		/* Exception type is stored in LSB of received data */
		const u8 exception_type = in.exception_type.u.val_u64 &
					  GPU_FAULTSTATUS_EXCEPTION_TYPE_MASK;

		/* Exception data is stored in the least significant 3Bytes
		 * of received data.
		 */
		const u32 exception_data = (u32)(in.exception_data.u.val_u64 &
						 (U32_MAX >> GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT))
					   << GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT;

		const u32 status = exception_type | exception_data;
		const u64 address = in.info_exception_data.u.val_u64;
		const u32 as_nr = GPU_FAULTSTATUS_JASID_GET(exception_data);
		const bool as_valid = status & GPU_FAULTSTATUS_JASID_VALID_MASK;

		kbase_mmu_gpu_fault_interrupt(kbdev, status, as_nr, address, as_valid);

		/* wait for fault handling to complete */
		while (atomic_read(&kbdev->faults_pending))
			;

		if (as_valid) {
			/* Only queue groups in the faulty context should
			 * have been terminated
			 */
			kctx = kbdev->as_to_kctx[as_nr];
			if (!kctx)
				kutf_test_fail(
					context,
					"Failed to find a kbase context from the address space");
			else {
				if (!check_all_groups_in_ctx_dead(kctx))
					kutf_test_fail(context,
						       "Failed to terminate a queue group");
				check_all_groups_in_most_ctx_live(context, kbdev, kctx);
			}
		} else {
			/* Check all groups terminated from at least one context */
			bool groups_terminated = false;

			mutex_lock(&kbdev->kctx_list_lock);

			list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
				if (check_all_groups_in_ctx_dead(kctx)) {
					groups_terminated = true;
					break;
				}
			}
			mutex_unlock(&kbdev->kctx_list_lock);

			if (!groups_terminated)
				kutf_test_fail(
					context,
					"Failed to terminate all queue group in one context");
		}
	}

	/* User space didn't specify a queue for which to simulate a fault
	 * because it's not appropriate for this test. Consequently this is
	 * not a real event action: it's just to signal completion.
	 */
	send_fault_event_action(context, FAULT_EVENT_CSG_TERMINATED);
	pr_debug("Test end\n");
}

static void check_dump_on_fatal(struct kutf_context *context, struct kbase_context *kctx,
				int csg_nr)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct kbase_device *const kbdev = kctx->kbdev;
	struct kbase_csf_cmd_stream_group_info *group = &kbdev->csf.global_iface.groups[csg_nr];
	u32 csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;
	u64 dummy = 0;

	if (kbdev->csf.dof.error_code != DF_FW_INTERNAL_ERROR)
		kutf_test_fail(context, "Error code not set as DF_FW_INTERNAL_ERROR");
	if (kbdev->csf.dof.kctx_tgid != kctx->tgid)
		kutf_test_fail(context, "Unexpected kctx tgid value");
	if (csg_state != CSG_REQ_STATE_START)
		kutf_test_fail(context, "Unexpected csg state");

	/* Wait here long enough for the bottom half for FW_INTERNAL_ERRO event
	 * to get scheduled and get blocked. Until Kbase is told that dumping is
	 * complete, the bottom half shall remain blocked and tha fault should
	 * not get acknowledged.
	 * Only after DOF_CHECK_DONE signal is sent to the Userspace it would
	 * unblock Kbase by indicating that dumping is complete.
	 */
	msleep(500);

	csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;

	if (kbdev->csf.dof.error_code != DF_FW_INTERNAL_ERROR)
		kutf_test_fail(context, "Error code not set as DF_FW_INTERNAL_ERROR");
	if (kbdev->csf.dof.kctx_tgid != kctx->tgid)
		kutf_test_fail(context, "Unexpected kctx tgid value");
	if (csg_state != CSG_REQ_STATE_START)
		kutf_test_fail(context, "Unexpected csg state");

	/* Signal to the Userspace to unblock Kbase */
	kutf_helper_send_named_u64(context, DOF_CHECK_DONE, dummy);

	kbase_debug_csf_fault_wait_completion(kbdev);

	if (kbdev->csf.dof.error_code != DF_NO_ERROR)
		kutf_test_fail(context, "Error code is not NO_ERROR after dump completion");
	if (kbdev->csf.dof.kctx_tgid)
		kutf_test_fail(context, "Unexpected kctx tgid value after dump completion");
#endif /* CONFIG_DEBUG_FS */
}

/**
 * mali_kutf_fatal_event_test_common() - Test for internal firmware CS fatal handling
 *
 * @context:   KUTF context
 * @test_dof:  Flag to indicate if dump on fault handling needs to be checked for CS fatal error.
 * @include_trace_info: Indicates whether trace info should be read
 */
static void mali_kutf_fatal_event_test_common(struct kutf_context *context, bool test_dof,
					      bool include_trace_info)
{
	struct kutf_fault_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kutf_user_data_in in;
	struct kbase_queue *queue = NULL;
	enum fault_event_action action = FAULT_EVENT_INCONSISTENT_CS_STATE;
	int err;

	CSTD_UNUSED(include_trace_info);

	pr_debug("Test start\n");

	err = kutf_user_data_in_new(context, &in, include_trace_info);
	if (!err) {
		struct kbase_context *const kctx = get_kbase_ctx_ptr(kbdev, &in);

		if (kctx)
			queue = get_scheduled_queue_ptr(&in, kctx);

		if (queue) {
			/* Record the CSI index and CSG number lest the info gets lost due to
			 * Driver terminating the CSG. This info is required later to inspect
			 * the respective output page of CS & CSG to determine the action taken
			 * for the fault event.
			 */
			int const csi_index = queue->csi_index;
			int const csg_nr = queue->group->csg_nr;

			set_fatal_event_info(&in, kbdev, csi_index, csg_nr);

			send_interrupt(kbdev, csi_index, csg_nr);
			wait_for_fw_fatal_event_irq_handling(kctx);
			if (test_dof)
				check_dump_on_fatal(context, kctx, csg_nr);
			wait_for_fw_fatal_event_work_handling(kctx);

			action = get_fault_event_action(kbdev, csi_index, csg_nr);

			if (!check_all_groups_dead(kbdev))
				kutf_test_fail(context,
					       "Failed to terminate queue groups in all context");
		}
	}

	send_fault_event_action(context, action);

	pr_debug("Test end\n");
}

static void mali_kutf_fatal_event_test(struct kutf_context *context)
{
	mali_kutf_fatal_event_test_common(context, false, false);
}

static void mali_kutf_fatal_event_test_with_dof(struct kutf_context *context)
{
	mali_kutf_fatal_event_test_common(context, true, false);
}

/**
 * mali_kutf_fatal_event_cs_unrecoverable_test() - Test for handling of CS fatal cs unrecoverable
 *                                                 event
 *
 * @context:   KUTF context
 */
static void mali_kutf_fatal_event_cs_unrecoverable_test(struct kutf_context *context)
{
	struct kutf_fault_fixture_data *data = context->fixture;
	struct kbase_device *kbdev = data->kbdev;
	struct kutf_user_data_in in;
	enum fault_event_action action = FAULT_EVENT_INCONSISTENT_CS_STATE;
	int err;

	dev_dbg(kbdev->dev, "Test start");

	err = kutf_user_data_in_new(context, &in, false);
	if (!err) {
		struct kbase_context *const kctx = get_kbase_ctx_ptr(kbdev, &in);

		/* assume csi_index & csg_nr are 0 (run on linux & single queue) */
		set_fatal_event_info(&in, kbdev, 0, 0);
		send_interrupt(kbdev, 0, 0);
		wait_for_fw_fatal_event_irq_handling(kctx);
		wait_for_fw_fatal_event_work_handling(kctx);
		action = FAULT_EVENT_CSG_TERMINATED;

		if (!check_all_groups_in_ctx_dead(kctx))
			kutf_test_fail(context, "Failed to terminate queue groups in all context");
	}

	send_fault_event_action(context, action);
	dev_dbg(kbdev->dev, "Test end");
}

static void init_cs_fault_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, CS_FAULT_UNIT_TEST_0, mali_kutf_fault_event_test,
				   filters);
	kutf_add_test_with_filters(suite, 0x1, CS_FAULT_UNIT_TEST_1,
				   mali_kutf_fault_event_with_suspend_test, filters);
	kutf_add_test_with_filters(suite, 0x2, CS_FAULT_UNIT_TEST_2,
				   mali_kutf_fault_event_with_dof_test, filters);
}

static void init_gpu_fault_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, GPU_FAULT_UNIT_TEST_0,
				   mali_kutf_gpu_fault_event_test, filters);
}

static void init_fw_cs_fault_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, FW_CS_FAULT_UNIT_TEST_0, mali_kutf_fatal_event_test,
				   filters);
	kutf_add_test_with_filters(suite, 0x1, FW_CS_FAULT_UNIT_TEST_1,
				   mali_kutf_fatal_event_cs_unrecoverable_test, filters);
	kutf_add_test_with_filters(suite, 0x2, FW_CS_FAULT_UNIT_TEST_2,
				   mali_kutf_fatal_event_test_with_dof, filters);
}

static void init_fault_info_fault_suite(struct kutf_suite *suite)
{
	unsigned int const filters = suite->suite_default_flags;

	kutf_add_test_with_filters(suite, 0x0, TRACE_INFO_FAULT_UNIT_TEST_0,
				   mali_kutf_trace_info_fault_event_test, filters);
}

static const struct suite_list {
	const char *name;
	unsigned int fixture_count;
	void (*init)(struct kutf_suite *suite);
} suite_list[] = {
	{ CS_FAULT_SUITE_NAME, CS_FAULT_SUITE_FIXTURES, init_cs_fault_suite },
	{ GPU_FAULT_SUITE_NAME, GPU_FAULT_SUITE_FIXTURES, init_gpu_fault_suite },
	{ FW_CS_FAULT_SUITE_NAME, FW_CS_FAULT_SUITE_FIXTURES, init_fw_cs_fault_suite },
	{ TRACE_INFO_FAULT_SUITE_NAME, TRACE_INFO_FAULT_SUITE_FIXTURES,
	  init_fault_info_fault_suite },
};

/**
 * mali_kutf_fault_unit_test_main_init() - Entry point for test module.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_fault_unit_test_main_init(void)
{
	unsigned int i;
	union kutf_callback_data suite_data = { NULL };

	pr_debug("Creating app\n");
	fault_app = kutf_create_application(CSF_FAULT_APP_NAME);

	if (!fault_app) {
		pr_warn("Creation of app " CSF_FAULT_APP_NAME " failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(suite_list); i++) {
		struct kutf_suite *suite;

		pr_debug("Create suite %s\n", suite_list[i].name);
		suite = kutf_create_suite_with_filters_and_data(fault_app, suite_list[i].name,
								suite_list[i].fixture_count,
								mali_kutf_fault_unit_create_fixture,
								mali_kutf_fault_unit_remove_fixture,
								KUTF_F_TEST_GENERIC, suite_data);

		if (!suite) {
			pr_warn("Creation of suite %s for app " CSF_FAULT_APP_NAME " failed!\n",
				suite_list[i].name);
			kutf_destroy_application(fault_app);
			return -ENOMEM;
		}

		suite_list[i].init(suite);
	}

	pr_debug("Init complete\n");
	return 0;
}

/**
 * mali_kutf_fault_unit_test_main_exit() - Module exit point for this test.
 */
static void __exit mali_kutf_fault_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(fault_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_fault_unit_test_main_init);
module_exit(mali_kutf_fault_unit_test_main_exit);

MODULE_LICENSE("GPL");
