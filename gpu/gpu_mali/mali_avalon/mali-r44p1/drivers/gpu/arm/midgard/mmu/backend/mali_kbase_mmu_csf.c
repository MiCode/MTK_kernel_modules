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

/**
 * DOC: Base kernel MMU management specific for CSF GPU.
 */

#include <mali_kbase.h>
#include <gpu/mali_kbase_gpu_fault.h>
#include <mali_kbase_ctx_sched.h>
#include <mali_kbase_reset_gpu.h>
#include <mali_kbase_as_fault_debugfs.h>
#include <mmu/mali_kbase_mmu_internal.h>

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include <platform/mtk_platform_common.h>
#endif /* CONFIG_MALI_MTK_DEBUG */
#include <platform/mtk_platform_utils.h> /* MTK_INLINE */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

void kbase_mmu_get_as_setup(struct kbase_mmu_table *mmut,
		struct kbase_mmu_setup * const setup)
{
	/* Set up the required caching policies at the correct indices
	 * in the memattr register.
	 */
	setup->memattr =
		(AS_MEMATTR_IMPL_DEF_CACHE_POLICY <<
			(AS_MEMATTR_INDEX_IMPL_DEF_CACHE_POLICY * 8)) |
		(AS_MEMATTR_FORCE_TO_CACHE_ALL <<
			(AS_MEMATTR_INDEX_FORCE_TO_CACHE_ALL * 8)) |
		(AS_MEMATTR_WRITE_ALLOC <<
			(AS_MEMATTR_INDEX_WRITE_ALLOC * 8)) |
		(AS_MEMATTR_AARCH64_OUTER_IMPL_DEF   <<
			(AS_MEMATTR_INDEX_OUTER_IMPL_DEF * 8)) |
		(AS_MEMATTR_AARCH64_OUTER_WA <<
			(AS_MEMATTR_INDEX_OUTER_WA * 8)) |
		(AS_MEMATTR_AARCH64_NON_CACHEABLE <<
			(AS_MEMATTR_INDEX_NON_CACHEABLE * 8)) |
		(AS_MEMATTR_AARCH64_SHARED <<
			(AS_MEMATTR_INDEX_SHARED * 8));

	setup->transtab = (u64)mmut->pgd & AS_TRANSTAB_BASE_MASK;
	setup->transcfg = AS_TRANSCFG_ADRMODE_AARCH64_4K;
}

/**
 * submit_work_pagefault() - Submit a work for MMU page fault.
 *
 * @kbdev:    Kbase device pointer
 * @as_nr:    Faulty address space
 * @fault:    Data relating to the fault
 *
 * This function submits a work for reporting the details of MMU fault.
 */
static void submit_work_pagefault(struct kbase_device *kbdev, u32 as_nr,
		struct kbase_fault *fault)
{
	unsigned long flags;
	struct kbase_as *const as = &kbdev->as[as_nr];
	struct kbase_context *kctx;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kctx = kbase_ctx_sched_as_to_ctx_nolock(kbdev, as_nr);

