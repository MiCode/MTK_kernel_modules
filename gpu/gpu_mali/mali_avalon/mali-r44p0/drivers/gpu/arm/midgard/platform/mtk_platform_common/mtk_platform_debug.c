// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
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

#define mtk_log_all(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_ALL, fmt "\n", ##args); \
	} while (0)
#define mtk_log_regular(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_REGULAR, fmt "\n", ##args); \
	} while (0)
#define mtk_log_exception(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_EXCEPTION, fmt "\n", ##args); \
	} while (0)

#else
#define mtk_log_all(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
	} while (0)
#define mtk_log_regular(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
	} while (0)
#define mtk_log_exception(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
	} while (0)

#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

extern void (*mtk_gpu_fence_debug_dump_fp)(int fd, int pid, int type, int timeouts);

static DEFINE_MUTEX(fence_debug_lock);
//#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
//void kbase_csf_dump_firmware_trace_buffer(struct kbase_device *kbdev);
//#endif
static int mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

static int mtk_debug_trylock(struct mutex *lock)
{
	int count = 3;
	int ret;

	do {
		ret = mutex_trylock(lock);
		if (ret)
			return ret;
		msleep(1);
	} while (count--);

	return ret;
}

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

	/* kbase_gpu_vm_lock */
	if (!mtk_debug_trylock(&kctx->reg_lock))
		return -EAGAIN;

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_same);
	if (ret != 0) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_custom);
	if (ret != 0) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_exec);
	if (ret != 0) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		goto out;
	}

#if MALI_USE_CSF
	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_exec_fixed);
	if (ret != 0) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		goto out;
	}

	ret = mtk_debug_mem_dump_zone_open(mem_dump_data, &kctx->reg_rbtree_fixed);
	if (ret != 0) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		goto out;
	}
#endif

	/* kbase_gpu_vm_unlock */
	mutex_unlock(&kctx->reg_lock);

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
	 * but we still don't want to hold kctx_list_lock too long.
	 */
	if (!mtk_debug_trylock(&kbdev->kctx_list_lock))
		return NULL;
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
			/* kbase_gpu_vm_lock */
			if (mtk_debug_trylock(&mem_dump_data->kctx->reg_lock)) {
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
				/* kbase_gpu_vm_unlock */
				mutex_unlock(&mem_dump_data->kctx->reg_lock);
			} else {
				/* packet_header already dumped, reuse it for unmapped page */
				memset(&mem_dump_data->packet_header, 0, PAGE_SIZE);
				((u64 *)&mem_dump_data->packet_header)[0] = MTK_DEBUG_MEM_DUMP_HEADER;
				((u64 *)&mem_dump_data->packet_header)[1] = MTK_DEBUG_MEM_DUMP_FAIL;
				seq_write(m, &mem_dump_data->packet_header, PAGE_SIZE);
			}
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

static const struct file_operations mtk_debug_mem_dump_mode_fops = {
	.open    = mtk_debug_mem_dump_mode_open,
	.release = mtk_debug_mem_dump_mode_release,
	.read    = seq_read,
	.write   = mtk_debug_mem_dump_mode_write,
	.llseek  = seq_lseek
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
	dev_info(kbdev->dev, "[CSF] firmware_hctl_core_pwr=%d glb_init_request_pending=%d scheduler.pm_active_count=%d",
			 kbdev->csf.firmware_hctl_core_pwr,
			 kbdev->csf.glb_init_request_pending,
			 kbdev->csf.scheduler.pm_active_count);
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
	dev_info(kbdev->dev, "[PM] policy_change_clamp_state_to_off=%d csf_pm_sched_flsgs=%d",
			 kbdev->pm.backend.policy_change_clamp_state_to_off,
			 kbdev->pm.backend.csf_pm_sched_flags);
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
#define MAX_CS_DUMP_COUNT_PER_CSI	128

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

		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx_node->kctx->reg_lock);
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
	if (reg == NULL || reg->gpu_alloc == NULL) {
		/* Empty region - ignore */
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: empty region", gpu_addr);
		return NULL;
	}

	if (reg->flags & KBASE_REG_PROTECTED) {
		/* CPU access to protected memory is forbidden - so
		 * skip this GPU virtual region.
		 */
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: protected memory", gpu_addr);
		return NULL;
	}

	offset = pfn - reg->start_pfn;
	if (offset >= reg->gpu_alloc->nents) {
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode, "%016llx: pfn out of range", gpu_addr);
		return NULL;
	}

	if (!(reg->flags & KBASE_REG_CPU_CACHED))
		prot = pgprot_writecombine(prot);

	page = as_page(reg->gpu_alloc->pages[offset]);
	cpu_addr = vmap(&page, 1, VM_MAP, prot);

	return cpu_addr;
}

static struct mtk_debug_cs_queue_dump_record_gpu_addr *mtk_debug_cs_queue_dump_record_map(
	struct kbase_context *kctx, u64 gpu_addr)
{
	struct mtk_debug_cs_queue_dump_record_kctx *kctx_node;
	struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
	void *cpu_addr;

