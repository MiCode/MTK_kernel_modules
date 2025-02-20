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

/**
 * DOC: Base kernel MMU management specific for CSF GPU.
 */

#include <mali_kbase.h>
#include <gpu/mali_kbase_gpu_fault.h>
#include <mali_kbase_ctx_sched.h>
#include <mali_kbase_reset_gpu.h>
#include <mali_kbase_as_fault_debugfs.h>
#include <mmu/mali_kbase_mmu_internal.h>
#include <mmu/mali_kbase_mmu_faults_decoder.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
#include <platform/mtk_platform_common.h>
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
#include <ged_mali_event.h>
#include <platform/mtk_platform_common/mtk_platform_mali_event.h>
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */

void kbase_mmu_get_as_setup(struct kbase_mmu_table *mmut, struct kbase_mmu_setup *const setup)
{
	/* Set up the required caching policies at the correct indices
	 * in the memattr register.
	 */
	setup->memattr =
		(KBASE_MEMATTR_IMPL_DEF_CACHE_POLICY
		 << (KBASE_MEMATTR_INDEX_IMPL_DEF_CACHE_POLICY * 8)) |
		(KBASE_MEMATTR_FORCE_TO_CACHE_ALL << (KBASE_MEMATTR_INDEX_FORCE_TO_CACHE_ALL * 8)) |
		(KBASE_MEMATTR_WRITE_ALLOC << (KBASE_MEMATTR_INDEX_WRITE_ALLOC * 8)) |
		(KBASE_MEMATTR_AARCH64_OUTER_IMPL_DEF << (KBASE_MEMATTR_INDEX_OUTER_IMPL_DEF * 8)) |
		(KBASE_MEMATTR_AARCH64_OUTER_WA << (KBASE_MEMATTR_INDEX_OUTER_WA * 8)) |
		(KBASE_MEMATTR_AARCH64_NON_CACHEABLE << (KBASE_MEMATTR_INDEX_NON_CACHEABLE * 8)) |
		(KBASE_MEMATTR_AARCH64_SHARED << (KBASE_MEMATTR_INDEX_SHARED * 8));

	setup->transtab = (u64)mmut->pgd & AS_TRANSTAB_BASE_MASK;
	setup->transcfg = AS_TRANSCFG_MODE_SET(0ULL, AS_TRANSCFG_MODE_AARCH64_4K);
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
static void submit_work_pagefault(struct kbase_device *kbdev, u32 as_nr, struct kbase_fault *fault)
{
	unsigned long flags;
	struct kbase_as *const as = &kbdev->as[as_nr];
	struct kbase_context *kctx;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kctx = kbase_ctx_sched_as_to_ctx_nolock(kbdev, as_nr);

	if (kctx) {
		kbase_ctx_sched_retain_ctx_refcount(kctx);

		as->pf_data = (struct kbase_fault){
			.status = fault->status,
			.addr = fault->addr,
		};

		/*
		 * A page fault work item could already be pending for the
		 * context's address space, when the page fault occurs for
		 * MCU's address space.
		 */
		if (!queue_work(as->pf_wq, &as->work_pagefault)) {
			dev_dbg(kbdev->dev, "Page fault is already pending for as %u", as_nr);
			kbase_ctx_sched_release_ctx(kctx);
		} else {
			atomic_inc(&kbdev->faults_pending);
		}
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_mmu_report_mcu_as_fault_and_reset(struct kbase_device *kbdev, struct kbase_fault *fault)
{
	/* decode the fault status */
	u32 exception_type = fault->status & 0xFF;
	u32 access_type = (fault->status >> 8) & 0x3;
	u32 source_id = (fault->status >> 16);
	u32 as_no;

	/* terminal fault, print info about the fault */
	if (kbdev->gpu_props.gpu_id.product_model < GPU_ID_MODEL_MAKE(14, 0)) {
		dev_err(kbdev->dev,
			"Unexpected Page fault in firmware address space at VA 0x%016llX\n"
			"raw fault status: 0x%X\n"
			"exception type 0x%X: %s\n"
			"access type 0x%X: %s\n"
			"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n",
			fault->addr, fault->status, exception_type,
			kbase_gpu_exception_name(exception_type), access_type,
			kbase_gpu_access_type_name(fault->status), source_id,
			FAULT_SOURCE_ID_CORE_ID_GET(source_id),
			FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
			fault_source_id_internal_requester_get(kbdev, source_id),
			fault_source_id_core_type_description_get(kbdev, source_id),
			fault_source_id_internal_requester_get_str(kbdev, source_id, access_type));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Unexpected Page fault in firmware address space at VA 0x%016llX\n"
			"raw fault status: 0x%X\n"
			"exception type 0x%X: %s\n"
			"access type 0x%X: %s\n"
			"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n",
			fault->addr, fault->status, exception_type,
			kbase_gpu_exception_name(exception_type), access_type,
			kbase_gpu_access_type_name(fault->status), source_id,
			FAULT_SOURCE_ID_CORE_ID_GET(source_id),
			FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
			fault_source_id_internal_requester_get(kbdev, source_id),
			fault_source_id_core_type_description_get(kbdev, source_id),
			fault_source_id_internal_requester_get_str(kbdev, source_id, access_type));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}


#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, NULL, MTK_DBG_HOOK_MMU_UNEXPECTEDPAGEFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

	kbase_debug_csf_fault_notify(kbdev, NULL, DF_GPU_PAGE_FAULT);

	/* Report MMU fault for all address spaces (except MCU_AS_NR) */
	for (as_no = 1u; as_no < (u32)kbdev->nr_hw_address_spaces; as_no++)
		submit_work_pagefault(kbdev, as_no, fault);

	/* GPU reset is required to recover */
	if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_HWC_UNRECOVERABLE_ERROR)) {
#if IS_ENABLED(CONFIG_MALI_MTK_MBRAIN_SUPPORT)
		ged_mali_event_update_gpu_reset_nolock(GPU_RESET_CSF_MMU_PAGE_FAULT);
#endif /* CONFIG_MALI_MTK_MBRAIN_SUPPORT */
		kbase_reset_gpu(kbdev);
	}

}
KBASE_EXPORT_TEST_API(kbase_mmu_report_mcu_as_fault_and_reset);

