// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_sync.h>
#include <mali_kbase_mem_linux.h>
#include <mali_kbase_reset_gpu.h>
#include <linux/delay.h>
#include <platform/mtk_platform_common.h>
#include <backend/gpu/mali_kbase_pm_defs.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
#include <csf/mali_kbase_csf_csg_debugfs.h>
#include <csf/mali_kbase_csf_kcpu_debugfs.h>
#include <csf/mali_kbase_csf_cpu_queue_debugfs.h>
#include <csf/mali_kbase_csf_cpu_queue.h>
#include <csf/mali_kbase_csf.h>
#endif
#include <platform/mtk_platform_common/mtk_platform_qinspect.h>

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>

#define qinspect_err(kbdev, fmt, args...) mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR, fmt "\n", ##args)

#define QINSPECT_DBG 0

#if QINSPECT_DBG

#define qinspect_dbg(kbdev, fmt, args...) mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR, fmt "\n", ##args)

#else

#define qinspect_dbg(...) do { } while (0)

#endif /* QINSPECT_DBG */

#else

#define qinspect_err(...) do { } while (0)

#define qinspect_dbg(...) do { } while (0)

#endif /* IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER) */

static int mtk_qinspect_mutex_trylock(struct mutex *lock)
{
	int count = 3;
	int ret;

	ret = mutex_trylock(lock);
	while (!ret && --count) {
		msleep(1);
		ret = mutex_trylock(lock);
	}

	return ret;
}

int mtk_qinspect_get_cqs_value(struct kbase_context *kctx, u64 addr, u32 *val)
{
	struct kbase_vmap_struct *mapping;
	u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, addr, &mapping);

	if (!cpu_ptr)
		return -1;

	*val = *cpu_ptr;
	kbase_phy_alloc_mapping_put(kctx, mapping);

	return 0;
}

int mtk_qinspect_get_cqs_value64(struct kbase_context *kctx, u64 addr, u64 *val64)
{
	struct kbase_vmap_struct *mapping;
	u64 *const cpu_ptr = (u64 *)kbase_phy_alloc_mapping_get(kctx, addr, &mapping);

	if (!cpu_ptr)
		return -1;

	*val64 = *cpu_ptr;
	kbase_phy_alloc_mapping_put(kctx, mapping);

	return 0;
}

static void *mtk_qinspect_gpuq_internal_map_cpu_addr(struct kbase_context *kctx, u64 gpu_addr)
{
	struct kbase_va_region *reg;
	u64 pfn = gpu_addr >> PAGE_SHIFT;
	u64 offset;
	struct page *page;
	void *cpu_addr;
	pgprot_t prot = PAGE_KERNEL;

	reg = kbase_region_tracker_find_region_enclosing_address(kctx, gpu_addr);
	if (reg == NULL || reg->gpu_alloc == NULL)
		/* Empty region - ignore */
		return NULL;

	if (reg->flags & KBASE_REG_PROTECTED)
		/* CPU access to protected memory is forbidden - so
		 * skip this GPU virtual region.
		 */
		return NULL;

	offset = pfn - reg->start_pfn;
	if (offset >= reg->gpu_alloc->nents)
		return NULL;

	if (!(reg->flags & KBASE_REG_CPU_CACHED))
		prot = pgprot_writecombine(prot);

	page = as_page(reg->gpu_alloc->pages[offset]);
	cpu_addr = vmap(&page, 1, VM_MAP, prot);

	return cpu_addr;
}

static int mtk_qinspect_gpuq_internal_get_u64(struct kbase_context *kctx, u64 gpu_addr, u64 *val)
{
	u64 *cpu_addr;

	if (gpu_addr & 0x7)
		return -1;

	cpu_addr = (u64 *)mtk_qinspect_gpuq_internal_map_cpu_addr(kctx, gpu_addr & PAGE_MASK);
	if (!cpu_addr)
		return -1;

	*val = cpu_addr[(gpu_addr & ~PAGE_MASK) >> 3];
	vunmap(cpu_addr);

	return 0;
}

/*
 * cpuq helper
 */

static int mtk_qinspect_cpuq_internal_dump(struct kbase_context *kctx, enum mtk_base_csf_notification_dump_cmd dump_cmd)
{
	unsigned long timeout;

	lockdep_assert_held(&kctx->csf.lock);

#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
		qinspect_err(kctx->kbdev,
			"[qinspect] cpuq_internal_dump: Ctx %d_%d dump request already started!",
			kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
		mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
		return -EBUSY;
	}

	atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
	kctx->csf.cpu_queue.dump_cmd = dump_cmd;
	init_completion(&kctx->csf.cpu_queue.dump_cmp);
	kbase_event_wakeup(kctx);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	mutex_unlock(&kctx->csf.lock);

	timeout = wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp, msecs_to_jiffies(3000));

	mutex_lock(&kctx->csf.lock);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	if (kctx->csf.cpu_queue.buffer) {
		WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_PENDING);

		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_DONE);
	} else {
		qinspect_err(kctx->kbdev,
			"[qinspect] cpuq_internal_dump: Ctx %d_%d dump error! (timeout = %lu)",
			kctx->tgid, kctx->id, timeout);
		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_DONE);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
		mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
		return -EBUSY;
	}
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	return 0;
}

union mtk_qinspect_cpu_command_buf *mtk_qinspect_cpuq_internal_get_buf(struct kbase_context *kctx)
{
	union mtk_qinspect_cpu_command_buf *queue_buf;
	int num_bufs;
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	do {
		num_bufs = kctx->csf.cpu_queue.buffer_size / sizeof(union mtk_qinspect_cpu_command_buf);
		if (!kctx->csf.cpu_queue.buffer || num_bufs <= 0) {
			queue_buf = NULL;
			break;
		}

		queue_buf = (union mtk_qinspect_cpu_command_buf *)kctx->csf.cpu_queue.buffer;
		if (queue_buf->queue.tag != MTK_QINSPECT_CPU_QUEUE_TAG) {
			queue_buf = NULL;
			break;
		}
	} while (false);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	return queue_buf;
}

static int mtk_qinspect_cpuq_internal_load_cpuq_internal(struct kbase_context *kctx)
{
	union mtk_qinspect_cpu_command_buf *buf;
	struct mtk_qinspect_cpu_command_queue *queue = NULL;
	int num_queues = 0;
	int num_bufs;
	int num_used_bufs = 0;
	bool terminated_existed = false;

	if (mtk_qinspect_cpuq_internal_dump(kctx, MTK_BASE_CSF_CPU_QUEUE_DUMP_RAW))
		return -1;

	buf = mtk_qinspect_cpuq_internal_get_buf(kctx);
	if (!buf)
		return -1;

#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
    mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	num_bufs = kctx->csf.cpu_queue.buffer_size / sizeof(union mtk_qinspect_cpu_command_buf);
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
    mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/

	while (num_used_bufs < num_bufs) {
		struct mtk_qinspect_cpu_command_queue *queue = &(buf[num_used_bufs].queue);
		int cmd_idx;

		/* check tag, queue, number of commands and buffer size */
		if (queue->tag != MTK_QINSPECT_CPU_QUEUE_TAG)
			return -1;		/* error tagging */
		if (queue->queue == 0) {	/* terminated command */
			terminated_existed = true;
			break;
		}
		if (queue->num_cmds >= queue->num_bufs || (num_used_bufs + queue->num_bufs) > num_bufs)
			return -1;
		num_used_bufs++;		/* queue loaded */

		for (cmd_idx = 0; cmd_idx < queue->num_cmds; cmd_idx++) {
			struct mtk_qinspect_cpu_command *command;

			/* load command */
			if (num_used_bufs >= num_bufs)				/* buffer shortage */
				return -1;
			command = &(buf[num_used_bufs].command);
			num_used_bufs++;

			/* load objects */
			if ((num_used_bufs + command->num_objs) > num_bufs)	/* buffer shortage */
				return -1;
			command->objs = &(buf[num_used_bufs]);
			command->kctx = kctx;
			num_used_bufs += command->num_objs;
		}

		num_queues++;
	}

	if (!terminated_existed && (num_used_bufs + 1) != num_bufs)
		/* terminated is not the last command or not existed */
		return -1;

	return num_queues;
}

