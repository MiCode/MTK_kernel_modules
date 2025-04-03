// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_sync.h>
#include <mali_kbase_mem_linux.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <platform/mtk_platform_common.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg_debugfs.h>
#include <csf/mali_kbase_csf_kcpu_debugfs.h>
#include <csf/mali_kbase_csf_cpu_queue_debugfs.h>
#include <csf/mali_kbase_csf.h>
#endif
#include "mtk_platform_debug.h"
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include <mtk_gpufreq.h>
#endif

static DEFINE_MUTEX(fence_debug_lock);
//#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
//void kbase_csf_dump_firmware_trace_buffer(struct kbase_device *kbdev);
//#endif

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
bool mtk_common_debug_logbuf_is_empty(struct mtk_debug_logbuf *logbuf)
{
	return logbuf->head == logbuf->tail;
}

bool mtk_common_debug_logbuf_is_full(struct mtk_debug_logbuf *logbuf)
{
	return logbuf->head == (logbuf->tail + 1) % logbuf->entry_num;
}

void mtk_common_debug_logbuf_clear(struct mtk_debug_logbuf *logbuf)
{
	unsigned long flags;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);
	logbuf->head = logbuf->tail = 0;
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

void mtk_common_debug_logbuf_print(struct mtk_debug_logbuf *logbuf, const char *fmt, ...)
{
	va_list args;
	unsigned long flags;
	uint8_t buffer[MTK_DEBUG_LOGBUF_ENTRY_SIZE];
	uint64_t ts_nsec = local_clock();
	uint32_t rem_nsec, offset;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (!logbuf->is_circular && mtk_common_debug_logbuf_is_full(logbuf))
		goto fail_overflow;

	offset = logbuf->tail * MTK_DEBUG_LOGBUF_ENTRY_SIZE;

	rem_nsec = do_div(ts_nsec, 1000000000);
	scnprintf(logbuf->entries + offset,
	          MTK_DEBUG_LOGBUF_ENTRY_SIZE, "[%5lu.%06lu] %s",
	          (uint32_t)ts_nsec, rem_nsec / 1000, buffer);

	/* Update the circular buffer indices */
	logbuf->tail = (logbuf->tail + 1) % logbuf->entry_num;
	if (logbuf->tail == logbuf->head)
		logbuf->head = (logbuf->head + 1) % logbuf->entry_num;

fail_overflow:
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

void mtk_common_debug_logbuf_dump(struct mtk_debug_logbuf *logbuf, struct seq_file *seq)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	uint32_t start, end, used_entry_num, offset;
	unsigned long flags;

	if (IS_ERR_OR_NULL(kbdev))
		return;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);

	start = logbuf->head;
	end = logbuf->tail;

	if (start > end)
		used_entry_num = logbuf->entry_num;
	else if (start == end)
		used_entry_num = 0;
	else
		used_entry_num = (end - start + 1);

	if (seq) {
		seq_printf(seq, "---------- %s (%d/%d) ----------\n",
		           logbuf->name, used_entry_num, logbuf->entry_num);
		while (start != end) {
			offset = start * MTK_DEBUG_LOGBUF_ENTRY_SIZE;
			seq_printf(seq, "%s\n", logbuf->entries + offset);
			start = (start + 1) % logbuf->entry_num;
		}
	} else {
		dev_info(kbdev->dev, "---------- %s (%d/%d) ----------",
				 logbuf->name, used_entry_num, logbuf->entry_num);
		while (start != end) {
			offset = start * MTK_DEBUG_LOGBUF_ENTRY_SIZE;
			dev_info(kbdev->dev, "%s", logbuf->entries + offset);
			start = (start + 1) % logbuf->entry_num;
		}
	}

	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