	if (kctx) {
		kbase_ctx_sched_retain_ctx_refcount(kctx);

		as->pf_data = (struct kbase_fault) {
			.status = fault->status,
			.addr = fault->addr,
		};

		/*
		 * A page fault work item could already be pending for the
		 * context's address space, when the page fault occurs for
		 * MCU's address space.
		 */
		if (!queue_work(as->pf_wq, &as->work_pagefault)) {
			dev_dbg(kbdev->dev,
				"Page fault is already pending for as %u", as_nr);
			kbase_ctx_sched_release_ctx(kctx);
		} else {
			atomic_inc(&kbdev->faults_pending);
		}
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_mmu_report_mcu_as_fault_and_reset(struct kbase_device *kbdev,
		struct kbase_fault *fault)
{
	/* decode the fault status */
	u32 exception_type = fault->status & 0xFF;
	u32 access_type = (fault->status >> 8) & 0x3;
	u32 source_id = (fault->status >> 16);
	int as_no;

	/* terminal fault, print info about the fault */
	dev_err(kbdev->dev,
		"Unexpected Page fault in firmware address space at VA 0x%016llX\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n",
		fault->addr,
		fault->status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(fault->status),
		source_id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Unexpected Page fault in firmware address space at VA 0x%016llX\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n",
		fault->addr,
		fault->status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(fault->status),
		source_id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, -1, MTK_DBG_HOOK_MMU_UNEXPECTEDPAGEFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG */

	kbase_debug_csf_fault_notify(kbdev, NULL, DF_GPU_PAGE_FAULT);

	/* Report MMU fault for all address spaces (except MCU_AS_NR) */
	for (as_no = 1; as_no < kbdev->nr_hw_address_spaces; as_no++)
		submit_work_pagefault(kbdev, as_no, fault);

	/* GPU reset is required to recover */
	if (kbase_prepare_to_reset_gpu(kbdev,
				       RESET_FLAGS_HWC_UNRECOVERABLE_ERROR))
		kbase_reset_gpu(kbdev);

#ifndef MALI_STRIP_KBASE_DEVELOPMENT
#if IS_ENABLED(CONFIG_MALI_BUSLOG) && IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	queue_work(kbdev->buslog_callback_wq, &kbdev->buslog_callback_work);
#endif /* CONFIG_MALI_BUSLOG && CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
#endif /* MALI_STRIP_KBASE_DEVELOPMENT */
}
KBASE_EXPORT_TEST_API(kbase_mmu_report_mcu_as_fault_and_reset);

void kbase_gpu_report_bus_fault_and_kill(struct kbase_context *kctx,
		struct kbase_as *as, struct kbase_fault *fault)
{
	struct kbase_device *kbdev = kctx->kbdev;
	u32 const status = fault->status;
	int exception_type = (status & GPU_FAULTSTATUS_EXCEPTION_TYPE_MASK) >>
				GPU_FAULTSTATUS_EXCEPTION_TYPE_SHIFT;
	int access_type = (status & GPU_FAULTSTATUS_ACCESS_TYPE_MASK) >>
				GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT;
	int source_id = (status & GPU_FAULTSTATUS_SOURCE_ID_MASK) >>
				GPU_FAULTSTATUS_SOURCE_ID_SHIFT;
	const char *addr_valid = (status & GPU_FAULTSTATUS_ADDRESS_VALID_MASK) ? "true" : "false";
	int as_no = as->number;
	unsigned long flags;
	const uintptr_t fault_addr = fault->addr;

	/* terminal fault, print info about the fault */
	dev_err(kbdev->dev,
		"GPU bus fault in AS%d at PA %pK\n"
		"PA_VALID: %s\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n"
		"pid: %d\n",
		as_no, (void *)fault_addr,
		addr_valid,
		status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(access_type),
		source_id,
		kctx->pid);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"GPU bus fault in AS%d at PA %pK\n"
		"PA_VALID: %s\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n"
		"pid: %d\n",
		as_no, (void *)fault_addr,
		addr_valid,
		status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(access_type),
		source_id,
		kctx->pid);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, kctx->pid, MTK_DBG_HOOK_MMU_BUSFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG */

	/* AS transaction begin */
	mutex_lock(&kbdev->mmu_hw_mutex);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_mmu_disable(kctx);
	kbase_ctx_flag_set(kctx, KCTX_AS_DISABLED_ON_FAULT);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->mmu_hw_mutex);

	/* Switching to UNMAPPED mode above would have enabled the firmware to
	 * recover from the fault (if the memory access was made by firmware)
	 * and it can then respond to CSG termination requests to be sent now.
	 * All GPU command queue groups associated with the context would be
	 * affected as they use the same GPU address space.
	 */
	kbase_csf_ctx_handle_fault(kctx, fault);

	/* Now clear the GPU fault */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
			GPU_COMMAND_CLEAR_FAULT);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

#ifndef MALI_STRIP_KBASE_DEVELOPMENT
#if IS_ENABLED(CONFIG_MALI_BUSLOG) && IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	queue_work(kbdev->buslog_callback_wq, &kbdev->buslog_callback_work);
#endif /* CONFIG_MALI_BUSLOG && CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
#endif /* MALI_STRIP_KBASE_DEVELOPMENT */
}

#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
static void print_group_queues_data(struct kbase_queue_group *const group)
{
	struct kbase_queue *queue;
	struct page *page;
	size_t size_mask;
	const unsigned int instruction_size = sizeof(u64);
	unsigned int i;
	u64 insert[5], extract[5], ringbuff[5];
	u64 *input_addr;
	u64 *output_addr;
	u64 cs_insert;
	u64 cs_extract;
	u64 start, stop;
	u64 page_off;
	u64 offset;
	u64 *ringbuffer;
	u64 *ptr;

	if (!group)
		return;

	for (i = 0; i < 5; i++) {
		queue = group->bound_queues[i];
		if (queue && queue->user_io_addr) {
			input_addr = (u64 *)queue->user_io_addr;
			output_addr = (u64 *)(queue->user_io_addr + PAGE_SIZE / sizeof(u64));

			insert[i] = input_addr[CS_INSERT_LO / sizeof(u64)];
			extract[i] = output_addr[CS_EXTRACT_LO / sizeof(u64)];
			ringbuff[i] = queue->base_addr;
		} else {
			insert[i] = 0;
			extract[i] = 0;
			ringbuff[i] = 0;
		}
	}

#if !IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG)
	dev_warn(group->kctx->kbdev->dev,
		"R0 %llx I0 %llx E0 %llx, R1 %llx I1 %llx E1 %llx, R2 %llx I2 %llx E2 %llx, R3 %llx I3 %llx E3 %llx, R4 %llx I4 %llx E4 %llx",
		ringbuff[0], insert[0], extract[0],
		ringbuff[1], insert[1], extract[1],
		ringbuff[2], insert[2], extract[2],
		ringbuff[3], insert[3], extract[3],
		ringbuff[4], insert[4], extract[4]);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG */

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"R0 %llx I0 %llx E0 %llx, R1 %llx I1 %llx E1 %llx, R2 %llx I2 %llx E2 %llx, R3 %llx I3 %llx E3 %llx, R4 %llx I4 %llx E4 %llx\n",
		ringbuff[0], insert[0], extract[0],
		ringbuff[1], insert[1], extract[1],
		ringbuff[2], insert[2], extract[2],
		ringbuff[3], insert[3], extract[3],
		ringbuff[4], insert[4], extract[4]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	for (i = 0; i < 5; i++) {
		queue = group->bound_queues[i];

		if (!queue)
			continue;

		cs_insert = insert[i];
		cs_extract = extract[i];

		if (!queue->queue_reg || !queue->queue_reg->nr_pages)
			continue;

		size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;

		if (cs_insert == cs_extract)
			continue;

		cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

		/* Go 32 instructions back */
		if (cs_extract > (32 * instruction_size))
			start = cs_extract - (32 * instruction_size);
		else
			start = 0;

		/* Print upto 64 instructions */
		stop = start + (64 * instruction_size);
		if (stop > cs_insert)
			stop = cs_insert;

		pr_err("\nQueue %u: Instructions from Extract offset %llx\n", i, start);

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"\nQueue %u: Instructions from Extract offset %llx\n", i, start);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

		while (start != stop) {
			page_off = (start & size_mask) >> PAGE_SHIFT;
			offset = (start & size_mask) & ~PAGE_MASK;

			if (!queue->queue_reg->gpu_alloc)
				break;

			page = as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			ringbuffer = vmap(&page, 1, VM_MAP, pgprot_noncached(PAGE_KERNEL));

			if (!ringbuffer)
				break;

			ptr = &ringbuffer[offset/8];

			pr_err("%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
				"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
				ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

			vunmap(ringbuffer);
			start += (8 * instruction_size);
		}
	}
}