int mtk_qinspect_cpuq_internal_load_cpuq(struct kbase_context *kctx)
{
	int num_queues = mtk_qinspect_cpuq_internal_load_cpuq_internal(kctx);

	if (num_queues < 0)
		mtk_qinspect_cpuq_internal_unload_cpuq(kctx);

	return num_queues;
}

void mtk_qinspect_cpuq_internal_unload_cpuq(struct kbase_context *kctx)
{
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
	WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_DONE);
	if (kctx->csf.cpu_queue.buffer) {
		kfree(kctx->csf.cpu_queue.buffer);
		kctx->csf.cpu_queue.buffer = NULL;
		kctx->csf.cpu_queue.buffer_size = 0;
		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
	}
#if IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT*/
}

union mtk_qinspect_cpu_command_buf *mtk_qinspect_search_cpuq_internal_by_queue_addr(struct kbase_context *kctx,
	u64 queue_addr)
{
	union mtk_qinspect_cpu_command_buf *queue;

	queue = mtk_qinspect_cpuq_internal_get_buf(kctx);
	if (!queue)
		return NULL;

	for (; queue->queue.queue; queue += queue->queue.num_bufs)
		if (queue->queue.queue == queue_addr)
			return queue;

	return NULL;
}

/*
 * gpuq helper
 */

#define QINSPECT_CSF_CACHE_LINE_SIZE 64
#define QINSPECT_CSF_CACHE_LINE_MASK (~(QINSPECT_CSF_CACHE_LINE_SIZE - 1))

#define MTK_QINSPECT_MAX_CALL_STACK_DEPTH 8

#define MTK_QINSPECT_CSF_REG_NUM_MAX 128

struct mtk_qinspect_csf_register_file {
	u64 valid_map[(MTK_QINSPECT_CSF_REG_NUM_MAX + (sizeof(u64) * 8 - 1)) / (sizeof(u64) * 8)];
	union {
		u32 reg32[MTK_QINSPECT_CSF_REG_NUM_MAX];
		u64 reg64[MTK_QINSPECT_CSF_REG_NUM_MAX / 2];
	};
};

static u64 mtk_qinspect_csf_reg_num;

struct mtk_qinspect_gpuq_it {
	union mtk_qinspect_csf_instruction inst;
	union {
		u64 cqs_addr;		/* for cqs set/add */
		u8 shared_entry;	/* for shared sb inc */
	};
	u64 value;

	/* gpu_queue search record */
	struct {
		u64 gpu_addr;
		void *cpu_addr;
	} gpu_map_cache[MTK_QINSPECT_MAX_CALL_STACK_DEPTH];
};

#define MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_SIZE (128 * PAGE_SIZE)
#define MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_COUNT 512
static unsigned int mtk_qinspect_gpuq_search_countdown;

static union mtk_qinspect_csf_instruction mtk_qinspect_gpuq_internal_search_inst(struct kbase_queue *queue,
	struct mtk_qinspect_gpuq_it *gpuq_it, struct mtk_qinspect_csf_register_file *rf,
	unsigned int depth, u64 start, u64 end, union mtk_qinspect_csf_instruction *inst_list);

static inline int rf_reg_load(struct mtk_qinspect_csf_register_file *rf, u64 reg, u32 *val)
{
	if (reg >= mtk_qinspect_csf_reg_num
		|| (rf->valid_map[reg / 64] & (((u64)0x1) << (reg % 64))) != (((u64)0x1) << (reg % 64)))
		return -1;

	*val = rf->reg32[reg];

	return 0;
}

static inline int rf_reg_load64(struct mtk_qinspect_csf_register_file *rf, u64 reg, u64 *val)
{
	if (reg >= mtk_qinspect_csf_reg_num || (reg & 1)
		|| (rf->valid_map[reg / 64] & (((u64)0x3) << (reg % 64))) != (((u64)0x3) << (reg % 64)))
		return -1;

	*val = rf->reg64[reg >> 1];

	return 0;
}

static inline int rf_reg_store(struct mtk_qinspect_csf_register_file *rf, u64 reg, u32 val)
{
	if (reg >= mtk_qinspect_csf_reg_num)
		return -1;

	rf->valid_map[reg / 64] |= ((u64)0x1) << (reg % 64);
	rf->reg32[reg] = val;

	return 0;
}

static inline int rf_reg_store64(struct mtk_qinspect_csf_register_file *rf, u64 reg, u64 val)
{
	if ((reg >= mtk_qinspect_csf_reg_num) || (reg & 1))
		return -1;

	rf->valid_map[reg / 64] |= ((u64)0x3) << (reg % 64);
	rf->reg64[reg >> 1] = val;

	return 0;
}

static union mtk_qinspect_csf_instruction mtk_qinspect_gpuq_internal_decode_inst(struct kbase_queue *queue,
	struct mtk_qinspect_gpuq_it *gpuq_it, struct mtk_qinspect_csf_register_file *rf,
	unsigned int depth, u64 start, u64 end, union mtk_qinspect_csf_instruction *inst_list)
{
	union mtk_qinspect_csf_instruction *inst = (union mtk_qinspect_csf_instruction *)start;
	union mtk_qinspect_csf_instruction *target, ret;
	u64 reg;
	u32 size, val;
	u64 buffer, val64;

	for (; (u64)inst < end; inst++) {
		switch (inst->inst.opcode) {
		case 0b00000001:			/* MOVE */
			rf_reg_store64(rf, inst->move.dest, inst->move.imm);
			break;
		case 0b00000010:			/* MOVE32 */
			rf_reg_store(rf, inst->move32.dest, (u32)inst->move32.imm);
			break;
		case 0b00100000:			/* CALL */
			if (rf_reg_load(rf, inst->call.src1, &size) || !size)
				break;
			if (rf_reg_load64(rf, inst->call.src0, &buffer) || !buffer || (buffer & 0x07))
				break;
			ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, rf, depth + 1,
				buffer, buffer + size, inst_list);
			if (ret.inst.opcode != 0)
				return ret;
			break;
		default:
			for (target = inst_list; target->inst.opcode != 0; target++) {
				if (inst->inst.opcode != target->inst.opcode)
					continue;
				switch (inst->inst.opcode) {
				case 0b00100101:	/* SYNC_ADD32 */
				case 0b00100110:	/* SYNC_SET32 */
					if (!rf_reg_load64(rf, inst->sync_set.src0, &val64)
						&& val64 == gpuq_it->cqs_addr) {
						if (rf_reg_load(rf, inst->sync_set.src1, &val))
							gpuq_it->value = 0xffffffffffffffff;
						else
							gpuq_it->value = (u64)val;
						gpuq_it->inst = *inst;
						return *inst;
					}
					break;
				case 0b00110011:	/* SYNC_ADD64 */
				case 0b00110100:	/* SYNC_SET64 */
					if (!rf_reg_load64(rf, inst->sync_set.src0, &val64)
						&& val64 == gpuq_it->cqs_addr) {
						if (rf_reg_load64(rf, inst->sync_set.src1, &val64))
							gpuq_it->value = 0xffffffffffffffff;
						else
							gpuq_it->value = val64;
						gpuq_it->inst = *inst;
						return *inst;
					}
					break;
				case 0b00011110:	/* SHARED_SB_INC */
					if (inst->shared_sb_inc.se == (u64)gpuq_it->shared_entry) {
						gpuq_it->inst = *inst;
						return *inst;
					}
					break;
				default:
					break;
				}
			}
			break;
		}
	}

	return (union mtk_qinspect_csf_instruction){{.opcode = 0}};
}