void kbase_gpu_report_bus_fault_and_kill(struct kbase_context *kctx, struct kbase_as *as,
					 struct kbase_fault *fault)
{
	struct kbase_device *kbdev = kctx->kbdev;
	u32 const status = fault->status;
	unsigned int exception_type = (status & GPU_FAULTSTATUS_EXCEPTION_TYPE_MASK) >>
				      GPU_FAULTSTATUS_EXCEPTION_TYPE_SHIFT;
	unsigned int access_type = (status & GPU_FAULTSTATUS_ACCESS_TYPE_MASK) >>
				   GPU_FAULTSTATUS_ACCESS_TYPE_SHIFT;
	unsigned int source_id = (status & GPU_FAULTSTATUS_SOURCE_ID_MASK) >>
				 GPU_FAULTSTATUS_SOURCE_ID_SHIFT;
	const char *addr_valid = (status & GPU_FAULTSTATUS_ADDRESS_VALID_MASK) ? "true" : "false";
	unsigned int as_no = as->number;
	unsigned long flags;
	const uintptr_t fault_addr = fault->addr;

	/* terminal fault, print info about the fault */
	if (kbdev->gpu_props.gpu_id.product_model < GPU_ID_MODEL_MAKE(14, 0)) {
		dev_err(kbdev->dev,
			"GPU bus fault in AS%u at PA %pK\n"
			"PA_VALID: %s\n"
			"raw fault status: 0x%X\n"
			"exception type 0x%X: %s\n"
			"access type 0x%X: %s\n"
			"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n"
			"pid: %d\n",
			as_no, (void *)fault_addr, addr_valid, status, exception_type,
			kbase_gpu_exception_name(exception_type), access_type,
			kbase_gpu_access_type_name(access_type), source_id,
			FAULT_SOURCE_ID_CORE_ID_GET(source_id),
			FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
			fault_source_id_internal_requester_get(kbdev, source_id),
			fault_source_id_core_type_description_get(kbdev, source_id),
			fault_source_id_internal_requester_get_str(kbdev, source_id, access_type),
			kctx->pid);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"GPU bus fault in AS%u at PA %pK\n"
			"PA_VALID: %s\n"
			"raw fault status: 0x%X\n"
			"exception type 0x%X: %s\n"
			"access type 0x%X: %s\n"
			"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n"
			"pid: %d\n",
			as_no, (void *)fault_addr, addr_valid, status, exception_type,
			kbase_gpu_exception_name(exception_type), access_type,
			kbase_gpu_access_type_name(access_type), source_id,
			FAULT_SOURCE_ID_CORE_ID_GET(source_id),
			FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
			fault_source_id_internal_requester_get(kbdev, source_id),
			fault_source_id_core_type_description_get(kbdev, source_id),
			fault_source_id_internal_requester_get_str(kbdev, source_id, access_type),
			kctx->pid);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, kctx, MTK_DBG_HOOK_MMU_BUSFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

