// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2024 ARM Limited. All rights reserved.
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
#include <mali_kbase_ctx_sched.h>
#include <hwcnt/mali_kbase_hwcnt_context.h>
#include <device/mali_kbase_device.h>
#include <backend/gpu/mali_kbase_irq_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <mali_kbase_regs_history_debugfs.h>
#include <csf/mali_kbase_csf_trace_buffer.h>
#include <csf/ipa_control/mali_kbase_csf_ipa_control.h>
#include <mali_kbase_reset_gpu.h>
#include <csf/mali_kbase_csf_firmware_log.h>

#if IS_ENABLED(CONFIG_MALI_MTK_GPUEB_IRQ)
#include <gpueb_ipi.h>
#endif /* CONFIG_MALI_MTK_GPUEB_IRQ */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#include <platform/mtk_platform_common.h>
#include <hw_access/mali_kbase_hw_access_regmap_legacy.h>
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
#include <ged_mali_event.h>
#include <platform/mtk_platform_common/mtk_platform_mali_event.h>
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */

enum kbasep_soft_reset_status {
	RESET_SUCCESS = 0,
	SOFT_RESET_FAILED,
	L2_ON_FAILED,
	MCU_REINIT_FAILED
};

static inline bool kbase_csf_reset_state_is_silent(enum kbase_csf_reset_gpu_state state)
{
	return (state == KBASE_CSF_RESET_GPU_COMMITTED_SILENT);
}

static inline bool kbase_csf_reset_state_is_committed(enum kbase_csf_reset_gpu_state state)
{
	return (state == KBASE_CSF_RESET_GPU_COMMITTED ||
		state == KBASE_CSF_RESET_GPU_COMMITTED_SILENT);
}

static inline bool kbase_csf_reset_state_is_active(enum kbase_csf_reset_gpu_state state)
{
	return (state == KBASE_CSF_RESET_GPU_HAPPENING);
}

/**
 * DOC: Mechanism for coherent access to the HW with respect to GPU reset
 *
 * Access to the HW from non-atomic context outside of the reset thread must
 * use kbase_reset_gpu_prevent_and_wait() / kbase_reset_gpu_try_prevent().
 *
 * This currently works by taking the &kbase_device's csf.reset.sem, for
 * 'write' access by the GPU reset thread and 'read' access by every other
 * thread. The use of this rw_semaphore means:
 *
 * - there will be mutual exclusion (and thus waiting) between the thread doing
 *   reset ('writer') and threads trying to access the GPU for 'normal'
 *   operations ('readers')
 *
 * - multiple threads may prevent reset from happening without serializing each
 *   other prematurely. Note that at present the wait for reset to finish has
 *   to be done higher up in the driver than actual GPU access, at a point
 *   where it won't cause lock ordering issues. At such a point, some paths may
 *   actually lead to no GPU access, but we would prefer to avoid serializing
 *   at that level
 *
 * - lockdep (if enabled in the kernel) will check such uses for deadlock
 *
 * If instead &kbase_device's csf.reset.wait &wait_queue_head_t were used on
 * its own, we'd also need to add a &lockdep_map and appropriate lockdep calls
 * to make use of lockdep checking in all places where the &wait_queue_head_t
 * is waited upon or signaled.
 *
 * Indeed places where we wait on &kbase_device's csf.reset.wait (such as
 * kbase_reset_gpu_wait()) are the only places where we need extra call(s) to
 * lockdep, and they are made on the existing rw_semaphore.
 *
 * For non-atomic access, the &kbase_device's csf.reset.state member should be
 * checked instead, such as by using kbase_reset_gpu_is_active().
 *
 * Ideally the &rw_semaphore should be replaced in future with a single mutex
 * that protects any access to the GPU, via reset or otherwise.
 */

int kbase_reset_gpu_prevent_and_wait(struct kbase_device *kbdev)
{
	down_read(&kbdev->csf.reset.sem);

	if (atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_FAILED) {
		up_read(&kbdev->csf.reset.sem);
		return -ENOMEM;
	}

	if (WARN_ON(kbase_reset_gpu_is_active(kbdev))) {
		up_read(&kbdev->csf.reset.sem);
		return -EFAULT;
	}

	return 0;
}
KBASE_EXPORT_TEST_API(kbase_reset_gpu_prevent_and_wait);

int kbase_reset_gpu_try_prevent(struct kbase_device *kbdev)
{
	if (!down_read_trylock(&kbdev->csf.reset.sem))
		return -EAGAIN;

	if (atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_FAILED) {
		up_read(&kbdev->csf.reset.sem);
		return -ENOMEM;
	}

	if (WARN_ON(kbase_reset_gpu_is_active(kbdev))) {
		up_read(&kbdev->csf.reset.sem);
		return -EFAULT;
	}

	return 0;
}

void kbase_reset_gpu_allow(struct kbase_device *kbdev)
{
	up_read(&kbdev->csf.reset.sem);
}
KBASE_EXPORT_TEST_API(kbase_reset_gpu_allow);

void kbase_reset_gpu_assert_prevented(struct kbase_device *kbdev)
{
#if KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE
	lockdep_assert_held_read(&kbdev->csf.reset.sem);
#else
	lockdep_assert_held(&kbdev->csf.reset.sem);
#endif
	WARN_ON(kbase_reset_gpu_is_active(kbdev));
}

void kbase_reset_gpu_assert_failed_or_prevented(struct kbase_device *kbdev)
{
	if (atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_FAILED)
		return;

#if KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE
	lockdep_assert_held_read(&kbdev->csf.reset.sem);
#else
	lockdep_assert_held(&kbdev->csf.reset.sem);
#endif
	WARN_ON(kbase_reset_gpu_is_active(kbdev));
}