static union mtk_qinspect_csf_instruction mtk_qinspect_gpuq_internal_search_inst(struct kbase_queue *queue,
	struct mtk_qinspect_gpuq_it *gpuq_it, struct mtk_qinspect_csf_register_file *rf,
	unsigned int depth, u64 start, u64 end, union mtk_qinspect_csf_instruction *inst_list)
{
	u64 page_addr;
	u64 cpu_addr;
	u64 offset, size, chunk_size;
	union mtk_qinspect_csf_instruction ret;

	if (!mtk_qinspect_gpuq_search_countdown) {
		qinspect_err(queue->kctx->kbdev,
			"[qinspect] gpuq_internal_search_inst: Ctx %d_%d Csi_%d hit maximal buf count (%d)",
			queue->kctx->tgid, queue->kctx->id, queue->csi_index, MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_COUNT);
		return (union mtk_qinspect_csf_instruction){{.opcode = 0xff}};
	}
	mtk_qinspect_gpuq_search_countdown--;

	if (depth >= MTK_QINSPECT_MAX_CALL_STACK_DEPTH) {
		qinspect_err(queue->kctx->kbdev,
			"[qinspect] gpuq_internal_search_inst: Ctx %d_%d Csi_%d hit maximal call stack depth (%d)",
			queue->kctx->tgid, queue->kctx->id, queue->csi_index, MTK_QINSPECT_MAX_CALL_STACK_DEPTH);
		return (union mtk_qinspect_csf_instruction){{.opcode = 0xff}};
	}

	/* decode buffers, using page as the decode unit */
	page_addr = start & PAGE_MASK;
	offset = start - page_addr;
	size = end - start;
	chunk_size = PAGE_SIZE - offset;

	if (size > MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_SIZE) {
		qinspect_err(queue->kctx->kbdev,
			"[qinspect] gpuq_internal_search_inst: Ctx %d_%d Csi_%d hit maximal buf size (0x%lx)",
			queue->kctx->tgid, queue->kctx->id, queue->csi_index, MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_SIZE);
		return (union mtk_qinspect_csf_instruction){{.opcode = 0xff}};
	}

	while (size) {
		if (chunk_size > size)
			chunk_size = size;

		if (gpuq_it->gpu_map_cache[depth].cpu_addr && gpuq_it->gpu_map_cache[depth].gpu_addr == page_addr)
			cpu_addr = (u64)gpuq_it->gpu_map_cache[depth].cpu_addr;
		else {
			cpu_addr = (u64)mtk_qinspect_gpuq_internal_map_cpu_addr(queue->kctx, page_addr);
			if (cpu_addr) {
				if (gpuq_it->gpu_map_cache[depth].cpu_addr)
					vunmap(gpuq_it->gpu_map_cache[depth].cpu_addr);
				gpuq_it->gpu_map_cache[depth].gpu_addr = page_addr;
				gpuq_it->gpu_map_cache[depth].cpu_addr = (void *)cpu_addr;
			}
		}

		if (likely(cpu_addr)) {
			ret = mtk_qinspect_gpuq_internal_decode_inst(queue, gpuq_it, rf, depth,
				cpu_addr + offset, cpu_addr + offset + chunk_size, inst_list);
			if (ret.inst.opcode != 0)
				return ret;
		}

		page_addr += PAGE_SIZE;
		offset = 0;
		size -= chunk_size;
		chunk_size = PAGE_SIZE;
	}

	return (union mtk_qinspect_csf_instruction){{.opcode = 0}};
}

static struct mtk_qinspect_gpuq_it *mtk_qinspect_gpuq_internal_search(struct kbase_queue *queue,
	struct mtk_qinspect_gpuq_it *gpuq_it, union mtk_qinspect_csf_instruction *inst_search_list)
{
	u64 *addr;
	u64 cs_extract;
	u64 cs_insert;
	u64 addr_start, addr_start_page;
	struct mtk_qinspect_csf_register_file rf;
	union mtk_qinspect_csf_instruction inst_empty_list[] = {{{0, 0}}};
	union mtk_qinspect_csf_instruction ret;
	int i;
    const u16 arch_major = queue->kctx->kbdev->gpu_props.gpu_id.arch_major;
    const u16 product_major = queue->kctx->kbdev->gpu_props.gpu_id.product_major;
    const u32 gpu_id = GPU_ID2_MODEL_MAKE(arch_major, product_major);

	addr = queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO / sizeof(*addr)];
	addr = queue->user_io_addr + PAGE_SIZE / sizeof(*addr);
	cs_extract = addr[CS_EXTRACT_LO / sizeof(*addr)];
	qinspect_dbg(queue->kctx->kbdev,
		"[qinspect] gpuq_internal_search csi_%d: insert = %llx, extract = %llx, size = %x",
		queue->csi_index, cs_insert, cs_extract, queue->size);
	if (cs_insert == cs_extract)
		return NULL;

	cs_extract = queue->base_addr + (cs_extract % queue->size);
	cs_insert = queue->base_addr + (cs_insert % queue->size);
	addr_start_page = cs_extract & PAGE_MASK;
	addr_start = (cs_extract & QINSPECT_CSF_CACHE_LINE_MASK) - QINSPECT_CSF_CACHE_LINE_SIZE;
	if (addr_start < addr_start_page)
		addr_start = addr_start_page;

	/* init gpu map cache */
	for (i = 0; i < ARRAY_SIZE(gpuq_it->gpu_map_cache); i++)
		gpuq_it->gpu_map_cache[i].cpu_addr = NULL;

	mtk_qinspect_gpuq_search_countdown = MTK_QINSPECT_MAX_GPUQ_SEARCH_BUF_COUNT;

	BUILD_BUG_ON(MTK_QINSPECT_CSF_REG_NUM_MAX < 128);
	if ((gpu_id & GPU_ID2_PRODUCT_MODEL) >= GPU_ID2_PRODUCT_TTIX)
		mtk_qinspect_csf_reg_num = 128;
	else
		mtk_qinspect_csf_reg_num = 96;
	memset(&rf, 0, sizeof(rf));
	do {
		if (cs_insert >= cs_extract) {
			if (addr_start < cs_extract) {
				ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, &rf, 0,
					addr_start, cs_extract, inst_empty_list);
				if (ret.inst.opcode == 0xff)
					break;
			}
			ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, &rf, 0,
				cs_extract, cs_insert, inst_search_list);
			if (ret.inst.opcode != 0)
				break;
		} else {
			if ((cs_extract & PAGE_MASK) == (cs_insert & PAGE_MASK)) {	/* is it possible? */
				ret = (union mtk_qinspect_csf_instruction){{.opcode = 0}};
				break;
			}
			/* 1. decode from start to end of buffer */
			if (addr_start < cs_extract) {
				ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, &rf, 0,
					addr_start, cs_extract, inst_empty_list);
				if (ret.inst.opcode == 0xff)
					break;
			}
			ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, &rf, 0,
				cs_extract, queue->base_addr + queue->size, inst_search_list);
			if (ret.inst.opcode != 0)
				break;
			/* 2. decode from start of buffer to cs_insert */
			ret = mtk_qinspect_gpuq_internal_search_inst(queue, gpuq_it, &rf, 0,
				queue->base_addr, cs_insert, inst_search_list);
			if (ret.inst.opcode != 0)
				break;
		}
	} while (0);

	/* cleanup gpu map cache */
	for (i = 0; i < ARRAY_SIZE(gpuq_it->gpu_map_cache); i++)
		if (gpuq_it->gpu_map_cache[i].cpu_addr)
			vunmap(gpuq_it->gpu_map_cache[i].cpu_addr);

	if (ret.inst.opcode == 0 || ret.inst.opcode == 0xff)
		return NULL;

	return gpuq_it;
}

