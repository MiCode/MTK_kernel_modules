// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <csf/mali_kbase_csf_firmware_log.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include "mtk_platform_logbuffer.h"
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
#if MALI_USE_CSF
#include "backend/gpu/mali_kbase_pm_internal.h"
#endif

static int enop_metadata_dump_enable = 0;

static bool global_debug_request_complete(struct kbase_device *const kbdev, u32 const req_mask)
{
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	bool complete = false;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	if ((kbase_csf_firmware_global_output(global_iface, GLB_DEBUG_ACK) & req_mask) ==
	    (kbase_csf_firmware_global_input_read(global_iface, GLB_DEBUG_REQ) & req_mask))
		complete = true;

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	return complete;
}

static bool global_request_complete(struct kbase_device *const kbdev,
				    u32 const req_mask)
{
	struct kbase_csf_global_iface *global_iface =
				&kbdev->csf.global_iface;
	bool complete = false;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	if ((kbase_csf_firmware_global_output(global_iface, GLB_ACK) & req_mask) ==
	    (kbase_csf_firmware_global_input_read(global_iface, GLB_REQ) & req_mask))
		complete = true;

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	return complete;
}

static int wait_for_global_request(struct kbase_device *const kbdev,
				   u32 const req_mask)
{
	const long wait_timeout =
		kbase_csf_timeout_in_jiffies(kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT));
	long remaining;
	int err = 0;

	remaining = wait_event_timeout(kbdev->csf.event_wait,
				       global_request_complete(kbdev, req_mask),
				       wait_timeout);

	if (!remaining) {
		dev_warn(kbdev->dev, "Timed out waiting for global request %x to complete",
			 req_mask);
		err = -ETIMEDOUT;
	}

	return err;
}

static void set_global_debug_request(const struct kbase_csf_global_iface *const global_iface,
				     u32 const req_mask)
{
	u32 glb_debug_req;

	kbase_csf_scheduler_spin_lock_assert_held(global_iface->kbdev);

	glb_debug_req = kbase_csf_firmware_global_output(global_iface, GLB_DEBUG_ACK);
	glb_debug_req ^= req_mask;

	kbase_csf_firmware_global_input_mask(global_iface, GLB_DEBUG_REQ, glb_debug_req, req_mask);
}

static void set_global_request(
	const struct kbase_csf_global_iface *const global_iface,
	u32 const req_mask)
{
	u32 glb_req;

	kbase_csf_scheduler_spin_lock_assert_held(global_iface->kbdev);

	glb_req = kbase_csf_firmware_global_output(global_iface, GLB_ACK);
	glb_req ^= req_mask;
	kbase_csf_firmware_global_input_mask(global_iface, GLB_REQ, glb_req,
					     req_mask);
}

void mtk_debug_dump_enop_metadata(struct kbase_device *kbdev)
{
	unsigned long flags;
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	/* Use the DDK native flow - GLB_DEBUG_RUN_MODE_TYPE_NOP for ENOP metadata dump */
	uint32_t run_mode = GLB_DEBUG_REQ_RUN_MODE_SET(0, GLB_DEBUG_RUN_MODE_TYPE_NOP);
	int ret = 0;

	/* Check if enop_metadata_dump_enable enabled */
	if (enop_metadata_dump_enable == 0)
		return;

	mutex_lock(&kbdev->csf.reg_lock);
	kbase_csf_scheduler_spin_lock(kbdev, &flags);
	/* Prepare GLB_DEBUG_REQ for ENOP metadata dump */
	set_global_debug_request(global_iface, GLB_DEBUG_REQ_DEBUG_RUN_MASK | run_mode);
	/* Prepare GLB_REQ for debug requeset */
	set_global_request(global_iface, GLB_REQ_DEBUG_CSF_REQ_MASK);
	/* Ring doorbell to CSFFW for debug request */
	kbase_csf_ring_doorbell(kbdev, CSF_KERNEL_DOORBELL_NR);
	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	/* Wait finish of CSFFW process nop metadata dump */
	ret = wait_for_global_request(kbdev, GLB_REQ_DEBUG_CSF_REQ_MASK);
	if (!ret) {
		/* Check if CSFFW process nop metadata dump complete */
		if (global_debug_request_complete(kbdev, GLB_DEBUG_REQ_DEBUG_RUN_MASK))
			dev_info(kbdev->dev, "ENOP metadata dump complete!");
		else
			dev_info(kbdev->dev, "ENOP metadata dump failed!");
	}

	mutex_unlock(&kbdev->csf.reg_lock);

	/* Dump fw log with nop metadata */
	kbase_csf_firmware_log_dump_buffer(kbdev);
}

static void mtk_enop_metadata_dump_worker(struct work_struct *const data)
{
	struct kbase_device *kbdev = container_of(data, struct kbase_device, mtk_enop_metadata_dump_work);
	mtk_debug_dump_enop_metadata(kbdev);
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_debug_enop_metadata_dump_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "enop_metadata_dump_enable = %d\n", enop_metadata_dump_enable);

	return 0;
}

static int mtk_debug_enop_metadata_dump_enable_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_debug_enop_metadata_dump_enable_show, in->i_private);
}

static int mtk_debug_enop_metadata_dump_enable_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_debug_enop_metadata_dump_enable_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	struct kbase_device *kbdev = (struct kbase_device *)file->private_data;
	int ret = 0;
	CSTD_UNUSED(ppos);

	ret = kstrtoint_from_user(ubuf, count, 0, &enop_metadata_dump_enable);
	if (ret)
		return ret;

	dev_info(kbdev->dev, "@%s: enop_metadata_dump_enable = %d ", __func__, enop_metadata_dump_enable);
	if (enop_metadata_dump_enable == 5566) {
		kbase_csf_scheduler_pm_active(kbdev);
		kbase_pm_wait_for_l2_powered(kbdev);
		mtk_debug_dump_enop_metadata(kbdev);
		kbase_csf_scheduler_pm_idle(kbdev);
	}

	return count;
}

static const struct file_operations mtk_debug_enop_metadata_dump_enable_fops = {
	.open    = mtk_debug_enop_metadata_dump_enable_open,
	.release = mtk_debug_enop_metadata_dump_enable_release,
	.read    = seq_read,
	.write   = mtk_debug_enop_metadata_dump_enable_write,
	.llseek  = seq_lseek
};

int mtk_debug_dump_enop_metadata_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("enop_metadata_dump_enable", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_debug_enop_metadata_dump_enable_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_debug_dump_enop_metadata_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

int mtk_debug_dump_enop_metadata_init(struct kbase_device *kbdev)
{
	kbdev->mtk_enop_metadata_dump_workq =
		alloc_workqueue("mtk_enop_metadata_dump_workq", WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (kbdev->mtk_enop_metadata_dump_workq == NULL)
		dev_info(kbdev->dev, "@%s: mtk_enop_metadata_dump_workq init failed", __func__);

	INIT_WORK(&kbdev->mtk_enop_metadata_dump_work, mtk_enop_metadata_dump_worker);
	return 0;
}
