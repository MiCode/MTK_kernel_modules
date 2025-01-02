// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include <csf/mali_kbase_csf.h>
#endif
#include <platform/mtk_platform_common/mtk_platform_debug.h>
#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG)
#include <mtk_gpufreq.h>
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

extern void (*mtk_gpu_fence_debug_dump_fp)(int fd, int pid, int type, int timeouts);

static DEFINE_MUTEX(fence_debug_lock);
//#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
//void kbase_csf_dump_firmware_trace_buffer(struct kbase_device *kbdev);
//#endif
static int mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_debug_mem_dump_zone_open(struct mtk_debug_mem_view_dump_data *mem_dump_data,
			struct rb_root *rbtree)
{
	int ret = 0;
	struct rb_node *p;
	struct kbase_va_region *reg;
	struct mtk_debug_mem_view_node_info *node_info;
	struct mtk_debug_mem_view_node *mem_view_node;

	if (rbtree == NULL)
		return -EFAULT;

	node_info = &mem_dump_data->packet_header.nodes[mem_dump_data->node_count];
	mem_view_node = &mem_dump_data->mem_view_nodes[mem_dump_data->node_count];

	for (p = rb_first(rbtree); p; p = rb_next(p)) {
		if (mem_dump_data->node_count >= MTK_DEBUG_MEM_DUMP_NODE_NUM) {
			mem_dump_data->packet_header.flags |= MTK_DEBUG_MEM_DUMP_OVERFLOW;
			break;
		}

		reg = rb_entry(p, struct kbase_va_region, rblink);
		if (reg == NULL) {
			ret = -EFAULT;
			goto out;
		}

		if (reg->gpu_alloc == NULL)
			/* Empty region - ignore */
			continue;

		if (reg->flags & KBASE_REG_PROTECTED) {
			/* CPU access to protected memory is forbidden - so
			 * skip this GPU virtual region.
			 */
			continue;
		}

		mem_view_node->alloc = kbase_mem_phy_alloc_get(reg->gpu_alloc);
		mem_view_node->nr_pages = (int)reg->nr_pages;
		if (mem_view_node->nr_pages > (int)mem_view_node->alloc->nents)
			mem_view_node->nr_pages = (int)mem_view_node->alloc->nents;
		if (mem_view_node->nr_pages) {
			node_info->nr_pages = mem_view_node->nr_pages;
			node_info->addr = reg->start_pfn << PAGE_SHIFT;
			node_info++;

			mem_dump_data->node_count++;
			mem_dump_data->page_count += mem_view_node->nr_pages;

			mem_view_node->start_pfn = reg->start_pfn;
			mem_view_node->flags = reg->flags;
			mem_view_node++;
		} else {
			kbase_mem_phy_alloc_put(reg->gpu_alloc);
		}
	}

out:
	return ret;
}

static int mtk_debug_mem_dump_init_mem_list(struct mtk_debug_mem_view_dump_data *mem_dump_data,
			struct kbase_context *kctx)
{
	int ret;

	if (get_file_rcu(kctx->filp) == 0)
		return -ENOENT;

	memset(&(mem_dump_data->packet_header), 0, sizeof(mem_dump_data->packet_header));
	mem_dump_data->node_count = 0;
	mem_dump_data->page_count = 0;
	mem_dump_data->page_offset = 0;

	kbase_gpu_vm_lock(kctx);

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_same);
	if (ret != 0) {
		kbase_gpu_vm_unlock(kctx);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_custom);
	if (ret != 0) {
		kbase_gpu_vm_unlock(kctx);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_exec);
	if (ret != 0) {
		kbase_gpu_vm_unlock(kctx);
		goto out;
	}

#if MALI_USE_CSF
	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_exec_fixed);
	if (ret != 0) {
		kbase_gpu_vm_unlock(kctx);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_fixed);
	if (ret != 0) {
		kbase_gpu_vm_unlock(kctx);
		goto out;
	}
#endif

	kbase_gpu_vm_unlock(kctx);

	if (mem_dump_data->node_count == 0) {
		ret = -ENOENT;
		goto out;
	}

	/* setup packet header */
	mem_dump_data->packet_header.tag = MTK_DEBUG_MEM_DUMP_HEADER;
	mem_dump_data->packet_header.tgid = (u32)kctx->tgid;
	mem_dump_data->packet_header.id = (u32)kctx->id;
	mem_dump_data->packet_header.nr_nodes = (u32)mem_dump_data->node_count;
	mem_dump_data->packet_header.nr_pages = (u32)mem_dump_data->page_count;

	/* calculate packet header size, node_idx < 0 means output starts from packet header */
	mem_dump_data->node_idx = -(((mem_dump_data->node_count * 8 + 32) + PAGE_SIZE - 1) >> PAGE_SHIFT);

	mem_dump_data->kctx_prev = kctx;
	mem_dump_data->kctx = kctx;
	return 0;

out:
	while (mem_dump_data->node_count)
		kbase_mem_phy_alloc_put(mem_dump_data->mem_view_nodes[--mem_dump_data->node_count].alloc);
	fput(kctx->filp);

	return ret;
}

static void mtk_debug_mem_dump_free_mem_list(struct mtk_debug_mem_view_dump_data *mem_dump_data)
{
	if (mem_dump_data->kctx == NULL)
		return;

	while (mem_dump_data->node_count)
		kbase_mem_phy_alloc_put(mem_dump_data->mem_view_nodes[--mem_dump_data->node_count].alloc);
	fput(mem_dump_data->kctx->filp);
	mem_dump_data->kctx = NULL;
}

static void *mtk_debug_mem_dump_next_kctx(struct mtk_debug_mem_view_dump_data *mem_dump_data)
{
	struct kbase_device *kbdev = mem_dump_data->kbdev;
	struct kbase_context *kctx_prev = mem_dump_data->kctx_prev;
	struct kbase_context *kctx;
	int match = (kctx_prev == NULL);

	/* stop dump previous kctx */
	mtk_debug_mem_dump_free_mem_list(mem_dump_data);

	/*
	 * Using kctx_prev to trace previous dumped kctx. Althought kctx_list maybe changed,
	 * but we still don't want to hold the list_lock too long.
	 */
	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		if (match) {
			/* init memory list for next kctx */
			mtk_debug_mem_dump_init_mem_list(mem_dump_data, kctx);
			if (mem_dump_data->kctx) {
				mutex_unlock(&kbdev->kctx_list_lock);
				return mem_dump_data->kctx;
			}
		} else {
			if (kctx == kctx_prev)
				match = 1;
		}
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	return NULL;
}

static void *mtk_debug_mem_dump_start(struct seq_file *m, loff_t *_pos)
{
	struct mtk_debug_mem_view_dump_data *mem_dump_data = m->private;
	struct kbase_device *kbdev = mem_dump_data->kbdev;

	if (mem_dump_data == NULL)
		return NULL;

	if (mem_dump_data->kctx == NULL)
		mtk_debug_mem_dump_next_kctx(mem_dump_data);

	if (mem_dump_data->kctx)
		return mem_dump_data->kctx;
	else
		return NULL;
}

static void mtk_debug_mem_dump_stop(struct seq_file *m, void *v)
{
}

static void *mtk_debug_mem_dump_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct mtk_debug_mem_view_dump_data *mem_dump_data = m->private;
	struct mtk_debug_mem_view_node *mem_view_node;

	if (mem_dump_data == NULL)
		return NULL;

	if (pos)
		++*pos;

	if (mem_dump_data->node_idx < 0) {
		if (++mem_dump_data->node_idx == 0)
			mem_dump_data->page_offset = 0;
		else
			mem_dump_data->page_offset++;
	} else {
		mem_view_node = &mem_dump_data->mem_view_nodes[mem_dump_data->node_idx];

		/* update and check page_offset */
		if (++mem_dump_data->page_offset >= mem_view_node->nr_pages) {
			/* move to next node, if next node is the last node then move to next kctx */
			if (++mem_dump_data->node_idx >= mem_dump_data->node_count)
				return mtk_debug_mem_dump_next_kctx(mem_dump_data);
			/* reset page_offset */
			mem_dump_data->page_offset = 0;
		}
	}

	return mem_dump_data->kctx;
}