	/* find kctx in list */
	list_for_each_entry(kctx_node, &cs_queue_dump_record.record_list, list_node) {
		if (kctx_node->kctx == kctx) {
			/* kctx found, find gpu_addr in list */
			list_for_each_entry(gpu_addr_node, &kctx_node->record_list, list_node) {
				if (gpu_addr_node->gpu_addr == gpu_addr)
					return gpu_addr_node;
			}

			cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
			if (!cpu_addr)
				return NULL;
			/* kctx found but gpu_addr does not existed, add new gpu_addr_node */
			gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
			if (!gpu_addr_node) {
				vunmap(cpu_addr);
				mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
						"%016llx: MAX_CS_DUMP_NUM_GPU_PAGES(%d) too small",
						gpu_addr, MAX_CS_DUMP_NUM_GPU_PAGES);
				return NULL;
			}
			gpu_addr_node->gpu_addr = gpu_addr;
			gpu_addr_node->cpu_addr = cpu_addr;
			memset(gpu_addr_node->bitmap, 0, sizeof(gpu_addr_node->bitmap));
			list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);

			return gpu_addr_node;
		}
	}

	/* kbase_gpu_vm_lock */
	if (!mtk_debug_trylock(&kctx->reg_lock)) {
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
				"%016llx: kbase_gpu_vm_lock failed!", gpu_addr);
		return NULL;
	}

	/* can not find kctx, add new kctx_node and gpu_addr_node */
	kctx_node = mtk_debug_cs_kctx_node_allocate();
	if (!kctx_node) {
		/* kbase_gpu_vm_unlock */
		mutex_unlock(&kctx->reg_lock);
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
				"%016llx: MAX_CS_DUMP_NUM_KCTX(%d) too small",
				gpu_addr, MAX_CS_DUMP_NUM_KCTX);
		return NULL;
	}
	INIT_LIST_HEAD(&kctx_node->record_list);
	kctx_node->kctx = kctx;
	list_add_tail(&kctx_node->list_node, &cs_queue_dump_record.record_list);

	cpu_addr = mtk_debug_cs_queue_dump_record_map_cpu_addr(kctx, gpu_addr);
	if (!cpu_addr)
		return NULL;
	gpu_addr_node = mtk_debug_cs_gpu_addr_node_allocate();
	if (!gpu_addr_node) {
		vunmap(cpu_addr);
		mtk_log_regular(kctx->kbdev, mtk_debug_cs_dump_mode,
				"%016llx: MAX_CS_DUMP_NUM_GPU_PAGES(%d) too small",
				gpu_addr, MAX_CS_DUMP_NUM_GPU_PAGES);
		return NULL;
	}
	gpu_addr_node->gpu_addr = gpu_addr;
	gpu_addr_node->cpu_addr = cpu_addr;
	memset(gpu_addr_node->bitmap, 0, sizeof(gpu_addr_node->bitmap));
	list_add_tail(&gpu_addr_node->list_node, &kctx_node->record_list);

	return gpu_addr_node;
}

static void *mtk_debug_cs_queue_mem_map_and_dump_once(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				u64 gpu_addr, u64 offset, u64 size)
{
	struct mtk_debug_cs_queue_dump_record_gpu_addr *gpu_addr_node;
	int bitmap_idx, bitmap_chk;
	u64 *ptr;
	unsigned int row_width, num_cols;
	u64 row;
	const u64 end = offset + size;
	const int rows_per_map = sizeof(gpu_addr_node->bitmap[0]) * BITS_PER_BYTE;
	unsigned int i, col;

	gpu_addr_node = mtk_debug_cs_queue_dump_record_map(queue_mem->kctx, gpu_addr);
	if (!gpu_addr_node)
		return NULL;

	row_width = 64;		/* cache line size as dump unit */
	num_cols = row_width / sizeof(*ptr);

	row = offset / row_width;
	ptr = ((typeof(ptr))gpu_addr_node->cpu_addr) + (row * num_cols);
	bitmap_idx = row / rows_per_map;
	bitmap_chk = 1 << (row % rows_per_map);
	offset = row * row_width;
	for (; offset < end; offset += row_width, ptr += num_cols) {
		/* bitmap check */
		if (gpu_addr_node->bitmap[bitmap_idx] & bitmap_chk) {
			if (bitmap_chk == 1 << (rows_per_map - 1)) {
				bitmap_idx += 1;
				bitmap_chk = 1;
			} else
				bitmap_chk <<= 1;
			continue;
		} else {
			gpu_addr_node->bitmap[bitmap_idx] |= bitmap_chk;
			if (bitmap_chk == 1 << (rows_per_map - 1)) {
				bitmap_idx += 1;
				bitmap_chk = 1;
			} else
				bitmap_chk <<= 1;
		}

		/* The dump unit is cache line size (64bytes) but the actual */
		/* printed size is 32 bytes per line, so we need dump twice. */
		/* skip the line that all the values in it are zero */
		for (col = 0; col < num_cols; col += 4) {
			for (i = col; i < col + 4; i++)
				if (ptr[i])
					break;
			if (i == col + 4)
				continue;
			mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: %016llx %016llx %016llx %016llx",
					gpu_addr + offset + col * sizeof(*ptr),
					ptr[col + 0], ptr[col + 1], ptr[col + 2], ptr[col + 3]);
		}
	}

	return gpu_addr_node->cpu_addr;
}