static void dump_cmd_ptr_instructions(struct kbase_context *kctx, u64 cmd_ptr)
{
	struct kbase_vmap_struct mapping;
	u64 address;
	u64 *ptr;

	if (!kctx)
		return;

	dev_err(kctx->kbdev->dev, "Dumping instructions around the CMD_PTR");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Dumping instructions around the CMD_PTR\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	/* Start from 4 instructions back */
	for (address = cmd_ptr - (4 * sizeof(u64));
	     address < (cmd_ptr + (4 * sizeof(u64)));
	     address += sizeof(u64)) {
		ptr = kbase_vmap(kctx, address, sizeof(u64), &mapping);

		if (!ptr)
			continue;

		pr_err("0x%llx: %016llx\n", address, *ptr);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kctx->kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"0x%llx: %016llx\n", address, *ptr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		kbase_vunmap(kctx, &mapping);
	}
}

#define CSHW_IT_COMP_REG(r) (CSHW_BASE + 0x1000 + r)
#define CSHW_IT_FRAG_REG(r) (CSHW_BASE + 0x2000 + r)
#define CSHW_IT_TILER_REG(r)(CSHW_BASE + 0x3000 + r)

#define CSHW_BASE 0x0030000
#define CSHW_CSHWIF_0 0x4000 /* () CSHWIF 0 registers */
#define CSHWIF(n) (CSHW_BASE + CSHW_CSHWIF_0 + (n)*256)
#define CSHWIF_REG(n, r) (CSHWIF(n) + r)
#define NR_HW_INTERFACES 4

static void dump_iterator_registers(struct kbase_device *kbdev)
{
	unsigned int i;
	u32 reg_offsets[8] = { 0x0, 0x4, 0x8, 0xD0, 0xDC, 0xA4, 0xA0, 0xE0 };
	u32 cshw_it_comp_reg[8], cshw_it_frag_reg[8], cshw_it_tiler_reg[8];

	if (kbdev->protected_mode)
		return;

	if (!kbdev->pm.backend.gpu_powered)
		return;

	for (i = 0; i < 8; i++) {
		cshw_it_comp_reg[i] = kbase_reg_read(kbdev, CSHW_IT_COMP_REG(reg_offsets[i]));
		cshw_it_frag_reg[i] = kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(reg_offsets[i]));
		cshw_it_tiler_reg[i] = kbase_reg_read(kbdev, CSHW_IT_TILER_REG(reg_offsets[i]));
	}

	dev_err(kbdev->dev, "Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x",
		cshw_it_comp_reg[0], cshw_it_comp_reg[1], cshw_it_comp_reg[2], cshw_it_comp_reg[3],
		cshw_it_comp_reg[4], cshw_it_comp_reg[5], cshw_it_comp_reg[6], cshw_it_comp_reg[7]);
	dev_err(kbdev->dev, "Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x",
		cshw_it_frag_reg[0], cshw_it_frag_reg[1], cshw_it_frag_reg[2], cshw_it_frag_reg[3],
		cshw_it_frag_reg[4], cshw_it_frag_reg[5], cshw_it_frag_reg[6], cshw_it_frag_reg[7]);
	dev_err(kbdev->dev, "Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x",
		cshw_it_tiler_reg[0], cshw_it_tiler_reg[1], cshw_it_tiler_reg[2], cshw_it_tiler_reg[3],
		cshw_it_tiler_reg[4], cshw_it_tiler_reg[5], cshw_it_tiler_reg[6], cshw_it_tiler_reg[7]);
	dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x\n",
			cshw_it_comp_reg[0], cshw_it_comp_reg[1], cshw_it_comp_reg[2], cshw_it_comp_reg[3],
			cshw_it_comp_reg[4], cshw_it_comp_reg[5], cshw_it_comp_reg[6], cshw_it_comp_reg[7]);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x\n",
			cshw_it_frag_reg[0], cshw_it_frag_reg[1], cshw_it_frag_reg[2], cshw_it_frag_reg[3],
			cshw_it_frag_reg[4], cshw_it_frag_reg[5], cshw_it_frag_reg[6], cshw_it_frag_reg[7]);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x\n",
			cshw_it_tiler_reg[0], cshw_it_tiler_reg[1], cshw_it_tiler_reg[2], cshw_it_tiler_reg[3],
			cshw_it_tiler_reg[4], cshw_it_tiler_reg[5], cshw_it_tiler_reg[6], cshw_it_tiler_reg[7]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
}