static int mtk_debug_mem_dump_show(struct seq_file *m, void *v)
{
	struct mtk_debug_mem_view_dump_data *mem_dump_data = m->private;
	char *buf;

	if (mem_dump_data == NULL)
		return 0;

	if (mem_dump_data->node_idx < 0) {
		/* dump packet header */
		buf = ((char *)&mem_dump_data->packet_header) + (mem_dump_data->page_offset << PAGE_SHIFT);
		seq_write(m, buf, PAGE_SIZE);
	} else if (mem_dump_data->node_idx < mem_dump_data->node_count) {
		/* dump mem_view data */
		struct mtk_debug_mem_view_node *mem_view_node;
		unsigned long long gpu_addr;
		struct page *page;
		void *cpu_addr;
		pgprot_t prot = PAGE_KERNEL;

		mem_view_node = &mem_dump_data->mem_view_nodes[mem_dump_data->node_idx];
		if (mem_dump_data->page_offset < mem_view_node->nr_pages) {
			kbase_gpu_vm_lock(mem_dump_data->kctx);
			if (!(mem_view_node->flags & KBASE_REG_CPU_CACHED))
				prot = pgprot_writecombine(prot);
			page = as_page(mem_view_node->alloc->pages[mem_dump_data->page_offset]);
			cpu_addr = vmap(&page, 1, VM_MAP, prot);
			if (cpu_addr) {
				seq_write(m, cpu_addr, PAGE_SIZE);
				vunmap(cpu_addr);
			} else {
				/* packet_header already dumped, reuse it for unmapped page */
				memset(&mem_dump_data->packet_header, 0, PAGE_SIZE);
				((u64 *)&mem_dump_data->packet_header)[0] = MTK_DEBUG_MEM_DUMP_HEADER;
				((u64 *)&mem_dump_data->packet_header)[1] = MTK_DEBUG_MEM_DUMP_FAIL;
				seq_write(m, &mem_dump_data->packet_header, PAGE_SIZE);
			}
			kbase_gpu_vm_unlock(mem_dump_data->kctx);
		}
	}

	return 0;
}

static const struct seq_operations full_mem_ops = {
	.start	= mtk_debug_mem_dump_start,
	.next	= mtk_debug_mem_dump_next,
	.stop	= mtk_debug_mem_dump_stop,
	.show	= mtk_debug_mem_dump_show,
};

static int mtk_debug_mem_dump_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	struct mtk_debug_mem_view_dump_data *mem_dump_data;
	int ret;

	if (file->f_mode & FMODE_WRITE)
		return -EPERM;

	if (!kbdev)
		return -ENODEV;

	ret = seq_open(file, &full_mem_ops);
	if (ret)
		return ret;

	if ((mem_dump_mode & MTK_DEBUG_MEM_DUMP_FULL_DUMP) == MTK_DEBUG_MEM_DUMP_FULL_DUMP) {
		mem_dump_data = kmalloc(sizeof(*mem_dump_data), GFP_KERNEL);
		if (!mem_dump_data) {
			seq_release(in, file);
			return -ENOMEM;
		}
		mem_dump_data->kbdev = kbdev;
		mem_dump_data->kctx_prev = NULL;
		mem_dump_data->kctx = NULL;
		((struct seq_file *)file->private_data)->private = mem_dump_data;
	} else
		((struct seq_file *)file->private_data)->private = NULL;

	return 0;
}

static int mtk_debug_mem_dump_release(struct inode *in, struct file *file)
{
	struct seq_file *m = (struct seq_file *)file->private_data;

	if (m) {
		struct mtk_debug_mem_view_dump_data *mem_dump_data = m->private;

		if (mem_dump_data) {
			mtk_debug_mem_dump_free_mem_list(mem_dump_data);
			kfree(mem_dump_data);
		}
		seq_release(in, file);
	}

	return 0;
}

static const struct file_operations mtk_debug_mem_dump_fops = {
	.open		= mtk_debug_mem_dump_open,
	.release	= mtk_debug_mem_dump_release,
	.read		= seq_read,
	.llseek		= seq_lseek
};

static int mtk_debug_mem_dump_mode_show(struct seq_file *m, void *v)
{
	seq_printf(m, "mem_dump_mode = %d\n", mem_dump_mode);

	return 0;
}

static int mtk_debug_mem_dump_mode_open(struct inode *in, struct file *file)
{
	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_debug_mem_dump_mode_show, in->i_private);
}

static int mtk_debug_mem_dump_mode_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static ssize_t mtk_debug_mem_dump_mode_write(struct file *file, const char __user *ubuf,
			size_t count, loff_t *ppos)
{
	int ret = 0;

	CSTD_UNUSED(ppos);

	ret = kstrtoint_from_user(ubuf, count, 0, &mem_dump_mode);
	if (ret)
		return ret;

	if (mem_dump_mode < MTK_DEBUG_MEM_DUMP_DISABLE || mem_dump_mode > MTK_DEBUG_MEM_DUMP_MASK)
		mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

	return count;
}

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
/**
 * mtk_debug_dump_kcpu_queues() - Print debug info for KCPU queues
 *
 * @file: The seq_file for printing to
 * @data: The debugfs dentry private data, a pointer to kbase_device
 *
 * Return: Negative error code or 0 on success.
 */
static int mtk_debug_dump_kcpu_queues(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct kbase_context *kctx;

	return 0;

	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		mutex_lock(&kctx->csf.lock);
		kbase_csf_scheduler_lock(kbdev);

		// Print per-context KCPU queues debug information
		{
		unsigned long idx;

		seq_printf(file, "[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u\n", MALI_CSF_CSG_DEBUGFS_VERSION);
		seq_printf(file, "[kcpu_queues] ##### Ctx %d_%d #####\n", kctx->tgid, kctx->id);
		seq_printf(file, "[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno\n",
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

			if (!mutex_trylock(&queue->lock)) {
				dev_info(kbdev->dev, "[%d_%d] %9lu(  lock held, bypass dump )",
						kctx->tgid, kctx->id, idx);
				continue;
			}

			seq_printf(file,
					   "[%d_%d] %9lu(  %s ), %16u, %11u, %7u, %12u, %13llu  %8u\n",
					   kctx->tgid,
					   kctx->id,
					   idx,
					   queue->has_error ? "InErr" : "NoErr",
					   queue->num_pending_cmds,
					   queue->enqueue_failed,
					   queue->command_started ? 1 : 0,
					   queue->start_offset,
					   queue->fence_context,
					   queue->fence_seqno);

			if (queue->command_started) {
				int i;
				for (i = 0; i < queue->num_pending_cmds; i++) {
				struct kbase_kcpu_command *cmd;
				u8 cmd_idx = queue->start_offset + i;
				if (cmd_idx > KBASEP_KCPU_QUEUE_SIZE) {
					seq_printf(file,
							   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
							   kctx->tgid,
							   kctx->id);
					seq_printf(file,
							   "[%d_%d] %9lu(  %s ), %7d,      None, (command index out of size limits %d)\n",
							   kctx->tgid,
							   kctx->id,
							   idx,
							   queue->has_error ? "InErr" : "NoErr",
							   cmd_idx,
							   KBASEP_KCPU_QUEUE_SIZE);
					break;
				}
				cmd = &queue->commands[cmd_idx];
				if (cmd->type < 0 || cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
					seq_printf(file,
							   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
							   kctx->tgid,
							   kctx->id);
					seq_printf(file,
							   "[%d_%d] %9lu(  %s ), %7d, %9d, (unknown blocking command)\n",
							   kctx->tgid,
							   kctx->id,
							   idx,
							   queue->has_error ? "InErr" : "NoErr",
							   cmd_idx,
							   cmd->type);
					continue;
				}
				switch (cmd->type) {
#if IS_ENABLED(CONFIG_SYNC_FILE)
				case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
				{
					struct kbase_sync_fence_info info = { 0 };

					if (cmd->info.fence.fence)
						kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
					else
						scnprintf(info.name, sizeof(info.name), "NULL");

					seq_printf(file,
							   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
							   kctx->tgid,
							   kctx->id);
					seq_printf(file,
							   "[%d_%d] %9lu(  %s ), %7d, Fence Signal, %pK %s %s\n",
							   kctx->tgid,
							   kctx->id,
							   idx,
							   queue->has_error ? "InErr" : "NoErr",
							   cmd_idx,
							   info.fence,
							   info.name,
							   kbase_sync_status_string(info.status));
					break;
				}
				case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
				{
					struct kbase_sync_fence_info info = { 0 };

					if (cmd->info.fence.fence)
						kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
					else
						scnprintf(info.name, sizeof(info.name), "NULL");

					seq_printf(file,
							   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
							   kctx->tgid,
							   kctx->id);
					seq_printf(file,
							   "[%d_%d] %9lu(  %s ), %7d, Fence Wait, %pK %s %s\n",
							   kctx->tgid,
							   kctx->id,
							   idx,
							   queue->has_error ? "InErr" : "NoErr",
							   cmd_idx,
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
						seq_printf(file,
								   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								   kctx->tgid,
								   kctx->id);
						seq_printf(file,
								   "[%d_%d] %9lu(  %s ), %7d,   CQS Set, %llx\n",
								   kctx->tgid,
								   kctx->id,
								   idx,
								   queue->has_error ? "InErr" : "NoErr",
								   cmd_idx,
								   sets->objs ? sets->objs[i].addr : 0);
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

						seq_printf(file,
								   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								   kctx->tgid,
								   kctx->id);
						seq_printf(file,
								   "[%d_%d] %9lu(  %s ), %7d,  CQS Wait, %llx(%u > %u, inherit_err: %s)\n",
								   kctx->tgid,
								   kctx->id,
								   idx,
								   queue->has_error ? "InErr" : "NoErr",
								   cmd_idx,
								   waits->objs ? waits->objs[i].addr : 0,
								   val,
								   waits->objs ? waits->objs[i].val : 0,
								   msg);
					}
					break;
				}
				default:
					seq_printf(file,
							   "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
							   kctx->tgid,
							   kctx->id);
					seq_printf(file,
							   "[%d_%d] %9lu(  %s ), %7d, %9d, (other blocking command)\n",
							   kctx->tgid,
							   kctx->id,
							   idx,
							   queue->has_error ? "InErr" : "NoErr",
							   cmd_idx,
							   cmd->type);
					break;
				}
				}
			}

			mutex_unlock(&queue->lock);
			idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
		}

		mutex_unlock(&kctx->csf.kcpu_queues.lock);
		}

		kbase_csf_scheduler_unlock(kbdev);
		mutex_unlock(&kctx->csf.lock);
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	return 0;
}