static unsigned int mtk_debug_cs_mem_dump_countdown;

static int mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end, bool skippable);

static int mtk_debug_cs_mem_dump_linear(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 buffer, u64 size, bool skippable)
{
	int ret;

	if (skippable) {
		if (size > PAGE_SIZE) {
			/* dump last page */
			u64 buffer_end = buffer + size;

			memset(rf, 0, sizeof(*rf));
			if (buffer_end & ~PAGE_MASK) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"%016llx: size of linear buffer > 1 pages, dump from 0x%016llx",
						buffer, buffer_end & PAGE_MASK);
				ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
					depth, buffer_end & PAGE_MASK, buffer_end, skippable);
				if (ret != 0) {
					mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
						buffer, depth, buffer_end & PAGE_MASK, buffer_end, skippable);
					return ret;
				}
			} else {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"%016llx: size of linear buffer > 1 pages, dump from 0x%016llx",
						buffer, buffer_end - PAGE_SIZE);
				ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
					depth, buffer_end - PAGE_SIZE, buffer_end, skippable);
				if (ret != 0) {
					mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
						buffer, depth, buffer_end - PAGE_SIZE, buffer_end, skippable);
					return ret;
				}
			}
		} else {
			ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
				depth, buffer, buffer + size, skippable);
			if (ret != 0) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
					buffer, depth, buffer, buffer + size, skippable);
				return ret;
			}
		}
	} else {
		if (size > 32 * PAGE_SIZE) {
			/* dump first 32 pages */
			mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: size of linear buffer > 32 pages", buffer);
			size = 32 * PAGE_SIZE;
			ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
				depth, buffer, (buffer + size) & PAGE_MASK, skippable);
			memset(rf, 0, sizeof(*rf));
			if (ret != 0) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
					buffer, depth, buffer, (buffer + size) & PAGE_MASK, skippable);
				return ret;
			}
		} else {
			ret = mtk_debug_cs_mem_dump(kbdev, queue_mem, rf,
				depth, buffer, buffer + size, skippable);
			if (ret != 0) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
					buffer, depth, buffer, buffer + size, skippable);
				return ret;
			}
		}
	}

	return 0;
}

static int mtk_debug_cs_decode_inst(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end, bool skippable)
{
	union mtk_debug_csf_instruction *inst = (union mtk_debug_csf_instruction *)start;
	int reg;
	u64 size, buffer, buffer_end;
	int ret;

	for (; (u64)inst < end; inst++) {
		switch (inst->inst.opcode) {
		case 0b00000001:			/* MOVE */
			reg = (int)inst->move.dest;
			if (reg >= MTK_DEBUG_CSF_REG_NUM || reg & 0x1)
				break;
			rf->reg64[reg >> 1] = inst->move.imm;
			break;
		case 0b00000010:			/* MOVE32 */
			reg = (int)inst->move.dest;
			if (reg >= MTK_DEBUG_CSF_REG_NUM)
				break;
			rf->reg32[reg] = inst->move32.imm;
			break;
		case 0b00100000:			/* CALL */
			reg = (int)inst->call.src0;
			if (reg >= MTK_DEBUG_CSF_REG_NUM || reg & 0x1)
				break;
			buffer = rf->reg64[reg >> 1];
			if (!buffer || buffer & (8 - 1))
				break;
			reg = (int)inst->call.src1;
			if (reg >= MTK_DEBUG_CSF_REG_NUM)
				break;
			size = rf->reg32[reg];
			if (!size || size & (8 - 1))
				break;
			ret = mtk_debug_cs_mem_dump_linear(kbdev, queue_mem, rf, depth + 1, buffer, size, skippable);
			if (ret != 0) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: mtk_debug_cs_mem_dump_linear failed (%d,%llx,%llx,%d)!",
					buffer, depth + 1, buffer, size, skippable);
				return ret;
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int mtk_debug_cs_mem_dump(struct kbase_device *kbdev,
				struct mtk_debug_cs_queue_mem_data *queue_mem,
				union mtk_debug_csf_register_file *rf,
				int depth, u64 start, u64 end, bool skippable)
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
	int ret;

	if (!mtk_debug_cs_mem_dump_countdown) {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
				"%s: Hit maximal dump count!", __func__);
		return -1;
	}
	mtk_debug_cs_mem_dump_countdown--;

	if (depth >= MAXIMUM_CALL_STACK_DEPTH) {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
				"%s: Hit MAXIMUM_CALL_STACK_DEPTH (%d)!", __func__, MAXIMUM_CALL_STACK_DEPTH);
		return -2;
	}

	/* dump buffers, using page as the dump unit */
	page_addr = start & PAGE_MASK;
	offset = start - page_addr;
	size = end - start;
	chunk_size = PAGE_SIZE - offset;
	while (size) {
		if (chunk_size > size)
			chunk_size = size;

		if (depth)	/* linear buffer */
			cpu_addr = (u64)mtk_debug_cs_queue_mem_map_and_dump_once(kbdev, queue_mem,
				page_addr, offset, chunk_size);
		else		/* ring buffer, adjust page_addr */
			cpu_addr = (u64)mtk_debug_cs_queue_mem_map_and_dump_once(kbdev, queue_mem,
				(queue_mem->base_addr + (page_addr % queue_mem->size)), offset, chunk_size);
		if (!cpu_addr)
			return -3;

		ret = mtk_debug_cs_decode_inst(kbdev, queue_mem, rf, depth,
			cpu_addr + offset, cpu_addr + offset + chunk_size, skippable);
		if (ret != 0) {
			mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
				"%s: mtk_debug_cs_decode_inst failed (%d,%llx,%llx,%d)!", __func__,
				depth, cpu_addr + offset, cpu_addr + offset + chunk_size, skippable);
			return ret;
		}

		page_addr += PAGE_SIZE;
		offset = 0;
		size -= chunk_size;
		chunk_size = PAGE_SIZE;
	}

	return 0;
}