/* Mark the reset as now happening, and synchronize with other threads that
 * might be trying to access the GPU
 */
static void kbase_csf_reset_begin_hw_access_sync(struct kbase_device *kbdev,
						 enum kbase_csf_reset_gpu_state initial_reset_state)
{
	unsigned long hwaccess_lock_flags;
	unsigned long scheduler_spin_lock_flags;

	/* Note this is a WARN/atomic_set because it is a software issue for a
	 * race to be occurring here
	 */
	WARN_ON(!kbase_csf_reset_state_is_committed(initial_reset_state));

	down_write(&kbdev->csf.reset.sem);

	/* Threads in atomic context accessing the HW will hold one of these
	 * locks, so synchronize with them too.
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, hwaccess_lock_flags);
	kbase_csf_scheduler_spin_lock(kbdev, &scheduler_spin_lock_flags);
	atomic_set(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_HAPPENING);
	kbase_csf_scheduler_spin_unlock(kbdev, scheduler_spin_lock_flags);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, hwaccess_lock_flags);
}

/* Mark the reset as finished and allow others threads to once more access the
 * GPU
 */
static void kbase_csf_reset_end_hw_access(struct kbase_device *kbdev, int err_during_reset,
					  bool firmware_inited)
{
	unsigned long hwaccess_lock_flags;
	unsigned long scheduler_spin_lock_flags;

	WARN_ON(!kbase_csf_reset_state_is_active(atomic_read(&kbdev->csf.reset.state)));

	/* Once again, we synchronize with atomic context threads accessing the
	 * HW, as otherwise any actions they defer could get lost
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, hwaccess_lock_flags);
	kbase_csf_scheduler_spin_lock(kbdev, &scheduler_spin_lock_flags);

#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	kbdev->is_reset_triggered_by_fence_timeout = false;
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

	if (!err_during_reset) {
		atomic_set(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_NOT_PENDING);
	} else {
		dev_err(kbdev->dev, "Reset failed to complete");
		atomic_set(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_FAILED);
#if IS_ENABLED(CONFIG_MALI_MTK_TRIGGER_KE)
		if (kbdev->exception_mask & (1u << EXCEPTION_RESET_FAILED))
			BUG_ON(1);
#endif /* CONFIG_MALI_MTK_TRIGGER_KE */
	}

	kbase_csf_scheduler_spin_unlock(kbdev, scheduler_spin_lock_flags);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, hwaccess_lock_flags);

	/* Invoke the scheduling tick after formally finishing the reset,
	 * otherwise the tick might start too soon and notice that reset
	 * is still in progress.
	 */
	up_write(&kbdev->csf.reset.sem);
	wake_up(&kbdev->csf.reset.wait);

	if (!err_during_reset && likely(firmware_inited))
		kbase_csf_scheduler_enable_tick_timer(kbdev);
}

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
static bool kbase_is_register_accessible(u32 offset)
{
#ifdef CONFIG_MALI_DEBUG
	if (((offset >= MCU_SUBSYSTEM_BASE) && (offset < IPA_CONTROL_BASE)) ||
	    ((offset >= GPU_CONTROL_MCU_BASE) && (offset < USER_BASE))) {
		WARN(1, "Invalid register offset 0x%x", offset);
		return false;
	}
#endif

	return true;
}

static u32 kbase_reg_read_directly(struct kbase_device *kbdev, u32 offset)
{
	u32 val;

	if (WARN_ON(!kbdev->pm.backend.gpu_powered))
		return 0;

	if (WARN_ON(kbdev->dev == NULL))
		return 0;

	if (!kbase_is_register_accessible(offset))
		return 0;

	val = readl(kbdev->reg + offset);

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (unlikely(kbdev->io_history.enabled))
		kbase_io_history_add(&kbdev->io_history, kbdev->reg + offset,
				     val, 0);
#endif /* CONFIG_DEBUG_FS */
	dev_dbg(kbdev->dev, "r: reg %08x val %08x", offset, val);

	return val;
}

#define DOORBELL_CFG_BASE 0x20000
#define MCUC_DB_VALUE_0 0x80

const char *kbase_mcu_state_to_string(enum kbase_mcu_state state);
const char *kbase_l2_core_state_to_string(enum kbase_l2_core_state state);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
void kbase_csf_debug_dump_registers(struct kbase_device *kbdev)
#else
static void kbase_csf_debug_dump_registers(struct kbase_device *kbdev)
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
{
	unsigned long flags;
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	struct kbase_csf_global_iface *global_iface = &kbdev->csf.global_iface;
	u32 glb_req, glb_ack, glb_db_req, glb_db_ack;
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

	kbase_io_history_dump(kbdev);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	if (kbdev->is_reset_triggered_by_fence_timeout) {
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"\tMCU desired = %d\n",
			kbase_pm_is_mcu_desired(kbdev));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"\tMCU sw state = %d(%s)\n",
			kbdev->pm.backend.mcu_state,
			kbase_mcu_state_to_string(kbdev->pm.backend.mcu_state));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"\tL2 sw state = %d(%s)\n",
			kbdev->pm.backend.l2_state,
			kbase_l2_core_state_to_string(kbdev->pm.backend.l2_state));
	} else {
		dev_err(kbdev->dev, "\tMCU desired = %d\n",
				kbase_pm_is_mcu_desired(kbdev));
		dev_err(kbdev->dev, "\tMCU sw state = %d(%s)\n",
				kbdev->pm.backend.mcu_state,
				kbase_mcu_state_to_string(kbdev->pm.backend.mcu_state));
		dev_err(kbdev->dev, "\tL2 sw state = %d(%s)\n",
				kbdev->pm.backend.l2_state,
				kbase_l2_core_state_to_string(kbdev->pm.backend.l2_state));
	}
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
	dev_err(kbdev->dev, "\tMCU desired = %d\n",
			kbase_pm_is_mcu_desired(kbdev));
	dev_err(kbdev->dev, "\tMCU sw state = %d(%s)\n",
			kbdev->pm.backend.mcu_state,
			kbase_mcu_state_to_string(kbdev->pm.backend.mcu_state));
	dev_err(kbdev->dev, "\tL2 sw state = %d(%s)\n",
			kbdev->pm.backend.l2_state,
			kbase_l2_core_state_to_string(kbdev->pm.backend.l2_state));
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	if (kbdev->is_reset_triggered_by_fence_timeout) {
		goto deferred_dump;
	} else {
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
	dev_err(kbdev->dev, "Register state:");
	dev_err(kbdev->dev, "  GPU_IRQ_RAWSTAT=0x%08x  GPU_STATUS=0x%08x MCU_STATUS=0x%08x",
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_STATUS)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(MCU_STATUS)));
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	dev_err(kbdev->dev, "\tMCU control = %d\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(MCU_CONTROL)));
	dev_err(kbdev->dev, "\tMCUC_DB_VALUE_0 = %d\n",
			kbase_reg_read_directly(kbdev, DOORBELL_CFG_BASE + MCUC_DB_VALUE_0));
	dev_err(kbdev->dev, "\tGPU_IRQ_MASK = %x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)));
	dev_err(kbdev->dev, "\tGPU_IRQ_RAWSTAT = %x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)));
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
	dev_err(kbdev->dev,
		"  JOB_IRQ_RAWSTAT=0x%08x  MMU_IRQ_RAWSTAT=0x%08x  GPU_FAULTSTATUS=0x%08x",
		kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_FAULTSTATUS)));
	dev_err(kbdev->dev, "  GPU_IRQ_MASK=0x%08x  JOB_IRQ_MASK=0x%08x  MMU_IRQ_MASK=0x%08x",
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)),
		kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_MASK)),
		kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK)));
	if (kbdev->gpu_props.gpu_id.arch_id < GPU_ID_ARCH_MAKE(14, 10, 0)) {
		dev_err(kbdev->dev, "  PWR_OVERRIDE0=0x%08x  PWR_OVERRIDE1=0x%08x",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE0)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE1)));
		dev_err(kbdev->dev,
			"  SHADER_CONFIG=0x%08x  L2_MMU_CONFIG=0x%08x  TILER_CONFIG=0x%08x",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(SHADER_CONFIG)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(L2_MMU_CONFIG)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(TILER_CONFIG)));
	}
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	glb_db_ack = kbase_csf_firmware_global_output(global_iface, GLB_DB_ACK);
	glb_db_req = kbase_csf_firmware_global_input_read(global_iface, GLB_DB_REQ);
	glb_ack = kbase_csf_firmware_global_output(global_iface, GLB_ACK);
	glb_req = kbase_csf_firmware_global_input_read(global_iface, GLB_REQ);
	dev_err(kbdev->dev, "\tglb_req %x glb_ack %x glb_db_req %x glb_db_ack %x\n",
			glb_req, glb_ack, glb_db_req, glb_db_ack);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	}
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
deferred_dump:
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	if (kbdev->is_reset_triggered_by_fence_timeout) {
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"Register state:\n");
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"  GPU_IRQ_RAWSTAT=0x%08x  GPU_STATUS=0x%08x MCU_STATUS=0x%08x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_STATUS)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(MCU_STATUS)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"  JOB_IRQ_RAWSTAT=0x%08x  MMU_IRQ_RAWSTAT=0x%08x  GPU_FAULTSTATUS=0x%08x\n",
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_FAULTSTATUS)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"  GPU_IRQ_MASK=0x%08x  JOB_IRQ_MASK=0x%08x  MMU_IRQ_MASK=0x%08x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)),
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_MASK)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK)));
		if (kbdev->gpu_props.gpu_id.arch_id < GPU_ID_ARCH_MAKE(14, 10, 0)) {
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"  PWR_OVERRIDE0=0x%08x  PWR_OVERRIDE1=0x%08x\n",
				kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE0)),
				kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE1)));
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"  SHADER_CONFIG=0x%08x  L2_MMU_CONFIG=0x%08x  TILER_CONFIG=0x%08x\n",
				kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(SHADER_CONFIG)),
				kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(L2_MMU_CONFIG)),
				kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(TILER_CONFIG)));
		}
	}
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
		"Register state:\n");
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
		"  GPU_IRQ_RAWSTAT=0x%08x  GPU_STATUS=0x%08x MCU_STATUS=0x%08x\n",
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_STATUS)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(MCU_STATUS)));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
		"  JOB_IRQ_RAWSTAT=0x%08x  MMU_IRQ_RAWSTAT=0x%08x  GPU_FAULTSTATUS=0x%08x\n",
		kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_RAWSTAT)),
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_FAULTSTATUS)));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
		"  GPU_IRQ_MASK=0x%08x  JOB_IRQ_MASK=0x%08x  MMU_IRQ_MASK=0x%08x\n",
		kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)),
		kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_MASK)),
		kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK)));
	if (kbdev->gpu_props.gpu_id.arch_id < GPU_ID_ARCH_MAKE(14, 10, 0)) {
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
			"  PWR_OVERRIDE0=0x%08x  PWR_OVERRIDE1=0x%08x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE0)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE1)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL,
			"  SHADER_CONFIG=0x%08x  L2_MMU_CONFIG=0x%08x  TILER_CONFIG=0x%08x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(SHADER_CONFIG)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(L2_MMU_CONFIG)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(TILER_CONFIG)));
	}
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
}

/**
 * kbase_csf_hwcnt_on_reset_error() - Sets HWCNT to appropriate state in the
 *                                    event of an error during GPU reset.
 * @kbdev: Pointer to KBase device
 */
static void kbase_csf_hwcnt_on_reset_error(struct kbase_device *kbdev)
{
	unsigned long flags;

	/* Treat this as an unrecoverable error for HWCNT */
	kbase_hwcnt_backend_csf_on_unrecoverable_error(&kbdev->hwcnt_gpu_iface);

	/* Re-enable counters to ensure matching enable/disable pair.
	 * This might reduce the hwcnt disable count to 0, and therefore
	 * trigger actual re-enabling of hwcnt.
	 * However, as the backend is now in the unrecoverable error state,
	 * re-enabling will immediately fail and put the context into the error
	 * state, preventing the hardware from being touched (which could have
	 * risked a hang).
	 */
	kbase_csf_scheduler_spin_lock(kbdev, &flags);
	kbase_hwcnt_context_enable(kbdev->hwcnt_gpu_ctx);
	kbase_csf_scheduler_spin_unlock(kbdev, flags);
}

static enum kbasep_soft_reset_status kbase_csf_reset_gpu_once(struct kbase_device *kbdev,
							      bool firmware_inited, bool silent)
{
	unsigned long flags;
	int err;
	enum kbasep_soft_reset_status ret = RESET_SUCCESS;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	spin_lock(&kbdev->mmu_mask_change);
	kbase_pm_reset_start_locked(kbdev);

	dev_dbg(kbdev->dev, "We're about to flush out the IRQs and their bottom halves\n");
	kbdev->irq_reset_flush = true;

	/* Disable IRQ to avoid IRQ handlers to kick in after releasing the
	 * spinlock; this also clears any outstanding interrupts
	 */
	kbase_pm_disable_interrupts_nolock(kbdev);

	spin_unlock(&kbdev->mmu_mask_change);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	dev_dbg(kbdev->dev, "Ensure that any IRQ handlers have finished\n");
	/* Must be done without any locks IRQ handlers will take. */
	kbase_synchronize_irqs(kbdev);

	dev_dbg(kbdev->dev, "Flush out any in-flight work items\n");
	kbase_flush_mmu_wqs(kbdev);

	dev_dbg(kbdev->dev, "The flush has completed so reset the active indicator\n");
	kbdev->irq_reset_flush = false;

	if (!silent) {
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
		if (kbdev->is_reset_triggered_by_fence_timeout)
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"Resetting GPU (allowing up to %d ms)\n", RESET_TIMEOUT);
		else
			dev_err(kbdev->dev, "Resetting GPU (allowing up to %d ms)", RESET_TIMEOUT);
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
		dev_err(kbdev->dev, "Resetting GPU (allowing up to %d ms)", RESET_TIMEOUT);
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Resetting GPU (allowing up to %d ms)\n", RESET_TIMEOUT);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

	/* Output the state of some interesting registers to help in the
	 * debugging of GPU resets, and dump the firmware trace buffer
	 */
	if (!silent) {
		kbase_csf_debug_dump_registers(kbdev);
		if (likely(firmware_inited))
			kbase_csf_firmware_log_dump_buffer(kbdev);
	}

	{
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		kbase_ipa_control_handle_gpu_reset_pre(kbdev);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	}

	/* Tell hardware counters a reset is about to occur.
	 * If the backend is in an unrecoverable error state (e.g. due to
	 * firmware being unresponsive) this will transition the backend out of
	 * it, on the assumption a reset will fix whatever problem there was.
	 */
	kbase_hwcnt_backend_csf_on_before_reset(&kbdev->hwcnt_gpu_iface);

	mutex_lock(&kbdev->pm.lock);
	/* Reset the GPU */
	err = kbase_pm_init_hw(kbdev, 0);

	mutex_unlock(&kbdev->pm.lock);

#if IS_ENABLED(CONFIG_MALI_MTK_RESET_RELOAD_ON_FW)
	if (kbdev->pm.backend.fw_reload_on_reset_worker == true)
	{
		kbdev->csf.firmware_reload_needed = false;
		kbase_csf_firmware_reload(kbdev);
		dev_info(kbdev->dev, "Reload fw on reset worker.");
	}
#endif
	if (WARN_ON(err)) {
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"kbase_pm_init_hw failed err=%d\n", err);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, NULL, MTK_DBG_HOOK_RESET_FAIL);
		mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, NULL, MTK_DBG_HOOK_RESET_FAIL);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
		return SOFT_RESET_FAILED;
	}

	mutex_lock(&kbdev->mmu_hw_mutex);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_ctx_sched_restore_all_as(kbdev);
	{
		kbase_ipa_control_handle_gpu_reset_post(kbdev);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->mmu_hw_mutex);

	kbase_pm_enable_interrupts(kbdev);

	mutex_lock(&kbdev->pm.lock);
	kbase_pm_reset_complete(kbdev);
	/* Synchronously wait for the reload of firmware to complete */
	err = kbase_pm_wait_for_desired_state(kbdev);
	mutex_unlock(&kbdev->pm.lock);

	if (err) {
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		if (!kbase_pm_l2_is_in_desired_state(kbdev))
			ret = L2_ON_FAILED;
		else if (!kbase_pm_mcu_is_in_desired_state(kbdev))
			ret = MCU_REINIT_FAILED;
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"kbase_pm_wait_for_desired_state failed err=%d\n", err);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, NULL, MTK_DBG_HOOK_FWRELOAD_FAIL);
		mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, NULL, MTK_DBG_HOOK_FWRELOAD_FAIL);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
	}

	return ret;
}

static int kbase_csf_reset_gpu_now(struct kbase_device *kbdev, bool firmware_inited, bool silent)
{
	unsigned long flags;
	enum kbasep_soft_reset_status ret;

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, NULL, MTK_DBG_HOOK_RESET);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */
	WARN_ON(kbdev->irq_reset_flush);
	/* The reset must now be happening otherwise other threads will not
	 * have been synchronized with to stop their access to the HW
	 */
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE
	lockdep_assert_held_write(&kbdev->csf.reset.sem);
#elif KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE
	lockdep_assert_held_exclusive(&kbdev->csf.reset.sem);
#else
	lockdep_assert_held(&kbdev->csf.reset.sem);
#endif
	WARN_ON(!kbase_reset_gpu_is_active(kbdev));

	/* Reset the scheduler state before disabling the interrupts as suspend
	 * of active CSG slots would also be done as a part of reset.
	 */
	if (likely(firmware_inited))
		kbase_csf_scheduler_reset(kbdev);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbdev->csf.firmware_reload_needed = false;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	cancel_work_sync(&kbdev->csf.firmware_reload_work);

	dev_dbg(kbdev->dev, "Disable GPU hardware counters.\n");
	/* This call will block until counters are disabled. */
	kbase_hwcnt_context_disable(kbdev->hwcnt_gpu_ctx);
#if IS_ENABLED(CONFIG_MALI_MTK_RESET_RELOAD_ON_FW)
	kbdev->pm.backend.fw_reload_on_reset_worker = false;
#endif
	ret = kbase_csf_reset_gpu_once(kbdev, firmware_inited, silent);

	if (ret == SOFT_RESET_FAILED) {
		dev_err(kbdev->dev, "Soft-reset failed");
		goto err;
	} else if (ret == L2_ON_FAILED) {
		dev_err(kbdev->dev, "[1]L2 power up failed after the soft-reset try again[%d].",ret);
		ret = kbase_csf_reset_gpu_once(kbdev, firmware_inited, true);
		if (ret != RESET_SUCCESS) {
			dev_err(kbdev->dev, "[2]L2 power up failed after the soft-reset[%d].",ret);
			goto err;
		}
	} else if (ret == MCU_REINIT_FAILED) {
#if IS_ENABLED(CONFIG_MALI_MTK_RESET_RELOAD_ON_FW)
		dev_err(kbdev->dev, "[1]MCU re-init failed, trying reload firmware on reset worker[%d]",ret);
		cancel_work_sync(&kbdev->csf.firmware_reload_work);
		kbdev->pm.backend.fw_reload_on_reset_worker = true;
		ret = kbase_csf_reset_gpu_once(kbdev, firmware_inited, true);
		dev_err(kbdev->dev,"Reload fw on reset worker ret = [%d]",ret);
		if (ret != RESET_SUCCESS) {
			dev_err(kbdev->dev, "[2]MCU re-init failed trying full firmware reload[%d]",ret);
			/* Since MCU reinit failed despite successful soft reset, we can try
			* the firmware full reload.
			*/
			kbdev->csf.firmware_full_reload_needed = true;
			kbdev->pm.backend.fw_reload_on_reset_worker = false;
			ret = kbase_csf_reset_gpu_once(kbdev, firmware_inited, true);
			if (ret != RESET_SUCCESS) {
				kbdev->csf.firmware_full_reload_needed = false;
				dev_err(kbdev->dev,
					"[3]MCU Re-init failed even after trying full firmware reload, ret = [%d]",
					ret);
				goto err;
			}
		}
#else
		dev_err(kbdev->dev, "MCU re-init failed trying full firmware reload");
		/* Since MCU reinit failed despite successful soft reset, we can try
		 * the firmware full reload.
		 */
		kbdev->csf.firmware_full_reload_needed = true;
		ret = kbase_csf_reset_gpu_once(kbdev, firmware_inited, true);
		if (ret != RESET_SUCCESS) {
			dev_err(kbdev->dev,
				"MCU Re-init failed even after trying full firmware reload, ret = [%d]",
				ret);
			goto err;
		}
#endif
	}

	/* Re-enable GPU hardware counters */
	kbase_csf_scheduler_spin_lock(kbdev, &flags);
	kbase_hwcnt_context_enable(kbdev->hwcnt_gpu_ctx);
	kbase_csf_scheduler_spin_unlock(kbdev, flags);
	if (!silent) {
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
		if (kbdev->is_reset_triggered_by_fence_timeout)
			mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
				"Reset complete\n");
		else
			dev_err(kbdev->dev, "Reset complete");
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
		dev_err(kbdev->dev, "Reset complete");
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
		ged_mali_event_notify_gpu_reset_done();
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Reset complete\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_REDUCE)
	/* set the fw timeout back to orignal */
	kbdev->csf.csg_term_timeout_ms =
                kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);

	/* set the csg suspend timeout back to orignal */
	//kbdev->csf.csg_suspend_timeout_ms =
    //            kbase_get_timeout_ms(kbdev, CSF_CSG_SUSPEND_TIMEOUT);
#endif /* CONFIG_MALI_MTK_TIMEOUT_REDUCE */

	return 0;
err:

	kbase_csf_hwcnt_on_reset_error(kbdev);
	return -1;
}

static void kbase_csf_reset_gpu_worker(struct work_struct *data)
{
	struct kbase_device *kbdev = container_of(data, struct kbase_device, csf.reset.work);
	bool gpu_sleep_mode_active = false;
	bool firmware_inited;
	unsigned long flags;
	int err = 0;
	const enum kbase_csf_reset_gpu_state initial_reset_state =
		atomic_read(&kbdev->csf.reset.state);
	const bool silent = kbase_csf_reset_state_is_silent(initial_reset_state);

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_REDUCE)
	/* set fw timeout to 0.5s after GPU reset */
	kbdev->csf.csg_term_timeout_ms =
		kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT_AFTER_ABNORMAL_TIMEOUT);
	/* set fw timeout to 1.0s after GPU reset */
	kbdev->csf.csg_suspend_timeout_ms =
		kbase_get_timeout_ms(kbdev, CSF_CSG_SUSPEND_TIMEOUT_AFTER_ABNORMAL_TIMEOUT);
#endif /* CONFIG_MALI_MTK_TIMEOUT_REDUCE */

	/* Ensure any threads (e.g. executing the CSF scheduler) have finished
	 * using the HW
	 */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Reset GPU Worker, start\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	kbase_csf_reset_begin_hw_access_sync(kbdev, initial_reset_state);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Reset GPU Worker, hw access sync done\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	firmware_inited = kbdev->csf.firmware_inited;
#ifdef KBASE_PM_RUNTIME
	gpu_sleep_mode_active = kbdev->pm.backend.gpu_sleep_mode_active;
#endif
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Reset GPU Worker, gpu sleep mode status updated\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	if (unlikely(gpu_sleep_mode_active)) {
#ifdef KBASE_PM_RUNTIME
		/* As prior to GPU reset all on-slot groups are suspended,
		 * need to wake up the MCU from sleep.
		 * No pm active reference is taken here since GPU is in sleep
		 * state and both runtime & system suspend synchronize with the
		 * GPU reset before they wake up the GPU to suspend on-slot
		 * groups. GPUCORE-29850 would add the proper handling.
		 */
		kbase_pm_lock(kbdev);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Reset GPU Worker, pm locked\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		if (kbase_pm_force_mcu_wakeup_after_sleep(kbdev))
			dev_warn(kbdev->dev, "Wait for MCU wake up failed on GPU reset");
		kbase_pm_unlock(kbdev);

		err = kbase_csf_reset_gpu_now(kbdev, firmware_inited, silent);
#endif
	} else if (!kbase_pm_context_active_handle_suspend(
			   kbdev, KBASE_PM_SUSPEND_HANDLER_DONT_REACTIVATE)) {
		err = kbase_csf_reset_gpu_now(kbdev, firmware_inited, silent);
		kbase_pm_context_idle(kbdev);
	}

	kbase_disjoint_state_down(kbdev);

	/* Allow other threads to once again use the GPU */
	kbase_csf_reset_end_hw_access(kbdev, err, firmware_inited);
}

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
char *gpu_reset_entry_name[] = {
	// error event entry
	"kbasep_ioctl_internal_fence_wait", //0
	"kcpu_fence_timeout_dump", //1
	"wait_mcu_as_inactive",//2
	"kbase_pm_timed_out",//3
	"handle_internal_firmware_fatal",//4
	"kbase_csf_wait_protected_mode_enter",//5
	"scheduler_force_protm_exit",//6
	"halt_stream_sync",//7
	"remove_group_from_runnable",//8
	"term_group_sync",//9
	"program_suspending_csg_slots",//10
	"wait_csg_slots_start",//11
	"wait_csg_slots_finish_prio_update",//12
	"suspend_active_groups_on_powerdown",//13
	"firmware_aliveness_monitor",//14
	"handle_fatal_event",//15
	"kbase_gpu_fault_interrupt",//16
	"kbase_gpu_interrupt",//17
	"kbase_mmu_report_mcu_as_fault_and_reset",//18
	"kbase_gpueb_irq_handler",//19
	"wait_ready",//20
	"busy_wait_cache_operation",//21
	"kbase_gpu_wait_cache_clean_timeout",//22
	"apply_hw_issue_GPU2019_3901_wa",//23

	// nromal event entry
	//"trigger_reset",
	//"kbase_pm_set_policy",
	//"int_id_overrides_write",
	//"propagate_bits_write",
};
unsigned int gpu_reset_entry_size = sizeof(gpu_reset_entry_name)/sizeof(gpu_reset_entry_name[0]);
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
bool __kbase_prepare_to_reset_gpu(struct kbase_device *kbdev, unsigned int flags)
#else
bool kbase_prepare_to_reset_gpu(struct kbase_device *kbdev, unsigned int flags)
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */
{
#ifdef CONFIG_MALI_ARBITER_SUPPORT
	if (kbase_pm_is_gpu_lost(kbdev)) {
		/* GPU access has been removed, reset will be done by Arbiter instead */
		return false;
	}
#endif

	if (flags & RESET_FLAGS_HWC_UNRECOVERABLE_ERROR)
		kbase_hwcnt_backend_csf_on_unrecoverable_error(&kbdev->hwcnt_gpu_iface);

	if (atomic_cmpxchg(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_NOT_PENDING,
			   KBASE_CSF_RESET_GPU_PREPARED) != KBASE_CSF_RESET_GPU_NOT_PENDING)
		/* Some other thread is already resetting the GPU */
		return false;

	/* Issue the wake up of threads waiting for PM state transition.
	 * They might want to exit the wait since GPU reset has been triggered.
	 */
	wake_up(&kbdev->pm.backend.gpu_in_desired_state_wait);
	return true;
}
#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
KBASE_EXPORT_TEST_API(__kbase_prepare_to_reset_gpu);
#else
KBASE_EXPORT_TEST_API(kbase_prepare_to_reset_gpu);
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
bool kbase_prepare_to_reset_gpu_ext(struct kbase_device *kbdev, unsigned int flags, const char* file, const char* func)
{
	int idx = 0;
	bool need_reset_flag = false;

	need_reset_flag = __kbase_prepare_to_reset_gpu(kbdev, flags);

	if (need_reset_flag && kbdev->reset_exception_mask != 0) {
		dev_err(kbdev->dev, "gpu reset entry: %s,%s", file, func);
		for (idx = 0; idx < gpu_reset_entry_size; idx++) {
			if (kbdev->reset_exception_mask & (1u << idx)) {
				if (strncmp(func, gpu_reset_entry_name[idx], strlen(func)) == 0) {
					// Add debug dump here
					mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, NULL, MTK_DBG_HOOK_NA);
					mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, NULL, MTK_DBG_HOOK_NA);
					mtk_common_debug(MTK_COMMON_DBG_CSF_DUMP_ITER_HWIF, NULL, MTK_DBG_HOOK_NA);
					kbase_csf_debug_dump_registers(kbdev);
					kbase_csf_firmware_log_dump_buffer(kbdev);
					BUG_ON(1);
				}
			}
		}
	}

	return need_reset_flag;
}
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
bool __kbase_prepare_to_reset_gpu_locked(struct kbase_device *kbdev, unsigned int flags)
#else
bool kbase_prepare_to_reset_gpu_locked(struct kbase_device *kbdev, unsigned int flags)
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	return kbase_prepare_to_reset_gpu(kbdev, flags);
}