static void mtk_common_debug_logbuf_init(struct mtk_debug_logbuf *logbuf, uint32_t entry_num,
                                         bool is_circular, const char *name)
{
	if (name && entry_num) {
		spin_lock_init(&logbuf->access_lock);
		logbuf->tail = 0;
		logbuf->head = 0;
		logbuf->name[0] = '\0';
		logbuf->entry_num = entry_num;
		logbuf->is_circular = is_circular;
		logbuf->entries = kcalloc(entry_num, MTK_DEBUG_LOGBUF_ENTRY_SIZE, GFP_KERNEL);
		if (logbuf->entries)
			snprintf(logbuf->name, MTK_DEBUG_LOGBUF_NAME_LEN, "%s", name);
	}
}

static void mtk_common_debug_logbuf_term(struct mtk_debug_logbuf *logbuf)
{
	unsigned long flags;
	uint32_t i;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);
	if (logbuf->entries) {
		kfree(logbuf->entries);
		logbuf->entries = NULL;
	}
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

int mtk_common_debug_init(void)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_KBASE)
	mtk_common_debug_logbuf_init(&kbdev->logbuf_kbase,
	                             8192  /* entry_num */,
	                             true  /* is_circular */,
	                             "logbuf_kbase");
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_EXCEPTION)
	mtk_common_debug_logbuf_init(&kbdev->logbuf_exception,
	                             4096  /* entry_num */,
	                             false /* is_circular */,
	                             "logbuf_exception");
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_CSFFW)
	mtk_common_debug_logbuf_init(&kbdev->logbuf_csffw,
	                             65536 /* entry_num */,
	                             false /* is_circular */,
	                             "logbuf_csffw");
#endif

	return 0;
}

int mtk_common_debug_term(void)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_KBASE)
	mtk_common_debug_logbuf_term(&kbdev->logbuf_kbase);
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_EXCEPTION)
	mtk_common_debug_logbuf_term(&kbdev->logbuf_exception);
#endif
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG_LOGBUF_CSFFW)
	mtk_common_debug_logbuf_term(&kbdev->logbuf_csffw);