static void mtk_debug_cs_queue_dump(struct kbase_device *kbdev, struct mtk_debug_cs_queue_mem_data *queue_mem)
{
	union mtk_debug_csf_register_file rf;
	u64 addr_start;
	int rc;

	if (queue_mem->group_type == 0) {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
			"[active_groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
			queue_mem->kctx->tgid, queue_mem->kctx->id,
			queue_mem->handle, queue_mem->csi_index);
	} else {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
			"[groups_mem] Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
			queue_mem->kctx->tgid, queue_mem->kctx->id,
			queue_mem->handle, queue_mem->csi_index);
	}

	/* adjust cs_extract and cs_insert to avoid overflow case */
	if (queue_mem->cs_extract >= (queue_mem->size * 2)) {
		queue_mem->cs_extract -= queue_mem->size;
		queue_mem->cs_insert -= queue_mem->size;
	}
	/* check cs_extract and cs_insert */
	if (queue_mem->cs_extract > queue_mem->cs_insert ||
		(queue_mem->cs_insert - queue_mem->cs_extract) > queue_mem->size ||
		queue_mem->cs_extract & (8 - 1) || queue_mem->cs_insert & (8 - 1) ||
		queue_mem->base_addr & ~PAGE_MASK)
		return;
	/* dump four extra cache lines before cs_extract */
	if ((queue_mem->cs_extract / 64) > 4)
		addr_start = (queue_mem->cs_extract & ~(64 - 1)) - (4 * 64);
	else
		addr_start = 0;

	mtk_debug_cs_mem_dump_countdown = MAX_CS_DUMP_COUNT_PER_CSI;
	memset(&rf, 0, sizeof(rf));
	if (addr_start < queue_mem->cs_extract) {
		rc = mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf,
			0, addr_start, queue_mem->cs_extract, true);
		if (rc != 0) {
			mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
				"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
				queue_mem->base_addr + (addr_start % queue_mem->size),
				0, addr_start, queue_mem->cs_extract, true);
			return;
		}
	}
	if (queue_mem->cs_extract < queue_mem->cs_insert) {
		rc = mtk_debug_cs_mem_dump(kbdev, queue_mem, &rf,
			0, queue_mem->cs_extract, queue_mem->cs_insert, false);
		if (rc != 0) {
			mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
				"%016llx: mtk_debug_cs_mem_dump failed (%d,%llx,%llx,%d)!",
				queue_mem->base_addr + (queue_mem->cs_extract % queue_mem->size),
				0, queue_mem->cs_extract, queue_mem->cs_insert, false);
			return;
		}
	}
}