/**
 * mtk_debug_dump_cpu_queues() - Print debug info for CPU queues
 *
 * @file: The seq_file for printing to
 * @data: The debugfs dentry private data, a pointer to kbase_device
 *
 * Return: Negative error code or 0 on success.
 */
static int mtk_debug_dump_cpu_queues(struct seq_file *file, void *data)
{
	struct kbase_device *kbdev = file->private;
	struct kbase_context *kctx;

	return 0;

#if 0
	mutex_lock(&kbdev->kctx_list_lock);
	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		mutex_lock(&kctx->csf.lock);
		kbase_csf_scheduler_lock(kbdev);

		// Print per-context CPU queues debug information
		{
		if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
				BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
			seq_printf(file, "[%d_%d] Dump request already started! (try again)\n", kctx->tgid, kctx->id);
			kbase_csf_scheduler_unlock(kbdev);
			mutex_unlock(&kctx->csf.lock);
			mutex_unlock(&kbdev->kctx_list_lock);
			return -EINVAL;
		}

		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
		init_completion(&kctx->csf.cpu_queue.dump_cmp);
		kbase_event_wakeup(kctx);

		kbase_csf_scheduler_unlock(kbdev);
		mutex_unlock(&kctx->csf.lock);

		seq_printf(file, "[cpu_queue] CPU Queues table (version:v%u):\n", MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
		seq_printf(file, "[cpu_queue] ##### Ctx %d_%d #####\n", kctx->tgid, kctx->id);

		wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp,
				msecs_to_jiffies(3000));

		mutex_lock(&kctx->csf.lock);
		kbase_csf_scheduler_lock(kbdev);

		if (kctx->csf.cpu_queue.buffer) {
			WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
						BASE_CSF_CPU_QUEUE_DUMP_PENDING);

			seq_printf(file, "[%d_%d] %s\n", kctx->tgid, kctx->id, kctx->csf.cpu_queue.buffer);

			kfree(kctx->csf.cpu_queue.buffer);
			kctx->csf.cpu_queue.buffer = NULL;
			kctx->csf.cpu_queue.buffer_size = 0;
		}
		else
			seq_printf(file, "[%d_%d] Dump error! (time out)\n", kctx->tgid, kctx->id);

		atomic_set(&kctx->csf.cpu_queue.dump_req_status,
			BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);

		}
		kbase_csf_scheduler_unlock(kbdev);
		mutex_unlock(&kctx->csf.lock);
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	return 0;
#endif
}
#else
static int mtk_debug_dump_kcpu_queues(struct seq_file *file, void *data)
{
	return 0;
}

static int mtk_debug_dump_cpu_queues(struct seq_file *file, void *data)
{
	return 0;
}
#endif /* CONFIG_MALI_CSF_SUPPORT && CONFIG_MALI_MTK_FENCE_DEBUG */

static int mtk_debug_kcpu_queues_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_dump_kcpu_queues,
	                   in->i_private);
}

static int mtk_debug_cpu_queues_open(struct inode *in, struct file *file)
{
	return single_open(file, mtk_debug_dump_cpu_queues,
	                   in->i_private);
}

static const struct file_operations mtk_debug_mem_dump_mode_fops = {
	.open    = mtk_debug_mem_dump_mode_open,
	.release = mtk_debug_mem_dump_mode_release,
	.read    = seq_read,
	.write   = mtk_debug_mem_dump_mode_write,
	.llseek  = seq_lseek
};