#endif

	return 0;
}
#endif

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
static const char *mtk_common_mcu_state_to_string(enum kbase_mcu_state state)
{
	const char *const strings[] = {
#define KBASEP_MCU_STATE(n) #n,
#include "mali_kbase_pm_mcu_states.h"
#undef KBASEP_MCU_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad MCU state";
	else
		return strings[state];
}
#else
static const char *mtk_common_core_state_to_string(enum kbase_shader_core_state state)
{
	const char *const strings[] = {
#define KBASEP_SHADER_STATE(n) #n,
#include "mali_kbase_pm_shader_states.h"
#undef KBASEP_SHADER_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad shader core state";
	else
		return strings[state];
}
#endif

static const char *mtk_common_l2_core_state_to_string(enum kbase_l2_core_state state)
{
	const char *const strings[] = {
#define KBASEP_L2_STATE(n) #n,
#include "mali_kbase_pm_l2_states.h"
#undef KBASEP_L2_STATE
	};
	if ((size_t)state >= ARRAY_SIZE(strings))
		return "Bad level 2 cache state";
	else
		return strings[state];
}

void mtk_common_debug_dump_status(void)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (!IS_ERR_OR_NULL(kbdev)) {
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
		dev_info(kbdev->dev, "[CSF] firmware_inited=%d firmware_reloaded=%d firmware_reload_needed=%d interrupt_received=%d",
		         kbdev->csf.firmware_inited,
		         kbdev->csf.firmware_reloaded,
		         kbdev->csf.firmware_reload_needed,
		         kbdev->csf.interrupt_received);
		dev_info(kbdev->dev, "[CSF] firmware_hctl_core_pwr=%d glb_init_request_pending=%d",
		         kbdev->csf.firmware_hctl_core_pwr,
		         kbdev->csf.glb_init_request_pending);
		dev_info(kbdev->dev, "[PM] in_reset=%d reset_done=%d gpu_powered=%d gpu_ready=%d mcu_state=%s l2_state=%s mcu_desired=%d l2_desired=%d l2_always_on=%d",
		         kbdev->pm.backend.in_reset,
		         kbdev->pm.backend.reset_done,
		         kbdev->pm.backend.gpu_powered,
		         kbdev->pm.backend.gpu_ready,
		         mtk_common_mcu_state_to_string(kbdev->pm.backend.mcu_state),
		         mtk_common_l2_core_state_to_string(kbdev->pm.backend.l2_state),
		         kbdev->pm.backend.mcu_desired,
		         kbdev->pm.backend.l2_desired,
		         kbdev->pm.backend.l2_always_on);
#if defined(CONFIG_MALI_MTK_DUMMY_CM)
		dev_info(kbdev->dev, "[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d debug_core_mask_en=%u",
		         kbdev->pm.backend.hwcnt_desired,
		         kbdev->pm.backend.hwcnt_disabled,
		         kbdev->pm.backend.poweroff_wait_in_progress,
		         kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
		         kbdev->pm.backend.poweron_required,
		         kbdev->pm.debug_core_mask_en);
#else
		dev_info(kbdev->dev, "[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d",
		         kbdev->pm.backend.hwcnt_desired,
		         kbdev->pm.backend.hwcnt_disabled,
		         kbdev->pm.backend.poweroff_wait_in_progress,
		         kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
		         kbdev->pm.backend.poweron_required);
#endif
#else
		dev_info(kbdev->dev, "[PM] in_reset=%d reset_done=%d gpu_powered=%d gpu_ready=%d shaders_state=%s l2_state=%s shaders_desired=%d l2_desired=%d l2_always_on=%d",
		         kbdev->pm.backend.in_reset,
		         kbdev->pm.backend.reset_done,
		         kbdev->pm.backend.gpu_powered,
		         kbdev->pm.backend.gpu_ready,
		         mtk_common_core_state_to_string(kbdev->pm.backend.shaders_state),
		         mtk_common_l2_core_state_to_string(kbdev->pm.backend.l2_state),
		         kbdev->pm.backend.shaders_desired,
		         kbdev->pm.backend.l2_desired,
		         kbdev->pm.backend.l2_always_on);
		dev_info(kbdev->dev, "[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d",
		         kbdev->pm.backend.hwcnt_desired,
		         kbdev->pm.backend.hwcnt_disabled,
		         kbdev->pm.backend.poweroff_wait_in_progress,
		         kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
		         kbdev->pm.backend.poweron_required);
#endif
	}
}

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
static const char *blocked_reason_to_string(u32 reason_id)
{
	/* possible blocking reasons of a cs */
	static const char *const cs_blocked_reason[] = {
		[CS_STATUS_BLOCKED_REASON_REASON_UNBLOCKED] = "UNBLOCKED",
		[CS_STATUS_BLOCKED_REASON_REASON_WAIT] = "WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_PROGRESS_WAIT] = "PROGRESS_WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_SYNC_WAIT] = "SYNC_WAIT",
		[CS_STATUS_BLOCKED_REASON_REASON_DEFERRED] = "DEFERRED",
		[CS_STATUS_BLOCKED_REASON_REASON_RESOURCE] = "RESOURCE",
		[CS_STATUS_BLOCKED_REASON_REASON_FLUSH] = "FLUSH"
	};

	if ((size_t)reason_id >= ARRAY_SIZE(cs_blocked_reason))
		return "UNKNOWN_BLOCKED_REASON_ID";

	return cs_blocked_reason[reason_id];
}