	/* AS transaction begin */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_mmu_disable(kctx);
	kbase_ctx_flag_set(kctx, KCTX_AS_DISABLED_ON_FAULT);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* Switching to UNMAPPED mode above would have enabled the firmware to
	 * recover from the fault (if the memory access was made by firmware)
	 * and it can then respond to CSG termination requests to be sent now.
	 * All GPU command queue groups associated with the context would be
	 * affected as they use the same GPU address space.
	 */
	kbase_csf_ctx_handle_fault(kctx, fault);

	/* Now clear the GPU fault */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(GPU_COMMAND), GPU_COMMAND_CLEAR_FAULT);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

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

	if (kbdev->protected_mode)
		return;

	if (!kbdev->pm.backend.gpu_powered)
		return;

	dev_err(kbdev->dev, "Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x84)) << 32));
	dev_err(kbdev->dev, "Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x84)) << 32));
	dev_err(kbdev->dev, "Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x84)) << 32));
	dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx \n",
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_COMP_REG(0x84)) << 32));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx \n",
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_FRAG_REG(0x84)) << 32));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx \n",
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x4)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x8)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xD0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xDC)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xA4)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0xA0)),
		kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x80)) | ((u64)kbase_reg_read32(kbdev, CSHW_IT_TILER_REG(0x84)) << 32));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
}

