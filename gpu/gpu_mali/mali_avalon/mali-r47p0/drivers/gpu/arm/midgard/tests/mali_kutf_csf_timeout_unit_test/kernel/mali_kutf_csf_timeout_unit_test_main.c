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

#include "../mali_kutf_csf_timeout_unit_test.h"

/* KUTF test application pointer for this test */
static struct kutf_application *timeout_app;

/**
 * struct kutf_timeout_in - Input values copied from user space, used by the
 *                             test functions.
 * @ctx_id:           Global ID of the Base context created by the test runner.
 * @gpu_queue_va:     GPU virtual address of the GPU command queue created by
 *                    the test runner.
 */
struct kutf_timeout_in {
	struct kutf_helper_named_val ctx_id;
	struct kutf_helper_named_val gpu_queue_va;
};

/**
 * mali_kutf_timeout_unit_create_fixture() - Creates the fixture data
 *                          required for all the tests in the timeout suite.
 * @context:	KUTF context.
 *
 * Return: Fixture data created on success or NULL on failure
 */
static void *mali_kutf_timeout_unit_create_fixture(struct kutf_context *context)
{
	struct kbase_device *kbdev = NULL;

	/* Acquire the kbase device */
	pr_debug("Finding device\n");
	kbdev = kbase_find_device(-1);
	if (kbdev == NULL) {
		kutf_test_fail(context, "Failed to find kbase device");
		return NULL;
	}

	pr_debug("Created fixture\n");

	return kbdev;
}

/**
 * mali_kutf_timeout_unit_remove_fixture() - Destroy fixture data previously
 * created by mali_kutf_timeout_unit_create_fixture.
 *
 * @context:             KUTF context.
 */
static void mali_kutf_timeout_unit_remove_fixture(struct kutf_context *context)
{
	struct kbase_device *kbdev = context->fixture;

	pr_debug("Destroying fixture\n");
	kbase_release_device(kbdev);
	pr_debug("Destroyed fixture\n");
}

/**
 * kutf_timeout_in_new() - Receive all variable values from user-side
 * @context: KUTF context.
 * @in:      Where to store input values copied from user-side, if successful.
 *
 * Return: 0 on success. Negative value on failure to receive from the 'run'
 *         file, positive value indicates an enum kutf_helper_err value for
 *         correct reception of data but invalid parsing.
 */
static int kutf_timeout_in_new(struct kutf_context *const context, struct kutf_timeout_in *const in)
{
	/* Receive Global ID of the Base context */
	int err = kutf_helper_receive_check_val(&in->ctx_id, context, BASE_CTX_ID,
						KUTF_HELPER_VALTYPE_U64);

	/* Receive GPU VA of the GPU queue */
	if (!err) {
		err = kutf_helper_receive_check_val(&in->gpu_queue_va, context, GPU_QUEUE_VA,
						    KUTF_HELPER_VALTYPE_U64);
	}
	return err;
}

static void check_timeout_event_action(struct kutf_context *const context,
				       struct kbase_device *const kbdev,
				       struct kbase_context *const kctx, int const csg_nr)
{
	struct kbase_csf_cmd_stream_group_info const *const group =
		&kbdev->csf.global_iface.groups[csg_nr];
	u32 const csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;

	if (csg_state != CSG_REQ_STATE_TERMINATE)
		kutf_test_fail(context, "CSG not terminated on timeout");
}

static void wait_for_timeout_event_irq_handling(struct kbase_queue_group *const group)
{
	pr_debug("Test waits for timer event interrupt to get handled\n");
	kbase_synchronize_irqs(group->kctx->kbdev);
}

static void wait_for_timeout_event_work_handling(struct kbase_queue_group *const group)
{
	pr_debug("Test waits for timeout event work item to finish\n");
	flush_work(&group->timer_event_work);
	pr_debug("Test finished waiting\n");
}