static const struct file_operations mtk_debug_kcpu_queues_fops = {
	.open    = mtk_debug_kcpu_queues_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static const struct file_operations mtk_debug_cpu_queues_fops = {
	.open    = mtk_debug_cpu_queues_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int mtk_debug_csf_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("mem_dump", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_debug_mem_dump_fops);
	debugfs_create_file("mem_dump_mode", 0644,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_debug_mem_dump_mode_fops);
	debugfs_create_file("kcpu_queues", 0444,
			kbdev->debugfs_ctx_directory, kbdev,
			&mtk_debug_kcpu_queues_fops);
	debugfs_create_file("cpu_queues", 0444,
			kbdev->debugfs_ctx_directory, kbdev,
			&mtk_debug_cpu_queues_fops);

	return 0;
}
#else
int mtk_debug_csf_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
static const char *mtk_debug_mcu_state_to_string(enum kbase_mcu_state state)
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
static const char *mtk_debug_core_state_to_string(enum kbase_shader_core_state state)
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

static const char *mtk_debug_l2_core_state_to_string(enum kbase_l2_core_state state)
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

void mtk_debug_dump_pm_status(struct kbase_device *kbdev)
{
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
			 mtk_debug_mcu_state_to_string(kbdev->pm.backend.mcu_state),
			 mtk_debug_l2_core_state_to_string(kbdev->pm.backend.l2_state),
			 kbdev->pm.backend.mcu_desired,
			 kbdev->pm.backend.l2_desired,
			 kbdev->pm.backend.l2_always_on);
#if IS_ENABLED(CONFIG_MALI_MTK_DUMMY_CM)
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
			 mtk_debug_core_state_to_string(kbdev->pm.backend.shaders_state),
			 mtk_debug_l2_core_state_to_string(kbdev->pm.backend.l2_state),
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

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
#define MAX_CS_DUMP_NUM_KCTX		16
#define MAX_CS_DUMP_NUM_CSG		(MAX_CS_DUMP_NUM_KCTX * 2)
#define MAX_CS_DUMP_NUM_CSI_PER_CSG	5
#define MAX_CS_DUMP_QUEUE_MEM		(MAX_CS_DUMP_NUM_CSG * MAX_CS_DUMP_NUM_CSI_PER_CSG)
#define MAX_CS_DUMP_NUM_GPU_PAGES	512

static struct mtk_debug_cs_queue_mem_data *cs_dump_queue_mem;
static int cs_dump_queue_mem_ptr;

static void *mtk_debug_cs_queue_mem_allocate(void)
{
	struct mtk_debug_cs_queue_mem_data *ptr;

	if (cs_dump_queue_mem_ptr >= MAX_CS_DUMP_QUEUE_MEM)
		return NULL;
	ptr = &(cs_dump_queue_mem[cs_dump_queue_mem_ptr++]);

	return ptr;
}

static struct mtk_debug_cs_queue_dump_record_kctx *cs_dump_kctx_node;
static int cs_dump_kctx_node_ptr;

static void *mtk_debug_cs_kctx_node_allocate(void)
{
	struct mtk_debug_cs_queue_dump_record_kctx *ptr;

	if (cs_dump_kctx_node_ptr >= MAX_CS_DUMP_NUM_KCTX)
		return NULL;
	ptr = &(cs_dump_kctx_node[cs_dump_kctx_node_ptr++]);

	return ptr;
}

static struct mtk_debug_cs_queue_dump_record_gpu_addr *cs_dump_gpu_addr_node;
static int cs_dump_gpu_addr_node_ptr;

static void *mtk_debug_cs_gpu_addr_node_allocate(void)
{
	struct mtk_debug_cs_queue_dump_record_gpu_addr *ptr;

	if (cs_dump_gpu_addr_node_ptr >= MAX_CS_DUMP_NUM_GPU_PAGES)
		return NULL;
	ptr = &(cs_dump_gpu_addr_node[cs_dump_gpu_addr_node_ptr++]);

	return ptr;
}

static int mtk_debug_cs_queue_allocate_memory(void)
{
	cs_dump_queue_mem = kmalloc(sizeof(*cs_dump_queue_mem) * MAX_CS_DUMP_QUEUE_MEM, GFP_KERNEL);
	if (!cs_dump_queue_mem)
		goto err_queue_mem_pre_allocate;
	cs_dump_queue_mem_ptr = 0;

	cs_dump_kctx_node = kmalloc(sizeof(*cs_dump_kctx_node) * MAX_CS_DUMP_NUM_KCTX, GFP_KERNEL);
	if (!cs_dump_kctx_node)
		goto err_kctx_node_pre_allocate;
	cs_dump_kctx_node_ptr = 0;

	cs_dump_gpu_addr_node = kmalloc(sizeof(*cs_dump_gpu_addr_node) * MAX_CS_DUMP_NUM_GPU_PAGES, GFP_KERNEL);
	if (!cs_dump_gpu_addr_node)
		goto err_gpu_addr_node_pre_allocate;
	cs_dump_gpu_addr_node_ptr = 0;

	return 1;

err_gpu_addr_node_pre_allocate:
	kfree(cs_dump_kctx_node);
	cs_dump_kctx_node = NULL;

err_kctx_node_pre_allocate:
	kfree(cs_dump_queue_mem);
	cs_dump_queue_mem = NULL;

err_queue_mem_pre_allocate:
	return 0;
}

static void mtk_debug_cs_queue_free_memory(void)
{
	if (cs_dump_gpu_addr_node) {
		kfree(cs_dump_gpu_addr_node);
		cs_dump_gpu_addr_node = NULL;
	}

	if (cs_dump_kctx_node) {
		kfree(cs_dump_kctx_node);
		cs_dump_kctx_node = NULL;
	}

	if (cs_dump_queue_mem) {
		kfree(cs_dump_queue_mem);
		cs_dump_queue_mem = NULL;
	}
}

static int mtk_debug_cs_dump_mode;
static int mtk_debug_cs_dump_count;
static struct mtk_debug_cs_queue_dump_record cs_queue_dump_record;

static void mtk_debug_cs_queue_dump_record_init(void)
{
	INIT_LIST_HEAD(&cs_queue_dump_record.record_list);
}

static void mtk_debug_cs_queue_dump_record_flush(void)
{
	struct mtk_debug_cs_queue_dump_record_kctx *kctx_node;
	struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
	void *cpu_addr;

	while (!list_empty(&cs_queue_dump_record.record_list)) {
		kctx_node = list_first_entry(&cs_queue_dump_record.record_list,
			struct mtk_debug_cs_queue_dump_record_kctx, list_node);

		while (!list_empty(&kctx_node->record_list)) {
			gpu_addr_node = list_first_entry(&kctx_node->record_list,
				struct mtk_debug_cs_queue_dump_record_gpu_addr, list_node);
			cpu_addr = gpu_addr_node->cpu_addr;
			if (cpu_addr)
				vunmap(cpu_addr);
			list_del(&gpu_addr_node->list_node);
		}

		kbase_gpu_vm_unlock(kctx_node->kctx);
		list_del(&kctx_node->list_node);
	}
}

static void *mtk_debug_cs_queue_dump_record_map_cpu_addr(struct kbase_context *kctx, u64 gpu_addr)
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

static void *mtk_debug_cs_queue_dump_record_map(struct kbase_context *kctx, u64 gpu_addr, int *new_map)
{
	struct mtk_debug_cs_queue_dump_record_kctx *kctx_node;
	struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
	void *cpu_addr;

	*new_map = 0;
	/* find kctx in list */
	list_for_each_entry(kctx_node, &cs_queue_dump_record.record_list, list_node) {
		if (kctx_node->kctx == kctx) {
			/* kctx found, find gpu_addr in list */
			list_for_each_entry(gpu_addr_node, &kctx_node->record_list, list_node) {
				if (gpu_addr_node->gpu_addr == gpu_addr)
					return gpu_addr_node->cpu_addr;
			}

			cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
			if (!cpu_addr)
				return NULL;
			/* kctx found but gpu_addr does not existed, add new gpu_addr_node */
			gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
			if (!gpu_addr_node) {
				vunmap(cpu_addr);
				return NULL;
			}
			gpu_addr_node->gpu_addr = gpu_addr;
			gpu_addr_node->cpu_addr = cpu_addr;
			list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);
			*new_map = 1;

			return cpu_addr;
		}
	}

	/* can not find kctx, add new kctx_node and gpu_addr_node */
	kctx_node = mtk_debug_cs_kctx_node_allocate();
	if (!kctx_node)
		return NULL;
	INIT_LIST_HEAD(&kctx_node->record_list);
	kctx_node->kctx = kctx;
	kbase_gpu_vm_lock(kctx_node->kctx);
	list_add_tail(&kctx_node->list_node, &cs_queue_dump_record.record_list);

	cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
	if (!cpu_addr)
		return NULL;
	gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
	if (!gpu_addr_node) {
		vunmap(cpu_addr);
		return NULL;
	}
	gpu_addr_node->gpu_addr = gpu_addr;
	gpu_addr_node->cpu_addr = cpu_addr;
	list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);
	*new_map = 1;

	return cpu_addr;
}

static void *mtk_debug_cs_queue_mem_map_and_dump_once(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				u64 gpu_addr)
{
	struct device *dev = kbdev->dev;
	void *cpu_addr;
	int new_map;

	cpu_addr = mtk_debug_cs_queue_dump_record_map(queue_mem->kctx, gpu_addr, &new_map);

	if (new_map) {
		unsigned int i, j;
		u64 *ptr = (typeof(ptr))cpu_addr;
		const unsigned int col_width = sizeof(*ptr);
		const unsigned int row_width = (col_width == sizeof(u64)) ? 32 : 16;
		const unsigned int num_cols = row_width / col_width;

		for (i = 0; i < PAGE_SIZE; i += row_width) {
			/* skip the line that all the values in it are zero */
			for (j = 0; j < num_cols; j++)
				if (ptr[j])
					break;
			if (j == num_cols) {
				ptr += num_cols;
				continue;
			}
			if (col_width == sizeof(u64)) {
				if (mtk_debug_cs_dump_mode)
					dev_info(dev, "%016llx: %016llx %016llx %016llx %016llx",
						gpu_addr + i, ptr[0], ptr[1], ptr[2], ptr[3]);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_print(&kbdev->logbuf_regular,
					"%016llx: %016llx %016llx %016llx %016llx\n",
					gpu_addr + i, ptr[0], ptr[1], ptr[2], ptr[3]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
			} else {
				if (mtk_debug_cs_dump_mode)
					dev_info(dev, "%016llx: %08x %08x %08x %08x",
						gpu_addr + i, ptr[0], ptr[1], ptr[2], ptr[3]);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_print(&kbdev->logbuf_regular,
					"%016llx: %08x %08x %08x %08x\n",
					gpu_addr + i, (unsigned int)ptr[0], (unsigned int)ptr[1],
					(unsigned int)ptr[2], (unsigned int)ptr[3]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
			}
			ptr += num_cols;
		}
	}

	return cpu_addr;
}

static void mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end);

static void mtk_debug_cs_decode_inst(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end)
{
	union mtk_debug_csf_instruction *inst = (union mtk_debug_csf_instruction *)start;
	int reg;
	u64 size, buffer;

	for (; (u64)inst < end; inst++) {
		switch (inst->inst.opcode) {
		case 0b00000001:			/* MOVE */
			reg = (int)(inst->move.dest >> 1);
			if (reg >= (MTK_DEBUG_CSF_REG_NUM / 2))
				break;
			rf->reg64[reg] = inst->move.imm;
			break;
		case 0b00000010:			/* MOVE32 */
			reg = (int)(inst->move.dest);
			if (reg >= MTK_DEBUG_CSF_REG_NUM)
				break;
			rf->reg32[reg] = inst->move32.imm;
			break;
		case 0b00100000:			/* CALL */
			reg = (int)(inst->call.src1);
			if (reg >= MTK_DEBUG_CSF_REG_NUM)
				break;
			size = rf->reg32[reg];
			if (!size)
				break;
			/* limit the maximum dump size */
			if (size > 4 * PAGE_SIZE)
				size = 4 * PAGE_SIZE;
			reg = (int)(inst->call.src0 >> 1);
			if (reg >= (MTK_DEBUG_CSF_REG_NUM / 2))
				break;
			buffer = rf->reg64[reg];
			if (!buffer || buffer & 0x07)
				break;
			mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
				depth + 1, buffer, buffer + size);
			break;
		default:
			break;
		}
	}
}

