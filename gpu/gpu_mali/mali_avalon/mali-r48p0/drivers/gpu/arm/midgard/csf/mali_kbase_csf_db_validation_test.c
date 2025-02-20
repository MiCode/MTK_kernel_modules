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

#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)

#include "mali_kbase_csf_csg_debugfs.h"
#include <mali_kbase.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <csf/mali_kbase_csf_db_validation.h>


#if IS_ENABLED(CONFIG_DEBUG_FS)

static u32 db_valid_test_loop_count = 100;

struct kbasep_csf_db_valid_test_debugfs_entry {
	const char *filename;
	bool (*test_func)(struct kbase_device *kbdev);
	struct kbase_device *kbdev;
};


static inline bool check_if_global_ack_done(struct kbase_device *kbdev, u32 req_mask)
{
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	bool complete = false;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	if ((kbase_csf_firmware_global_output(global_iface, GLB_ACK) & req_mask) ==
		(kbase_csf_firmware_global_input_read(global_iface, GLB_REQ) & req_mask))
		complete = true;

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	return complete;
}

static bool waiting_for_global_ack(struct kbase_device *kbdev, u32 req_mask)
{
	const unsigned int fw_timeout_ms = kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);
	long wt = kbase_csf_timeout_in_jiffies(fw_timeout_ms);
	long remaining;

	remaining = wait_event_timeout(kbdev->csf.event_wait,
			check_if_global_ack_done(kbdev, req_mask), wt);

	return remaining > 0;
}

static inline bool check_if_cs_ack_done(struct kbase_device *kbdev, struct kbase_csf_cmd_stream_info const *const stream, u32 req_mask)
{
	bool complete = false;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	if ((kbase_csf_firmware_cs_output(stream, CS_ACK) & req_mask) ==
		(kbase_csf_firmware_cs_input_read(stream, CS_REQ) & req_mask))
		complete = true;

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	return complete;
}

static bool waiting_for_cs_ack(struct kbase_device *kbdev, struct kbase_csf_cmd_stream_info const *const stream, u32 req_mask)
{
	const unsigned int fw_timeout_ms = kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);
	long wt = kbase_csf_timeout_in_jiffies(fw_timeout_ms);
	long remaining;

	remaining = wait_event_timeout(kbdev->csf.event_wait,
			check_if_cs_ack_done(kbdev, stream, req_mask), wt);

	return remaining > 0;
}


static bool kbasep_csf_db_valid_test_glb_prfcnt_enable(struct kbase_device *kbdev)
{
	static struct kbase_hwcnt_backend_csf_if_ring_buf *ring_buf = NULL;

	if (kbdev && kbdev->hwcnt_backend_csf_if_fw.ctx) {
		struct kbase_hwcnt_backend_csf_if_enable enable = { 0 };

		if (ring_buf == NULL) {
			void *cpu_dump_base;
			kbdev->hwcnt_backend_csf_if_fw.ring_buf_alloc(kbdev->hwcnt_backend_csf_if_fw.ctx, 32, &cpu_dump_base, &ring_buf);
		}

		kbdev->hwcnt_backend_csf_if_fw.dump_enable(kbdev->hwcnt_backend_csf_if_fw.ctx, ring_buf, &enable);

		if (!waiting_for_global_ack(kbdev, GLB_REQ_PRFCNT_ENABLE_MASK)) {
			return false;
		}

		kbdev->hwcnt_backend_csf_if_fw.dump_disable(kbdev->hwcnt_backend_csf_if_fw.ctx);

		if (!waiting_for_global_ack(kbdev, GLB_REQ_PRFCNT_ENABLE_MASK)) {
			return false;
		}

		return true;
	}

	return false;
}

static bool kbasep_csf_db_valid_test_glb_prfcnt_sample(struct kbase_device *kbdev)
{
	if (kbdev && kbdev->hwcnt_backend_csf_if_fw.ctx) {
		kbdev->hwcnt_backend_csf_if_fw.dump_request(kbdev->hwcnt_backend_csf_if_fw.ctx);

		if (!waiting_for_global_ack(kbdev, GLB_REQ_PRFCNT_SAMPLE_MASK)) {
			return false;
		}

		return true;
	}

	return false;
}

static bool kbasep_csf_db_valid_test_glb_counter_enable(struct kbase_device *kbdev)
{
	u32 glb_req;
	unsigned long flags;
	struct kbase_csf_global_iface *global_iface;

	global_iface = &kbdev->csf.global_iface;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	kbase_csf_db_valid_push_event(DOORBELL_GLB_COUNTER_ENABLE);
	glb_req = kbase_csf_firmware_global_input_read(global_iface, GLB_REQ);
	glb_req ^= GLB_REQ_COUNTER_ENABLE_MASK;
	kbase_csf_firmware_global_input_mask(global_iface, GLB_REQ, glb_req,
						 GLB_REQ_COUNTER_ENABLE_MASK);
	kbase_csf_ring_doorbell(kbdev, CSF_KERNEL_DOORBELL_NR);

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	if (!waiting_for_global_ack(kbdev, GLB_REQ_COUNTER_ENABLE_MASK)) {
		return false;
	}

	return true;
}


static bool kbasep_csf_db_valid_test_ping(struct kbase_device *kbdev)
{
	return (kbase_csf_firmware_ping_wait(kbdev, 100) == 0);
}