static void dump_hwif_registers(struct kbase_device *kbdev, int faulty_as)
{
	struct kbase_context *kctx;
	unsigned int i, j;
	int as_nr;
	u64 cmd_ptr;
	u64 cmd_ptr_end;

	if (kbdev->protected_mode)
		return;

	for (i = 0; kbdev->pm.backend.gpu_powered && (i < NR_HW_INTERFACES); i++) {
		cmd_ptr = kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x0)) |
			((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x4)) << 32);
		cmd_ptr_end = kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x8)) |
			((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xC)) << 32);
		as_nr = kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x34));

		if (!cmd_ptr)
			continue;

		dev_err(kbdev->dev, "Register dump of CSHWIF %d", i);
		dev_err(kbdev->dev, "CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x",
			cmd_ptr,
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x8)) | ((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xC)) << 32),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x24)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x34)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x60)) | ((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x64)) << 32),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x74)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x78)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x7C)));
		dev_err(kbdev->dev, "CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx",
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x80)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x98)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xA4)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xAC)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xB0)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xB8)) | ((u64)kbase_reg_read64(kbdev, CSHWIF_REG(i, 0xBC)) << 32));
		dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "Register dump of CSHWIF %d", i);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x",
			cmd_ptr,
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x8)) | ((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xC)) << 32),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x24)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x34)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x60)) | ((u64)kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x64)) << 32),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x74)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x78)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x7C)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx",
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x80)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0x98)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xA4)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xAC)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xB0)),
			kbase_reg_read32(kbdev, CSHWIF_REG(i, 0xB8)) | ((u64)kbase_reg_read64(kbdev, CSHWIF_REG(i, 0xBC)) << 32));
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
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm, kbdev->mmu_dbg[idx].mmu_op_type,
			kbdev->mmu_dbg[idx+1].time, kbdev->mmu_dbg[idx+1].pgds, kbdev->mmu_dbg[idx+1].va, kbdev->mmu_dbg[idx+1].tgid, kbdev->mmu_dbg[idx+1].id, kbdev->mmu_dbg[idx+1].as_nr, kbdev->mmu_dbg[idx+1].ipm, kbdev->mmu_dbg[idx+1].mmu_op_type,
			kbdev->mmu_dbg[idx+2].time, kbdev->mmu_dbg[idx+2].pgds, kbdev->mmu_dbg[idx+2].va, kbdev->mmu_dbg[idx+2].tgid, kbdev->mmu_dbg[idx+2].id, kbdev->mmu_dbg[idx+2].as_nr, kbdev->mmu_dbg[idx+2].ipm, kbdev->mmu_dbg[idx+2].mmu_op_type,
			kbdev->mmu_dbg[idx+3].time, kbdev->mmu_dbg[idx+3].pgds, kbdev->mmu_dbg[idx+3].va, kbdev->mmu_dbg[idx+3].tgid, kbdev->mmu_dbg[idx+3].id, kbdev->mmu_dbg[idx+3].as_nr, kbdev->mmu_dbg[idx+3].ipm, kbdev->mmu_dbg[idx+3].mmu_op_type,
			kbdev->mmu_dbg[idx+4].time, kbdev->mmu_dbg[idx+4].pgds, kbdev->mmu_dbg[idx+4].va, kbdev->mmu_dbg[idx+4].tgid, kbdev->mmu_dbg[idx+4].id, kbdev->mmu_dbg[idx+4].as_nr, kbdev->mmu_dbg[idx+4].ipm, kbdev->mmu_dbg[idx+4].mmu_op_type);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR,
			"[mmu] "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d "
			"%llu,%zu,%llx,%d,%d,%d,%d,%d\n",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm, kbdev->mmu_dbg[idx].mmu_op_type,
			kbdev->mmu_dbg[idx+1].time, kbdev->mmu_dbg[idx+1].pgds, kbdev->mmu_dbg[idx+1].va, kbdev->mmu_dbg[idx+1].tgid, kbdev->mmu_dbg[idx+1].id, kbdev->mmu_dbg[idx+1].as_nr, kbdev->mmu_dbg[idx+1].ipm, kbdev->mmu_dbg[idx+1].mmu_op_type,
			kbdev->mmu_dbg[idx+2].time, kbdev->mmu_dbg[idx+2].pgds, kbdev->mmu_dbg[idx+2].va, kbdev->mmu_dbg[idx+2].tgid, kbdev->mmu_dbg[idx+2].id, kbdev->mmu_dbg[idx+2].as_nr, kbdev->mmu_dbg[idx+2].ipm, kbdev->mmu_dbg[idx+2].mmu_op_type,
			kbdev->mmu_dbg[idx+3].time, kbdev->mmu_dbg[idx+3].pgds, kbdev->mmu_dbg[idx+3].va, kbdev->mmu_dbg[idx+3].tgid, kbdev->mmu_dbg[idx+3].id, kbdev->mmu_dbg[idx+3].as_nr, kbdev->mmu_dbg[idx+3].ipm, kbdev->mmu_dbg[idx+3].mmu_op_type,
			kbdev->mmu_dbg[idx+4].time, kbdev->mmu_dbg[idx+4].pgds, kbdev->mmu_dbg[idx+4].va, kbdev->mmu_dbg[idx+4].tgid, kbdev->mmu_dbg[idx+4].id, kbdev->mmu_dbg[idx+4].as_nr, kbdev->mmu_dbg[idx+4].ipm, kbdev->mmu_dbg[idx+4].mmu_op_type);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}

	/* Dump the rest of the records. */
	for (; idx < tail; idx++) {
#if !IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG)
		dev_err(kbdev->dev, "[mmu] %llu,%zu,%llx,%d,%d,%d,%d,%d",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm, kbdev->mmu_dbg[idx].mmu_op_type);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG_NO_KLOG */
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR,
			"[mmu] %llu,%zu,%llx,%d,%d,%d,%d,%d\n",
			kbdev->mmu_dbg[idx].time, kbdev->mmu_dbg[idx].pgds, kbdev->mmu_dbg[idx].va, kbdev->mmu_dbg[idx].tgid, kbdev->mmu_dbg[idx].id, kbdev->mmu_dbg[idx].as_nr, kbdev->mmu_dbg[idx].ipm, kbdev->mmu_dbg[idx].mmu_op_type);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

	mutex_unlock(&kbdev->mmu_debug_info_lock);
}
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

/*
 * The caller must ensure it's retained the ctx to prevent it from being
 * scheduled out whilst it's being worked on.
 */