static void mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end)
{
	/*
	 * There is an implementation defined maximum call stack depth,
	 * which is guaranteed to be a minimum of 8 levels.
	 * A total of 5 levels (ring buffer, dumped ring buffer, Vulkan primary,
	 * Vulkan secondary and canned sequences) are currently used.
	 */
#define MAXIMUM_CALL_STACK_DEPTH 8

	u64 page_addr;
	u64 cpu_addr;
	u64 offset, size, chunk_size;

	if (depth >= MAXIMUM_CALL_STACK_DEPTH)
		return;

	/* dump buffers, using page as the dump unit */
	page_addr = start & PAGE_MASK;
	offset = start - page_addr;
	size = end - start;
	chunk_size = PAGE_SIZE - offset;
	while (size) {
		if (chunk_size > size)
			chunk_size = size;
		cpu_addr = (u64)mtk_debug_cs_queue_mem_map_and_dump_once(kbdev, queue_mem, page_addr);
		if (cpu_addr)
			mtk_debug_cs_decode_inst(kbdev, queue_mem, rf, depth,
				cpu_addr + offset, cpu_addr + offset + chunk_size);
		page_addr += PAGE_SIZE;
		offset = 0;
		size -= chunk_size;
		chunk_size = PAGE_SIZE;
	}
}

static void mtk_debug_cs_queue_data_dump(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_data *cs_queue_data)
{
	struct device *dev = kbdev->dev;
	struct mtk_debug_cs_queue_mem_data *queue_mem;
	u64 addr_start, addr_end;
	union mtk_debug_csf_register_file rf;

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	dev_info(dev, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[cs_mem_dump] start: %d\n", mtk_debug_cs_dump_count);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	mtk_debug_cs_queue_dump_record_init();
	while (!list_empty(&cs_queue_data->queue_list)) {
		queue_mem = list_first_entry(&cs_queue_data->queue_list,
				struct mtk_debug_cs_queue_mem_data, node);
		if (queue_mem->group_type == 0) {
			if (mtk_debug_cs_dump_mode)
				dev_info(dev, "[active_groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
					queue_mem->kctx->tgid, queue_mem->kctx->id,
					queue_mem->handle, queue_mem->csi_index);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_print(&kbdev->logbuf_regular,
				"[active_groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d\n",
				queue_mem->kctx->tgid, queue_mem->kctx->id,
				queue_mem->handle, queue_mem->csi_index);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		} else {
			if (mtk_debug_cs_dump_mode)
				dev_info(dev, "[groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
					queue_mem->kctx->tgid, queue_mem->kctx->id,
					queue_mem->handle, queue_mem->csi_index);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_print(&kbdev->logbuf_regular,
				"[groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d\n",
				queue_mem->kctx->tgid, queue_mem->kctx->id,
				queue_mem->handle, queue_mem->csi_index);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}
		memset(&rf, 0, sizeof(rf));

		if (queue_mem->cs_insert >= queue_mem->cs_extract) {
			/* dump from start of cs_extract page head to cs_insert-8 */
			addr_start = (queue_mem->base_addr + queue_mem->cs_extract) & PAGE_MASK;
			addr_end = queue_mem->base_addr + queue_mem->cs_insert;
			mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf, 0, addr_start, addr_end);
		} else {
			/* two stage dumps */
			/* 1. If cs_extract and cs_insert are in the same page then
			 *    dump from cs_insert to end of buffer, or dump from start
			 *    of cs_extract page head to end of buffer.
			 */
			if ((queue_mem->cs_extract & PAGE_MASK) == (queue_mem->cs_insert & PAGE_MASK))
				addr_start = queue_mem->base_addr + queue_mem->cs_insert;
			else
				addr_start = (queue_mem->base_addr + queue_mem->cs_extract) & PAGE_MASK;
			addr_end = queue_mem->base_addr + queue_mem->size;
			mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf, 0, addr_start, addr_end);
			/* 2. dump from start of buffer to cs_insert-8 */
			addr_start = queue_mem->base_addr;
			addr_end = queue_mem->base_addr + queue_mem->cs_insert;
			mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf, 0, addr_start, addr_end);
		}

		list_del(&queue_mem->node);
	}
	mtk_debug_cs_queue_dump_record_flush();

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[cs_mem_dump] stop: %d\n", mtk_debug_cs_dump_count);
	dev_info(dev, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count++);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
}
#endif /* CONFIG_MALI_CSF_SUPPORT && CONFIG_MALI_MTK_FENCE_DEBUG */

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

static bool sb_source_supported(u32 glb_version)
{
	bool supported = false;

	if (((GLB_VERSION_MAJOR_GET(glb_version) == 3) &&
	     (GLB_VERSION_MINOR_GET(glb_version) >= 5)) ||
	    ((GLB_VERSION_MAJOR_GET(glb_version) == 2) &&
	     (GLB_VERSION_MINOR_GET(glb_version) >= 6)) ||
	    ((GLB_VERSION_MAJOR_GET(glb_version) == 1) &&
	     (GLB_VERSION_MINOR_GET(glb_version) >= 3)))
		supported = true;

	return supported;
}

static void mtk_debug_csf_scheduler_dump_active_queue_cs_status_wait(
	struct kbase_device *kbdev, pid_t tgid, u32 id,
	u32 glb_version, u32 wait_status, u32 wait_sync_value, u64 wait_sync_live_value,
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s\n",
		tgid,
		id,
		CS_STATUS_WAIT_SB_MASK_GET(wait_status),
		CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	if (sb_source_supported(glb_version)) {
		dev_info(kbdev->dev,
		         "[%d_%d] SB_SOURCE: %d",
		         tgid,
		         id,
		         CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] SB_SOURCE: %d\n",
			tgid,
			id,
			CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx\n",
		tgid,
		id,
		CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ? "greater than" : "less or equal",
		wait_sync_pointer);
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u\n",
		tgid,
		id,
		wait_sync_value,
		wait_sync_live_value,
		CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] BLOCKED_REASON: %s\n",
		tgid,
		id,
		blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	// BLOCKED_REASON:
	//     - WAIT: Blocked on scoreboards in some way.
	//     - RESOURCE: Blocked on waiting for resource allocation. e.g., compute, tiler, and fragment resources.
	//     - SYNC_WAIT: Blocked on a SYNC_WAIT{32|64} instruction.
}