static void send_interrupt(struct kbase_device *const kbdev, int const csg_nr)
{
	u32 const host_irq_bit = (u32)1 << csg_nr;

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

static void set_timeout_event_info(struct kbase_device *const kbdev, int const csg_nr)
{
	struct kbase_csf_cmd_stream_group_info const *const group =
		&kbdev->csf.global_iface.groups[csg_nr];
	u32 csg_req = ((u32 *)group->input)[CSG_REQ / 4];
	u32 csg_ack = ((u32 *)group->output)[CSG_ACK / 4];

	pr_debug("Setting timeout event for the Driver\n");
	((u32 *)group->output)[CSG_ACK / 4] = (csg_ack & ~CSG_ACK_PROGRESS_TIMER_EVENT_MASK) |
					      (~csg_req & CSG_ACK_PROGRESS_TIMER_EVENT_MASK);
}

static struct kbase_queue *get_scheduled_queue_ptr(struct kutf_context *const context,
						   struct kutf_timeout_in *const in,
						   struct kbase_context *const kctx)
{
	struct kbase_queue *pos;
	struct kbase_queue *queue = NULL;

	list_for_each_entry(pos, &kctx->csf.queue_list, link) {
		pr_debug("Queue 0x%llx\n", pos->base_addr);
		if (pos->base_addr == in->gpu_queue_va.u.val_u64) {
			queue = pos;
			break;
		}
	}

	if (!queue) {
		kutf_test_fail(context, "Queue not found");
		return NULL;
	}

	if (queue->csi_index == KBASEP_IF_NR_INVALID) {
		kutf_test_fail(context, "CS interface index is incorrect");
		return NULL;
	}

	if (!queue->group) {
		kutf_test_fail(context, "Queue is unbound");
		return NULL;
	}

	pr_debug("CSG number %d\n", queue->group->csg_nr);

	if (queue->group->csg_nr == KBASEP_CSG_NR_INVALID) {
		pr_warn("Group-%d not on slot\n", queue->group->handle);
		return NULL;
	}

	return queue;
}

static struct kbase_context *get_kbase_ctx_ptr(struct kbase_device *kbdev,
					       struct kutf_timeout_in *in)
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

static void check_dump_on_fault(struct kutf_context *context, struct kbase_context *kctx,
				int csg_nr)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct kbase_device *const kbdev = kctx->kbdev;
	struct kbase_csf_cmd_stream_group_info const *const group =
		&kbdev->csf.global_iface.groups[csg_nr];
	u32 csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;
	u64 dummy = 0;

	if (kbdev->csf.dof.error_code != DF_PROGRESS_TIMER_TIMEOUT)
		kutf_test_fail(context, "Error code not as DF_CS_FAULT");
	if (kbdev->csf.dof.kctx_tgid != kctx->tgid)
		kutf_test_fail(context, "Unexpected kctx tgid value");
	if (csg_state != CSG_REQ_STATE_START)
		kutf_test_fail(context, "Unexpected csg state");

	/* Wait here long enough for the bottom half for progress timer timeout
	 * event to get scheduled and get blocked. Until Kbase is told that dumping
	 * is complete, the bottom half shall remain blocked and the CSG termination
	 * should not happen.
	 * Only after DOF_CHECK_DONE signal is sent to the Userspace it would
	 * unblock Kbase by indicating that dumping is complete.
	 */
	msleep(500);

	csg_state = ((u32 *)group->input)[CSG_REQ / 4] & CSG_REQ_STATE_MASK;

	if (kbdev->csf.dof.error_code != DF_PROGRESS_TIMER_TIMEOUT)
		kutf_test_fail(context, "Error code not as DF_CS_FAULT");
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

static void mali_kutf_timeout_event_test_core(struct kutf_context *context, bool test_dof)
{
	struct kbase_device *const kbdev = context->fixture;
	struct kutf_timeout_in in;
	struct kbase_queue_group *group;
	struct kbase_queue *queue;
	struct kbase_context *kctx;
	int csg_nr;
	int err;

	pr_debug("Test start\n");

	err = kutf_timeout_in_new(context, &in);
	if (err)
		return;

	kctx = get_kbase_ctx_ptr(kbdev, &in);
	if (!kctx)
		return;

	queue = get_scheduled_queue_ptr(context, &in, kctx);
	if (!queue)
		return;

	/* Record the CSG number lest the info gets lost due to
	 * Driver terminating the CSG. This info is required later to inspect
	 * the output page of CSG to determine the action taken
	 * for the timeout event.
	 * Similarly also save the queue group pointer, needed to wait for the
	 * completion of timeout event.
	 */
	group = queue->group;
	csg_nr = group->csg_nr;

	set_timeout_event_info(kbdev, csg_nr);
	send_interrupt(kbdev, csg_nr);

	wait_for_timeout_event_irq_handling(group);
	if (test_dof)
		check_dump_on_fault(context, kctx, csg_nr);
	wait_for_timeout_event_work_handling(group);

	check_timeout_event_action(context, kbdev, kctx, csg_nr);

	pr_debug("Test end\n");
}

/**
 * mali_kutf_timeout_event_test() - Test handling of timer events by the driver.
 * @context: Address of the kernel unit test framework context
 */
static void mali_kutf_timeout_event_test(struct kutf_context *context)
{
	mali_kutf_timeout_event_test_core(context, false);

	/* Synchronize with the user-space part of the test to ensure that it
	 * doesn't delete the base context and other objects while they are in
	 * use by the kernel part. We can report failures before or after
	 * sending this dummy value.
	 */
	WARN_ON(kutf_helper_send_named_u64(context, TIMER_EVENT_SYNC, 0));
	pr_debug("Test sent sync\n");
}

static void mali_kutf_timeout_event_test_wth_dof(struct kutf_context *context)
{
	mali_kutf_timeout_event_test_core(context, true);

	/* Synchronize with the user-space part of the test to ensure that it
	 * doesn't delete the base context and other objects while they are in
	 * use by the kernel part. We can report failures before or after
	 * sending this dummy value.
	 */
	WARN_ON(kutf_helper_send_named_u64(context, TIMER_EVENT_SYNC, 0));
	pr_debug("Test sent sync\n");
}

static const struct suite_list {
	const char *name;
} suite_list[] = {
	{ CSF_TIMEOUT_SUITE_NAME },
};

/**
 * mali_kutf_timeout_unit_test_main_init() - Entry point for test module.
 *
 * Return: 0 on success.
 */
static int __init mali_kutf_timeout_unit_test_main_init(void)
{
	struct kutf_suite *suite;
	unsigned int filters;
	union kutf_callback_data suite_data = { NULL };
	int i;

	pr_debug("Creating app\n");
	timeout_app = kutf_create_application(CSF_TIMEOUT_APP_NAME);

	if (!timeout_app) {
		pr_warn("Creation of app " CSF_TIMEOUT_APP_NAME " failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(suite_list); i++) {
		pr_debug("Create suite %s\n", suite_list[i].name);
		suite = kutf_create_suite_with_filters_and_data(
			timeout_app, suite_list[i].name, CSF_TIMEOUT_SUITE_FIXTURES,
			mali_kutf_timeout_unit_create_fixture,
			mali_kutf_timeout_unit_remove_fixture, KUTF_F_TEST_GENERIC, suite_data);

		if (!suite) {
			pr_warn("Creation of suite %s for app " CSF_TIMEOUT_APP_NAME " failed!\n",
				suite_list[i].name);
			kutf_destroy_application(timeout_app);
			return -ENOMEM;
		}

		filters = suite->suite_default_flags;
		kutf_add_test_with_filters(suite, 0x0, UNIT_TEST_0, mali_kutf_timeout_event_test,
					   filters);
		kutf_add_test_with_filters(suite, 0x1, UNIT_TEST_1,
					   mali_kutf_timeout_event_test_wth_dof, filters);
	}

	pr_debug("Init complete\n");
	return 0;
}

/**
 * mali_kutf_timeout_unit_test_main_exit() - Module exit point for this test.
 */
static void __exit mali_kutf_timeout_unit_test_main_exit(void)
{
	pr_debug("Exit start\n");
	kutf_destroy_application(timeout_app);
	pr_debug("Exit complete\n");
}

module_init(mali_kutf_timeout_unit_test_main_init);
module_exit(mali_kutf_timeout_unit_test_main_exit);

MODULE_LICENSE("GPL");