/*
 * command search internal helper
 */

static struct mtk_qinspect_fence_wait_on *mtk_qinspect_query_internal_fence_signal_search_kcpuq(
	struct kbase_kcpu_command_queue *queue, struct mtk_qinspect_fence_wait_it *wait_it)
{
#if IS_ENABLED(CONFIG_SYNC_FILE)
	int i;

	qinspect_dbg(wait_it->kctx->kbdev,
		"[qinspect] search fence_signal cmd at kcpu_queue_%u",
		queue->id);

	for (i = 0; i < queue->num_pending_cmds; i++) {
		struct kbase_kcpu_command *cmd;
		u8 cmd_idx = (u8)(queue->start_offset + i);

		/* The offset to the first command that is being processed or yet to
		 * be processed is of u8 type, so the number of commands inside the
		 * queue cannot be more than 256. The current implementation expects
		 * exactly 256, any other size will require the addition of wrapping
		 * logic.
		 */
		BUILD_BUG_ON(KBASEP_KCPU_QUEUE_SIZE != 256);

		cmd = &queue->commands[cmd_idx];
		if (cmd->type == BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL) {
			struct kbase_kcpu_command_fence_info *fence_info = &cmd->info.fence;

			if (fence_info->fence) {
				if ((u64)fence_info->fence->context == wait_it->context
					&& (u64)fence_info->fence->seqno == wait_it->seqno) {
					qinspect_dbg(wait_it->kctx->kbdev,
						"[qinspect] cmd_idx_%u fence %llu#%llu matched, force_signaled = %d",
						cmd_idx, wait_it->context, wait_it->seqno,
						(int)fence_info->fence_has_force_signaled);
					wait_it->wait_on.kctx = wait_it->kctx_next;
					wait_it->wait_on.kcpu_queue = queue;
					return &wait_it->wait_on;
				}
			} else
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] cmd_idx_%u is fence_signal, fence = NULL, force_signaled = %d",
					cmd_idx,
					(int)fence_info->fence_has_force_signaled);
		}
	}
#endif

	return NULL;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_set_search_cpuq(
	union mtk_qinspect_cpu_command_buf *queue_buf, struct mtk_qinspect_cqs_wait_it *wait_it)
{
	struct mtk_qinspect_cpu_command_queue *queue = &(queue_buf->queue);
	union mtk_qinspect_cpu_command_buf *command_next;
	struct mtk_qinspect_cpu_command *command;
	struct mtk_qinspect_cpu_object *object;
	int cmd_idx;
	u32 obj_no;

	qinspect_dbg(wait_it->kctx->kbdev,
		"[qinspect] search cqs_set cmd at cpu_queue_%llx",
		queue_buf->queue.queue);

	if (!queue->num_cmds)
		return NULL;

	command_next = queue_buf + 1;
	for (cmd_idx = 0; cmd_idx < queue->num_cmds; cmd_idx++) {
		command = &(command_next->command);
		command_next += 1 + command->num_objs;	/* command + objects */

		if (!command->num_objs)
			continue;

		switch (command->work_type) {
		case MTK_BASE_CPU_QUEUE_WORK_SET:
		case MTK_BASE_CPU_QUEUE_WORK_SET_OP:
			for (obj_no = 0; obj_no < command->num_objs; obj_no++) {
				object = &(command->objs[obj_no].object);
				if (object->address == wait_it->cqs_addr) {
					qinspect_dbg(wait_it->kctx->kbdev,
						"[qinspect] cqs_addr matched!");
					wait_it->wait_on.queue_type = QINSPECT_CPU_QUEUE;
					wait_it->wait_on.cpu_queue_buf = queue_buf;
					return &wait_it->wait_on;
				}
			}
			break;
		default:
			break;
		}
	}

	return NULL;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_set_search_kcpuq(
	struct kbase_kcpu_command_queue *queue, struct mtk_qinspect_cqs_wait_it *wait_it)
{
	int i;

	qinspect_dbg(wait_it->kctx->kbdev,
		"[qinspect] search cqs_set cmd at kcpu_queue_%u",
		queue->id);

	for (i = 0; i < queue->num_pending_cmds; i++) {
		struct kbase_kcpu_command *cmd;
		u8 cmd_idx = (u8)(queue->start_offset + i);

		/* The offset to the first command that is being processed or yet to
		 * be processed is of u8 type, so the number of commands inside the
		 * queue cannot be more than 256. The current implementation expects
		 * exactly 256, any other size will require the addition of wrapping
		 * logic.
		 */
		BUILD_BUG_ON(KBASEP_KCPU_QUEUE_SIZE != 256);

		cmd = &queue->commands[cmd_idx];
		switch (cmd->type) {
		case BASE_KCPU_COMMAND_TYPE_CQS_SET:
		{
			unsigned int nr;
			struct kbase_kcpu_command_cqs_set_info *set_info = &cmd->info.cqs_set;

			if (!set_info->objs) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] warning: cmd_idx_%u set_info->objs = NULL", cmd_idx);
				break;
			}
			for (nr = 0; nr < set_info->nr_objs; nr++) {
				if (set_info->objs[nr].addr == wait_it->cqs_addr) {
					qinspect_dbg(wait_it->kctx->kbdev,
						"[qinspect] cqs_addr matched!");
					wait_it->wait_on.queue_type = QINSPECT_KCPU_QUEUE;
					wait_it->wait_on.kcpu_queue = queue;
					return &wait_it->wait_on;
				}
			}
			break;
		}
		case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
		{
			unsigned int nr;
			struct kbase_kcpu_command_cqs_set_operation_info *set_op_info = &cmd->info.cqs_set_operation;

			if (!set_op_info->objs) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] warning: cmd_idx_%u set_operation_info->objs = NULL", cmd_idx);
				break;
			}
			for (nr = 0; nr < set_op_info->nr_objs; nr++) {
				if (set_op_info->objs[nr].addr == wait_it->cqs_addr) {
					qinspect_dbg(wait_it->kctx->kbdev,
						"[qinspect] cqs_addr matched!");
					wait_it->wait_on.queue_type = QINSPECT_KCPU_QUEUE;
					wait_it->wait_on.kcpu_queue = queue;
					return &wait_it->wait_on;
				}
			}
			break;
		}
		default:
			break;
		}
	}

	return NULL;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_sync_set_search_gpuq(
	struct kbase_queue *queue, struct mtk_qinspect_cqs_wait_it *wait_it)
{
	union mtk_qinspect_csf_instruction inst_search_list[] = {{{0, 0b00100101}},	/* SYNC_ADD32 */
								 {{0, 0b00100110}},	/* SYNC_SET32 */
								 {{0, 0b00110011}},	/* SYNC_ADD64 */
								 {{0, 0b00110100}},	/* SYNC_SET64 */
								 {{0, 0}}};
	struct mtk_qinspect_gpuq_it gpuq_it;

	gpuq_it.cqs_addr = wait_it->cqs_addr;
	if (mtk_qinspect_gpuq_internal_search(queue, &gpuq_it, inst_search_list)) {
		wait_it->gpu_sync_info_ret.inst = gpuq_it.inst;
		wait_it->gpu_sync_info_ret.value = gpuq_it.value;
		qinspect_dbg(wait_it->kctx->kbdev,
			"[qinspect] cqs_addr matched!");

		wait_it->wait_on.queue_type = QINSPECT_GPU_QUEUE;
		wait_it->wait_on.gpu_queue = queue;

		return &wait_it->wait_on;
	}

	return NULL;
}