static void mtk_debug_csf_scheduler_dump_active_queue(pid_t tgid, u32 id,
                                                      struct kbase_queue *queue,
                                                      struct mtk_debug_cs_queue_data *cs_queue_data)
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
	u32 glb_version;

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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d\n",
			tgid,
			id,
			queue->csi_index,
			queue->base_addr,
			queue->priority,
			queue->doorbell_nr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		return;
	}

	glb_version = queue->kctx->kbdev->csf.global_iface.version;

	/* Ring the doorbell to have firmware update CS_EXTRACT */
	kbase_csf_ring_cs_user_doorbell(queue->kctx->kbdev, queue);
	msleep(100);

	addr = (u32 *)queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO/4] | ((u64)addr[CS_INSERT_HI/4] << 32);

	addr = (u32 *)(queue->user_io_addr + PAGE_SIZE);
	cs_extract = addr[CS_EXTRACT_LO/4] | ((u64)addr[CS_EXTRACT_HI/4] << 32);
	cs_active = addr[CS_ACTIVE/4];

	if (cs_queue_data) {
		struct mtk_debug_cs_queue_mem_data *queue_mem = mtk_debug_cs_queue_mem_allocate();

		if (queue_mem) {
			queue_mem->kctx = cs_queue_data->kctx;
			queue_mem->group_type = cs_queue_data->group_type;
			queue_mem->handle = cs_queue_data->handle;
			queue_mem->csi_index = queue->csi_index;
			queue_mem->base_addr = queue->base_addr;
			queue_mem->size = queue->size;
			queue_mem->cs_insert = cs_insert % queue->size;
			queue_mem->cs_extract = cs_extract % queue->size;
			list_add_tail(&queue_mem->node, &cs_queue_data->queue_list);
		}
	}

	dev_info(queue->kctx->kbdev->dev,
			"[%d_%d] Bind Idx,     Ringbuf addr,     Size, Prio,    Insert offset,   Extract offset, Active, Doorbell",
			tgid,
			id);
	dev_info(queue->kctx->kbdev->dev,
			 "[%d_%d] %8d, %16llx, %8x, %4u, %16llx, %16llx, %6u, %8d",
			 tgid,
			 id,
			 queue->csi_index,
			 queue->base_addr,
			 queue->size,
			 queue->priority,
			 cs_insert,
			 cs_extract,
			 cs_active,
			 queue->doorbell_nr);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] Bind Idx,     Ringbuf addr,     Size, Prio,    Insert offset,   Extract offset, Active, Doorbell\n",
		tgid,
		id);
	mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%d_%d] %8d, %16llx, %8x, %4u, %16llx, %16llx, %6u, %8d\n",
		tgid,
		id,
		queue->csi_index,
		queue->base_addr,
		queue->size,
		queue->priority,
		cs_insert,
		cs_extract,
		cs_active,
		queue->doorbell_nr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

	/* if have command didn't complete print the last command's before and after 4 commands */
	if (cs_insert != cs_extract) {
		size_t size_mask = (queue->queue_reg->nr_pages << PAGE_SHIFT) - 1;
		const unsigned int instruction_size = sizeof(u64);
		u64 start, stop, aligned_cs_extract;

		dev_info(queue->kctx->kbdev->dev,"Dumping instructions around the last Extract offset");
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"Dumping instructions around the last Extract offset\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"Instructions from Extract offset %llx\n",
			start);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

		while (start != stop) {
			u64 page_off = (start & size_mask) >> PAGE_SHIFT;
			u64 offset = (start & size_mask) & ~PAGE_MASK;
			struct page *page =
				as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			u64 *ringbuffer = kmap_atomic(page);
			u64 *ptr = &ringbuffer[offset/8];

			dev_info(queue->kctx->kbdev->dev,"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
					ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
				"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n",
				ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

			kunmap_atomic(ringbuffer);
			start += (8 * instruction_size);
		}
	}

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		dev_info(queue->kctx->kbdev->dev,
		         "[%d_%d] SAVED_CMD_PTR: 0x%llx",
		         tgid,
		         id,
		         queue->saved_cmd_ptr);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] SAVED_CMD_PTR: 0x%llx\n",
			tgid,
			id,
			queue->saved_cmd_ptr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
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

			mtk_debug_csf_scheduler_dump_active_queue_cs_status_wait(
				queue->kctx->kbdev, tgid, id,
				glb_version, wait_status, wait_sync_value,
				wait_sync_live_value, wait_sync_pointer,
				sb_status, blocked_reason);
		}
	} else {
		struct kbase_device const *const kbdev = queue->group->kctx->kbdev;
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[queue->group->csg_nr];
		struct kbase_csf_cmd_stream_info const *const stream = &ginfo->streams[queue->csi_index];
		u64 cmd_ptr;
		u32 req_res;

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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] CMD_PTR: 0x%llx\n",
			tgid,
			id,
			cmd_ptr);
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] REQ_RESOURCE [COMPUTE]: %d\n",
			tgid,
			id,
			CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] REQ_RESOURCE [FRAGMENT]: %d\n",
			tgid,
			id,
			CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] REQ_RESOURCE [TILER]: %d\n",
			tgid,
			id,
			CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] REQ_RESOURCE [IDVS]: %d\n",
			tgid,
			id,
			CS_STATUS_REQ_RESOURCE_IDVS_RESOURCES_GET(req_res));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

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

		mtk_debug_csf_scheduler_dump_active_queue_cs_status_wait(
			queue->kctx->kbdev, tgid, id,
			glb_version, wait_status, wait_sync_value,
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
				"[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u\n",
				tgid,
				id,
				addr,
				val);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

			/* Write offset variable address (pointer) */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
			addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
					 tgid,
					 id,
					 addr);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
				"[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx\n",
				tgid,
				id,
				addr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

			/* EVENT_SIZE and EVENT_STATEs */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
			dev_info(queue->kctx->kbdev->dev,
					 "[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
					 tgid,
					 id,
					 CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
					 CS_INSTR_CONFIG_EVENT_STATE_GET(val));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
				"[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x\n",
				tgid,
				id,
				CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
				CS_INSTR_CONFIG_EVENT_STATE_GET(val));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		} else {
			dev_info(queue->kctx->kbdev->dev,
			         "[%d_%d] NO CS_TRACE",
					 tgid,
					 id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
			mtk_logbuffer_type_print(queue->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
				"[%d_%d] NO CS_TRACE\n",
				tgid,
				id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}
	}
}

/* Waiting timeout for STATUS_UPDATE acknowledgment, in milliseconds */
#define CSF_STATUS_UPDATE_TO_MS (100)

static void update_active_group_status(struct kbase_queue_group *const group)
{
       struct kbase_device *const kbdev = group->kctx->kbdev;
      struct kbase_csf_cmd_stream_group_info const *const ginfo =
               &kbdev->csf.global_iface.groups[group->csg_nr];
       long remaining =
               kbase_csf_timeout_in_jiffies(CSF_STATUS_UPDATE_TO_MS);
       unsigned long flags;

       /* Global doorbell ring for CSG STATUS_UPDATE request or User doorbell
        * ring for Extract offset update, shall not be made when MCU has been
        * put to sleep otherwise it will undesirably make MCU exit the sleep
        * state. Also it isn't really needed as FW will implicitly update the
        * status of all on-slot groups when MCU sleep request is sent to it.
        */
       if (kbdev->csf.scheduler.state == SCHED_SLEEPING)
               return;

       /* Ring the User doobell shared between the queues bound to this
        * group, to have FW update the CS_EXTRACT for all the queues
        * bound to the group. Ring early so that FW gets adequate time
        * for the handling.
        */
       kbase_csf_ring_doorbell(kbdev, group->doorbell_nr);

       kbase_csf_scheduler_spin_lock(kbdev, &flags);
       kbase_csf_firmware_csg_input_mask(ginfo, CSG_REQ,
                       ~kbase_csf_firmware_csg_output(ginfo, CSG_ACK),
                       CSG_REQ_STATUS_UPDATE_MASK);
       kbase_csf_ring_csg_doorbell(kbdev, group->csg_nr);
       kbase_csf_scheduler_spin_unlock(kbdev, flags);

       remaining = wait_event_timeout(kbdev->csf.event_wait,
               !((kbase_csf_firmware_csg_input_read(ginfo, CSG_REQ) ^
               kbase_csf_firmware_csg_output(ginfo, CSG_ACK)) &
               CSG_REQ_STATUS_UPDATE_MASK), remaining);

       if (!remaining) {
               dev_info(kbdev->dev,
                        "[%d_%d] Timed out for STATUS_UPDATE on group %d on slot %d",
                        group->kctx->tgid,
                        group->kctx->id,
                        group->handle,
                        group->csg_nr);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] Timed out for STATUS_UPDATE on group %d on slot %d\n",
			group->kctx->tgid,
			group->kctx->id,
			group->handle,
			group->csg_nr);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
       }
}