static void mtk_common_csf_scheduler_dump_active_queue_cs_status_wait(
	struct kbase_device *kbdev, pid_t tgid, u32 id,
	u32 wait_status, u32 wait_sync_value, u64 wait_sync_live_value,
	u64 wait_sync_pointer, u32 sb_status, u32 blocked_reason)
{
#define WAITING "Waiting"
#define NOT_WAITING "Not waiting"

	dev_info(kbdev->dev,
	         "[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
	         tgid,
	         id,
	         CS_STATUS_WAIT_SB_MASK_GET(wait_status),
	         CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
	         CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
	dev_info(kbdev->dev,
	         "[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
	         tgid,
	         id,
	         CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
	         CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ? "greater than" : "less or equal",
	         wait_sync_pointer);
	dev_info(kbdev->dev,
	         "[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
	         tgid,
	         id,
	         wait_sync_value,
	         wait_sync_live_value,
	         CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	dev_info(kbdev->dev,
	         "[%d_%d] BLOCKED_REASON: %s",
	         tgid,
	         id,
	         blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));

	// BLOCKED_REASON:
	//     - WAIT: Blocked on scoreboards in some way.
	//     - RESOURCE: Blocked on waiting for resource allocation. e.g., compute, tiler, and fragment resources.
	//     - SYNC_WAIT: Blocked on a SYNC_WAIT{32|64} instruction.

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	if (blocked_reason == CS_STATUS_BLOCKED_REASON_REASON_WAIT) {
		ged_log_buf_print2(kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
			 "    [%d_%d] BLOCKED_REASON: %s\n",
	         tgid,
	         id,
	         blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));
		mtk_common_debug_logbuf_print(&kbdev->logbuf_exception,
			"    [%d_%d] BLOCKED_REASON: %s",
			tgid, id, blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));
	}
#endif
}