void kbase_mmu_report_fault_and_kill(struct kbase_context *kctx, struct kbase_as *as,
				     const char *reason_str, struct kbase_fault *fault)
{
	unsigned long flags;
	struct kbase_device *kbdev = kctx->kbdev;
#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
	u32 csg_nr;
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */

	/* Make sure the context was active */
	if (WARN_ON(atomic_read(&kctx->refcount) <= 0))
		return;

	if (!kbase_ctx_flag(kctx, KCTX_PAGE_FAULT_REPORT_SKIP)) {
		const u32 status = fault->status;
		/* decode the fault status */
		unsigned int exception_type = AS_FAULTSTATUS_EXCEPTION_TYPE_GET(status);
		unsigned int access_type = AS_FAULTSTATUS_ACCESS_TYPE_GET(status);
		unsigned int source_id = AS_FAULTSTATUS_SOURCE_ID_GET(status);
		unsigned int as_no = as->number;

		/* terminal fault, print info about the fault */
		if (kbdev->gpu_props.gpu_id.product_model < GPU_ID_MODEL_MAKE(14, 0)) {
#if IS_ENABLED(CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG)
			dev_err(kbdev->dev,
				"Unhandled Page fault (p-mode %d) in AS%u at VA 0x%016llX\n"
				"Reason: %s\n"
				"raw fault status: 0x%X\n"
				"exception type 0x%X: %s\n"
				"access type 0x%X: %s\n"
				"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n"
				"ctx_id: %d_%d, pid: %d\n"
				"group_leader : %s, comm: %s\n",
				kbdev->protected_mode, as_no, fault->addr,
				reason_str,
				status,
				exception_type, kbase_gpu_exception_name(exception_type),
				access_type, kbase_gpu_access_type_name(status),
				source_id,
				FAULT_SOURCE_ID_CORE_ID_GET(source_id),
				FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
				fault_source_id_internal_requester_get(kbdev, source_id),
				fault_source_id_core_type_description_get(kbdev, source_id),
				fault_source_id_internal_requester_get_str(kbdev, source_id,
									   access_type),
				kctx->tgid, kctx->id, kctx->pid,
				kctx->group_leader_comm, kctx->comm);
#else
			dev_err(kbdev->dev,
				"Unhandled Page fault in AS%u at VA 0x%016llX\n"
				"Reason: %s\n"
				"raw fault status: 0x%X\n"
				"exception type 0x%X: %s\n"
				"access type 0x%X: %s\n"
				"source id 0x%X (core_id:utlb:IR 0x%X:0x%X:0x%X): %s, %s\n"
				"pid: %d\n",
				as_no, fault->addr, reason_str, status, exception_type,
				kbase_gpu_exception_name(exception_type), access_type,
				kbase_gpu_access_type_name(status), source_id,
				FAULT_SOURCE_ID_CORE_ID_GET(source_id),
				FAULT_SOURCE_ID_UTLB_ID_GET(source_id),
				fault_source_id_internal_requester_get(kbdev, source_id),
				fault_source_id_core_type_description_get(kbdev, source_id),
				fault_source_id_internal_requester_get_str(kbdev, source_id,
									   access_type),
				kctx->pid);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
		}

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Unhandled Page fault in AS%d at VA 0x%016llX\n"
			"Reason: %s\n"
			"raw fault status: 0x%X\n"
			"exception type 0x%X: %s\n"
			"access type 0x%X: %s\n"
			"source id 0x%X\n"
			"pid: %d\n",
			as_no, fault->addr, reason_str, status, exception_type,
			kbase_gpu_exception_name(exception_type), access_type,
			kbase_gpu_access_type_name(status), source_id, kctx->pid);
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

			spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
			//dump_hwif_registers(kbdev, as_no);
			//dump_iterator_registers(kbdev);
			spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
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

		mutex_unlock(&kbdev->register_check_lock);

		dump_mmu_teardown_records(kbdev);
#endif /* CONFIG_MALI_MTK_UNHANDLED_PAGE_FAULT_DEBUG */
	}
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_DUMP)
	mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, NULL, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, NULL, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, kctx, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
	mtk_common_debug(MTK_COMMON_DBG_DUMP_ENOP_METADATA, NULL, MTK_DBG_HOOK_MMU_UNHANDLEDPAGEFAULT);