static void mtk_debug_cs_queue_data_dump(struct kbase_device *kbdev, struct mtk_debug_cs_queue_data *cs_queue_data)
{
	struct kbase_context *kctx, *kctx_prev = NULL;
	struct mtk_debug_cs_queue_mem_data *queue_mem;

	mtk_log_all(kbdev, true, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);

	mtk_debug_cs_queue_dump_record_init();
	while (!list_empty(&cs_queue_data->queue_list)) {
		queue_mem = list_first_entry(&cs_queue_data->queue_list, struct mtk_debug_cs_queue_mem_data, node);
		/* make sure the queue_mem->kctx is still valid */
		if (likely(queue_mem->kctx == kctx_prev))
			mtk_debug_cs_queue_dump(kbdev, queue_mem);
		else {
			list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
				if (queue_mem->kctx == kctx) {
					kctx_prev = queue_mem->kctx;
					break;
				}
			}
			if (likely(queue_mem->kctx == kctx_prev))
				mtk_debug_cs_queue_dump(kbdev, queue_mem);
			else {
				mtk_log_all(kbdev, true, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);
				if (queue_mem->group_type == 0) {
					mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"[active_groups_mem] Invalid kctx, skip Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
						queue_mem->tgid, queue_mem->id,
						queue_mem->handle, queue_mem->csi_index);
				} else {
					mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"[groups_mem] Invalid kctx, skip Ctx: %d_%d, GroupID: %d, Bind Idx: %d",
						queue_mem->tgid, queue_mem->id,
						queue_mem->handle, queue_mem->csi_index);
				}
			}
		}
		list_del(&queue_mem->node);
	}
	mtk_debug_cs_queue_dump_record_flush();

	mtk_log_all(kbdev, true, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count);
	mtk_debug_cs_dump_count++;
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

	mtk_log_all(kbdev, true,
		"[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
		tgid, id,
		CS_STATUS_WAIT_SB_MASK_GET(wait_status),
		CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
	if (sb_source_supported(glb_version)) {
		mtk_log_all(kbdev, true,
			"[%d_%d] SB_SOURCE: %d",
			tgid, id,
			CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
	}
	mtk_log_all(kbdev, true,
		"[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
		tgid, id,
		CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status) ? "greater than" : "less or equal",
		wait_sync_pointer);
	mtk_log_all(kbdev, true,
		"[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
		tgid, id,
		wait_sync_value,
		wait_sync_live_value,
		CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	mtk_log_all(kbdev, true,
		"[%d_%d] BLOCKED_REASON: %s",
		tgid, id,
		blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));

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
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d",
			tgid, id,
			queue->csi_index,
			queue->base_addr,
			queue->priority,
			queue->doorbell_nr);
		return;
	}

	glb_version = queue->kctx->kbdev->csf.global_iface.version;

	addr = (u32 *)queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO/4] | ((u64)addr[CS_INSERT_HI/4] << 32);

	addr = (u32 *)(queue->user_io_addr + PAGE_SIZE);
	cs_extract = addr[CS_EXTRACT_LO/4] | ((u64)addr[CS_EXTRACT_HI/4] << 32);
	cs_active = addr[CS_ACTIVE/4];

	if (cs_queue_data) {
		struct mtk_debug_cs_queue_mem_data *queue_mem = mtk_debug_cs_queue_mem_allocate();

		if (queue_mem && queue->size) {
			queue_mem->kctx = cs_queue_data->kctx;
			queue_mem->tgid = queue_mem->kctx->tgid;
			queue_mem->id = queue_mem->kctx->id;
			queue_mem->group_type = cs_queue_data->group_type;
			queue_mem->handle = cs_queue_data->handle;
			queue_mem->csi_index = queue->csi_index;
			queue_mem->base_addr = queue->base_addr;
			queue_mem->size = queue->size;
			queue_mem->cs_insert = cs_insert;
			queue_mem->cs_extract = cs_extract;
			list_add_tail(&queue_mem->node, &cs_queue_data->queue_list);
		}
	}

	mtk_log_all(queue->kctx->kbdev, true,
		"[%d_%d] Bind Idx,     Ringbuf addr,     Size, Prio,    Insert offset,   Extract offset, Active, Doorbell",
		tgid, id);
	mtk_log_all(queue->kctx->kbdev, true,
		"[%d_%d] %8d, %16llx, %8x, %4u, %16llx, %16llx, %6u, %8d",
		tgid, id,
		queue->csi_index,
		queue->base_addr,
		queue->size,
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
		int dump_countdown = 4;		/* 4 * 8 = 32, maximal dump instructions */

		mtk_log_all(queue->kctx->kbdev, true, "Dumping instructions around the last Extract offset");

		aligned_cs_extract = ALIGN_DOWN(cs_extract, 8 * instruction_size);

		/* Go 16 instructions back */
		if (aligned_cs_extract > (16 * instruction_size))
			start = aligned_cs_extract - (16 * instruction_size);
		else
			start = 0;

		/* Print upto 32 instructions */
		stop = start + (dump_countdown * 8 * instruction_size);
		if (stop > cs_insert)
			stop = cs_insert;

		mtk_log_all(queue->kctx->kbdev, true, "Instructions from Extract offset %llx", start);

		while (start != stop && dump_countdown--) {
			u64 page_off = (start & size_mask) >> PAGE_SHIFT;
			u64 offset = (start & size_mask) & ~PAGE_MASK;
			struct page *page =
				as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			u64 *ringbuffer = kmap_atomic(page);
			u64 *ptr = &ringbuffer[offset/8];

			mtk_log_all(queue->kctx->kbdev, true,
				"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx",
				ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);

			kunmap_atomic(ringbuffer);
			start += (8 * instruction_size);
		}
	}

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] SAVED_CMD_PTR: 0x%llx",
			tgid, id,
			queue->saved_cmd_ptr);
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

		if (!stream) {
			mtk_log_all(queue->kctx->kbdev, true, "[%d_%d] stream is NULL!", tgid, id);
			return;
		}

		cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_LO);
		cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_HI) << 32;
		req_res = kbase_csf_firmware_cs_output(stream, CS_STATUS_REQ_RESOURCE);

		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] CMD_PTR: 0x%llx",
			tgid, id,
			cmd_ptr);
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [COMPUTE]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [FRAGMENT]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [TILER]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		mtk_log_all(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [IDVS]: %d",
			tgid, id,
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

			mtk_log_all(queue->kctx->kbdev, true,
				"[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u",
				tgid, id,
				addr,
				val);

			/* Write offset variable address (pointer) */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
			addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
			mtk_log_all(queue->kctx->kbdev, true,
				"[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
				tgid, id,
				addr);

			/* EVENT_SIZE and EVENT_STATEs */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
			mtk_log_all(queue->kctx->kbdev, true,
				"[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
				tgid, id,
				CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
				CS_INSTR_CONFIG_EVENT_STATE_GET(val));
		} else {
			mtk_log_all(queue->kctx->kbdev, true, "[%d_%d] NO CS_TRACE", tgid, id);
		}
	}
}