static struct mtk_qinspect_shared_sb_wait_on *mtk_qinspect_query_internal_shared_sb_inc_search_gpuq(
	struct kbase_queue *queue, struct mtk_qinspect_shared_sb_wait_it *wait_it)
{
	union mtk_qinspect_csf_instruction inst_search_list[] = {{{0, 0b00011110}},	/* SHARED_SB_INC */
								 {{0, 0}}};
	struct mtk_qinspect_gpuq_it gpuq_it;

	gpuq_it.shared_entry = wait_it->gpu_shared_sb_wait->shared_entry;
	if (mtk_qinspect_gpuq_internal_search(queue, &gpuq_it, inst_search_list)) {
		qinspect_dbg(wait_it->kctx->kbdev,
			"[qinspect] shared entry matched!");

		wait_it->wait_on.queue_type = QINSPECT_GPU_QUEUE;
		wait_it->wait_on.gpu_queue = queue;

		return &wait_it->wait_on;
	}

	return NULL;
}

/*
 * top_wait_cmd query operations
 */

struct mtk_qinspect_cpu_command *mtk_qinspect_query_cpuq_internal_top_wait_cmd(
	union mtk_qinspect_cpu_command_buf *queue_buf, int *completed)
{
	struct mtk_qinspect_cpu_command_queue *queue = &(queue_buf->queue);

	*completed = (int)queue->completed;

	if (queue->num_cmds && queue_buf[1].command.cmd_idx == 0)
		return &(queue_buf[1].command);

	return NULL;
}

struct kbase_kcpu_command *mtk_qinspect_query_kcpuq_internal_top_wait_cmd(struct kbase_kcpu_command_queue *queue,
	int *blocked)
{
	*blocked = queue->command_started ? 1 : 0;

	if (queue->num_pending_cmds)
		return &queue->commands[queue->start_offset];

	return NULL;
}

struct mtk_qinspect_gpu_command *mtk_qinspect_query_gpuq_internal_top_wait_cmd(struct kbase_queue *queue,
	u32 *blocked_reason, struct mtk_qinspect_gpu_command *gpu_cmd)
{
	union mtk_qinspect_csf_instruction inst;

	if (queue->group->csg_nr < 0) {
		/* save_slot_cs() will indicate wait status in queue->status_wait, check it first. */
		if (!queue->status_wait) {
			*blocked_reason = 0;
			return NULL;
		}

		*blocked_reason = queue->blocked_reason;
		switch (*blocked_reason) {
		case CS_STATUS_BLOCKED_REASON_REASON_SYNC_WAIT:
			gpu_cmd->gpu_sync_info.value = (u64)queue->sync_value;
			gpu_cmd->gpu_sync_info.cqs_addr = queue->sync_ptr;
			gpu_cmd->gpu_sync_info.cond = CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(queue->status_wait);
			gpu_cmd->gpu_sync_info.size = CS_STATUS_WAIT_SYNC_WAIT_SIZE_GET(queue->status_wait);
			gpu_cmd->type = GPU_COMMAND_TYPE_SYNC_WAIT;
			break;
		case CS_STATUS_BLOCKED_REASON_REASON_WAIT:
			if (!mtk_qinspect_gpuq_internal_get_u64(queue->kctx, queue->saved_cmd_ptr, (u64 *)&inst) &&
				inst.inst.opcode == (u64)0x1f) {
				gpu_cmd->gpu_shared_sb_info.shared_entry = (u8)inst.shared_sb_dec.se;
				gpu_cmd->type = GPU_COMMAND_TYPE_SHARED_SB_WAIT;
			} else
				gpu_cmd->type = GPU_COMMAND_TYPE_OTHERS;
			break;
		default:
			gpu_cmd->type = GPU_COMMAND_TYPE_OTHERS;
			break;
		}
	} else {
		u32 status_wait;
		struct kbase_device const *const kbdev =
			queue->group->kctx->kbdev;
		struct kbase_csf_cmd_stream_group_info const *const ginfo =
			&kbdev->csf.global_iface.groups[queue->group->csg_nr];
		struct kbase_csf_cmd_stream_info *stream;
		u64 cmd_ptr;

		if (!ginfo->streams) {
			*blocked_reason = 0;
			return NULL;
		}
		stream = &ginfo->streams[queue->csi_index];

		status_wait = (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT);
		*blocked_reason = (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_BLOCKED_REASON);
		switch (*blocked_reason) {
		case CS_STATUS_BLOCKED_REASON_REASON_SYNC_WAIT:
			gpu_cmd->gpu_sync_info.value = kbase_csf_firmware_cs_output(stream,
								CS_STATUS_WAIT_SYNC_VALUE);
			gpu_cmd->gpu_sync_info.cqs_addr = kbase_csf_firmware_cs_output(stream,
								CS_STATUS_WAIT_SYNC_POINTER_LO);
			gpu_cmd->gpu_sync_info.cqs_addr |= ((u64)kbase_csf_firmware_cs_output(stream,
								CS_STATUS_WAIT_SYNC_POINTER_HI)) << 32;
			gpu_cmd->gpu_sync_info.cond = CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(status_wait);
			gpu_cmd->gpu_sync_info.size = CS_STATUS_WAIT_SYNC_WAIT_SIZE_GET(status_wait);
			gpu_cmd->type = GPU_COMMAND_TYPE_SYNC_WAIT;
			break;
		case CS_STATUS_BLOCKED_REASON_REASON_WAIT:
			cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_LO);
			cmd_ptr |= ((u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_WAIT_SYNC_POINTER_HI)) << 32;
			if (!mtk_qinspect_gpuq_internal_get_u64(queue->kctx, queue->saved_cmd_ptr, (u64 *)&inst) &&
				inst.inst.opcode == (u64)0x1f) {
				gpu_cmd->gpu_shared_sb_info.shared_entry = (u8)inst.shared_sb_dec.se;
				gpu_cmd->type = GPU_COMMAND_TYPE_SHARED_SB_WAIT;
			} else
				gpu_cmd->type = GPU_COMMAND_TYPE_OTHERS;
			break;
		default:
			gpu_cmd->type = GPU_COMMAND_TYPE_OTHERS;
			break;
		}
	}

	return gpu_cmd;
}

/*
 * wait_cmd build operations
 */

void mtk_qinspect_build_fence_info(struct mtk_qinspect_fence_info *fence_info, u64 context, u64 seqno)
{
	fence_info->context = context;
	fence_info->seqno = seqno;
}

void mtk_qinspect_build_cqs_wait_obj(struct mtk_qinspect_cqs_wait_obj *cmd,
	u64 addr, u64 val, u8 operation, u8 data_type)
{
	cmd->addr = addr;
	cmd->val = val;
	cmd->operation = operation;
	cmd->data_type = data_type;
}

/*
 * fence iteration
 */

#if IS_ENABLED(CONFIG_SYNC_FILE)

void mtk_qinspect_query_internal_fence_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *fence_info, struct mtk_qinspect_fence_wait_it *wait_it)
{
	switch (queue_type) {
	case QINSPECT_KCPU_QUEUE:
		wait_it->kctx = ((struct kbase_kcpu_command_queue *)queue)->kctx;
		wait_it->kbdev = wait_it->kctx->kbdev;
		if (!((struct kbase_kcpu_command_fence_info *)fence_info)->fence) {
			qinspect_err(wait_it->kbdev, "[qinspect] warning: fence_info->fence = NULL");
			wait_it->kctx_next = NULL;
			return;
		}
		wait_it->context = (u64)((struct kbase_kcpu_command_fence_info *)fence_info)->fence->context;
		wait_it->seqno = (u64)((struct kbase_kcpu_command_fence_info *)fence_info)->fence->seqno;
		break;
	case QINSPECT_NONE_QUEUE:
		wait_it->kctx = (struct kbase_context *)queue;
		wait_it->kbdev = wait_it->kctx->kbdev;
		wait_it->context = (u64)((struct mtk_qinspect_fence_info *)fence_info)->context;
		wait_it->seqno = (u64)((struct mtk_qinspect_fence_info *)fence_info)->seqno;
		break;
	default:
		wait_it->kctx_next = NULL;
		return;
	}

	/* init kctx search record */
	/* search start from wait_it->kctx and then by the order in kctx_list */
	wait_it->kctx_next = wait_it->kctx;

	/* init kcpu_queue search record */
	wait_it->kcpu_queue_idx = find_first_bit(wait_it->kctx_next->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);
}