int kbase_device_csf_iterator_trace_test(struct kbase_device *kbdev);

static bool kbasep_csf_db_valid_test_glb_iter_trace_enable(struct kbase_device *kbdev)
{
	mdelay(1);
	return (kbase_device_csf_iterator_trace_test(kbdev) == 0);
}

static bool kbasep_csf_db_valid_test_csi_extract_event(struct kbase_device *kbdev)
{
	u32 csg_nr;
	u32 num_groups = kbdev->csf.global_iface.group_num;
	bool res = false;

	kbase_csf_scheduler_lock(kbdev);
	for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
		struct kbase_queue_group *const group = kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

		if (!group)
			continue;

		if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
			int i;

			for (i = 0 ; i < MAX_SUPPORTED_STREAMS_PER_GROUP ; i++) {
				struct kbase_queue *queue = group->bound_queues[i];

				if (queue) {
					struct kbase_csf_cmd_stream_group_info const *const ginfo =
						&kbdev->csf.global_iface.groups[queue->group->csg_nr];
					struct kbase_csf_cmd_stream_info const *const stream =
						&ginfo->streams[queue->csi_index];
					unsigned long flags;

					kbase_csf_scheduler_spin_lock(kbdev, &flags);
					kbase_csf_db_valid_push_event(DOORBELL_CSI_EXTRACT_EVENT(csg_nr, queue->csi_index));
					kbase_csf_firmware_cs_input_mask(stream, CS_REQ, ~kbase_csf_firmware_cs_output(stream, CS_ACK),
									 CS_REQ_EXTRACT_EVENT_MASK);
					kbase_csf_ring_cs_kernel_doorbell(kbdev, queue->csi_index, csg_nr, true);
					kbase_csf_scheduler_spin_unlock(kbdev, flags);

					if (waiting_for_cs_ack(kbdev, stream, CS_REQ_EXTRACT_EVENT_MASK)) {
						res = true;
						break;
					}
				}
			}

			if (res) {
				break;
			}
		}
	}
	kbase_csf_scheduler_unlock(kbdev);

	return res;
}

static int kbasep_csf_db_valid_standalone_test(struct seq_file *file, void *data)
{
	struct kbasep_csf_db_valid_test_debugfs_entry *entry = file->private;
	int i, pass, fail;

	pass = fail = 0;

	if (!entry->kbdev->pm.backend.gpu_powered) {
		seq_printf(file, "GPU is powered off, please enable always-on first.\n");
		return 0;
	}

	for (i = 0 ; i < db_valid_test_loop_count ; ++i) {
		if (entry->test_func(entry->kbdev)) {
			++pass;
		}
		else {
			++fail;
		}
	}

	seq_printf(file, "Test %s: %d / %d\n", entry->filename + 8, pass, pass + fail);

	return 0;
}

static int kbasep_csf_db_valid_standalone_test_debugfs_open(struct inode *in,
		struct file *file)
{
	return single_open(file, kbasep_csf_db_valid_standalone_test,
			in->i_private);
}

static const struct file_operations
	kbasep_csf_db_valid_standalone_test_debugfs_fops = {
	.open = kbasep_csf_db_valid_standalone_test_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct kbasep_csf_db_valid_test_debugfs_entry debugfs_entry[] = {
	{ "db_test_GLB_PRFCNT_ENABLE", kbasep_csf_db_valid_test_glb_prfcnt_enable, NULL },
	{ "db_test_GLB_PRFCNT_SAMPLE", kbasep_csf_db_valid_test_glb_prfcnt_sample, NULL },
	{ "db_test_GLB_COUNTER_ENABLE", kbasep_csf_db_valid_test_glb_counter_enable, NULL },
	{ "db_test_GLB_PING", kbasep_csf_db_valid_test_ping, NULL },
	{ "db_test_GLB_ITER_TRACE_ENABLE", kbasep_csf_db_valid_test_glb_iter_trace_enable, NULL },
	{ "db_test_CSI_EXTRACT_EVENT", kbasep_csf_db_valid_test_csi_extract_event, NULL },
};

static int kbasep_csf_db_valid_standalone_test_loop_count_debugfs_set(
		void *data, u64 val)
{
	db_valid_test_loop_count = (u32) val;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(kbasep_csf_db_valid_standalone_test_loop_count_debugfs_fops,
		NULL,
		&kbasep_csf_db_valid_standalone_test_loop_count_debugfs_set,
		"%llu\n");

void kbase_csf_db_valid_test_debugfs_init(struct kbase_device *kbdev)
{
	int i;

	debugfs_create_file("db_test_loop_count", 0200,
			kbdev->mali_debugfs_directory, kbdev,
			&kbasep_csf_db_valid_standalone_test_loop_count_debugfs_fops);

	for (i = 0 ; i < sizeof(debugfs_entry) / sizeof(struct kbasep_csf_db_valid_test_debugfs_entry) ; ++i) {
		debugfs_entry[i].kbdev = kbdev;
		debugfs_create_file(debugfs_entry[i].filename, 0444,
				kbdev->mali_debugfs_directory, &debugfs_entry[i],
				&kbasep_csf_db_valid_standalone_test_debugfs_fops);
	}
}

#else

void kbase_csf_db_valid_test_debugfs_init(struct kbase_device *kbdev)
{
}

#endif /* CONFIG_DEBUG_FS */
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */
