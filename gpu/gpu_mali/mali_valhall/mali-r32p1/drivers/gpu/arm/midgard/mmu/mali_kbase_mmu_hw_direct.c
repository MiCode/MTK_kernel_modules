// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2014-2021 ARM Limited. All rights reserved.
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

#include <linux/bitops.h>
#include <mali_kbase.h>
#include <mali_kbase_mem.h>
#include <mmu/mali_kbase_mmu_hw.h>
#include <tl/mali_kbase_tracepoints.h>
#include <device/mali_kbase_device.h>
#include <linux/delay.h>

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include <mtk_gpufreq.h>
#include <platform/mtk_platform_common.h>
#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
#include <mali_kbase_reset_gpu.h>
#endif

/* Max loop is 100000000 which roughly equals to 50s.
 * 1s roughly equals to 2000000 loops.
 */
#define KBASE_AS_INACTIVE_DUMP_POINT_1S     (KBASE_AS_INACTIVE_MAX_LOOPS - (2000000 * 1))
#define KBASE_AS_INACTIVE_DUMP_POINT_3S     (KBASE_AS_INACTIVE_MAX_LOOPS - (2000000 * 3))
#define KBASE_AS_INACTIVE_DUMP_POINT_5S     (KBASE_AS_INACTIVE_MAX_LOOPS - (2000000 * 5))
#define KBASE_AS_INACTIVE_DUMP_POINT_8S     (KBASE_AS_INACTIVE_MAX_LOOPS - (2000000 * 8))
#endif

/**
 * lock_region() - Generate lockaddr to lock memory region in MMU
 * @pfn:       Starting page frame number of the region to lock
 * @num_pages: Number of pages to lock. It must be greater than 0.
 * @lockaddr:  Address and size of memory region to lock
 *
 * The lockaddr value is a combination of the starting address and
 * the size of the region that encompasses all the memory pages to lock.
 *
 * Bits 5:0 are used to represent the size, which must be a power of 2.
 * The smallest amount of memory to be locked corresponds to 32 kB,
 * i.e. 8 memory pages, because a MMU cache line is made of 64 bytes
 * and every page table entry is 8 bytes. Therefore it is not possible
 * to lock less than 8 memory pages at a time.
 *
 * The size is expressed as a logarithm minus one:
 * - A value of 14 is thus interpreted as log(32 kB) = 15, where 32 kB
 *   is the smallest possible size.
 * - Likewise, a value of 47 is interpreted as log(256 TB) = 48, where 256 TB
 *   is the largest possible size (implementation defined value according
 *   to the HW spec).
 *
 * Bits 11:6 are reserved.
 *
 * Bits 63:12 are used to represent the base address of the region to lock.
 * Only the upper bits of the address are used; lowest bits are cleared
 * to avoid confusion.
 *
 * The address is aligned to a multiple of the region size. This has profound
 * implications on the region size itself: often the MMU will lock a region
 * larger than the given number of pages, because the lock region cannot start
 * from any arbitrary address.
 *
 * Return: 0 if success, or an error code on failure.
 */
static int lock_region(u64 pfn, u32 num_pages, u64 *lockaddr)
{
	const u64 lockaddr_base = pfn << PAGE_SHIFT;
	const u64 lockaddr_end = ((pfn + num_pages) << PAGE_SHIFT) - 1;
	u64 lockaddr_size_log2;

	if (num_pages == 0)
		return -EINVAL;

	/* The MMU lock region is a self-aligned region whose size
	 * is a power of 2 and that contains both start and end
	 * of the address range determined by pfn and num_pages.
	 * The size of the MMU lock region can be defined as the
	 * largest divisor that yields the same result when both
	 * start and end addresses are divided by it.
	 *
	 * For instance: pfn=0x4F000 num_pages=2 describe the
	 * address range between 0x4F000 and 0x50FFF. It is only
	 * 2 memory pages. However there isn't a single lock region
	 * of 8 kB that encompasses both addresses because 0x4F000
	 * would fall into the [0x4E000, 0x4FFFF] region while
	 * 0x50000 would fall into the [0x50000, 0x51FFF] region.
	 * The minimum lock region size that includes the entire
	 * address range is 128 kB, and the region would be
	 * [0x40000, 0x5FFFF].
	 *
	 * The region size can be found by comparing the desired
	 * start and end addresses and finding the highest bit
	 * that differs. The smallest naturally aligned region
	 * must include this bit change, hence the desired region
	 * starts with this bit (and subsequent bits) set to 0
	 * and ends with the bit (and subsequent bits) set to 1.
	 *
	 * In the example above: 0x4F000 ^ 0x50FFF = 0x1FFFF
	 * therefore the highest bit that differs is bit #16
	 * and the region size (as a logarithm) is 16 + 1 = 17, i.e. 128 kB.
	 */
	lockaddr_size_log2 = fls(lockaddr_base ^ lockaddr_end);

	/* Cap the size against minimum and maximum values allowed. */
	if (lockaddr_size_log2 > KBASE_LOCK_REGION_MAX_SIZE_LOG2)
		return -EINVAL;

	lockaddr_size_log2 =
		MAX(lockaddr_size_log2, KBASE_LOCK_REGION_MIN_SIZE_LOG2);

	/* Represent the result in a way that is compatible with HW spec.
	 *
	 * Upper bits are used for the base address, whose lower bits
	 * are cleared to avoid confusion because they are going to be ignored
	 * by the MMU anyway, since lock regions shall be aligned with
	 * a multiple of their size and cannot start from any address.
	 *
	 * Lower bits are used for the size, which is represented as
	 * logarithm minus one of the actual size.
	 */
	*lockaddr = lockaddr_base & ~((1ull << lockaddr_size_log2) - 1);
	*lockaddr |= lockaddr_size_log2 - 1;

	return 0;
}

static int wait_ready(struct kbase_device *kbdev,
		unsigned int as_nr)
{
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	bool early_timeouts = false;
#endif
	unsigned int max_loops = KBASE_AS_INACTIVE_MAX_LOOPS;
	u32 val = kbase_reg_read(kbdev, MMU_AS_REG(as_nr, AS_STATUS));

	/* Wait for the MMU status to indicate there is no active command, in
	 * case one is pending. Do not log remaining register accesses.
	 */
	while (--max_loops && (val & AS_STATUS_AS_ACTIVE)) {
		val = kbase_reg_read(kbdev, MMU_AS_REG(as_nr, AS_STATUS));
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		if((max_loops == KBASE_AS_INACTIVE_DUMP_POINT_1S) ||
			(max_loops == KBASE_AS_INACTIVE_DUMP_POINT_3S) ||
			(max_loops == KBASE_AS_INACTIVE_DUMP_POINT_5S) ||
			(max_loops == KBASE_AS_INACTIVE_DUMP_POINT_8S)) {
			dev_info(kbdev->dev, "%s: Early dump for MMU not ready, remain loops: %d", __func__, max_loops);
			ged_log_buf_print2(
				kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
				"%s: Early dump for MMU not ready, remain loops: %d\n",
				__func__, max_loops);
		}
		if (max_loops == KBASE_AS_INACTIVE_DUMP_POINT_3S) {
			early_timeouts = true;
			break;
		}
#endif /* CONFIG_MALI_MTK_DEBUG */
	}

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	if (early_timeouts || max_loops == 0) {
		dev_info(kbdev->dev,
			"AS_ACTIVE bit stuck for as %u, might be caused by slow/unstable GPU clock or possible faulty FPGA connector, early_timeouts=%d max_loops=%d",
			as_nr, early_timeouts, max_loops);
		ged_log_buf_print2(
			kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"AS_ACTIVE bit stuck for as %u, might be caused by slow/unstable GPU clock or possible faulty FPGA connector, early_timeouts=%d max_loops=%d\n",
			as_nr, early_timeouts, max_loops);
		if (!mtk_common_gpufreq_bringup()) {
			mtk_common_debug_dump();
#if defined(CONFIG_MTK_GPUFREQ_V2)
			gpufreq_dump_infra_status();
#else
			mt_gpufreq_dump_infra_status();
#endif /* CONFIG_MTK_GPUFREQ_V2 */
		}

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
		spin_lock(&kbdev->reset_force_change);
		kbdev->reset_force_evict_group_work = true;
		kbdev->reset_force_hard_reset = true;
		spin_unlock(&kbdev->reset_force_change);
		if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
			dev_info(kbdev->dev, "Trigger GPU reset for MMU as command timeouts, early_timeouts=%d max_loops=%d",
			         early_timeouts, max_loops);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
			ged_log_buf_print2(
				kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
				"Trigger GPU reset for MMU as command timeouts, early_timeouts=%d max_loops=%d\n",
				early_timeouts, max_loops);
#endif
			kbase_reset_gpu(kbdev);
		} else {
			dev_info(kbdev->dev, "MMU as command timeouts! Other threads are already resetting the GPU, early_timeouts=%d max_loops=%d",
			         early_timeouts, max_loops);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
			ged_log_buf_print2(
				kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
				"MMU as command timeouts! Other threads are already resetting the GPU, early_timeouts=%d max_loops=%d\n",
				early_timeouts, max_loops);
#endif
		}
#endif
		return -1;
	}
#else
	if (WARN_ON_ONCE(max_loops == 0)) {
		dev_info(kbdev->dev,
			"AS_ACTIVE bit stuck for as %u, might be caused by slow/unstable GPU clock or possible faulty FPGA connector",
			as_nr);
		return -1;
	}
#endif

	/* If waiting in loop was performed, log last read value. */
	if (KBASE_AS_INACTIVE_MAX_LOOPS - 1 > max_loops)
		kbase_reg_read(kbdev, MMU_AS_REG(as_nr, AS_STATUS));

	return 0;
}

static int write_cmd(struct kbase_device *kbdev, int as_nr, u32 cmd)
{
	int status;

	/* write AS_COMMAND when MMU is ready to accept another command */
	status = wait_ready(kbdev, as_nr);
	if (status == 0)
		kbase_reg_write(kbdev, MMU_AS_REG(as_nr, AS_COMMAND), cmd);
	else {
		dev_info(kbdev->dev,
			"Wait for AS_ACTIVE bit failed for as %u, before sending MMU command %u",
			as_nr, cmd);
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
		ged_log_buf_print2(
			kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			"Wait for AS_ACTIVE bit failed for as %u, before sending MMU command %u\n",
			as_nr, cmd);
#endif
	}

	return status;
}

#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)
static int wait_cores_power_trans_complete(struct kbase_device *kbdev)
{
#define WAIT_TIMEOUT 1000 /* 1ms timeout */
#define DELAY_TIME_IN_US 1
	const int max_iterations = WAIT_TIMEOUT;
	int loop;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	for (loop = 0; loop < max_iterations; loop++) {
		u32 lo =
		    kbase_reg_read(kbdev, GPU_CONTROL_REG(SHADER_PWRTRANS_LO));
		u32 hi =
		    kbase_reg_read(kbdev, GPU_CONTROL_REG(SHADER_PWRTRANS_HI));

		if (!lo && !hi)
			break;

		udelay(DELAY_TIME_IN_US);
	}

	if (loop == max_iterations) {
		dev_info(kbdev->dev, "SHADER_PWRTRANS set for too long");
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * apply_hw_issue_GPU2019_3901_wa - Apply WA for the HW issue GPU2019_3901
 *
 * @kbdev:             Kbase device to issue the MMU operation on.
 * @mmu_cmd:           Pointer to the variable contain the value of MMU command
 *                     that needs to be sent to flush the L2 cache and do an
 *                     implicit unlock.
 * as_nr:              Address space number for which MMU command needs to be
 *                     sent.
 * @hwaccess_locked:   Flag to indicate if hwaccess_lock is held by the caller.
 *
 * This functions ensures that the flush of LSC is not missed for the pages that
 * were unmapped from the GPU, due to the power down transition of shader cores.
 *
 * Return: 0 if the WA was successfully applied, non-zero otherwise.
 */
static int apply_hw_issue_GPU2019_3901_wa(struct kbase_device *kbdev,
			u32 *mmu_cmd, unsigned int as_nr, bool hwaccess_locked)
{
	unsigned long flags = 0;
	int ret = 0;

	if (!hwaccess_locked)
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);

	/* Check if L2 is OFF. The cores also must be OFF if L2 is not up, so
	 * the workaround can be safely skipped.
	 */
	if (kbdev->pm.backend.l2_state != KBASE_L2_OFF) {
		if (*mmu_cmd != AS_COMMAND_FLUSH_MEM) {
			dev_info(kbdev->dev,
				 "Unexpected mmu command received");
			ret = -EINVAL;
			goto unlock;
		}

		/* Wait for the LOCK MMU command to complete, issued by the caller */
		ret = wait_ready(kbdev, as_nr);
		if (ret)
			goto unlock;

		ret = kbase_gpu_cache_flush_and_busy_wait(kbdev,
				GPU_COMMAND_CACHE_CLN_INV_LSC);
		if (ret)
			goto unlock;

		ret = wait_cores_power_trans_complete(kbdev);
		if (ret)
			goto unlock;

		/* As LSC is guaranteed to have been flushed we can use FLUSH_PT
		 * MMU command to only flush the L2.
		 */
		*mmu_cmd = AS_COMMAND_FLUSH_PT;
	}

unlock:
	if (!hwaccess_locked)
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return ret;
}
#endif

void kbase_mmu_hw_configure(struct kbase_device *kbdev, struct kbase_as *as)
{
	struct kbase_mmu_setup *current_setup = &as->current_setup;
	u64 transcfg = 0;

	lockdep_assert_held(&kbdev->hwaccess_lock);
	lockdep_assert_held(&kbdev->mmu_hw_mutex);

	transcfg = current_setup->transcfg;

	/* Set flag AS_TRANSCFG_PTW_MEMATTR_WRITE_BACK
	 * Clear PTW_MEMATTR bits
	 */
	transcfg &= ~AS_TRANSCFG_PTW_MEMATTR_MASK;
	/* Enable correct PTW_MEMATTR bits */
	transcfg |= AS_TRANSCFG_PTW_MEMATTR_WRITE_BACK;
	/* Ensure page-tables reads use read-allocate cache-policy in
	 * the L2
	 */
	transcfg |= AS_TRANSCFG_R_ALLOCATE;

	if (kbdev->system_coherency != COHERENCY_NONE) {
		/* Set flag AS_TRANSCFG_PTW_SH_OS (outer shareable)
		 * Clear PTW_SH bits
		 */
		transcfg = (transcfg & ~AS_TRANSCFG_PTW_SH_MASK);
		/* Enable correct PTW_SH bits */
		transcfg = (transcfg | AS_TRANSCFG_PTW_SH_OS);
	}

	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_TRANSCFG_LO),
			transcfg);
	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_TRANSCFG_HI),
			(transcfg >> 32) & 0xFFFFFFFFUL);

	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_TRANSTAB_LO),
			current_setup->transtab & 0xFFFFFFFFUL);
	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_TRANSTAB_HI),
			(current_setup->transtab >> 32) & 0xFFFFFFFFUL);

	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_MEMATTR_LO),
			current_setup->memattr & 0xFFFFFFFFUL);
	kbase_reg_write(kbdev, MMU_AS_REG(as->number, AS_MEMATTR_HI),
			(current_setup->memattr >> 32) & 0xFFFFFFFFUL);

	KBASE_TLSTREAM_TL_ATTRIB_AS_CONFIG(kbdev, as,
			current_setup->transtab,
			current_setup->memattr,
			transcfg);

	write_cmd(kbdev, as->number, AS_COMMAND_UPDATE);
#if MALI_USE_CSF
	/* Wait for UPDATE command to complete */
	wait_ready(kbdev, as->number);
#endif
}

static int mmu_hw_do_operation(struct kbase_device *kbdev, struct kbase_as *as,
		u64 vpfn, u32 nr, u32 op,
		bool hwaccess_locked)
{
	int ret;

	lockdep_assert_held(&kbdev->mmu_hw_mutex);

	if (op == AS_COMMAND_UNLOCK) {
		/* Unlock doesn't require a lock first */
		write_cmd(kbdev, as->number, AS_COMMAND_UNLOCK);

		/* Wait for UNLOCK command to complete */
		ret = wait_ready(kbdev, as->number);
	} else {
		u64 lock_addr;

		ret = lock_region(vpfn, nr, &lock_addr);

		if (!ret) {
			/* Lock the region that needs to be updated */
			kbase_reg_write(kbdev,
				MMU_AS_REG(as->number, AS_LOCKADDR_LO),
				lock_addr & 0xFFFFFFFFUL);
			kbase_reg_write(kbdev,
				MMU_AS_REG(as->number, AS_LOCKADDR_HI),
				(lock_addr >> 32) & 0xFFFFFFFFUL);
			write_cmd(kbdev, as->number, AS_COMMAND_LOCK);

#if MALI_USE_CSF && !IS_ENABLED(CONFIG_MALI_NO_MALI)
			/* WA for the BASE_HW_ISSUE_GPU2019_3901. No runtime check is used here
			 * as the WA is applicable to all CSF GPUs where FLUSH_MEM/PT command is
			 * supported, and this function doesn't gets called for the GPUs where
			 * FLUSH_MEM/PT command is deprecated.
			 */
			if (op == AS_COMMAND_FLUSH_MEM) {
				ret = apply_hw_issue_GPU2019_3901_wa(kbdev, &op,
						as->number, hwaccess_locked);
				if (ret)
					return ret;
			}
#endif

			/* Run the MMU operation */
			write_cmd(kbdev, as->number, op);

			/* Wait for the flush to complete */
			ret = wait_ready(kbdev, as->number);
		}
	}

	return ret;
}

int kbase_mmu_hw_do_operation_locked(struct kbase_device *kbdev, struct kbase_as *as,
		u64 vpfn, u32 nr, u32 type,
		unsigned int handling_irq)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	return mmu_hw_do_operation(kbdev, as, vpfn, nr, type, true);
}

int kbase_mmu_hw_do_operation(struct kbase_device *kbdev, struct kbase_as *as,
		u64 vpfn, u32 nr, u32 type,
		unsigned int handling_irq)
{
	return mmu_hw_do_operation(kbdev, as, vpfn, nr, type, false);
}

void kbase_mmu_hw_clear_fault(struct kbase_device *kbdev, struct kbase_as *as,
		enum kbase_mmu_fault_type type)
{
	unsigned long flags;
	u32 pf_bf_mask;

	spin_lock_irqsave(&kbdev->mmu_mask_change, flags);

	/*
	 * A reset is in-flight and we're flushing the IRQ + bottom half
	 * so don't update anything as it could race with the reset code.
	 */
	if (kbdev->irq_reset_flush)
		goto unlock;

	/* Clear the page (and bus fault IRQ as well in case one occurred) */
	pf_bf_mask = MMU_PAGE_FAULT(as->number);
#if !MALI_USE_CSF
	if (type == KBASE_MMU_FAULT_TYPE_BUS ||
			type == KBASE_MMU_FAULT_TYPE_BUS_UNEXPECTED)
		pf_bf_mask |= MMU_BUS_ERROR(as->number);
#endif
	kbase_reg_write(kbdev, MMU_REG(MMU_IRQ_CLEAR), pf_bf_mask);

unlock:
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);
}

void kbase_mmu_hw_enable_fault(struct kbase_device *kbdev, struct kbase_as *as,
		enum kbase_mmu_fault_type type)
{
	unsigned long flags;
	u32 irq_mask;

	/* Enable the page fault IRQ
	 * (and bus fault IRQ as well in case one occurred)
	 */
	spin_lock_irqsave(&kbdev->mmu_mask_change, flags);

	/*
	 * A reset is in-flight and we're flushing the IRQ + bottom half
	 * so don't update anything as it could race with the reset code.
	 */
	if (kbdev->irq_reset_flush)
		goto unlock;

	irq_mask = kbase_reg_read(kbdev, MMU_REG(MMU_IRQ_MASK)) |
			MMU_PAGE_FAULT(as->number);

#if !MALI_USE_CSF
	if (type == KBASE_MMU_FAULT_TYPE_BUS ||
			type == KBASE_MMU_FAULT_TYPE_BUS_UNEXPECTED)
		irq_mask |= MMU_BUS_ERROR(as->number);
#endif
	kbase_reg_write(kbdev, MMU_REG(MMU_IRQ_MASK), irq_mask);

unlock:
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);
}