static void mtk_common_csf_scheduler_dump_active_queue(pid_t tgid, u32 id, struct kbase_queue *queue)
{
	u32 *addr;
	u64 cs_extract;
	u64 cs_insert;
	u32 cs_active;
	u64 wait_sync_pointer;
	u32 wait_status, wait_sync_value;
	u32 sb_status;
	u32 blocked_reason;
	struct kbase_vmap_struct *mapping;
	u64 *evt;
	u64 wait_sync_live_value;

	if (!queue)
		return;

	if (queue->csi_index == KBASEP_IF_NR_INVALID || !queue->group)
		return;

	if (!queue->user_io_addr) {
		dev_info(queue->kctx->kbdev->dev,
			 "[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d",
			 tgid,
			 id,
			 queue->csi_index,
			 queue->base_addr,
			 queue->priority,
			 queue->doorbell_nr);
		return;
	}

	/* Ring the doorbell to have firmware update CS_EXTRACT */
	kbase_csf_ring_cs_user_doorbell(queue->kctx->kbdev, queue);
	msleep(100);

	addr = (u32 *)queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO/4] | ((u64)addr[CS_INSERT_HI/4] << 32);

	addr = (u32 *)(queue->user_io_addr + PAGE_SIZE);
	cs_extract = addr[CS_EXTRACT_LO/4] | ((u64)addr[CS_EXTRACT_HI/4] << 32);
	cs_active = addr[CS_ACTIVE/4];

	dev_info(queue->kctx->kbdev->dev,
			"[%d_%d] Bind Idx,     Ringbuf addr, Prio,    Insert offset,   Extract offset, Active, Doorbell",
			tgid,
			id);
	dev_info(queue->kctx->kbdev->dev,
			 "[%d_%d] %8d, %16llx, %4u, %16llx, %16llx, %6u, %8d",
			 tgid,
			 id,
			 queue->csi_index,
			 queue->base_addr,
			 queue->priority,
			 cs_insert,
			 cs_extract,
			 cs_active,
			 queue->doorbell_nr);

	/* if have command didn't complete print the last command's before and after 4 commands */
	if (cs_insert != cs_extract) {
		size_t size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;
		const unsigned int instruction_size = sizeof(u64);
		u64 start, stop, aligned_cs_extract;

		dev_info(queue->kctx->kbdev->dev,"Dumping instructions around the last Extract offset");

		aligned_cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

		/* Go 16 instructions back */
		if (aligned_cs_extract > (16 * instruction_size))
			start = aligned_cs_extract - (16 * instruction_size);
		else
			start = 0;

		/* Print upto 32 instructions */
		stop = start + (32 * instruction_size);
		if (stop > cs_insert)
			stop = cs_insert;

		dev_info(queue->kctx->kbdev->dev,"Instructions from Extract offset %llx\n",  start);

		while (start != stop) {
			u64 page_off = (start & size_mask) >> PAGE_SHIFT;
			u64 offset = (start & size_mask) & ~PAGE_MASK;
			struct page *page =
				as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			u64 *ringbuffer = kmap_atomic(page);
			u64 *ptr = &ringbuffer[offset/8];

			dev_info(queue->kctx->kbdev->dev,"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);

			kunmap_atomic(ringbuffer);
			start += (8 * instruction_size);
		}
	}

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		if (CS_STATUS_WAIT_SYNC_WAIT_GET(queue->status_wait)) {
			wait_status = queue->status_wait;
			wait_sync_value = queue->sync_value;
			wait_sync_pointer = queue->sync_ptr;
			sb_status = queue->sb_status;
			blocked_reason = queue->blocked_reason;

			evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
			if (evt) {
				wait_sync_live_value = evt[0];
				kbase_phy_alloc_mapping_put(queue->kctx, mapping);
			} else {
				wait_sync_live_value = U64_MAX;
			}

			mtk_common_csf_scheduler_dump_active_queue_cs_status_wait(
				queue->kctx->kbdev, tgid, id,
				wait_status, wait_sync_value,
				wait_sync_live_value, wait_sync_pointer,
				sb_status, blocked_reason);
		}
	} else {
		struct kbase_device const *const kbdev = queue->group->kctx->kbdev;
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[queue->group->csg_nr];
		struct kbase_csf_cmd_stream_info const *const stream = &ginfo->streams[queue->csi_index];
		u64 cmd_ptr;
		u32 req_res;

		if (!stream)
			return;

		cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_LO);
		cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_HI) << 32;
		req_res = kbase_csf_firmware_cs_output(stream, CS_STATUS_REQ_RESOURCE);

		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] CMD_PTR: 0x%llx",
		         tgid,
		         id,
		         cmd_ptr);
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [COMPUTE]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [FRAGMENT]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [TILER]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] REQ_RESOURCE [IDVS]: %d",
		         tgid,
		         id,
		         CS_STATUS_REQ_RESOURCE_IDVS_RESOURCES_GET(req_res));

		wait_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT);
		wait_sync_value = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_VALUE);
		wait_sync_pointer = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_LO);
		wait_sync_pointer |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_HI) << 32;

		sb_status = kbase_csf_firmware_cs_output(stream, CS_STATUS_SCOREBOARDS);
		blocked_reason = kbase_csf_firmware_cs_output( stream, CS_STATUS_BLOCKED_REASON);

		evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
		if (evt) {
			wait_sync_live_value = evt[0];
			kbase_phy_alloc_mapping_put(queue->kctx, mapping);
		} else {
			wait_sync_live_value = U64_MAX;
		}

		mtk_common_csf_scheduler_dump_active_queue_cs_status_wait(
			queue->kctx->kbdev, tgid, id,
			wait_status, wait_sync_value,
			wait_sync_live_value, wait_sync_pointer, sb_status,
			blocked_reason);
		/* Dealing with cs_trace */
		if (kbase_csf_scheduler_queue_has_trace(queue)) {
			u32 val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_LO);
			u64 addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_BASE_HI) << 32) | val;
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_SIZE);

			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u",
					 tgid,
					 id,
					 addr,
					 val);

			/* Write offset variable address (pointer) */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
			addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
					 tgid,
					 id,
					 addr);

			/* EVENT_SIZE and EVENT_STATEs */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
					 tgid,
					 id,
					 CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
					 CS_INSTR_CONFIG_EVENT_STATE_GET(val));
		} else {
			dev_info(queue->kctx->kbdev->dev,
			         "[%d_%d] NO CS_TRACE",
					 tgid,
					 id);
		}
	}
}

/* Waiting timeout for STATUS_UPDATE acknowledgment, in milliseconds */
#define CSF_STATUS_UPDATE_TO_MS (100)