#endif /* CONFIG_MALI_MTK_DEBUG_DUMP */

	/* AS transaction begin */

	/* switch to UNMAPPED mode,
	 * will abort all jobs and stop any hw counter dumping
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_mmu_disable(kctx);
	kbase_ctx_flag_set(kctx, KCTX_AS_DISABLED_ON_FAULT);
	kbase_debug_csf_fault_notify(kbdev, kctx, DF_GPU_PAGE_FAULT);
	kbase_csf_ctx_report_page_fault_for_active_groups(kctx, fault);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* AS transaction end */

	/* Switching to UNMAPPED mode above would have enabled the firmware to
	 * recover from the fault (if the memory access was made by firmware)
	 * and it can then respond to CSG termination requests to be sent now.
	 * All GPU command queue groups associated with the context would be
	 * affected as they use the same GPU address space.
	 */
	kbase_csf_ctx_handle_fault(kctx, fault);

	/* Clear down the fault */
	kbase_mmu_hw_clear_fault(kbdev, as, KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
	kbase_mmu_hw_enable_fault(kbdev, as, KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);

}

/**
 * kbase_mmu_interrupt_process() - Process a bus or page fault.
 * @kbdev:	The kbase_device the fault happened on
 * @kctx:	The kbase_context for the faulting address space if one was
 *		found.
 * @as:		The address space that has the fault
 * @fault:	Data relating to the fault
 *
 * This function will process a fault on a specific address space.
 * The function must be called with the ref_count of the kctx already increased/acquired.
 * If it fails to queue the work, the ref_count will be decreased.
 */
static void kbase_mmu_interrupt_process(struct kbase_device *kbdev, struct kbase_context *kctx,
					struct kbase_as *as, struct kbase_fault *fault)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	if (!kctx) {
		if (kbase_as_has_bus_fault(as, fault)) {
			dev_warn(
				kbdev->dev,
				"Bus error in AS%d at PA 0x%pK with no context present! Spurious IRQ or SW Design Error?\n",
				as->number, (void *)(uintptr_t)fault->addr);
		} else {
			dev_warn(
				kbdev->dev,
				"Page fault in AS%d at VA 0x%016llx with no context present! Spurious IRQ or SW Design Error?\n",
				as->number, fault->addr);
		}

		/* Since no ctx was found, the MMU must be disabled. */
		WARN_ON(as->current_setup.transtab);

		if (kbase_as_has_bus_fault(as, fault))
			kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(GPU_COMMAND),
					  GPU_COMMAND_CLEAR_FAULT);
		else if (kbase_as_has_page_fault(as, fault)) {
			kbase_mmu_hw_clear_fault(kbdev, as, KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
			kbase_mmu_hw_enable_fault(kbdev, as, KBASE_MMU_FAULT_TYPE_PAGE_UNEXPECTED);
		}

		return;
	}

	if (kbase_as_has_bus_fault(as, fault)) {
		/*
		 * We need to switch to UNMAPPED mode - but we do this in a
		 * worker so that we can sleep
		 */
		if (!queue_work(as->pf_wq, &as->work_busfault)) {
			dev_warn(kbdev->dev, "Bus fault is already pending for as %u", as->number);
			kbase_ctx_sched_release_ctx(kctx);
		} else {
			atomic_inc(&kbdev->faults_pending);
		}
	} else {
		if (!queue_work(as->pf_wq, &as->work_pagefault)) {
			dev_warn(kbdev->dev, "Page fault is already pending for as %u", as->number);
			kbase_ctx_sched_release_ctx(kctx);
		} else
			atomic_inc(&kbdev->faults_pending);
	}
}