static void dump_hwif_registers(struct kbase_device *kbdev, int faulty_as)
{
	struct kbase_context *kctx;
	unsigned int i, j;
	int as_nr;
	u32 reg_offsets[17] = { 0x24, 0x34, 0x60, 0x64, 0x74, 0x78, 0x7C, 0x80, 0x98, 0xA4, 0xAC, 0xB0, 0xB8, 0xBC, 0x28, 0x2C, 0x30 };
	u32 cshwif_reg[17];
	u64 cmd_ptr;
	u64 cmd_ptr_end;

	if (kbdev->protected_mode)
		return;

	for (i = 0; kbdev->pm.backend.gpu_powered && (i < NR_HW_INTERFACES); i++) {
		cmd_ptr = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x0)) |
			((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x4)) << 32);
		cmd_ptr_end = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x8)) |
			((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xC)) << 32);
		as_nr = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x34));

		if (!cmd_ptr)
			continue;

		for (j = 0; j < 17; j++)
			cshwif_reg[j] = kbase_reg_read(kbdev, CSHWIF_REG(i, reg_offsets[j]));

		dev_err(kbdev->dev, "Register dump of CSHWIF %d", i);
		dev_err(kbdev->dev, "CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x",
			cmd_ptr,
			cmd_ptr_end,
			cshwif_reg[0],
			cshwif_reg[1],
			cshwif_reg[2] | ((u64)cshwif_reg[3] << 32),
			cshwif_reg[4],
			cshwif_reg[5],
			cshwif_reg[6]);
		dev_err(kbdev->dev, "CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx",
			cshwif_reg[7],
			cshwif_reg[8],
			cshwif_reg[9],
			cshwif_reg[10],
			cshwif_reg[11],
			cshwif_reg[12] | ((u64)cshwif_reg[13] << 32));
		dev_err(kbdev->dev, "ITER_COMPUTE: %x ITER_FRAGMENT: %x ITER_TILER: %x",
			cshwif_reg[14],
			cshwif_reg[15],
			cshwif_reg[16]);
		dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x\n",
			cmd_ptr,
			cmd_ptr_end,
			cshwif_reg[0],
			cshwif_reg[1],
			cshwif_reg[2] | ((u64)cshwif_reg[3] << 32),
			cshwif_reg[4],
			cshwif_reg[5],
			cshwif_reg[6]);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx\n",
			cshwif_reg[7],
			cshwif_reg[8],
			cshwif_reg[9],
			cshwif_reg[10],
			cshwif_reg[11],
			cshwif_reg[12] | ((u64)cshwif_reg[13] << 32));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"ITER_COMPUTE: %x ITER_FRAGMENT: %x ITER_TILER: %x\n",
			cshwif_reg[14],
			cshwif_reg[15],
			cshwif_reg[16]);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

		if ((cmd_ptr != cmd_ptr_end) && (as_nr == faulty_as)) {
			kctx = kbdev->as_to_kctx[as_nr];
			if (kctx)
				dump_cmd_ptr_instructions(kctx, cmd_ptr);
		}
	}
}