#if IS_ENABLED(CONFIG_MALI_MTK_GPU_RESET_DEBUG)
bool kbase_prepare_to_reset_gpu_ext_locked(struct kbase_device *kbdev, unsigned int flags, const char* file, const char* func)
{
	int idx = 0;
	bool need_reset_flag = false;
	lockdep_assert_held(&kbdev->hwaccess_lock);

	need_reset_flag = __kbase_prepare_to_reset_gpu_locked(kbdev, flags);

	if (need_reset_flag && kbdev->reset_exception_mask != 0) {
		dev_err(kbdev->dev, "gpu reset entry: %s,%s", file, func);
		for (idx = 0; idx < gpu_reset_entry_size; idx++) {
			if (kbdev->reset_exception_mask & (1u << idx)) {
				if (strncmp(func, gpu_reset_entry_name[idx], strlen(func)) == 0) {
					// Add debug dump here
					mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, NULL, MTK_DBG_HOOK_NA);
					mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, NULL, MTK_DBG_HOOK_NA);
					mtk_common_debug(MTK_COMMON_DBG_CSF_DUMP_ITER_HWIF_LOCKED, NULL, MTK_DBG_HOOK_NA);
					kbase_csf_firmware_log_dump_buffer(kbdev);
					BUG_ON(1);
				}
			}
		}
	}

	return need_reset_flag;
}
#endif /* CONFIG_MALI_MTK_GPU_RESET_DEBUG */

void kbase_reset_gpu(struct kbase_device *kbdev)
{
	/* Note this is a WARN/atomic_set because it is a software issue for
	 * a race to be occurring here
	 */
	if (WARN_ON(atomic_read(&kbdev->csf.reset.state) != KBASE_RESET_GPU_PREPARED))
		return;

	atomic_set(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_COMMITTED);
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
	if (kbdev->is_reset_triggered_by_fence_timeout)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_DEFERRED,
			"Preparing to soft-reset GPU\n");
	else
		dev_err(kbdev->dev, "Preparing to soft-reset GPU\n");
#else /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
	dev_err(kbdev->dev, "Preparing to soft-reset GPU\n");
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Preparing to soft-reset GPU\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	kbase_disjoint_state_up(kbdev);

	queue_work(kbdev->csf.reset.workq, &kbdev->csf.reset.work);
}
KBASE_EXPORT_TEST_API(kbase_reset_gpu);

void kbase_reset_gpu_locked(struct kbase_device *kbdev)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	kbase_reset_gpu(kbdev);
}

int kbase_reset_gpu_silent(struct kbase_device *kbdev)
{
	if (atomic_cmpxchg(&kbdev->csf.reset.state, KBASE_CSF_RESET_GPU_NOT_PENDING,
			   KBASE_CSF_RESET_GPU_COMMITTED_SILENT) !=
	    KBASE_CSF_RESET_GPU_NOT_PENDING) {
		/* Some other thread is already resetting the GPU */
		return -EAGAIN;
	}

	kbase_disjoint_state_up(kbdev);

	queue_work(kbdev->csf.reset.workq, &kbdev->csf.reset.work);

	return 0;
}
KBASE_EXPORT_TEST_API(kbase_reset_gpu_silent);

bool kbase_reset_gpu_is_active(struct kbase_device *kbdev)
{
	enum kbase_csf_reset_gpu_state reset_state = atomic_read(&kbdev->csf.reset.state);

	/* For CSF, the reset is considered active only when the reset worker
	 * is actually executing and other threads would have to wait for it to
	 * complete
	 */
	return kbase_csf_reset_state_is_active(reset_state);
}

bool kbase_reset_gpu_is_not_pending(struct kbase_device *kbdev)
{
	return atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_NOT_PENDING;
}