static void mtk_debug_csf_scheduler_dump_active_group(struct kbase_queue_group *const group,
						struct mtk_debug_cs_queue_data *cs_queue_data)
{
	struct kbase_device *const kbdev = group->kctx->kbdev;

	if (kbase_csf_scheduler_group_get_slot(group) >= 0) {
		u32 ep_c, ep_r;
		char exclusive;
		char idle = 'N';
		struct kbase_csf_cmd_stream_group_info const *const ginfo =
			&kbdev->csf.global_iface.groups[group->csg_nr];
		u8 slot_priority = kbdev->csf.scheduler.csg_slots[group->csg_nr].priority;

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

		mtk_log_all(kbdev, true,
			"[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive, Idle",
			group->kctx->tgid,
			group->kctx->id);
		mtk_log_all(kbdev, true,
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
	} else {
		mtk_log_all(kbdev, true,
			"[%d_%d] GroupID, CSG NR, Run State, Priority",
			group->kctx->tgid,
			group->kctx->id);
		mtk_log_all(kbdev, true,
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

		mtk_log_all(kbdev, true,
			"[%d_%d] Bound queues:",
			group->kctx->tgid,
			group->kctx->id);

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
			mtk_debug_csf_scheduler_dump_active_queue(
				group->kctx->tgid,
				group->kctx->id,
				group->bound_queues[i],
				cs_queue_data);
		}
	}
}

static void mtk_debug_csf_dump_kcpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
	unsigned long idx;

	mtk_log_all(kbdev, true,
		"[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u",
		MALI_CSF_CSG_DEBUGFS_VERSION);
	mtk_log_all(kbdev, true,
		"[kcpu_queues] ##### Ctx %d_%d #####",
		kctx->tgid, kctx->id);

	if (!mtk_debug_trylock(&kctx->csf.kcpu_queues.lock)) {
		mtk_log_all(kbdev, true,
			"[%d_%d] lock csf.kcpu_queues.lock failed!",
			kctx->tgid, kctx->id);
		return;
	}

	mtk_log_all(kbdev, true,
		"[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno",
		kctx->tgid, kctx->id);
	idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

	while (idx < KBASEP_MAX_KCPU_QUEUES) {
		struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];

		if (!queue) {
			idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
			continue;
		}

		if (!mtk_debug_trylock(&queue->lock)) {
			mtk_log_all(kbdev, true,
				"[%d_%d] %9lu( lock held, bypass dump )",
				kctx->tgid, kctx->id, idx);
			continue;
		}