static void dump_mmu_teardown_records(struct kbase_device *kbdev) {
	size_t idx;
	size_t tail;

	mutex_lock(&kbdev->mmu_debug_info_lock);

	tail = kbdev->mmu_debug_info_head < MMU_DEBUG_INFO_BUFFER_SIZE ? kbdev->mmu_debug_info_head : MMU_DEBUG_INFO_BUFFER_SIZE;

	dev_err(kbdev->dev, "[mmu] Dumping mmu debug info : %zu records", tail);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR, "[mmu] Dumping mmu debug info : %zu records\n", tail);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	/* Each line include 5 records. */
	for (idx = 0; (idx + 5) <= tail; idx += 5) {
#if !IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG)
		dev_err(kbdev->dev,
			"[mmu] "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm,
			kbdev->mmu_dbg[idx+1].time, kbdev->mmu_dbg[idx+1].pgds, kbdev->mmu_dbg[idx+1].va, kbdev->mmu_dbg[idx+1].tgid, kbdev->mmu_dbg[idx+1].id, kbdev->mmu_dbg[idx+1].as_nr, kbdev->mmu_dbg[idx+1].ipm,
			kbdev->mmu_dbg[idx+2].time, kbdev->mmu_dbg[idx+2].pgds, kbdev->mmu_dbg[idx+2].va, kbdev->mmu_dbg[idx+2].tgid, kbdev->mmu_dbg[idx+2].id, kbdev->mmu_dbg[idx+2].as_nr, kbdev->mmu_dbg[idx+2].ipm,
			kbdev->mmu_dbg[idx+3].time, kbdev->mmu_dbg[idx+3].pgds, kbdev->mmu_dbg[idx+3].va, kbdev->mmu_dbg[idx+3].tgid, kbdev->mmu_dbg[idx+3].id, kbdev->mmu_dbg[idx+3].as_nr, kbdev->mmu_dbg[idx+3].ipm,
			kbdev->mmu_dbg[idx+4].time, kbdev->mmu_dbg[idx+4].pgds, kbdev->mmu_dbg[idx+4].va, kbdev->mmu_dbg[idx+4].tgid, kbdev->mmu_dbg[idx+4].id, kbdev->mmu_dbg[idx+4].as_nr, kbdev->mmu_dbg[idx+4].ipm);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR,
			"[mmu] "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d\n",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm,
			kbdev->mmu_dbg[idx+1].time, kbdev->mmu_dbg[idx+1].pgds, kbdev->mmu_dbg[idx+1].va, kbdev->mmu_dbg[idx+1].tgid, kbdev->mmu_dbg[idx+1].id, kbdev->mmu_dbg[idx+1].as_nr, kbdev->mmu_dbg[idx+1].ipm,
			kbdev->mmu_dbg[idx+2].time, kbdev->mmu_dbg[idx+2].pgds, kbdev->mmu_dbg[idx+2].va, kbdev->mmu_dbg[idx+2].tgid, kbdev->mmu_dbg[idx+2].id, kbdev->mmu_dbg[idx+2].as_nr, kbdev->mmu_dbg[idx+2].ipm,
			kbdev->mmu_dbg[idx+3].time, kbdev->mmu_dbg[idx+3].pgds, kbdev->mmu_dbg[idx+3].va, kbdev->mmu_dbg[idx+3].tgid, kbdev->mmu_dbg[idx+3].id, kbdev->mmu_dbg[idx+3].as_nr, kbdev->mmu_dbg[idx+3].ipm,
			kbdev->mmu_dbg[idx+4].time, kbdev->mmu_dbg[idx+4].pgds, kbdev->mmu_dbg[idx+4].va, kbdev->mmu_dbg[idx+4].tgid, kbdev->mmu_dbg[idx+4].id, kbdev->mmu_dbg[idx+4].as_nr, kbdev->mmu_dbg[idx+4].ipm);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}

	/* Dump the rest of the records. */
	for (; idx < tail; idx++) {
#if !IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG)
		dev_err(kbdev->dev, "[mmu] %llu,%zu,%llx,%d,%d,%d,%d",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR,
			"[mmu] %llu,%zu,%llx,%d,%d,%d,%d\n",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

	mutex_unlock(&kbdev->mmu_debug_info_lock);
}
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

/*
 * The caller must ensure it's retained the ctx to prevent it from being
 * scheduled out whilst it's being worked on.
 */
void kbase_mmu_report_fault_and_kill(struct kbase_context *kctx,
		struct kbase_as *as, const char *reason_str,
		struct kbase_fault *fault)
{
	unsigned long flags;
	unsigned int exception_type;
	unsigned int access_type;
	unsigned int source_id;
	int as_no;
	struct kbase_device *kbdev;
	const u32 status = fault->status;
#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	u32 csg_nr;
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

	as_no = as->number;
	kbdev = kctx->kbdev;

	/* Make sure the context was active */
	if (WARN_ON(atomic_read(&kctx->refcount) <= 0))
		return;

	/* decode the fault status */
	exception_type = AS_FAULTSTATUS_EXCEPTION_TYPE_GET(status);
	access_type = AS_FAULTSTATUS_ACCESS_TYPE_GET(status);
	source_id = AS_FAULTSTATUS_SOURCE_ID_GET(status);

#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	dev_err(kbdev->dev,
		"Unhandled Page fault (protected mode %d) in AS%d at VA 0x%016llX\n"
		"Reason: %s\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n"
		"ctx_id: %d_%d, pid: %d\n"
		"group_leader : %s, comm: %s\n",
		kbdev->protected_mode, as_no, fault->addr,
		reason_str,
		status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(status),
		source_id,
		kctx->tgid, kctx->id, kctx->pid,
		kctx->group_leader_comm, kctx->comm);
#else
	/* terminal fault, print info about the fault */
	dev_err(kbdev->dev,
		"Unhandled Page fault in AS%d at VA 0x%016llX\n"
		"Reason: %s\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n"
		"pid: %d\n",
		as_no, fault->addr,
		reason_str,
		status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(status),
		source_id,
		kctx->pid);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Unhandled Page fault (protected mode %d) in AS%d at VA 0x%016llX\n"
		"Reason: %s\n"
		"raw fault status: 0x%X\n"
		"exception type 0x%X: %s\n"
		"access type 0x%X: %s\n"
		"source id 0x%X\n",
		kbdev->protected_mode, as_no, fault->addr,
		reason_str,
		status,
		exception_type, kbase_gpu_exception_name(exception_type),
		access_type, kbase_gpu_access_type_name(status),
		source_id);