struct mtk_qinspect_fence_wait_on *mtk_qinspect_query_internal_fence_wait_it(
	struct mtk_qinspect_fence_wait_it *wait_it)
{
	struct mtk_qinspect_fence_wait_on *wait_on;

	while (wait_it->kctx_next) {
		/* a. search kcpu_queue at wait_it->kctx_next */
		lockdep_assert_held(&wait_it->kctx_next->csf.kcpu_queues.lock);

		qinspect_dbg(wait_it->kctx->kbdev,
			"[qinspect] search fence_signal %llu#%llu at Ctx %d_%d",
			wait_it->context, wait_it->seqno,
			wait_it->kctx_next->tgid, wait_it->kctx_next->id);
		while (wait_it->kcpu_queue_idx < KBASEP_MAX_KCPU_QUEUES) {
			struct kbase_kcpu_command_queue *queue;

			queue = wait_it->kctx_next->csf.kcpu_queues.array[wait_it->kcpu_queue_idx];
			wait_it->kcpu_queue_idx = find_next_bit(wait_it->kctx_next->csf.kcpu_queues.in_use,
								KBASEP_MAX_KCPU_QUEUES,
								wait_it->kcpu_queue_idx + 1);
			if (!queue)
				continue;
			if (!mtk_qinspect_mutex_trylock(&queue->lock)) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] kcpu_queue_%u lock held, skip it!",
					queue->id);
				continue;
			}
			wait_on = mtk_qinspect_query_internal_fence_signal_search_kcpuq(queue, wait_it);
			mutex_unlock(&queue->lock);

			if (wait_on)
				return wait_on;
		}
		if (wait_it->kctx_next != wait_it->kctx)
			mutex_unlock(&wait_it->kctx_next->csf.kcpu_queues.lock);

		/* b. move wait_it->kctx_next */
		if (!mtk_qinspect_mutex_trylock(&wait_it->kbdev->kctx_list_lock)) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] Ctx %d_%d kctx_list_lock held!",
				wait_it->kctx->tgid, wait_it->kctx->id);
			wait_it->kctx_next = NULL;
			break;
		}
		while (1) {
			struct kbase_context *kctx, *kctx_next;
			bool kctx_next_matched;

			/* find kctx_next (wait_it->kctx is the first one and already been processed) */
			kctx_next = NULL;
			if (wait_it->kctx_next == wait_it->kctx) {
				list_for_each_entry(kctx, &wait_it->kbdev->kctx_list, kctx_list_link) {
					if (kctx != wait_it->kctx) {
						kctx_next = kctx;
						break;
					}
				}
			} else {
				kctx_next_matched = false;
				list_for_each_entry(kctx, &wait_it->kbdev->kctx_list, kctx_list_link) {
					if (kctx_next_matched) {
						if (kctx != wait_it->kctx) {
							kctx_next = kctx;
							break;
						}
					} else if (kctx == wait_it->kctx_next)
						kctx_next_matched = true;
				}
			}

			wait_it->kctx_next = kctx_next;
			if (!wait_it->kctx_next)
				break;

			if (!mtk_qinspect_mutex_trylock(&wait_it->kctx_next->csf.kcpu_queues.lock)) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] Ctx %d_%d csf.kcpu_queues.lock held, skip it!",
					wait_it->kctx_next->tgid, wait_it->kctx_next->id);
				continue;
			}

			/* init kcpu_queue search record */
			wait_it->kcpu_queue_idx = find_first_bit(wait_it->kctx_next->csf.kcpu_queues.in_use,
				KBASEP_MAX_KCPU_QUEUES);
			break;
		}
		mutex_unlock(&wait_it->kbdev->kctx_list_lock);
	};

	return NULL;
}

void mtk_qinspect_query_internal_fence_wait_it_done(struct mtk_qinspect_fence_wait_it *wait_it)
{
	if (wait_it->kctx_next) {
		if (wait_it->kctx_next != wait_it->kctx) {
			lockdep_assert_held(&wait_it->kctx_next->csf.kcpu_queues.lock);
			mutex_unlock(&wait_it->kctx_next->csf.kcpu_queues.lock);
		}
		wait_it->kctx_next = NULL;
	}
}

#endif /* IS_ENABLED(CONFIG_SYNC_FILE) */

/*
 * cqs iteration
 */

void mtk_qinspect_query_internal_cqs_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *cmd, struct mtk_qinspect_cqs_wait_it *wait_it)
{
	unsigned int objs_nr;

	wait_it->queue_type = queue_type;
	switch (queue_type) {
	case QINSPECT_CPU_QUEUE:
		wait_it->cpu_queue_buf = (union mtk_qinspect_cpu_command_buf *)queue;
		wait_it->kctx = ((struct mtk_qinspect_cpu_command *)cmd)->kctx;
		wait_it->cpu_command = (struct mtk_qinspect_cpu_command *)cmd;
		objs_nr = wait_it->cpu_command->num_objs;
		break;
	case QINSPECT_KCPU_QUEUE:
		wait_it->kcpu_queue = (struct kbase_kcpu_command_queue *)queue;
		wait_it->kctx = wait_it->kcpu_queue->kctx;
		wait_it->kcpu_cmd = (struct kbase_kcpu_command *)cmd;
		if (wait_it->kcpu_cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT)
			objs_nr = wait_it->kcpu_cmd->info.cqs_wait.nr_objs;
		else if (wait_it->kcpu_cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION)
			objs_nr = wait_it->kcpu_cmd->info.cqs_wait_operation.nr_objs;
		else
			objs_nr = 0;
		break;
	case QINSPECT_GPU_QUEUE:
		wait_it->gpu_queue = (struct kbase_queue *)queue;
		wait_it->kctx = wait_it->gpu_queue->kctx;
		wait_it->gpu_sync_info_wait = &((struct mtk_qinspect_gpu_command *)cmd)->gpu_sync_info;
		objs_nr = 1;
		break;
	case QINSPECT_NONE_QUEUE:
		wait_it->kctx = (struct kbase_context *)queue;
		wait_it->cqs_wait = (struct mtk_qinspect_cqs_wait_obj *)cmd;
		objs_nr = 1;
		break;
	default:
		objs_nr = 0;
		break;
	}

	wait_it->cqs_addr = 0;
	wait_it->wait_obj_nr = 0;
	wait_it->objs_signaled_map = 0;
	wait_it->objs_failure_map = 0;
	wait_it->objs_match_map = 0;
	wait_it->objs_deadlock_map = 0;
	wait_it->objs_map_mask = (1ull << (objs_nr)) - 1;
	if (objs_nr > sizeof(wait_it->objs_signaled_map) * 8)
		qinspect_err(wait_it->kctx->kbdev,
			"[qinspect] query_internal_cqs_wait_it_init: number of objects > %lu!",
			sizeof(wait_it->objs_signaled_map) * 8);
}

static void mtk_qinspect_query_internal_cqs_wait_it_update_signaled_map(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	const unsigned int update_nr = wait_it->wait_obj_nr - 1;

	if (update_nr < sizeof(wait_it->objs_signaled_map) * 8)
		wait_it->objs_signaled_map |= 1ull << update_nr;
}

static void mtk_qinspect_query_internal_cqs_wait_it_update_failure_map(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	const unsigned int update_nr = wait_it->wait_obj_nr - 1;

	if (update_nr < sizeof(wait_it->objs_failure_map) * 8)
		wait_it->objs_failure_map |= 1ull << update_nr;
}