		mtk_log_all(kbdev, true,
			"[%d_%d] %9lu(  %s ), %16u, %11u, %7u, %12u, %13llu  %8u",
			kctx->tgid, kctx->id,
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
				u8 cmd_idx = (u8)(queue->start_offset + i);

				/* The offset to the first command that is being processed or yet to
				 * be processed is of u8 type, so the number of commands inside the
				 * queue cannot be more than 256. The current implementation expects
				 * exactly 256, any other size will require the addition of wrapping
				 * logic.
				 */
				BUILD_BUG_ON(KBASEP_KCPU_QUEUE_SIZE != 256);

				cmd = &queue->commands[cmd_idx];
				if (cmd->type >= BASE_KCPU_COMMAND_TYPE_COUNT) {
					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					mtk_log_all(kbdev, true,
						"[%d_%d] %9lu(  %s ), %7d, %9d, (unknown blocking command)",
						kctx->tgid, kctx->id,
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

					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					mtk_log_all(kbdev, true,
						"[%d_%d] %9lu(  %s ), %7d, Fence Signal, %pK %s %s",
						kctx->tgid, kctx->id,
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

					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					mtk_log_all(kbdev, true,
						"[%d_%d] %9lu(  %s ), %7d, Fence Wait, %pK %s %s",
						kctx->tgid, kctx->id,
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

					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					if (sets->nr_objs == 0) {
						mtk_log_all(kbdev, true,
							"[%d_%d] %9lu(  %s ), %7d,   CQS Set, nr_objs == 0",
							kctx->tgid, kctx->id,
							idx,
							queue->has_error ? "InErr" : "NoErr",
							cmd_idx);
					}
					for (i = 0; i < sets->nr_objs; i++) {
						mtk_log_all(kbdev, true,
							"[%d_%d] %9lu(  %s ), %7d,   CQS Set, %llx",
							kctx->tgid, kctx->id,
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

					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					if (waits->nr_objs == 0) {
						mtk_log_all(kbdev, true,
							"[%d_%d] %9lu(  %s ), %7d,  CQS Wait, nr_objs == 0\n",
							kctx->tgid, kctx->id,
							idx,
							queue->has_error ? "InErr" : "NoErr",
							cmd_idx);
					}
					for (i = 0; i < waits->nr_objs; i++) {
						struct kbase_vmap_struct *mapping;
						u32 val;
						char const *msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";
						u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);

						if (cpu_ptr) {
							val = *cpu_ptr;
							kbase_phy_alloc_mapping_put(kctx, mapping);

							mtk_log_all(kbdev, true,
								"[%d_%d] %9lu(  %s ), %7d,  CQS Wait, %llx(%u > %u, inherit_err: %s)",
								kctx->tgid, kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								waits->objs ? waits->objs[i].addr : 0,
								val,
								waits->objs ? waits->objs[i].val : 0,
								msg);
						} else {
							mtk_log_all(kbdev, true,
								"[%d_%d] %9lu(  %s ), %7d,  CQS Wait, %llx(??val?? > %u, inherit_err: %s)",
								kctx->tgid, kctx->id,
								idx,
								queue->has_error ? "InErr" : "NoErr",
								cmd_idx,
								waits->objs ? waits->objs[i].addr : 0,
								waits->objs ? waits->objs[i].val : 0,
								msg);
						}
					}
					break;
				}
				default:
					mtk_log_all(kbdev, true,
						"[%d_%d] Queue Idx(err-mode), CMD Idx, Wait Type, Additional info",
						kctx->tgid, kctx->id);
					mtk_log_all(kbdev, true,
						"[%d_%d] %9lu(  %s ), %7d, %9d, (other blocking command)",
						kctx->tgid, kctx->id,
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

static void mtk_debug_csf_dump_cpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
	if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
		mtk_log_all(kbdev, true,
			"[%d_%d] Dump request already started!",
			kctx->tgid, kctx->id);
		mutex_unlock(&kctx->csf.lock);
		return;
	}

	atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
	init_completion(&kctx->csf.cpu_queue.dump_cmp);
	kbase_event_wakeup(kctx);

	mutex_unlock(&kctx->csf.lock);

	mtk_log_all(kbdev, true,
		"[cpu_queue] CPU Queues table (version:v%u):",
		MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
	mtk_log_all(kbdev, true,
		"[cpu_queue] ##### Ctx %d_%d #####",
		kctx->tgid, kctx->id);

	if (!wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp, msecs_to_jiffies(3000))) {
		mtk_log_all(kbdev, true,
			"[%d_%d] Timeout waiting for dump completion",
			kctx->tgid, kctx->id);
		return;
	}

	if (!mtk_debug_trylock(&kctx->csf.lock)) {
		mtk_log_all(kbdev, true, "[%d_%d] lock csf.lock failed!", kctx->tgid, kctx->id);
		return;
	}

	if (kctx->csf.cpu_queue.buffer) {
		int i;
		int next_str_idx = 0;

		WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_PENDING);

		for (i = 0; i < kctx->csf.cpu_queue.buffer_size; i++) {
			if (kctx->csf.cpu_queue.buffer[i] == '\n') {
				kctx->csf.cpu_queue.buffer[i] = '\0';
				mtk_log_all(kbdev, true,
					"%s",
					&(kctx->csf.cpu_queue.buffer[next_str_idx]));
				next_str_idx = i + 1;
			}
		}

		kfree(kctx->csf.cpu_queue.buffer);
		kctx->csf.cpu_queue.buffer = NULL;
		kctx->csf.cpu_queue.buffer_size = 0;
	} else {
		mtk_log_all(kbdev, true,
			"[%d_%d] Dump error! (time out)",
			kctx->tgid, kctx->id);
	}

	atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
	mutex_unlock(&kctx->csf.lock);
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

	do {
		struct kbase_context *kctx;
		int found;
		int ret;

		mtk_log_all(kbdev, true,
			"[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u", MALI_CSF_CSG_DEBUGFS_VERSION);

		/* find kctx by pid and lock kctx->csf.lock */
		if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
			mtk_log_all(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
			break;
		}
		found = false;
		list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
			if (kctx->tgid == pid) {
				found = true;
				break;
			}
		}
		if (!found) {
			mutex_unlock(&kbdev->kctx_list_lock);
			break;
		}
		ret = mtk_debug_trylock(&kctx->csf.lock);
		mutex_unlock(&kbdev->kctx_list_lock);
		if (!ret) {
			mtk_log_all(kbdev, true, "[%d_%d] lock csf.lock failed!", kctx->tgid, kctx->id);
			break;
		}

		/* lock csf.scheduler.lock and update active groups status */
		/* previous locks: kctx->csf.lock */
		if (!mtk_debug_trylock(&kbdev->csf.scheduler.lock)) {
			mutex_unlock(&kctx->csf.lock);
			mtk_log_all(kbdev, true, "%s lock csf.scheduler.lock failed!", __func__);
			break;
		}
		kbase_csf_debugfs_update_active_groups_status(kbdev);

		/* dump active groups */
		/* previous locks: kctx->csf.lock, csf.scheduler.lock */
		{
			u32 csg_nr;
			u32 num_groups = kbdev->csf.global_iface.group_num;

			for (csg_nr = 0; csg_nr < num_groups; csg_nr++) {
				struct kbase_queue_group *const group =
					kbdev->csf.scheduler.csg_slots[csg_nr].resident_group;

				if (!group)
					continue;

				mtk_log_all(kbdev, true,
					"[active_groups] ##### Ctx %d_%d #####",
					group->kctx->tgid, group->kctx->id);

				if (dump_queue_data) {
					cs_queue_data.kctx = group->kctx;
					cs_queue_data.group_type = 0;
					cs_queue_data.handle = group->handle;
					mtk_debug_csf_scheduler_dump_active_group(group, &cs_queue_data);
				} else
					mtk_debug_csf_scheduler_dump_active_group(group, NULL);
			}
		}

		/* dump groups */
		/* previous locks: kctx->csf.lock, csf.scheduler.lock */
		{
			u32 gr;

			for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
				struct kbase_queue_group *const group = kctx->csf.queue_groups[gr];

				if (!group)
					continue;

				mtk_log_all(kbdev, true,
					"[groups] ##### Ctx %d_%d #####",
					group->kctx->tgid, group->kctx->id);

				if (dump_queue_data) {
					cs_queue_data.kctx = group->kctx;
					cs_queue_data.group_type = 1;
					cs_queue_data.handle = group->handle;
					mtk_debug_csf_scheduler_dump_active_group(group, &cs_queue_data);
				} else
					mtk_debug_csf_scheduler_dump_active_group(group, NULL);
			}
		}

		/* unlock csf.scheduler.lock */
		/* previous locks: kctx->csf.lock, csf.scheduler.lock */
		mutex_unlock(&kbdev->csf.scheduler.lock);

		/* dump kcpu queues */
		/* previous locks: kctx->csf.lock */
		mtk_debug_csf_dump_kcpu_queues(kbdev, kctx);

		/* dump firmware trace buffer */
		/* previous locks: kctx->csf.lock */
		KBASE_KTRACE_DUMP(kbdev);

		/* dump cpu queues and unlock kctx->csf.lock */
		/* previous locks: kctx->csf.lock */
		mtk_debug_csf_dump_cpu_queues(kbdev, kctx);

		/* dump command stream buffer */
		/* previous locks: none */
		if (dump_queue_data) {
			if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
				mtk_log_all(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
				break;
			}
			mtk_debug_cs_queue_data_dump(kbdev, &cs_queue_data);
			mutex_unlock(&kbdev->kctx_list_lock);
		}
	} while (0);

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

static void __attribute__((unused)) mtk_debug_dump_for_external_fence(int fd, int pid, int type, int timeouts)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

#if !MALI_USE_CSF
	struct kbase_context *kctx = NULL;
	struct kbase_jd_atom *katom;
	struct list_head *entry, *tmp;
#endif

	if (IS_ERR_OR_NULL(kbdev))
		return;

	mutex_lock(&fence_debug_lock);

	mtk_log_all(kbdev, true,
		"%s: mali fence timeouts(%d ms)! fence_fd=%d pid=%d",
		fence_timeout_type_to_string(type),
		timeouts,
		fd,
		pid);

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
#if !MALI_USE_CSF
		/*
		 * While holding the struct kbase_jd_context lock clean up jobs which are known to kbase but are
		 * queued outside the job scheduler.
		 */
		mtk_log_all(kbdev, true,
			"External fence timeouts(%d ms)! Cancel soft job",
			timeouts);
		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
			mutex_lock(&kctx->jctx.lock);

			del_timer_sync(&kctx->soft_job_timeout);
			list_for_each_safe(entry, tmp, &kctx->waiting_soft_jobs) {
				katom = list_entry(entry, struct kbase_jd_atom, queue);
				kbase_cancel_soft_job(katom);
			}
			mutex_unlock(&kctx->jctx.lock);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
#endif

		if (kbase_prepare_to_reset_gpu(kbdev, RESET_FLAGS_NONE)) {
			mtk_log_all(kbdev, true,
				"External fence timeouts(%d ms)! Trigger GPU reset",
				timeouts);
			kbase_reset_gpu(kbdev);
		} else {
			mtk_log_all(kbdev, true,
				"External fence timeouts(%d ms)! Other threads are already resetting the GPU",
				timeouts);
		}
	}
#endif /* CONFIG_MALI_MTK_TIMEOUT_RESET */

	mutex_unlock(&fence_debug_lock);
}

int mtk_debug_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	/* Hook null to deprecated the debug dump for GED swd fence monitor */
	mtk_gpu_fence_debug_dump_fp = NULL;

	return 0;
}

int mtk_debug_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_gpu_fence_debug_dump_fp = NULL;

	return 0;
}