static void mtk_common_csf_scheduler_dump_active_group(struct kbase_queue_group *const group)
{
	if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
		struct kbase_device *const kbdev = group->kctx->kbdev;
		unsigned long flags;
		u32 ep_c, ep_r;
		char exclusive;
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[group->csg_nr];
		long remaining = kbase_csf_timeout_in_jiffies(CSF_STATUS_UPDATE_TO_MS);
		u8 slot_priority = kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

		kbase_csf_scheduler_spin_lock(kbdev, &flags);
		kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ,
				~kbase_csf_firmware_csg_output(ginfo, CSG_ACK),
				CSG_REQ_STATUS_UPDATE_MASK);
		kbase_csf_scheduler_spin_unlock(kbdev, flags);
		kbase_csf_ring_csg_doorbell(kbdev, group->csg_nr);

		remaining = wait_event_timeout(kbdev->csf.event_wait,
			!((kbase_csf_firmware_csg_input_read(ginfo, CSG_REQ) ^
			   kbase_csf_firmware_csg_output(ginfo, CSG_ACK)) &
			   CSG_REQ_STATUS_UPDATE_MASK), remaining);

		ep_c = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_CURRENT);
		ep_r = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_REQ);

		if (CSG_STATUS_EP_REQ_EXCLUSIVE_COMPUTE_GET(ep_r))
			exclusive = 'C';
		else if (CSG_STATUS_EP_REQ_EXCLUSIVE_FRAGMENT_GET(ep_r))
			exclusive = 'F';
		else
			exclusive = '0';

		if (!remaining) {
			dev_info(kbdev->dev,
			         "[%d_%d] Timed out for STATUS_UPDATE on group %d on slot %d",
			         group->kctx->tgid,
			         group->kctx->id,
			         group->handle,
			         group->csg_nr);
		}

		dev_info(kbdev->dev,
		        "[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive",
		        group->kctx->tgid,
		        group->kctx->id);
		dev_info(kbdev->dev,
		        "[%d_%d] %7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c",
		        group->kctx->tgid,
		        group->kctx->id,
		        group->handle,
		        group->csg_nr,
		        slot_priority,
		        group->run_state,
		        group->priority,
		        CSG_STATUS_EP_CURRENT_COMPUTE_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_COMPUTE_EP_GET(ep_r),
		        CSG_STATUS_EP_CURRENT_FRAGMENT_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_FRAGMENT_EP_GET(ep_r),
		        CSG_STATUS_EP_CURRENT_TILER_EP_GET(ep_c),
		        CSG_STATUS_EP_REQ_TILER_EP_GET(ep_r),
		        exclusive);
	} else {
		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] GroupID, CSG NR, Run State, Priority",
		        group->kctx->tgid,
		        group->kctx->id);
		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] %7d, %6d, %9d, %8d",
		        group->kctx->tgid,
		        group->kctx->id,
		        group->handle,
		        group->csg_nr,
		        group->run_state,
		        group->priority);
	}

	if (group->run_state != KBASE_CSF_GROUP_TERMINATED) {
		unsigned int i;

		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] Bound queues:",
		        group->kctx->tgid,
		        group->kctx->id);

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			mtk_common_csf_scheduler_dump_active_queue(
				group->kctx->tgid,
				group->kctx->id,
				group->bound_queues[i]);
		}
	}
}
#endif

static const char *fence_timeout_type_to_string(int type)
{
#define FENCE_STATUS_TIMEOUT_TYPE_DEQUEUE 0x0
#define FENCE_STATUS_TIMEOUT_TYPE_QUEUE   0x1

	static const char *const fence_timeout_type[] = {
		[FENCE_STATUS_TIMEOUT_TYPE_DEQUEUE] = "DEQUEUE_BUFFER",
		[FENCE_STATUS_TIMEOUT_TYPE_QUEUE] = "QUEUE_BUFFER",
	};

	if ((size_t)type >= ARRAY_SIZE(fence_timeout_type))
		return "UNKNOWN";

	return fence_timeout_type[type];
}

void mtk_common_gpu_fence_debug_dump(int fd, int pid, int type, int timeouts)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return;

	lockdep_off();

	mutex_lock(&fence_debug_lock);

	dev_info(kbdev->dev, "@%s: %s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d",
	         __func__,
	         fence_timeout_type_to_string(type),
	         timeouts,
	         fd,
	         pid);

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
	ged_log_buf_print2(kbdev->ged_log_buf_hnd_kbase, GED_LOG_ATTR_TIME,
		 "%s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d\n",
		 fence_timeout_type_to_string(type),
		 timeouts,
		 fd,
		 pid);
	mtk_common_debug_logbuf_print(&kbdev->logbuf_exception,
		"%s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d",
		fence_timeout_type_to_string(type), timeouts, fd, pid);