static void mtk_qinspect_query_internal_cqs_wait_it_update_match_map(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	const unsigned int update_nr = wait_it->wait_obj_nr - 1;

	if (update_nr < sizeof(wait_it->objs_match_map) * 8)
		wait_it->objs_match_map |= 1ull << update_nr;
}

void mtk_qinspect_query_internal_cqs_wait_it_update_deadlock_map(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	const unsigned int update_nr = wait_it->wait_obj_nr - 1;

	if (update_nr < sizeof(wait_it->objs_deadlock_map) * 8)
		wait_it->objs_deadlock_map |= 1ull << update_nr;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_wait_it_search_cpuq(
	struct mtk_qinspect_cqs_wait_it *wait_it)
{
	union mtk_qinspect_cpu_command_buf *queue_buf;
	struct mtk_qinspect_cqs_wait_on *wait_on;

	while (wait_it->cpuq_next && wait_it->cpuq_next->queue.queue) {
		queue_buf = wait_it->cpuq_next;
		wait_it->cpuq_next += queue_buf->queue.num_bufs;
		if (queue_buf == wait_it->cpu_queue_buf)
			continue;

		wait_on = mtk_qinspect_query_internal_cqs_set_search_cpuq(queue_buf, wait_it);
		if (wait_on)
			return wait_on;
	}

	return NULL;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_wait_it_search_kcpuq(
	struct mtk_qinspect_cqs_wait_it *wait_it)
{
	struct kbase_kcpu_command_queue *queue;
	struct mtk_qinspect_cqs_wait_on *wait_on;

	while (wait_it->kcpu_queue_idx < KBASEP_MAX_KCPU_QUEUES) {
		queue = wait_it->kctx->csf.kcpu_queues.array[wait_it->kcpu_queue_idx];
		wait_it->kcpu_queue_idx = find_next_bit(wait_it->kctx->csf.kcpu_queues.in_use,
							KBASEP_MAX_KCPU_QUEUES,
							wait_it->kcpu_queue_idx + 1);
		if (!queue || queue == wait_it->kcpu_queue)
			continue;

		if (!mtk_qinspect_mutex_trylock(&queue->lock)) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] kcpu_queue_%u lock held, skip it!",
				queue->id);
			continue;
		}
		wait_on = mtk_qinspect_query_internal_cqs_set_search_kcpuq(queue, wait_it);
		mutex_unlock(&queue->lock);
		if (wait_on)
			return wait_on;
	}

	return NULL;
}

static struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_wait_it_search_gpuq(
	struct mtk_qinspect_cqs_wait_it *wait_it)
{
	struct kbase_queue *queue;
	struct mtk_qinspect_cqs_wait_on *wait_on;

	do {
		/* search current queue_group */
		if (wait_it->queue_group) {
			while (wait_it->gpu_queue_idx < MAX_SUPPORTED_STREAMS_PER_GROUP) {
				queue = wait_it->queue_group->bound_queues[wait_it->gpu_queue_idx++];
				if (!queue || queue == wait_it->gpu_queue)
					continue;

				wait_on = mtk_qinspect_query_internal_sync_set_search_gpuq(queue, wait_it);
				if (wait_on)
					return wait_on;
			}
			wait_it->queue_group = NULL;	/* stop search current queue_group */
		}

		/* move to next queue_group */
		while (wait_it->queue_group_idx < MAX_QUEUE_GROUP_NUM) {
			wait_it->queue_group = wait_it->kctx->csf.queue_groups[wait_it->queue_group_idx++];
			if (wait_it->queue_group) {
				wait_it->gpu_queue_idx = 0;
				qinspect_dbg(wait_it->kctx->kbdev,
					"[qinspect] search cmd at queue_groups_%u",
					wait_it->queue_group->handle);
				break;
			}
		}
	} while (wait_it->queue_group);

	return NULL;
}

int mtk_qinspect_query_internal_cqs_wait_obj(struct mtk_qinspect_cqs_wait_it *wait_it, unsigned int obj_nr,
	struct mtk_qinspect_cqs_wait_obj *wait_obj)
{
	if (wait_it->queue_type == QINSPECT_CPU_QUEUE) {
		struct mtk_qinspect_cpu_command *const cmd = wait_it->cpu_command;
		struct mtk_qinspect_cpu_object *object;

		if (obj_nr >= cmd->num_objs)
			return 1;

		if (!cmd->objs) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] query_internal_cqs_wait_obj failed, objs is NULL!");
			return -1;
		}
		object = &(cmd->objs[obj_nr].object);

		wait_obj->addr = object->address;
		wait_obj->val = object->data_value;
		wait_obj->operation = object->data_operation;
		wait_obj->data_type = object->data_type;
		return 0;
	} else if (wait_it->queue_type == QINSPECT_KCPU_QUEUE) {
		struct kbase_kcpu_command *const cmd = wait_it->kcpu_cmd;

		if (cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT) {
			struct base_cqs_wait_info *obj;

			if (obj_nr >= cmd->info.cqs_wait.nr_objs)
				return 1;

			if (!cmd->info.cqs_wait.objs) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] query_internal_cqs_wait_obj failed, objs is NULL!");
				return -1;
			}
			obj = &(cmd->info.cqs_wait.objs[obj_nr]);

			wait_obj->addr = obj->addr;
			wait_obj->val = obj->val;
			wait_obj->operation = MTK_BASEP_CQS_WAIT_OPERATION_GT;
			wait_obj->data_type = BASEP_CQS_DATA_TYPE_U32;
			return 0;
		} else if (cmd->type == BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION) {
			struct base_cqs_wait_operation_info *obj;

			if (obj_nr >= cmd->info.cqs_wait_operation.nr_objs)
				return 1;

			if (!cmd->info.cqs_wait_operation.objs) {
				qinspect_err(wait_it->kctx->kbdev,
					"[qinspect] query_internal_cqs_wait_obj failed, objs is NULL!");
				return -1;
			}
			obj = &(cmd->info.cqs_wait_operation.objs[obj_nr]);

			wait_obj->addr = obj->addr;
			wait_obj->val = obj->val;
			wait_obj->operation = obj->operation;
			wait_obj->data_type = obj->data_type;
			return 0;
		} else {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] query_cqs_wait_obj failed, unsupported kcpu_command type %d!",
				cmd->type);
			return -1;
		}
	} else if (wait_it->queue_type == QINSPECT_GPU_QUEUE) {
		struct mtk_qinspect_gpu_sync_info *gpu_sync_info;

		/* gpuq sync_wait contains one object only */
		if (obj_nr)
			return 1;

		if (!wait_it->gpu_sync_info_wait) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] query_internal_cqs_wait_obj failed, objs is NULL!");
			return -1;
		}
		gpu_sync_info = wait_it->gpu_sync_info_wait;

		wait_obj->addr = gpu_sync_info->cqs_addr;
		wait_obj->val = gpu_sync_info->value;
		wait_obj->operation = gpu_sync_info->cond;
		wait_obj->data_type = gpu_sync_info->size ? BASEP_CQS_DATA_TYPE_U64 : BASEP_CQS_DATA_TYPE_U32;
		return 0;
	} else if (wait_it->queue_type == QINSPECT_NONE_QUEUE) {
		struct mtk_qinspect_cqs_wait_obj *obj;

		/* none_queue command contains one object only */
		if (obj_nr)
			return 1;

		if (!wait_it->cqs_wait) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] query_internal_cqs_wait_obj failed, objs is NULL!");
			return -1;
		}
		obj = wait_it->cqs_wait;

		wait_obj->addr = obj->addr;
		wait_obj->val = obj->val;
		wait_obj->operation = obj->operation;
		wait_obj->data_type = obj->data_type;
		return 0;
	} else {
		qinspect_err(wait_it->kctx->kbdev,
			"[qinspect] query_cqs_wait_obj failed, unsupported queue type %d!",
			wait_it->queue_type);
		return 2;
	}
}