#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"\nctx_id: %d_%d, pid: %d\n"
		"group_leader : %s, comm: %s\n",
		kctx->tgid, kctx->id, kctx->pid,
		kctx->group_leader_comm, kctx->comm);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	mutex_lock(&kbdev->register_check_lock);
	kbdev->bypass_register_check = true;

	dump_hwif_registers(kbdev, as_no);
	dump_iterator_registers(kbdev);
	mutex_lock(&kctx->csf.lock);
	for (csg_nr = 0; csg_nr < kbdev->csf.global_iface.group_num; csg_nr++) {
		struct kbase_queue_group *const group =
			kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

		if (!group)
			continue;
		if (group->kctx != kctx)
			continue;

		dev_err(kbdev->dev, "Dumping data of queues of group %d on slot %d in run_state %d",
			group->handle, group->csg_nr, group->run_state);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Dumping data of queues of group %d on slot %d in run_state %d\n",
			group->handle, group->csg_nr, group->run_state);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		print_group_queues_data(group);
	}
	mutex_unlock(&kctx->csf.lock);

	kbdev->bypass_register_check = false;
	mutex_unlock(&kbdev->register_check_lock);

	dump_mmu_teardown_records(kbdev);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, -1, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, -1, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, kctx->pid, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_CSF_DUMP_GROUPS_QUEUES, kctx->tgid, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG */

	/* AS transaction begin */
	mutex_lock(&kbdev->mmu_hw_mutex);

	/* switch to UNMAPPED mode,
	 * will abort all jobs and stop any hw counter dumping
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_mmu_disable(kctx);
	kbase_ctx_flag_set(kctx, KCTX_AS_DISABLED_ON_FAULT);
	kbase_debug_csf_fault_notify(kbdev, kctx, DF_GPU_PAGE_FAULT);
	kbase_csf_ctx_report_page_fault_for_active_groups(kctx, fault);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	mutex_unlock(&kbdev->mmu_hw_mutex);
	/* AS transaction end */

	/* Switching to UNMAPPED mode above would have enabled the firmware to
	 * recover from the fault (if the memory access was made by firmware)
	 * and it can then respond to CSG termination requests to be sent now.
	 * All GPU command queue groups associated with the context would be
	 * affected as they use the same GPU address space.
	 */
	kbase_csf_ctx_handle_fault(kctx, fault);

	/* Clear down the fault */
	kbase_mmu_hw_clear_fault(kbdev, as,
			KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
	kbase_mmu_hw_enable_fault(kbdev, as,
			KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);

#ifndef MALI_STRIP_KBASE_DEVELOPMENT
#if IS_ENABLED(CONFIG_MALI_BUSLOG) && IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	queue_work(kbdev->buslog_callback_wq, &kbdev->buslog_callback_work);
#endif /* CONFIG_MALI_BUSLOG && CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
#endif /* MALI_STRIP_KBASE_DEVELOPMENT */
}

/**
 * kbase_mmu_interrupt_process() - Process a bus or page fault.
 * @kbdev:	The kbase_device the fault happened on
 * @kctx:	The kbase_context for the faulting address space if one was
 *		found.
 * @as:		The address space that has the fault
 * @fault:	Data relating to the fault
 *
 * This function will process a fault on a specific address space
 */
static void kbase_mmu_interrupt_process(struct kbase_device *kbdev,
		struct kbase_context *kctx, struct kbase_as *as,
		struct kbase_fault *fault)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	if (!kctx) {
		dev_warn(kbdev->dev, "%s in AS%d at 0x%016llx with no context present! Spurious IRQ or SW Design Error?\n",
				kbase_as_has_bus_fault(as, fault) ?
						"Bus error" : "Page fault",
				as->number, fault->addr);

		/* Since no ctx was found, the MMU must be disabled. */
		WARN_ON(as->current_setup.transtab);

		if (kbase_as_has_bus_fault(as, fault))
			kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
				GPU_COMMAND_CLEAR_FAULT);
		else if (kbase_as_has_page_fault(as, fault)) {
			kbase_mmu_hw_clear_fault(kbdev, as,
					KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
			kbase_mmu_hw_enable_fault(kbdev, as,
					KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
		}

		return;
	}

	if (kbase_as_has_bus_fault(as, fault)) {
		/*
		 * We need to switch to UNMAPPED mode - but we do this in a
		 * worker so that we can sleep
		 */
		if (!queue_work(as->pf_wq, &as->work_busfault))
		{
			dev_warn(kbdev->dev,
					"Bus fault is already pending for as %u", as->number);
			kbase_ctx_sched_release_ctx(kctx);
		}
		else
			atomic_inc(&kbdev->faults_pending);
	} else {
		if (!queue_work(as->pf_wq, &as->work_pagefault))
		{
			dev_warn(kbdev->dev,
					"Page fault is already pending for as %u", as->number);
			kbase_ctx_sched_release_ctx(kctx);
		}
		else
			atomic_inc(&kbdev->faults_pending);
	}
}