#endif

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
	mutex_lock(&kbdev->kctx_list_lock);
	{
		struct kbase_context *kctx;

		list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
			if (kctx->tgid == pid) {
				mutex_lock(&kctx->csf.lock);
				kbase_csf_scheduler_lock(kbdev);
				// cat /sys/kernel/debug/mali0/active_groups
				// Print debug info for active GPU command queue groups
				{
				u32 csg_nr;
				u32 num_groups = kbdev->csf.global_iface.group_num;

				dev_info(kbdev->dev, "[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u\n", MALI_CSF_CSG_DEBUGFS_VERSION);

				for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
					struct kbase_queue_group *const group = kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

					if (!group)
						continue;

					dev_info(kbdev->dev, "[active_groups] ##### Ctx %d_%d #####", group->kctx->tgid, group->kctx->id);

					mtk_common_csf_scheduler_dump_active_group(group);
				}
				//kbase_csf_scheduler_unlock(kbdev);
				}

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/groups
				// Print per-context GPU command queue group debug information
				{
				u32 gr;

				for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
					struct kbase_queue_group *const group = kctx->csf.queue_groups[gr];

					if (!group)
						continue;

					dev_info(kbdev->dev, "[groups] ##### Ctx %d_%d #####", group->kctx->tgid, group->kctx->id);

					if (group)
						mtk_common_csf_scheduler_dump_active_group(group);
				}
				}
				//kbase_csf_scheduler_unlock(kbdev);

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/kcpu_queues
				// Print per-context KCPU queues debug information
				{
				unsigned long idx;

				dev_info(kbdev->dev, "[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u\n", MALI_CSF_CSG_DEBUGFS_VERSION);
				dev_info(kbdev->dev, "[kcpu_queues] ##### Ctx %d_%d #####", kctx->tgid, kctx->id);
				dev_info(kbdev->dev,
				         "[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Fence context  &  seqno",
				         kctx->tgid,
				         kctx->id);

				mutex_lock(&kctx->csf.kcpu_queues.lock);

				idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

				while (idx < KBASEP_MAX_KCPU_QUEUES) {
					struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];

					if (!queue) {
						idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
						continue;
					}

					dev_info(kbdev->dev,
					         "[%d_%d] %9lu( %s ), %16u, %11u, %7u, %13llu  %8u",
							kctx->tgid,
							kctx->id,
							idx,
							queue->has_error ? "InErr" : "NoErr",
							queue->num_pending_cmds,
							queue->enqueue_failed,
							queue->command_started ? 1 : 0,
							queue->fence_context,
							queue->fence_seqno);

					if (queue->command_started) {
						int i;
						for (i = 0; i < queue->num_pending_cmds; i++) {
						struct kbase_kcpu_command *cmd = &queue->commands[queue->start_offset + i];
						if (cmd->type < 0 || cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu,         %d, (unknown blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         cmd->type);
							continue;
						}
						switch (cmd->type) {
#if IS_ENABLED(CONFIG_SYNC_FILE)
						case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
						{
							struct kbase_sync_fence_info info;

							kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							dev_info(kbdev->dev,
										 "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
										 kctx->tgid,
										 kctx->id);
							dev_info(kbdev->dev,
										 "[%d_%d] %9lu( %s ),     Fence Signal, %pK %s %s",
										 kctx->tgid,
										 kctx->id,
										 idx,
										 queue->has_error ? "InErr" : "NoErr",
										 info.fence,
										 info.name,
										 kbase_sync_status_string(info.status));
							break;
						}
						case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
						{
							struct kbase_sync_fence_info info;

							kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu( %s ),     Fence Wait, %pK %s %s",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         info.fence,
							         info.name,
							         kbase_sync_status_string(info.status));
							break;
						}
#endif
						case BASE_KCPU_COMMAND_TYPE_CQS_SET:
						{
							unsigned int i;
							struct kbase_kcpu_command_cqs_set_info *sets = &cmd->info.cqs_set;

							for (i = 0; i < sets->nr_objs; i++) {
								dev_info(kbdev->dev,
								         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								        "[%d_%d] %9lu,       CQS Set %llx",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         sets->objs[i].addr);
							}
							break;
						}
						case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
						{
							unsigned int i;
							struct kbase_kcpu_command_cqs_wait_info *waits = &cmd->info.cqs_wait;

							for (i = 0; i < waits->nr_objs; i++) {
								struct kbase_vmap_struct *mapping;
								u32 val;
								char const *msg;
								u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);

								if (!cpu_ptr)
									break;

								val = *cpu_ptr;
								kbase_phy_alloc_mapping_put(kctx, mapping);

								msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";

								dev_info(kbdev->dev,
								         "[%d_%d] Queue Idx(err-mode), Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								         "[%d_%d] %9lu( %s ),       CQS Wait, %llx(%u > %u, inherit_err: %s)",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         queue->has_error ? "InErr" : "NoErr",
								         waits->objs[i].addr,
								         val,
								         waits->objs[i].val,
								         msg);
							}
							break;
						}
						default:
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu,         %d, (other blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         cmd->type);
							break;
						}
						}
					}

					idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
				}

				mutex_unlock(&kctx->csf.kcpu_queues.lock);
				}

				// dump firmware trace buffer
				//kbase_csf_dump_firmware_trace_buffer(kbdev);
				// dump ktrace log
				KBASE_KTRACE_DUMP(kbdev);

				// cat /sys/kernel/debug/mali0/ctx/{PID}_*/cpu_queue
				// Print per-context CPU queues debug information
				{
				if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
						BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
					dev_info(kbdev->dev, "[%d_%d] Dump request already started! (try again)", kctx->tgid, kctx->id);
					kbase_csf_scheduler_unlock(kbdev);
					mutex_unlock(&kctx->csf.lock);
					mutex_unlock(&kbdev->kctx_list_lock);
					mutex_unlock(&fence_debug_lock);
					return;
				}

				atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
				init_completion(&kctx->csf.cpu_queue.dump_cmp);
				kbase_event_wakeup(kctx);
				//mutex_unlock(&kctx->csf.lock);

				kbase_csf_scheduler_unlock(kbdev);
				mutex_unlock(&kctx->csf.lock);

				dev_info(kbdev->dev, "[cpu_queue] CPU Queues table (version:v%u):", MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
				dev_info(kbdev->dev, "[cpu_queue] ##### Ctx %d_%d #####", kctx->tgid, kctx->id);

				wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp,
						msecs_to_jiffies(3000));

				mutex_lock(&kctx->csf.lock);
				kbase_csf_scheduler_lock(kbdev);

				if (kctx->csf.cpu_queue.buffer) {
					WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
							    BASE_CSF_CPU_QUEUE_DUMP_PENDING);

					dev_info(kbdev->dev, "[%d_%d] %s", kctx->tgid, kctx->id, kctx->csf.cpu_queue.buffer);

					kfree(kctx->csf.cpu_queue.buffer);
					kctx->csf.cpu_queue.buffer = NULL;
					kctx->csf.cpu_queue.buffer_size = 0;
				}
				else
					dev_info(kbdev->dev, "[%d_%d] Dump error! (time out)", kctx->tgid, kctx->id);

				atomic_set(&kctx->csf.cpu_queue.dump_req_status,
					BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
				}

				kbase_csf_scheduler_unlock(kbdev);
				mutex_unlock(&kctx->csf.lock);
			}
		}
	}

	mutex_unlock(&kbdev->kctx_list_lock);
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
	if (!mtk_common_gpufreq_bringup()) {
		if (kbdev->pm.backend.gpu_powered)
			gpufreq_dump_infra_status();
		mtk_common_debug_dump();
	}
#endif

	mutex_unlock(&fence_debug_lock);

	lockdep_on();
}