static int mtk_qinspect_query_internal_cqs_wait_activity(struct mtk_qinspect_cqs_wait_it *wait_it,
	struct mtk_qinspect_cqs_wait_obj *wait_obj)
{
	u64 val64;

	if (wait_obj->data_type == BASEP_CQS_DATA_TYPE_U32) {
		u32 val;

		if (mtk_qinspect_get_cqs_value(wait_it->kctx, wait_obj->addr, &val) != 0) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] Ctx %d_%d get_cqs_value() failed, addr = 0x%llx!",
				wait_it->kctx->tgid, wait_it->kctx->id, wait_obj->addr);
			return -1;
		}
		val64 = (u64)val;
	} else if (wait_obj->data_type == BASEP_CQS_DATA_TYPE_U64) {
		if (mtk_qinspect_get_cqs_value64(wait_it->kctx, wait_obj->addr, &val64) != 0) {
			qinspect_err(wait_it->kctx->kbdev,
				"[qinspect] Ctx %d_%d get_cqs_value64() failed, addr = 0x%llx!",
				wait_it->kctx->tgid, wait_it->kctx->id, wait_obj->addr);
			return -1;
		}
	} else {
		qinspect_err(wait_it->kctx->kbdev,
			"[qinspect] Ctx %d_%d unsupported CQS wait operation data type %d!",
			wait_it->kctx->tgid, wait_it->kctx->id, wait_obj->data_type);
		return -1;
	}

	if (wait_obj->operation == MTK_BASEP_CQS_WAIT_OPERATION_LE) {
		if (val64 <= wait_obj->val)	/* cqs is signaled */
			return 1;
	} else if (wait_obj->operation == MTK_BASEP_CQS_WAIT_OPERATION_GT) {
		if (val64 > wait_obj->val)	/* cqs is signaled */
			return 1;
	} else if (wait_obj->operation == MTK_BASEP_CQS_WAIT_OPERATION_GE) {
		if (val64 >= wait_obj->val)	/* cqs is signaled */
			return 1;
	} else {
		qinspect_err(wait_it->kctx->kbdev,
			"[qinspect] Ctx %d_%d unsupported CQS wait operation %d!",
			wait_it->kctx->tgid, wait_it->kctx->id, wait_obj->operation);
		return -1;
	}

	return 0;
}

static void mtk_qinspect_query_internal_cqs_wait_it_next_obj(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	int rc;
	struct mtk_qinspect_cqs_wait_obj wait_obj;
	int activity_status;

	while (1) {
		/* get cqs_wait_obj */
		rc = mtk_qinspect_query_internal_cqs_wait_obj(wait_it, wait_it->wait_obj_nr, &wait_obj);
		if (rc > 0)		/* reach maximal obj_nr, stop query */
			break;
		wait_it->wait_obj_nr++;
		if (rc < 0)		/* get object failed, try next */
			continue;

		/* check cqs activity */
		rc = mtk_qinspect_query_internal_cqs_wait_activity(wait_it, &wait_obj);
		if (rc < 0) {		/* get cqs value failed or unsupported wait operation */
			mtk_qinspect_query_internal_cqs_wait_it_update_failure_map(wait_it);
			continue;
		} else if (rc > 0) {	/* cqs is signaled */
			mtk_qinspect_query_internal_cqs_wait_it_update_signaled_map(wait_it);
			continue;
		}

		/* cqs is active */
		wait_it->cqs_addr = wait_obj.addr;
		wait_it->cqs_val = wait_obj.val;
		wait_it->cqs_operation = wait_obj.operation;
		wait_it->cqs_data_type = wait_obj.data_type;
		break;
	};
}

struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_wait_it(struct mtk_qinspect_cqs_wait_it *wait_it)
{
	struct mtk_qinspect_cqs_wait_on *wait_on;

	while (true) {
		/* I. search at current wait addr */
		if (wait_it->cqs_addr) {
			/* 1. search at cpu_queues */
			wait_on = mtk_qinspect_query_internal_cqs_wait_it_search_cpuq(wait_it);
			if (wait_on) {
				mtk_qinspect_query_internal_cqs_wait_it_update_match_map(wait_it);
				return wait_on;
			}

			/* 2. search at kcpu_queues */
			wait_on = mtk_qinspect_query_internal_cqs_wait_it_search_kcpuq(wait_it);
			if (wait_on) {
				mtk_qinspect_query_internal_cqs_wait_it_update_match_map(wait_it);
				return wait_on;
			}

			/* 3. search at gpu_queues */
			wait_on = mtk_qinspect_query_internal_cqs_wait_it_search_gpuq(wait_it);
			if (wait_on) {
				mtk_qinspect_query_internal_cqs_wait_it_update_match_map(wait_it);
				return wait_on;
			}

			/* 4. stop search for wait_it->cqs_addr, move to next */
			qinspect_dbg(wait_it->kctx->kbdev,
				"[qinspect] search cqs_set for %llx done",
				wait_it->cqs_addr);
			wait_it->cqs_addr = 0;
		}

		/* II. move to next wait obj */
		mtk_qinspect_query_internal_cqs_wait_it_next_obj(wait_it);
		if (wait_it->cqs_addr) {
			qinspect_dbg(wait_it->kctx->kbdev,
				"[qinspect] search cqs_set for %llx",
				wait_it->cqs_addr);

			/* 1. init cpu_queue search record */
			wait_it->cpuq_next = mtk_qinspect_cpuq_internal_get_buf(wait_it->kctx);

			/* 2. init kcpu_queue search record */
			wait_it->kcpu_queue_idx = find_first_bit(wait_it->kctx->csf.kcpu_queues.in_use,
				KBASEP_MAX_KCPU_QUEUES);

			/* 3. init gpu_queue search record */
			wait_it->queue_group = NULL;
			wait_it->queue_group_idx = 0;
		} else
			break;
	};

	return NULL;
}

/*
 * shared scoreboard iteration
 */

void mtk_qinspect_query_internal_shared_sb_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *cmd, struct mtk_qinspect_shared_sb_wait_it *wait_it)
{
	switch (queue_type) {
	case QINSPECT_GPU_QUEUE:
		wait_it->queue_type = queue_type;
		wait_it->gpu_queue = (struct kbase_queue *)queue;
		wait_it->kctx = wait_it->gpu_queue->kctx;
		wait_it->queue_group = wait_it->gpu_queue->group;
		wait_it->gpu_shared_sb_wait = &((struct mtk_qinspect_gpu_command *)cmd)->gpu_shared_sb_info;
		wait_it->gpu_queue_idx = 0;
		break;
	default:
		wait_it->gpu_queue_idx = MAX_SUPPORTED_STREAMS_PER_GROUP;
		break;
	}
}

struct mtk_qinspect_shared_sb_wait_on *mtk_qinspect_query_internal_shared_sb_wait_it(
	struct mtk_qinspect_shared_sb_wait_it *wait_it)
{
	struct kbase_queue *queue;
	struct mtk_qinspect_shared_sb_wait_on *wait_on;

	while (wait_it->gpu_queue_idx < MAX_SUPPORTED_STREAMS_PER_GROUP) {
		queue = wait_it->queue_group->bound_queues[wait_it->gpu_queue_idx++];
		if (!queue || queue == wait_it->gpu_queue)
			continue;

		wait_on = mtk_qinspect_query_internal_shared_sb_inc_search_gpuq(queue, wait_it);
		if (wait_on)
			return wait_on;
	}

	return NULL;
}

#endif /* CONFIG_MALI_CSF_SUPPORT */