int kbase_mmu_bus_fault_interrupt(struct kbase_device *kbdev,
		u32 status, u32 as_nr)
{
	struct kbase_context *kctx;
	unsigned long flags;
	struct kbase_as *as;
	struct kbase_fault *fault;

	if (WARN_ON(as_nr == MCU_AS_NR))
		return -EINVAL;

	if (WARN_ON(as_nr >= BASE_MAX_NR_AS))
		return -EINVAL;

	as = &kbdev->as[as_nr];
	fault = &as->bf_data;
	fault->status = status;
	fault->addr = (u64) kbase_reg_read(kbdev,
		GPU_CONTROL_REG(GPU_FAULTADDRESS_HI)) << 32;
	fault->addr |= kbase_reg_read(kbdev,
		GPU_CONTROL_REG(GPU_FAULTADDRESS_LO));
	fault->protected_mode = false;

	/* report the fault to debugfs */
	kbase_as_fault_debugfs_new(kbdev, as_nr);

	kctx = kbase_ctx_sched_as_to_ctx_refcount(kbdev, as_nr);

	/* Process the bus fault interrupt for this address space */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_mmu_interrupt_process(kbdev, kctx, as, fault);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return 0;
}

void kbase_mmu_interrupt(struct kbase_device *kbdev, u32 irq_stat)
{
	const int num_as = 16;
	const int pf_shift = 0;
	const unsigned long as_bit_mask = (1UL << num_as) - 1;
	unsigned long flags;
	u32 new_mask;
	u32 tmp;
	u32 pf_bits = ((irq_stat >> pf_shift) & as_bit_mask);

	/* remember current mask */
	spin_lock_irqsave(&kbdev->mmu_mask_change, flags);
	new_mask = kbase_reg_read(kbdev, MMU_CONTROL_REG(MMU_IRQ_MASK));
	/* mask interrupts for now */
	kbase_reg_write(kbdev, MMU_CONTROL_REG(MMU_IRQ_MASK), 0);
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);

	while (pf_bits) {
		struct kbase_context *kctx;
		int as_no = ffs(pf_bits) - 1;
		struct kbase_as *as = &kbdev->as[as_no];
		struct kbase_fault *fault = &as->pf_data;

		/* find faulting address */
		fault->addr = kbase_reg_read(kbdev,
					     MMU_STAGE1_REG(MMU_AS_REG(as_no, AS_FAULTADDRESS_HI)));
		fault->addr <<= 32;
		fault->addr |= kbase_reg_read(
			kbdev, MMU_STAGE1_REG(MMU_AS_REG(as_no, AS_FAULTADDRESS_LO)));

		/* Mark the fault protected or not */
		fault->protected_mode = false;

		/* report the fault to debugfs */
		kbase_as_fault_debugfs_new(kbdev, as_no);

		/* record the fault status */
		fault->status =
			kbase_reg_read(kbdev, MMU_STAGE1_REG(MMU_AS_REG(as_no, AS_FAULTSTATUS)));

		fault->extra_addr =
			kbase_reg_read(kbdev, MMU_STAGE1_REG(MMU_AS_REG(as_no, AS_FAULTEXTRA_HI)));
		fault->extra_addr <<= 32;
		fault->extra_addr |=
			kbase_reg_read(kbdev, MMU_STAGE1_REG(MMU_AS_REG(as_no, AS_FAULTEXTRA_LO)));

		/* Mark page fault as handled */
		pf_bits &= ~(1UL << as_no);

		/* remove the queued PF from the mask */
		new_mask &= ~MMU_PAGE_FAULT(as_no);

		if (as_no == MCU_AS_NR) {
			kbase_mmu_report_mcu_as_fault_and_reset(kbdev, fault);
			/* Pointless to handle remaining faults */
			break;
		}

		/*
		 * Refcount the kctx - it shouldn't disappear anyway, since
		 * Page faults _should_ only occur whilst GPU commands are
		 * executing, and a command causing the Page fault shouldn't
		 * complete until the MMU is updated.
		 * Reference is released at the end of bottom half of page
		 * fault handling.
		 */
		kctx = kbase_ctx_sched_as_to_ctx_refcount(kbdev, as_no);

		/* Process the interrupt for this address space */
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		kbase_mmu_interrupt_process(kbdev, kctx, as, fault);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	}

	/* reenable interrupts */
	spin_lock_irqsave(&kbdev->mmu_mask_change, flags);
	tmp = kbase_reg_read(kbdev, MMU_CONTROL_REG(MMU_IRQ_MASK));
	new_mask |= tmp;
	kbase_reg_write(kbdev, MMU_CONTROL_REG(MMU_IRQ_MASK), new_mask);
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);
}