int kbase_reset_gpu_wait(struct kbase_device *kbdev)
{
	const long wait_timeout =
		kbase_csf_timeout_in_jiffies(kbase_get_timeout_ms(kbdev, CSF_GPU_RESET_TIMEOUT));
	long remaining;

	/* Inform lockdep we might be trying to wait on a reset (as
	 * would've been done with down_read() - which has no 'timeout'
	 * variant), then use wait_event_timeout() to implement the timed
	 * wait.
	 *
	 * in CONFIG_PROVE_LOCKING builds, this should catch potential 'time
	 * bound' deadlocks such as:
	 * - incorrect lock order with respect to others locks
	 * - current thread has prevented reset
	 * - current thread is executing the reset worker
	 */
	might_lock_read(&kbdev->csf.reset.sem);

	remaining = wait_event_timeout(
		kbdev->csf.reset.wait,
		(atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_NOT_PENDING) ||
			(atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_FAILED),
		wait_timeout);

	if (!remaining) {
		dev_warn(kbdev->dev, "Timed out waiting for the GPU reset to complete");


		return -ETIMEDOUT;
	} else if (atomic_read(&kbdev->csf.reset.state) == KBASE_CSF_RESET_GPU_FAILED) {
		return -ENOMEM;
	}

	return 0;
}
KBASE_EXPORT_TEST_API(kbase_reset_gpu_wait);

#if IS_ENABLED(CONFIG_MALI_MTK_GPUEB_IRQ)
static irqreturn_t kbase_gpueb_irq_handler(int irq, void *data) {
	struct kbase_device *kbdev = data;
	unsigned int gpueb_irq_status = 0;

	gpueb_irq_status = gpueb_get_mbox1_irq();
	if (gpueb_irq_status & BIT(0)) {
		kbdev->low_volt_count++;
		dev_err(kbdev->dev, "kbase_gpueb_irq_handler: ++LOW_VOLT[%llu] BRCAST_TIMEOUT[%llu]",
			kbdev->low_volt_count, kbdev->brcast_timeout_count);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"kbase_gpueb_irq_handler: ++LOW_VOLT[%llu] BRCAST_TIMEOUT[%llu]\n",
			kbdev->low_volt_count, kbdev->brcast_timeout_count);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	} else if (gpueb_irq_status & BIT(1)) {
		kbdev->brcast_timeout_count++;
		dev_err(kbdev->dev, "kbase_gpueb_irq_handler: LOW_VOLT[%llu] ++BRCAST_TIMEOUT[%llu]",
			kbdev->low_volt_count, kbdev->brcast_timeout_count);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"kbase_gpueb_irq_handler: LOW_VOLT[%llu] ++BRCAST_TIMEOUT[%llu]\n",
			kbdev->low_volt_count, kbdev->brcast_timeout_count);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

	if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
		ged_mali_event_update_gpu_reset_nolock(GPU_RESET_GPUEB_IRQ_NOTIFY);
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */
		kbase_reset_gpu(kbdev);
	}
	else
		dev_err(kbdev->dev, "kbase_gpueb_irq_handler: GPU is already under reset");

	if (gpueb_irq_status & BIT(0))
		gpueb_clr_mbox1_irq(BIT(0));
	else if (gpueb_irq_status & BIT(1))
		gpueb_clr_mbox1_irq(BIT(1));

	return IRQ_HANDLED;
}
#endif /* CONFIG_MALI_MTK_GPUEB_IRQ */

int kbase_reset_gpu_init(struct kbase_device *kbdev)
{
#if IS_ENABLED(CONFIG_MALI_MTK_GPUEB_IRQ)
	int ret = -1;
	int irq;
	struct platform_device *pdev;
#endif /* CONFIG_MALI_MTK_GPUEB_IRQ */
	kbdev->csf.reset.workq = alloc_workqueue("Mali reset workqueue", 0, 1);
	if (kbdev->csf.reset.workq == NULL)
		return -ENOMEM;

	INIT_WORK(&kbdev->csf.reset.work, kbase_csf_reset_gpu_worker);

	init_waitqueue_head(&kbdev->csf.reset.wait);
	init_rwsem(&kbdev->csf.reset.sem);

#if IS_ENABLED(CONFIG_MALI_MTK_GPUEB_IRQ)
	kbdev->gpueb_irq = 0;
	kbdev->low_volt_count = 0;
	kbdev->brcast_timeout_count = 0;

	pdev = to_platform_device(kbdev->dev);
#if IS_ENABLED(CONFIG_OF)
	irq = platform_get_irq_byname(pdev, "GPUEB_MBOX1");
	dev_info(kbdev->dev, "get GPUEB_MBOX1 interrupt %d\n", irq);
#else
	irq = platform_get_irq(pdev, 5);
	dev_info(kbdev->dev, "get mali 5 interrupt %d\n", irq);
#endif /* CONFIG_OF */
	if (irq <= 0) {
		dev_err(kbdev->dev, "fail to get GPUEB_MBOX1 IRQ, err_code = %d\n", irq);
		return -EINVAL;
	}

	kbdev->gpueb_irq = irq;
	ret = request_irq(kbdev->gpueb_irq, kbase_gpueb_irq_handler,
		irqd_get_trigger_type(irq_get_irq_data(kbdev->gpueb_irq)) | IRQF_SHARED,
		dev_name(kbdev->dev), kbdev);

	if (ret) {
		dev_err(kbdev->dev, "fail to request GPUEB_MBOX1 IRQ %d, err_code = %d\n",
			kbdev->gpueb_irq, ret);
		return -EINVAL;
	}
#endif /* CONFIG_MALI_MTK_GPUEB_IRQ */
	return 0;
}

void kbase_reset_gpu_term(struct kbase_device *kbdev)
{
	destroy_workqueue(kbdev->csf.reset.workq);

#if IS_ENABLED(CONFIG_MALI_MTK_GPUEB_IRQ)
	if (kbdev->gpueb_irq)
		free_irq(kbdev->gpueb_irq, kbdev);
#endif /* CONFIG_MALI_MTK_GPUEB_IRQ */
}