static void mtk_debug_csf_scheduler_dump_active_group(struct kbase_queue_group *const group,
                                                      struct mtk_debug_cs_queue_data *cs_queue_data)
{
	if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
		struct kbase_device *const kbdev = group->kctx->kbdev;
		u32 ep_c, ep_r;
		char exclusive;
		char idle = 'N';
		struct kbase_csf_cmd_stream_group_info const *const ginfo = &kbdev->csf.global_iface.groups[group->csg_nr];
		u8 slot_priority = kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

		update_active_group_status(group);

		ep_c = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_CURRENT);
		ep_r = kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_EP_REQ);

		if (CSG_STATUS_EP_REQ_EXCLUSIVE_COMPUTE_GET(ep_r))
			exclusive = 'C';
		else if (CSG_STATUS_EP_REQ_EXCLUSIVE_FRAGMENT_GET(ep_r))
			exclusive = 'F';
		else
			exclusive = '0';

		if (kbase_csf_firmware_csg_output(ginfo, CSG_STATUS_STATE) &
				CSG_STATUS_STATE_IDLE_MASK)
			idle = 'Y';

		dev_info(kbdev->dev,
		        "[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive, Idle",
		        group->kctx->tgid,
		        group->kctx->id);
		dev_info(kbdev->dev,
		        "[%d_%d] %7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c, %4c",
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
		        exclusive, idle);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive, Idle\n",
			group->kctx->tgid,
			group->kctx->id);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] %7d, %6d, %8d, %9d, %8d, %11d/%3d, %11d/%3d, %11d/%3d, %9c, %4c\n",
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
			exclusive, idle);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] GroupID, CSG NR, Run State, Priority\n",
			group->kctx->tgid,
			group->kctx->id);
		mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] %7d, %6d, %9d, %8d\n",
			group->kctx->tgid,
			group->kctx->id,
			group->handle,
			group->csg_nr,
			group->run_state,
			group->priority);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}

	if (group->run_state != KBASE_CSF_GROUP_TERMINATED) {
		unsigned int i;

		dev_info(group->kctx->kbdev->dev,
		        "[%d_%d] Bound queues:",
		        group->kctx->tgid,
		        group->kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(group->kctx->kbdev, MTK_LOGBUFFER_TYPE_ALL,
			"[%d_%d] Bound queues:\n",
			group->kctx->tgid,
			group->kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			mtk_debug_csf_scheduler_dump_active_queue(
				group->kctx->tgid,
				group->kctx->id,
				group->bound_queues[i],
				cs_queue_data);
		}
	}
}

void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid)
{
	bool dump_queue_data;
	static struct mtk_debug_cs_queue_data cs_queue_data;

	/* init mtk_debug_cs_queue_data for dump bound queues */
	mtk_debug_cs_dump_mode = mem_dump_mode & MTK_DEBUG_MEM_DUMP_CS_BUFFER;
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	dump_queue_data = mtk_debug_cs_queue_allocate_memory();
#else
	if (mtk_debug_cs_dump_mode)
		dump_queue_data = mtk_debug_cs_queue_allocate_memory();
	else
		dump_queue_data = 0;
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	if (dump_queue_data)
		INIT_LIST_HEAD(&cs_queue_data.queue_list);

	mutex_lock(&kbdev->kctx_list_lock);
	{
		struct kbase_context *kctx;
		int ret;

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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u\n",
					MALI_CSF_CSG_DEBUGFS_VERSION);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
				if (kbdev->csf.scheduler.state == SCHED_SLEEPING) {
					/* Wait for the MCU sleep request to complete. Please refer the
					 * update_active_group_status() function for the explanation.
					 */
					kbase_pm_wait_for_desired_state(kbdev);
				}

				for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
					struct kbase_queue_group *const group = kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

					if (!group)
						continue;

					dev_info(kbdev->dev, "[active_groups] ##### Ctx %d_%d #####", group->kctx->tgid, group->kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[active_groups] ##### Ctx %d_%d #####\n",
						group->kctx->tgid, group->kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

					if (dump_queue_data) {
						cs_queue_data.kctx = group->kctx;
						cs_queue_data.group_type = 0;
						cs_queue_data.handle = group->handle;
						mtk_debug_csf_scheduler_dump_active_group(group, &cs_queue_data);
					} else
						mtk_debug_csf_scheduler_dump_active_group(group, NULL);
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[groups] ##### Ctx %d_%d #####\n",
						group->kctx->tgid, group->kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

					if (dump_queue_data) {
						cs_queue_data.kctx = group->kctx;
						cs_queue_data.group_type = 1;
						cs_queue_data.handle = group->handle;
						mtk_debug_csf_scheduler_dump_active_group(group, &cs_queue_data);
					} else
						mtk_debug_csf_scheduler_dump_active_group(group, NULL);
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
				         "[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno",
				         kctx->tgid,
				         kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u\n",
					MALI_CSF_CSG_DEBUGFS_VERSION);
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[kcpu_queues] ##### Ctx %d_%d #####\n",
					kctx->tgid, kctx->id);
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno\n",
					kctx->tgid, kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

				mutex_lock(&kctx->csf.kcpu_queues.lock);

				idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

				while (idx < KBASEP_MAX_KCPU_QUEUES) {
					struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];

					if (!queue) {
						idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
						continue;
					}

					if (!mutex_trylock(&queue->lock)) {
						dev_info(kbdev->dev, "[%d_%d] %9lu(  lock held, bypass dump )",
								kctx->tgid, kctx->id, idx);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
						mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
							"[%d_%d] %9lu(  lock held, bypass dump )\n",
							kctx->tgid, kctx->id, idx);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
						continue;
					}

					dev_info(kbdev->dev,
					         "[%d_%d] %9lu(  %s ), %16u, %11u, %7u, %12u, %13llu  %8u",
							kctx->tgid,
							kctx->id,
							idx,
							queue->has_error ? "InErr" : "NoErr",
							queue->num_pending_cmds,
							queue->enqueue_failed,
							queue->command_started ? 1 : 0,
							queue->start_offset,
							queue->fence_context,
							queue->fence_seqno);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[%d_%d] %9lu(  %s ), %16u, %11u, %7u, %12u, %13llu  %8u\n",
						kctx->tgid,
						kctx->id,
						idx,
						queue->has_error ? "InErr" : "NoErr",
						queue->num_pending_cmds,
						queue->enqueue_failed,
						queue->command_started ? 1 : 0,
						queue->start_offset,
						queue->fence_context,
						queue->fence_seqno);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

					if (queue->command_started) {
						int i;
						for (i = 0; i < queue->num_pending_cmds; i++) {
						struct kbase_kcpu_command *cmd;
						u8 cmd_idx = queue->start_offset + i;
						if (cmd_idx > KBASEP_KCPU_QUEUE_SIZE) {
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu(  %s ), %7d,      None, (command index out of size limits %d)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         cmd_idx,
							         KBASEP_KCPU_QUEUE_SIZE);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								kctx->tgid,
								kctx->id);
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] %9lu(  %s ), %7d,      None, (command index out of size limits %d)\n",
								kctx->tgid,
								kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								KBASEP_KCPU_QUEUE_SIZE);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							break;
						}
						cmd = &queue->commands[cmd_idx];
						if (cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu(  %s ), %7d, %9d, (unknown blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         cmd_idx,
							         cmd->type);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								kctx->tgid,
								kctx->id);
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] %9lu(  %s ), %7d, %9d, (unknown blocking command)\n",
								kctx->tgid,
								kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								cmd->type);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							continue;
						}
						switch (cmd->type) {
#if IS_ENABLED(CONFIG_SYNC_FILE)
						case BASE_KCPU_COMMAND_TYPE_FENCE_SIGNAL:
						{
							struct kbase_sync_fence_info info = { 0 };

							if (cmd->info.fence.fence)
								kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							else
								scnprintf(info.name, sizeof(info.name), "NULL");

							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu(  %s ), %7d, Fence Signal, %pK %s %s",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         cmd_idx,
							         info.fence,
							         info.name,
							         kbase_sync_status_string(info.status));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								kctx->tgid,
								kctx->id);
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] %9lu(  %s ), %7d, Fence Signal, %pK %s %s\n",
								kctx->tgid,
								kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								info.fence,
								info.name,
								kbase_sync_status_string(info.status));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							break;
						}
						case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
						{
							struct kbase_sync_fence_info info = { 0 };

							if (cmd->info.fence.fence)
								kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
							else
								scnprintf(info.name, sizeof(info.name), "NULL");

							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu(  %s ), %7d, Fence Wait, %pK %s %s",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         cmd_idx,
							         info.fence,
							         info.name,
							         kbase_sync_status_string(info.status));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								kctx->tgid,
								kctx->id);
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] %9lu(  %s ), %7d, Fence Wait, %pK %s %s\n",
								kctx->tgid,
								kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								info.fence,
								info.name,
								kbase_sync_status_string(info.status));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							break;
						}
#endif
						case BASE_KCPU_COMMAND_TYPE_CQS_SET:
						{
							unsigned int i;
							struct kbase_kcpu_command_cqs_set_info *sets = &cmd->info.cqs_set;

							for (i = 0; i < sets->nr_objs; i++) {
								dev_info(kbdev->dev,
								         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								        "[%d_%d] %9lu(  %s ), %7d,   CQS Set, %llx",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         queue->has_error ? "InErr" : "NoErr",
								         cmd_idx,
								         sets->objs ? sets->objs[i].addr : 0);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
								mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
									"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
									kctx->tgid,
									kctx->id);
								mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
									"[%d_%d] %9lu(  %s ), %7d,   CQS Set, %llx\n",
									kctx->tgid,
									kctx->id,
									idx,
									queue->has_error ? "InErr" : "NoErr",
									cmd_idx,
									sets->objs ? sets->objs[i].addr : 0);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
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
								         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
								         kctx->tgid,
								         kctx->id);
								dev_info(kbdev->dev,
								         "[%d_%d] %9lu(  %s ), %7d,  CQS Wait, %llx(%u > %u, inherit_err: %s)",
								         kctx->tgid,
								         kctx->id,
								         idx,
								         queue->has_error ? "InErr" : "NoErr",
								         cmd_idx,
								         waits->objs ? waits->objs[i].addr : 0,
								         val,
								         waits->objs ? waits->objs[i].val : 0,
								         msg);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
								mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
									"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
									kctx->tgid,
									kctx->id);
								mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
									"[%d_%d] %9lu(  %s ), %7d,  CQS Wait, %llx(%u > %u, inherit_err: %s)\n",
									kctx->tgid,
									kctx->id,
									idx,
									queue->has_error ? "InErr" : "NoErr",
									cmd_idx,
									waits->objs ? waits->objs[i].addr : 0,
									val,
									waits->objs ? waits->objs[i].val : 0,
									msg);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							}
							break;
						}
						default:
							dev_info(kbdev->dev,
							         "[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
							         kctx->tgid,
							         kctx->id);
							dev_info(kbdev->dev,
							         "[%d_%d] %9lu(  %s ), %7d, %9d, (other blocking command)",
							         kctx->tgid,
							         kctx->id,
							         idx,
							         queue->has_error ? "InErr" : "NoErr",
							         cmd_idx,
							         cmd->type);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info\n",
								kctx->tgid,
								kctx->id);
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"[%d_%d] %9lu(  %s ), %7d, %9d, (other blocking command)\n",
								kctx->tgid,
								kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								cmd->type);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							break;
						}
						}
					}

					mutex_unlock(&queue->lock);
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[%d_%d] Dump request already started! (try again)\n",
						kctx->tgid, kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
					kbase_csf_scheduler_unlock(kbdev);
					mutex_unlock(&kctx->csf.lock);
					mutex_unlock(&kbdev->kctx_list_lock);
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
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[cpu_queue] CPU Queues table (version:v%u):\n",
					MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[cpu_queue] ##### Ctx %d_%d #####\n",
					kctx->tgid, kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

				ret = wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp, msecs_to_jiffies(3000));
				if (!ret) {
					dev_info(kbdev->dev, "[%d_%d] Timeout waiting for dump completion", kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[%d_%d] Timeout waiting for dump completion\n",
						kctx->tgid, kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
					mutex_unlock(&kbdev->kctx_list_lock);
					return;
				}

				mutex_lock(&kctx->csf.lock);
				kbase_csf_scheduler_lock(kbdev);

				if (kctx->csf.cpu_queue.buffer) {
					int i;
					int next_str_idx = 0;

					WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) !=
							    BASE_CSF_CPU_QUEUE_DUMP_PENDING);

					for (i = 0; i < kctx->csf.cpu_queue.buffer_size; i++) {
						if (kctx->csf.cpu_queue.buffer[i] == '\n') {
							kctx->csf.cpu_queue.buffer[i] = '\0';
							dev_info(kbdev->dev, "%s", &(kctx->csf.cpu_queue.buffer[next_str_idx]));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
							mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
								"%s\n",
								&(kctx->csf.cpu_queue.buffer[next_str_idx]));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
							next_str_idx = i + 1;
						}
					}

					kfree(kctx->csf.cpu_queue.buffer);
					kctx->csf.cpu_queue.buffer = NULL;
					kctx->csf.cpu_queue.buffer_size = 0;
				}
				else {
					dev_info(kbdev->dev, "[%d_%d] Dump error! (time out)", kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
					mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
						"[%d_%d] Dump error! (time out)\n",
						kctx->tgid, kctx->id);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
				}

				atomic_set(&kctx->csf.cpu_queue.dump_req_status,
					BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
				}

				kbase_csf_scheduler_unlock(kbdev);
				mutex_unlock(&kctx->csf.lock);
			}
		}

		/* dump command stream buffer */
		if (dump_queue_data)
			mtk_debug_cs_queue_data_dump(kbdev, &cs_queue_data);
	}
	mutex_unlock(&kbdev->kctx_list_lock);

	if (dump_queue_data)
		mtk_debug_cs_queue_free_memory();
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

static void mtk_debug_dump_for_external_fence(int fd, int pid, int type, int timeouts)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return;
	mutex_lock(&fence_debug_lock);

	dev_info(kbdev->dev, "%s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d",
	         fence_timeout_type_to_string(type),
	         timeouts,
	         fd,
	         pid);

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
		"[%llxt] %s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d\n",
		mtk_logbuffer_get_timestamp(kbdev, &kbdev->logbuf_exception),
		fence_timeout_type_to_string(type),
		timeouts,
		fd,
		pid);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

#if IS_ENABLED(CONFIG_MALI_MTK_DEBUG) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
#ifdef CONFIG_MALI_FENCE_DEBUG
	if (timeouts > 3000)
#endif
	{
		mtk_common_debug(MTK_COMMON_DBG_DUMP_PM_STATUS, pid, MTK_DBG_HOOK_FENCE_EXTERNAL_TIMEOUT);
		mtk_common_debug(MTK_COMMON_DBG_DUMP_INFRA_STATUS, pid, MTK_DBG_HOOK_FENCE_EXTERNAL_TIMEOUT);
		mtk_common_debug(MTK_COMMON_DBG_DUMP_DB_BY_SETTING, pid, MTK_DBG_HOOK_FENCE_EXTERNAL_TIMEOUT);
		mtk_common_debug(MTK_COMMON_DBG_CSF_DUMP_GROUPS_QUEUES, pid, MTK_DBG_HOOK_FENCE_EXTERNAL_TIMEOUT);
	}
#endif

#if IS_ENABLED(CONFIG_MALI_MTK_TIMEOUT_RESET)
	if (timeouts > 3000) {
		spin_lock(&kbdev->reset_force_change);
		kbdev->reset_force_evict_group_work = true;
		spin_unlock(&kbdev->reset_force_change);

		if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
			dev_info(kbdev->dev, "External fence timeouts(%d ms)! Trigger GPU reset", timeouts);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[%llxt] External fence timeouts(%d ms)! Trigger GPU reset\n",
					mtk_logbuffer_get_timestamp(kbdev, &kbdev->logbuf_exception),
					timeouts);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
			kbase_reset_gpu(kbdev);
		} else {
			dev_info(kbdev->dev, "External fence timeouts(%d ms)! Other threads are already resetting the GPU", timeouts);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
				mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL,
					"[%llxt] External fence timeouts(%d ms)! Other threads are already resetting the GPU\n",
					mtk_logbuffer_get_timestamp(kbdev, &kbdev->logbuf_exception),
					timeouts);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
		}
	}
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */
	mutex_unlock(&fence_debug_lock);
}

int mtk_debug_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_gpu_fence_debug_dump_fp = mtk_debug_dump_for_external_fence;

	return 0;
}

int mtk_debug_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_gpu_fence_debug_dump_fp = NULL;

	return 0;
}