int kbase_mmu_switch_to_ir(struct kbase_context *const kctx,
	struct kbase_va_region *const reg)
{
	/* Can't soft-stop the provoking job */
	return -EPERM;
}

/**
 * kbase_mmu_gpu_fault_worker() - Process a GPU fault for the device.
 *
 * @data:  work_struct passed by queue_work()
 *
 * Report a GPU fatal error for all GPU command queue groups that are
 * using the address space and terminate them.
 */
static void kbase_mmu_gpu_fault_worker(struct work_struct *data)
{
	struct kbase_as *const faulting_as = container_of(data, struct kbase_as,
			work_gpufault);
	const u32 as_nr = faulting_as->number;
	struct kbase_device *const kbdev = container_of(faulting_as, struct
			kbase_device, as[as_nr]);
	struct kbase_fault *fault;
	struct kbase_context *kctx;
	u32 status;
	u64 address;
	u32 as_valid;
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	fault = &faulting_as->gf_data;
	status = fault->status;
	as_valid = status & GPU_FAULTSTATUS_JASID_VALID_MASK;
	address = fault->addr;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	dev_warn(kbdev->dev,
		 "GPU Fault 0x%08x (%s) in AS%u at 0x%016llx\n"
		 "ASID_VALID: %s,  ADDRESS_VALID: %s\n",
		 status, kbase_gpu_exception_name(GPU_FAULTSTATUS_EXCEPTION_TYPE_GET(status)),
		 as_nr, address, as_valid ? "true" : "false",
		 status & GPU_FAULTSTATUS_ADDRESS_VALID_MASK ? "true" : "false");

	kctx = kbase_ctx_sched_as_to_ctx(kbdev, as_nr);
	kbase_csf_ctx_handle_fault(kctx, fault);
	kbase_ctx_sched_release_ctx_lock(kctx);

	/* A work for GPU fault is complete.
	 * Till reaching here, no further GPU fault will be reported.
	 * Now clear the GPU fault to allow next GPU fault interrupt report.
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
			GPU_COMMAND_CLEAR_FAULT);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	atomic_dec(&kbdev->faults_pending);
}

/**
 * submit_work_gpufault() - Submit a work for GPU fault.
 *
 * @kbdev:    Kbase device pointer
 * @status:   GPU fault status
 * @as_nr:    Faulty address space
 * @address:  GPU fault address
 *
 * This function submits a work for reporting the details of GPU fault.
 */
static void submit_work_gpufault(struct kbase_device *kbdev, u32 status,
		u32 as_nr, u64 address)
{
	unsigned long flags;
	struct kbase_as *const as = &kbdev->as[as_nr];
	struct kbase_context *kctx;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kctx = kbase_ctx_sched_as_to_ctx_nolock(kbdev, as_nr);

	if (kctx) {
		kbase_ctx_sched_retain_ctx_refcount(kctx);

		as->gf_data = (struct kbase_fault) {
			.status = status,
			.addr = address,
		};

		if (WARN_ON(!queue_work(as->pf_wq, &as->work_gpufault)))
			kbase_ctx_sched_release_ctx(kctx);
		else
			atomic_inc(&kbdev->faults_pending);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_mmu_gpu_fault_interrupt(struct kbase_device *kbdev, u32 status,
		u32 as_nr, u64 address, bool as_valid)
{
	if (!as_valid || (as_nr == MCU_AS_NR)) {
		int as;

		/* Report GPU fault for all contexts (except MCU_AS_NR) in case either
		 * the address space is invalid or it's MCU address space.
		 */
		for (as = 1; as < kbdev->nr_hw_address_spaces; as++)
			submit_work_gpufault(kbdev, status, as, address);
	} else
		submit_work_gpufault(kbdev, status, as_nr, address);
}
KBASE_EXPORT_TEST_API(kbase_mmu_gpu_fault_interrupt);

int kbase_mmu_as_init(struct kbase_device *kbdev, unsigned int i)
{
	kbdev->as[i].number = i;
	kbdev->as[i].bf_data.addr = 0ULL;
	kbdev->as[i].pf_data.addr = 0ULL;
	kbdev->as[i].gf_data.addr = 0ULL;

	kbdev->as[i].pf_wq = alloc_workqueue("mali_mmu%d", WQ_UNBOUND, 0, i);
	if (!kbdev->as[i].pf_wq)
		return -ENOMEM;

	INIT_WORK(&kbdev->as[i].work_pagefault, kbase_mmu_page_fault_worker);
	INIT_WORK(&kbdev->as[i].work_busfault, kbase_mmu_bus_fault_worker);
	INIT_WORK(&kbdev->as[i].work_gpufault, kbase_mmu_gpu_fault_worker);

	return 0;
}