int kbase_mmu_bus_fault_interrupt(struct kbase_device *kbdev, u32 status, u32 as_nr)
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
	fault->addr = kbase_reg_read64(kbdev, GPU_CONTROL_ENUM(GPU_FAULTADDRESS));
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
	new_mask = kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK));
	/* mask interrupts for now */
	kbase_reg_write32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK), 0);
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);

	while (pf_bits) {
		struct kbase_context *kctx;
		unsigned int as_no = (unsigned int)ffs((int)pf_bits) - 1;
		struct kbase_as *as = &kbdev->as[as_no];
		struct kbase_fault *fault = &as->pf_data;

		/* find faulting address */
		fault->addr = kbase_reg_read64(kbdev, MMU_AS_OFFSET(as_no, FAULTADDRESS));

		/* Mark the fault protected or not */
		fault->protected_mode = false;

		/* report the fault to debugfs */
		kbase_as_fault_debugfs_new(kbdev, as_no);

		/* record the fault status */
		fault->status = kbase_reg_read32(kbdev, MMU_AS_OFFSET(as_no, FAULTSTATUS));

		if (kbase_reg_is_valid(kbdev, MMU_AS_OFFSET(as_no, FAULTEXTRA)))
			fault->extra_addr =
				kbase_reg_read64(kbdev, MMU_AS_OFFSET(as_no, FAULTEXTRA));

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
	tmp = kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK));
	new_mask |= tmp;
	kbase_reg_write32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK), new_mask);
	spin_unlock_irqrestore(&kbdev->mmu_mask_change, flags);
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
	struct kbase_as *const faulting_as = container_of(data, struct kbase_as, work_gpufault);
	const u32 as_nr = faulting_as->number;
	struct kbase_device *const kbdev =
		container_of(faulting_as, struct kbase_device, as[as_nr]);
	struct kbase_fault *fault;
	struct kbase_context *kctx;
	u32 status;
	uintptr_t phys_addr;
	u32 as_valid;
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	fault = &faulting_as->gf_data;
	status = fault->status;
	as_valid = status & GPU_FAULTSTATUS_JASID_VALID_MASK;
	phys_addr = (uintptr_t)fault->addr;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	dev_warn(kbdev->dev,
		 "GPU Fault 0x%08x (%s) in AS%u at PA 0x%pK\n"
		 "ASID_VALID: %s,  ADDRESS_VALID: %s\n",
		 status, kbase_gpu_exception_name(GPU_FAULTSTATUS_EXCEPTION_TYPE_GET(status)),
		 as_nr, (void *)phys_addr, as_valid ? "true" : "false",
		 status & GPU_FAULTSTATUS_ADDRESS_VALID_MASK ? "true" : "false");

	kctx = kbase_ctx_sched_as_to_ctx(kbdev, as_nr);
	kbase_csf_ctx_handle_fault(kctx, fault);
	kbase_ctx_sched_release_ctx_lock(kctx);

	/* A work for GPU fault is complete.
	 * Till reaching here, no further GPU fault will be reported.
	 * Now clear the GPU fault to allow next GPU fault interrupt report.
	 */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(GPU_COMMAND), GPU_COMMAND_CLEAR_FAULT);
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
static void submit_work_gpufault(struct kbase_device *kbdev, u32 status, u32 as_nr, u64 address)
{
	unsigned long flags;
	struct kbase_as *const as = &kbdev->as[as_nr];
	struct kbase_context *kctx;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kctx = kbase_ctx_sched_as_to_ctx_nolock(kbdev, as_nr);

	if (kctx) {
		kbase_ctx_sched_retain_ctx_refcount(kctx);

		as->gf_data = (struct kbase_fault){
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

void kbase_mmu_gpu_fault_interrupt(struct kbase_device *kbdev, u32 status, u32 as_nr, u64 address,
				   bool as_valid)
{
	if (!as_valid || (as_nr == MCU_AS_NR)) {
		int as;

		/* Report GPU fault for all contexts (except MCU_AS_NR) in case either
		 * the address space is invalid or it's MCU address space.
		 */
		for (as = 1; as < kbdev->nr_hw_address_spaces; as++)
			submit_work_gpufault(kbdev, status, (u32)as, address);
	} else
		submit_work_gpufault(kbdev, status, as_nr, address);
}
KBASE_EXPORT_TEST_API(kbase_mmu_gpu_fault_interrupt);

int kbase_mmu_as_init(struct kbase_device *kbdev, unsigned int i)
{
	kbdev->as[i].number = i;

	kbdev->as[i].pf_wq = alloc_workqueue("mali_mmu%d", WQ_UNBOUND, 0, i);
	if (!kbdev->as[i].pf_wq)
		return -ENOMEM;

	INIT_WORK(&kbdev->as[i].work_pagefault, kbase_mmu_page_fault_worker);
	INIT_WORK(&kbdev->as[i].work_busfault, kbase_mmu_bus_fault_worker);
	INIT_WORK(&kbdev->as[i].work_gpufault, kbase_mmu_gpu_fault_worker);

	return 0;
}
