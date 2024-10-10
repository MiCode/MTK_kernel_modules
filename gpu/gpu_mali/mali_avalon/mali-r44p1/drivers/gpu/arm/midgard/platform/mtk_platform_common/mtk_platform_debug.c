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

#define mtk_log_critical_exception(kbdev, to_dev, fmt, args...) \
	do { \
		if (to_dev) \
			dev_info(kbdev->dev, fmt, ##args); \
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, fmt "\n", ##args); \
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
#define mtk_log_critical_exception(kbdev, to_dev, fmt, args...) \
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

#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
#define NO_ISSUE_FOUND 0
#define ISSUE_BLOCKED_IN_RESOURCE 1

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif /* CONFIG_MTK_AEE_FEATURE */
#include "csf/mali_kbase_csf_firmware_log.h"
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

extern void (*mtk_gpu_fence_debug_dump_fp)(int fd, int pid, int type, int timeouts);

static DEFINE_MUTEX(fence_debug_lock);
//#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT) && IS_ENABLED(CONFIG_MALI_MTK_FENCE_DEBUG)
//void kbase_csf_dump_firmware_trace_buffer(struct kbase_device *kbdev);
//#endif
static int mem_dump_mode = MTK_DEBUG_MEM_DUMP_DISABLE;

__attribute__((unused)) static int mtk_debug_trylock(struct mutex *lock)
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

#if IS_ENABLED(CONFIG_DEBUG_FS)
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
	dev_info(kbdev->dev, "[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d",
			 kbdev->pm.backend.hwcnt_desired,
			 kbdev->pm.backend.hwcnt_disabled,
			 kbdev->pm.backend.poweroff_wait_in_progress,
			 kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
			 kbdev->pm.backend.poweron_required);
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[CSF] firmware_inited=%d firmware_reloaded=%d firmware_reload_needed=%d interrupt_received=%d\n",
			 kbdev->csf.firmware_inited,
			 kbdev->csf.firmware_reloaded,
			 kbdev->csf.firmware_reload_needed,
			 kbdev->csf.interrupt_received);
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[CSF] firmware_hctl_core_pwr=%d glb_init_request_pending=%d scheduler.pm_active_count=%d\n",
			 kbdev->csf.firmware_hctl_core_pwr,
			 kbdev->csf.glb_init_request_pending,
			 kbdev->csf.scheduler.pm_active_count);
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[PM] in_reset=%d reset_done=%d gpu_powered=%d gpu_ready=%d mcu_state=%s l2_state=%s mcu_desired=%d l2_desired=%d l2_always_on=%d\n",
			 kbdev->pm.backend.in_reset,
			 kbdev->pm.backend.reset_done,
			 kbdev->pm.backend.gpu_powered,
			 kbdev->pm.backend.gpu_ready,
			 mtk_debug_mcu_state_to_string(kbdev->pm.backend.mcu_state),
			 mtk_debug_l2_core_state_to_string(kbdev->pm.backend.l2_state),
			 kbdev->pm.backend.mcu_desired,
			 kbdev->pm.backend.l2_desired,
			 kbdev->pm.backend.l2_always_on);
	mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
		"[PM] hwcnt_desired=%d hwcnt_disabled=%d poweroff_wait_in_progress=%d invoke_poweroff_wait_wq_when_l2_off=%d poweron_required=%d\n",
			 kbdev->pm.backend.hwcnt_desired,
			 kbdev->pm.backend.hwcnt_disabled,
			 kbdev->pm.backend.poweroff_wait_in_progress,
			 kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off,
			 kbdev->pm.backend.poweron_required);
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
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
#define USING_ZLIB_COMPRESSING		1
#define MAX_CS_DUMP_NUM_KCTX		16
#define MAX_CS_DUMP_NUM_CSG		(MAX_CS_DUMP_NUM_KCTX * 2)
#define MAX_CS_DUMP_NUM_CSI_PER_CSG	5
#define MAX_CS_DUMP_QUEUE_MEM		(MAX_CS_DUMP_NUM_CSG * MAX_CS_DUMP_NUM_CSI_PER_CSG)
#define MAX_CS_DUMP_NUM_GPU_PAGES	256
#define MAX_CS_DUMP_COUNT_PER_CSI	128
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
#define MAX_CSI_DUMP_CACHE_LINES	1024
#else
#define MAX_CSI_DUMP_CACHE_LINES	512
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

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
	if (unlikely(ZERO_OR_NULL_PTR(cs_dump_queue_mem)))
		goto err_queue_mem_pre_allocate;
	cs_dump_queue_mem_ptr = 0;

	cs_dump_kctx_node = kmalloc(sizeof(*cs_dump_kctx_node) * MAX_CS_DUMP_NUM_KCTX, GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR(cs_dump_kctx_node)))
		goto err_kctx_node_pre_allocate;
	cs_dump_kctx_node_ptr = 0;

	cs_dump_gpu_addr_node = kmalloc(sizeof(*cs_dump_gpu_addr_node) * MAX_CS_DUMP_NUM_GPU_PAGES, GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR(cs_dump_gpu_addr_node)))
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
static unsigned int mtk_debug_csi_dump_cache_line_count;

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

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
#include <linux/zlib.h>
#include <linux/zutil.h>

#define STREAM_END_SPACE 0
#define ZLIB_OUT_SIZE ((uint32_t)PAGE_SIZE)

static z_stream def_strm;
static bool mtk_debug_cs_using_zlib = 1;
static unsigned char *mtk_debug_cs_zlib_out;

static int mtk_debug_cs_alloc_workspaces(struct kbase_device *kbdev)
{
	int workspacesize = zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL);

	def_strm.workspace = vmalloc(workspacesize);
	if (unlikely(ZERO_OR_NULL_PTR(def_strm.workspace))) {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
			"Allocat %d bytes for deflate workspace failed!", workspacesize);
		return -ENOMEM;
	}

	mtk_debug_cs_zlib_out = kmalloc(ZLIB_OUT_SIZE, GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR(mtk_debug_cs_zlib_out))) {
		mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
			"Allocat %u bytes for deflate output buffer failed!", ZLIB_OUT_SIZE);
		vfree(def_strm.workspace);
		return -ENOMEM;
	}

	return 0;
}

static void mtk_debug_cs_free_workspaces(void)
{
	kfree(mtk_debug_cs_zlib_out);
	vfree(def_strm.workspace);
}

static int mtk_debug_cs_zlib_compress(unsigned char *data_in, unsigned char *cpage_out,
	uint32_t *sourcelen, uint32_t *dstlen)
{
	int ret;

	if (*dstlen <= STREAM_END_SPACE)
		return -1;

	if (zlib_deflateInit(&def_strm, Z_DEFAULT_COMPRESSION) != Z_OK)
		return -1;

	def_strm.next_in = data_in;
	def_strm.total_in = 0;

	def_strm.next_out = cpage_out;
	def_strm.total_out = 0;

	while (def_strm.total_out < *dstlen - STREAM_END_SPACE && def_strm.total_in < *sourcelen) {
		def_strm.avail_out = *dstlen - (def_strm.total_out + STREAM_END_SPACE);
		def_strm.avail_in = min_t(unsigned long,
			(*sourcelen - def_strm.total_in), def_strm.avail_out);
		ret = zlib_deflate(&def_strm, Z_PARTIAL_FLUSH);
		if (ret != Z_OK) {
			zlib_deflateEnd(&def_strm);
			return -1;
		}
	}
	def_strm.avail_out += STREAM_END_SPACE;
	def_strm.avail_in = 0;
	ret = zlib_deflate(&def_strm, Z_FINISH);
	zlib_deflateEnd(&def_strm);

	if (ret != Z_STREAM_END)
		return -1;

	if (def_strm.total_out > def_strm.total_in)
		return -1;

	*dstlen = def_strm.total_out;
	*sourcelen = def_strm.total_in;

	return 0;
}
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

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
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
	unsigned char *zlib_ptr = NULL;
	uint32_t srclen;
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

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
			if (mtk_debug_csi_dump_cache_line_count >= MAX_CSI_DUMP_CACHE_LINES) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
						"%016llx: MAX_CSI_DUMP_CACHE_LINES(%d) too small",
						gpu_addr, MAX_CSI_DUMP_CACHE_LINES);
				return NULL;
			}
			mtk_debug_csi_dump_cache_line_count++;

			gpu_addr_node->bitmap[bitmap_idx] |= bitmap_chk;
			if (bitmap_chk == 1 << (rows_per_map - 1)) {
				bitmap_idx += 1;
				bitmap_chk = 1;
			} else
				bitmap_chk <<= 1;
		}

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
		if (!mtk_debug_cs_using_zlib)
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
		{
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
			continue;
		}

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
		if (zlib_ptr) {
			if (zlib_ptr + srclen == (unsigned char *)ptr)
				srclen += 64;
			else {
				typeof(ptr) ptr_cpu;
				u64 ptr_gpu = gpu_addr + (((u64)zlib_ptr) & ~PAGE_MASK);
				uint32_t dstlen = ZLIB_OUT_SIZE;
				int ret;

				ret = mtk_debug_cs_zlib_compress(zlib_ptr, mtk_debug_cs_zlib_out, &srclen, &dstlen);
				if (ret) {
					ptr_cpu = (typeof(ptr_cpu))zlib_ptr;
					for (col = 0; col < srclen / sizeof(*ptr); col += 4) {
						mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
							"%016llx: %016llx %016llx %016llx %016llx",
							ptr_gpu + col * sizeof(*ptr),
							ptr_cpu[col + 0], ptr_cpu[col + 1],
							ptr_cpu[col + 2], ptr_cpu[col + 3]);
					}
				} else {
					ptr_cpu = (typeof(ptr_cpu))mtk_debug_cs_zlib_out;
					for (col = 0; col < (dstlen + sizeof(*ptr) - 1) / sizeof(*ptr); col += 4) {
						mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
							"%010llx(%02lx/%03x/%02x): %016llx %016llx %016llx %016llx",
							ptr_gpu, (col * sizeof(*ptr)) / 32, dstlen, srclen / 64,
							ptr_cpu[col + 0], ptr_cpu[col + 1],
							ptr_cpu[col + 2], ptr_cpu[col + 3]);
					}
				}
				zlib_ptr = (unsigned char *)ptr;
				srclen = 64;
			}
		} else {
			zlib_ptr = (unsigned char *)ptr;
			srclen = 64;
		}
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
	}

#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
	if (mtk_debug_cs_using_zlib && zlib_ptr) {
		typeof(ptr) ptr_cpu;
		u64 ptr_gpu = gpu_addr + (((u64)zlib_ptr) & ~PAGE_MASK);
		uint32_t dstlen = ZLIB_OUT_SIZE;
		int ret;

		ret = mtk_debug_cs_zlib_compress(zlib_ptr, mtk_debug_cs_zlib_out, &srclen, &dstlen);
		if (ret) {
			ptr_cpu = (typeof(ptr_cpu))zlib_ptr;
			for (col = 0; col < srclen / sizeof(*ptr); col += 4) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%016llx: %016llx %016llx %016llx %016llx",
					ptr_gpu + col * sizeof(*ptr),
					ptr_cpu[col + 0], ptr_cpu[col + 1],
					ptr_cpu[col + 2], ptr_cpu[col + 3]);
			}
		} else {
			ptr_cpu = (typeof(ptr_cpu))mtk_debug_cs_zlib_out;
			for (col = 0; col < (dstlen + sizeof(*ptr) - 1) / sizeof(*ptr); col += 4) {
				mtk_log_regular(kbdev, mtk_debug_cs_dump_mode,
					"%010llx(%02lx/%03x/%02x): %016llx %016llx %016llx %016llx",
					ptr_gpu, (col * sizeof(*ptr)) / 32, dstlen, srclen / 64,
					ptr_cpu[col + 0], ptr_cpu[col + 1],
					ptr_cpu[col + 2], ptr_cpu[col + 3]);
			}
		}
	}
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */

	return gpu_addr_node->cpu_addr;
}

#define MTK_DEBUG_CSF_REG_NUM_MAX 128

union mtk_debug_csf_register_file {
	u32 reg32[MTK_DEBUG_CSF_REG_NUM_MAX];
	u64 reg64[MTK_DEBUG_CSF_REG_NUM_MAX / 2];
};

static int mtk_debug_csf_reg_num;
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
			if (reg >= mtk_debug_csf_reg_num || reg & 0x1)
				break;
			rf->reg64[reg >> 1] = inst->move.imm;
			break;
		case 0b00000010:			/* MOVE32 */
			reg = (int)inst->move.dest;
			if (reg >= mtk_debug_csf_reg_num)
				break;
			rf->reg32[reg] = inst->move32.imm;
			break;
		case 0b00100000:			/* CALL */
			reg = (int)inst->call.src0;
			if (reg >= mtk_debug_csf_reg_num || reg & 0x1)
				break;
			buffer = rf->reg64[reg >> 1];
			if (!buffer || buffer & (8 - 1))
				break;
			reg = (int)inst->call.src1;
			if (reg >= mtk_debug_csf_reg_num)
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
	mtk_debug_csi_dump_cache_line_count = 0;
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
	const u32 gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;

	BUILD_BUG_ON(MTK_DEBUG_CSF_REG_NUM_MAX < 128);
	if ((gpu_id & GPU_ID2_PRODUCT_MODEL) >= GPU_ID2_PRODUCT_TTIX)
		mtk_debug_csf_reg_num = 128;
	else
		mtk_debug_csf_reg_num = 96;

	mtk_log_critical_exception(kbdev, true, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);
	mtk_log_regular(kbdev, false, "[cs_mem_dump] start: %d", mtk_debug_cs_dump_count);

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

	mtk_log_regular(kbdev, false, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count);
	mtk_log_critical_exception(kbdev, true, "[cs_mem_dump] stop: %d", mtk_debug_cs_dump_count);
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

static const char *condition_to_string(u32 condition)
{
	static char condition_str_buf[16];
	int ret;

	if (condition == CS_STATUS_WAIT_SYNC_WAIT_CONDITION_LE)
		return "less or equal";
	else if (condition == CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GT)
		return "greater than";
	else if (condition == CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GE)
		return "greater or equal";

	ret = snprintf(condition_str_buf, sizeof(condition_str_buf), "unknown(%u)", condition & 0xF);
	if (ret > 0)
		return condition_str_buf;
	else
		return "unknown";
}

#define WAITING "Waiting"
#define NOT_WAITING "Not waiting"

static void mtk_debug_csf_scheduler_dump_active_queue_cs_status_wait(
	struct kbase_device *kbdev, pid_t tgid, u32 id,
	u32 glb_version, u32 wait_status, u32 wait_sync_value, u64 wait_sync_live_value,
	u64 wait_sync_pointer, u32 sb_status, u32 blocked_reason)
{
	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
		tgid, id,
		CS_STATUS_WAIT_SB_MASK_GET(wait_status),
		CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
	if (sb_source_supported(glb_version)) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] SB_SOURCE: %d",
			tgid, id,
			CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
	}
	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
		tgid, id,
		CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
		condition_to_string(CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status)),
		wait_sync_pointer);
	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
		tgid, id,
		wait_sync_value,
		wait_sync_live_value,
		CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] BLOCKED_REASON: %s",
		tgid, id,
		blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));

	// BLOCKED_REASON:
	//     - WAIT: Blocked on scoreboards in some way.
	//     - RESOURCE: Blocked on waiting for resource allocation. e.g., compute, tiler, and fragment resources.
	//     - SYNC_WAIT: Blocked on a SYNC_WAIT{32|64} instruction.
}

static void mtk_debug_csf_scheduler_dump_queue_cs_status_wait(
	struct kbase_device *kbdev, pid_t tgid, u32 id,
	u32 glb_version, u32 wait_status, u32 wait_sync_value, u64 wait_sync_live_value,
	u64 wait_sync_pointer, u32 sb_status, u32 blocked_reason)
{
	if (CS_STATUS_WAIT_SB_MASK_GET(wait_status) ||
	    CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ||
	    CS_STATUS_WAIT_PROTM_PEND_GET(wait_status)) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] SB_MASK: %d, PROGRESS_WAIT: %s, PROTM_PEND: %s",
			tgid, id,
			CS_STATUS_WAIT_SB_MASK_GET(wait_status),
			CS_STATUS_WAIT_PROGRESS_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
			CS_STATUS_WAIT_PROTM_PEND_GET(wait_status) ? WAITING : NOT_WAITING);
		if (sb_source_supported(glb_version)) {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] SB_SOURCE: %d",
				tgid, id,
				CS_STATUS_WAIT_SB_SOURCE_GET(wait_status));
		}
	}
	if (CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status)) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] SYNC_WAIT: %s, WAIT_CONDITION: %s, SYNC_POINTER: 0x%llx",
			tgid, id,
			CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status) ? WAITING : NOT_WAITING,
			condition_to_string(CS_STATUS_WAIT_SYNC_WAIT_CONDITION_GET(wait_status)),
			wait_sync_pointer);
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] SYNC_VALUE: %d, SYNC_LIVE_VALUE: 0x%016llx, SB_STATUS: %u",
			tgid, id,
			wait_sync_value,
			wait_sync_live_value,
			CS_STATUS_SCOREBOARDS_NONZERO_GET(sb_status));
	}
	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] BLOCKED_REASON: %s",
		tgid, id,
		blocked_reason_to_string(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason)));

	// BLOCKED_REASON:
	//     - WAIT: Blocked on scoreboards in some way.
	//     - RESOURCE: Blocked on waiting for resource allocation. e.g., compute, tiler, and fragment resources.
	//     - SYNC_WAIT: Blocked on a SYNC_WAIT{32|64} instruction.
}
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
static int mtk_debug_csf_scheduler_dump_active_queue(pid_t tgid, u32 id,
						struct kbase_queue *queue,
						struct mtk_debug_cs_queue_data *cs_queue_data,
						bool check_resource)
#else
static void mtk_debug_csf_scheduler_dump_active_queue(pid_t tgid, u32 id,
						struct kbase_queue *queue,
						struct mtk_debug_cs_queue_data *cs_queue_data)
#endif
{
	u64 *addr;
	u32 *addr32;
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
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
		return NO_ISSUE_FOUND;
#else
		return;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

	if (queue->csi_index == KBASEP_IF_NR_INVALID || !queue->group)
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
                return NO_ISSUE_FOUND;
#else
                return;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

	if (!queue->user_io_addr) {
		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] user_io_addr is NULL! csi_index=%8d base_addr=%16llx priority=%4u doorbell_nr=%8d",
			tgid, id,
			queue->csi_index,
			queue->base_addr,
			queue->priority,
			queue->doorbell_nr);
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
                return NO_ISSUE_FOUND;
#else
                return;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
	}

	glb_version = queue->kctx->kbdev->csf.global_iface.version;

	addr = queue->user_io_addr;
	cs_insert = addr[CS_INSERT_LO / sizeof(*addr)];

	addr = queue->user_io_addr + PAGE_SIZE / sizeof(*addr);
	cs_extract = addr[CS_EXTRACT_LO / sizeof(*addr)];

	addr32 = (u32 *)(queue->user_io_addr + PAGE_SIZE / sizeof(*addr));
	cs_active = addr32[CS_ACTIVE / sizeof(*addr32)];

	/* record queue dump info, skip off slot CS when cs_extract != cs_insert */
	if (cs_queue_data && (cs_queue_data->group_type == 0 || cs_extract != cs_insert)) {
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

	mtk_log_critical_exception(queue->kctx->kbdev, true,
		"[%d_%d] Bind Idx,     Ringbuf addr,     Size, Prio,    Insert offset,   Extract offset, Active, Doorbell",
		tgid, id);
	mtk_log_critical_exception(queue->kctx->kbdev, true,
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

		mtk_log_critical_exception(queue->kctx->kbdev, true, "Dumping instructions around the last Extract offset");

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

		mtk_log_critical_exception(queue->kctx->kbdev, true, "Instructions from Extract offset %llx", start);

		while (start != stop && dump_countdown--) {
			u64 page_off = (start & size_mask) >> PAGE_SHIFT;
			u64 offset = (start & size_mask) & ~PAGE_MASK;
			struct page *page =
				as_page(queue->queue_reg->gpu_alloc->pages[page_off]);
			u64 *ringbuffer = vmap(&page, 1, VM_MAP, pgprot_noncached(PAGE_KERNEL));

			if (!ringbuffer) {
				mtk_log_critical_exception(queue->kctx->kbdev, true, "%s failed to map the buffer page for read a command!",
					__func__);
			} else {
				u64 *ptr = &ringbuffer[offset / 8];
				mtk_log_critical_exception(queue->kctx->kbdev, true,
					"%016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx",
					ptr[0], ptr[1], ptr[2], ptr[3],	ptr[4], ptr[5], ptr[6], ptr[7]);
				vunmap(ringbuffer);
			}
			start += (8 * instruction_size);
		}
	}

	/* Print status information for blocked group waiting for sync object. For on-slot queues,
	 * if cs_trace is enabled, dump the interface's cs_trace configuration.
	 */
	if (kbase_csf_scheduler_group_get_slot(queue->group) < 0) {
		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] SAVED_CMD_PTR: 0x%llx",
			tgid, id,
			queue->saved_cmd_ptr);

		if (queue->status_wait) {
			wait_status = queue->status_wait;
			wait_sync_value = queue->sync_value;
			wait_sync_pointer = queue->sync_ptr;
			sb_status = queue->sb_status;
			blocked_reason = queue->blocked_reason;
			if (CS_STATUS_WAIT_SYNC_WAIT_GET(wait_status)) {
				evt = (u64 *)kbase_phy_alloc_mapping_get(queue->kctx, wait_sync_pointer, &mapping);
				if (evt) {
					wait_sync_live_value = evt[0];
					kbase_phy_alloc_mapping_put(queue->kctx, mapping);
				} else
					wait_sync_live_value = U64_MAX;
			} else
				wait_sync_live_value = U64_MAX;

			mtk_debug_csf_scheduler_dump_queue_cs_status_wait(
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
			mtk_log_critical_exception(queue->kctx->kbdev, true, "[%d_%d] stream is NULL!", tgid, id);
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	                return NO_ISSUE_FOUND;
#else
        	        return;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
		}

		cmd_ptr = kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_LO);
		cmd_ptr |= (u64)kbase_csf_firmware_cs_output(stream, CS_STATUS_CMD_PTR_HI) << 32;
		req_res = kbase_csf_firmware_cs_output(stream, CS_STATUS_REQ_RESOURCE);

		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] CMD_PTR: 0x%llx",
			tgid, id,
			cmd_ptr);
		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [COMPUTE]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_COMPUTE_RESOURCES_GET(req_res));
		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [FRAGMENT]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_FRAGMENT_RESOURCES_GET(req_res));
		mtk_log_critical_exception(queue->kctx->kbdev, true,
			"[%d_%d] REQ_RESOURCE [TILER]: %d",
			tgid, id,
			CS_STATUS_REQ_RESOURCE_TILER_RESOURCES_GET(req_res));
		mtk_log_critical_exception(queue->kctx->kbdev, true,
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

			mtk_log_critical_exception(queue->kctx->kbdev, true,
				"[%d_%d] CS_TRACE_BUF_ADDR: 0x%16llx, SIZE: %u",
				tgid, id,
				addr,
				val);

			/* Write offset variable address (pointer) */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_LO);
			addr = ((u64)kbase_csf_firmware_cs_input_read(stream, CS_INSTR_BUFFER_OFFSET_POINTER_HI) << 32) | val;
			mtk_log_critical_exception(queue->kctx->kbdev, true,
				"[%d_%d] CS_TRACE_BUF_OFFSET_PTR: 0x%16llx",
				tgid, id,
				addr);

			/* EVENT_SIZE and EVENT_STATEs */
			val = kbase_csf_firmware_cs_input_read(stream, CS_INSTR_CONFIG);
			mtk_log_critical_exception(queue->kctx->kbdev, true,
				"[%d_%d] TRACE_EVENT_SIZE: 0x%x, TRACE_EVENT_STAES 0x%x",
				tgid, id,
				CS_INSTR_CONFIG_EVENT_SIZE_GET(val),
				CS_INSTR_CONFIG_EVENT_STATE_GET(val));
		} else {
			mtk_log_critical_exception(queue->kctx->kbdev, true, "[%d_%d] NO CS_TRACE", tgid, id);
		}
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
		if ( check_resource &&
			(queue->csi_index == 3) &&
			(CS_STATUS_BLOCKED_REASON_REASON_GET(blocked_reason) == CS_STATUS_BLOCKED_REASON_REASON_RESOURCE) ) {
			return ISSUE_BLOCKED_IN_RESOURCE;
		}
#endif
	}
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	return NO_ISSUE_FOUND;
#endif
}
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
static int mtk_debug_csf_scheduler_dump_active_group_extra(struct kbase_queue_group *const group,
                                                struct mtk_debug_cs_queue_data *cs_queue_data, bool check_resource);
static void mtk_debug_csf_scheduler_dump_active_group(struct kbase_queue_group *const group,
						struct mtk_debug_cs_queue_data *cs_queue_data) {
	mtk_debug_csf_scheduler_dump_active_group_extra(group, cs_queue_data, false);
}
static int mtk_debug_csf_scheduler_dump_active_group_extra(struct kbase_queue_group *const group,
						struct mtk_debug_cs_queue_data *cs_queue_data, bool check_resource)
#else
static void mtk_debug_csf_scheduler_dump_active_group(struct kbase_queue_group *const group,
						struct mtk_debug_cs_queue_data *cs_queue_data)
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
{
	struct kbase_device *const kbdev = group->kctx->kbdev;
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	int ret = NO_ISSUE_FOUND;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG*/

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

		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] GroupID, CSG NR, CSG Prio, Run State, Priority, C_EP(Alloc/Req), F_EP(Alloc/Req), T_EP(Alloc/Req), Exclusive, Idle",
			group->kctx->tgid,
			group->kctx->id);
		mtk_log_critical_exception(kbdev, true,
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
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] GroupID, CSG NR, Run State, Priority",
			group->kctx->tgid,
			group->kctx->id);
		mtk_log_critical_exception(kbdev, true,
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

		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] Bound queues:",
			group->kctx->tgid,
			group->kctx->id);

		for (i = 0; i < MAX_SUPPORTED_STREAMS_PER_GROUP; i++) {
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
			if ( mtk_debug_csf_scheduler_dump_active_queue(
				 group->kctx->tgid,
				 group->kctx->id,
				 group->bound_queues[i],
				 cs_queue_data, check_resource) == ISSUE_BLOCKED_IN_RESOURCE 
				) {
				ret = ISSUE_BLOCKED_IN_RESOURCE;
			}
#else
			mtk_debug_csf_scheduler_dump_active_queue(
				group->kctx->tgid,
				group->kctx->id,
				group->bound_queues[i],
				cs_queue_data);
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG*/
		}
	}

#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	return ret;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG*/
}

#if IS_ENABLED(CONFIG_SYNC_FILE)

static void mtk_debug_csf_dump_kcpu_cmd_fence_signal(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	struct kbase_sync_fence_info info = { 0 };

	if (cmd->info.fence.fence)
		kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
	else
		scnprintf(info.name, sizeof(info.name), "NULL");

	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] %9lu(  %s ), %4d(-), Fence Signal, %pK %s %s",
		kctx->tgid, kctx->id,
		idx,
		queue->has_error ? "InErr" : "NoErr",
		cmd_idx,
		info.fence,
		info.name,
		kbase_sync_status_string(info.status));
}

static void mtk_debug_csf_dump_kcpu_cmd_fence_wait(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	struct kbase_sync_fence_info info = { 0 };

	if (cmd->info.fence.fence)
		kbase_sync_fence_info_get(cmd->info.fence.fence, &info);
	else
		scnprintf(info.name, sizeof(info.name), "NULL");

	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] %9lu(  %s ), %4d(-),   Fence Wait, %pK %s %s",
		kctx->tgid, kctx->id,
		idx,
		queue->has_error ? "InErr" : "NoErr",
		cmd_idx,
		info.fence,
		info.name,
		kbase_sync_status_string(info.status));
}

#endif /* IS_ENABLED(CONFIG_SYNC_FILE) */

static void mtk_debug_csf_dump_kcpu_cmd_cqs_wait(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	unsigned int i;
	struct kbase_kcpu_command_cqs_wait_info *waits = &cmd->info.cqs_wait;

	if (waits->nr_objs == 0) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),     CQS Wait, nr_objs == 0\n",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	if (waits->objs == NULL) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),     CQS Wait, objs == NULL\n",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	for (i = 0; i < waits->nr_objs; i++) {
		struct kbase_vmap_struct *mapping;
		u32 val;
		char const *msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";
		u32 *const cpu_ptr = (u32 *)kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);

		if (cpu_ptr) {
			val = *cpu_ptr;
			kbase_phy_alloc_mapping_put(kctx, mapping);

			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %9lu(  %s ), %4d(%d),     CQS Wait, %llx(%u > %u, inherit_err: %s)",
				kctx->tgid, kctx->id,
				idx,
				queue->has_error ? "InErr" : "NoErr",
				cmd_idx, i,
				waits->objs[i].addr,
				val,
				waits->objs[i].val,
				msg);
		} else {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %9lu(  %s ), %4d(%d),     CQS Wait, %llx(??val?? > %u, inherit_err: %s)",
				kctx->tgid, kctx->id,
				idx,
				queue->has_error ? "InErr" : "NoErr",
				cmd_idx, i,
				waits->objs[i].addr,
				waits->objs[i].val,
				msg);
		}
	}
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_set(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	unsigned int i;
	struct kbase_kcpu_command_cqs_set_info *sets = &cmd->info.cqs_set;

	if (sets->nr_objs == 0) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),      CQS Set, nr_objs == 0",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	if (sets->objs == NULL) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),      CQS Set, objs == NULL\n",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	for (i = 0; i < sets->nr_objs; i++) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(%d),      CQS Set, %llx",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx, i,
			sets->objs[i].addr);
	}
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_wait_operation(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	unsigned int i;
	struct kbase_kcpu_command_cqs_wait_operation_info *waits = &cmd->info.cqs_wait_operation;

	if (waits->nr_objs == 0) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),   CQS WaitOp, nr_objs == 0\n",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	if (waits->objs == NULL) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),   CQS WaitOp, objs == NULL\n",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	for (i = 0; i < waits->nr_objs; i++) {
		char const *msg = (waits->inherit_err_flags && (1U << i)) ? "true" : "false";
		struct kbase_vmap_struct *mapping;
		void *const cpu_ptr = kbase_phy_alloc_mapping_get(kctx, waits->objs[i].addr, &mapping);
		u64 val;

		if (cpu_ptr == NULL) {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(??val?? > %llu, inherit_err: %s)",
				kctx->tgid, kctx->id,
				idx,
				queue->has_error ? "InErr" : "NoErr",
				cmd_idx, i,
				waits->objs[i].addr,
				waits->objs[i].val,
				msg);
			continue;
		}

		if (waits->objs[i].data_type == BASEP_CQS_DATA_TYPE_U32)
			val = (u64)*((u32 *)cpu_ptr);
		else if (waits->objs[i].data_type == BASEP_CQS_DATA_TYPE_U64)
			val = *((u64 *)cpu_ptr);
		else {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(invalid data_type (%d), inherit_err: %s)",
				kctx->tgid, kctx->id,
				idx,
				queue->has_error ? "InErr" : "NoErr",
				cmd_idx, i,
				waits->objs[i].addr,
				waits->objs[i].data_type,
				msg);
			kbase_phy_alloc_mapping_put(kctx, mapping);
			continue;
		}
		kbase_phy_alloc_mapping_put(kctx, mapping);

		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(%d),   CQS WaitOp, %llx(%llu %s %llu, inherit_err: %s)",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx, i,
			waits->objs[i].addr,
			val,
			waits->objs[i].operation == BASEP_CQS_WAIT_OPERATION_LE ?
				"<=" : (waits->objs[i].operation == BASEP_CQS_WAIT_OPERATION_GT ? ">" : "InvalidOp"),
			waits->objs[i].val,
			msg);
	}
}

static void mtk_debug_csf_dump_kcpu_cmd_cqs_set_operation(struct kbase_device *kbdev, struct kbase_context *kctx,
	unsigned long idx, struct kbase_kcpu_command_queue *queue, u8 cmd_idx, struct kbase_kcpu_command *cmd)
{
	unsigned int i;
	struct kbase_kcpu_command_cqs_set_operation_info *sets = &cmd->info.cqs_set_operation;

	if (sets->nr_objs == 0) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),    CQS SetOp, nr_objs == 0",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	if (sets->objs == NULL) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(-),    CQS SetOp, objs == NULL",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx);
		return;
	}
	for (i = 0; i < sets->nr_objs; i++) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] %9lu(  %s ), %4d(%d),    CQS SetOp, %llx(%llu)",
			kctx->tgid, kctx->id,
			idx,
			queue->has_error ? "InErr" : "NoErr",
			cmd_idx, i,
			sets->objs[i].addr,
			sets->objs[i].val);
	}
}

static void mtk_debug_csf_dump_kcpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
	unsigned long idx;

	mtk_log_critical_exception(kbdev, true,
		"[kcpu_queues] MALI_CSF_KCPU_DEBUGFS_VERSION: v%u",
		MALI_CSF_CSG_DEBUGFS_VERSION);
	mtk_log_critical_exception(kbdev, true,
		"[kcpu_queues] ##### Ctx %d_%d #####",
		kctx->tgid, kctx->id);

	if (!mtk_debug_trylock(&kctx->csf.kcpu_queues.lock)) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] lock csf.kcpu_queues.lock failed!",
			kctx->tgid, kctx->id);
		return;
	}

	mtk_log_critical_exception(kbdev, true,
		"[%d_%d] Queue Idx(err-mode), Pending Commands, Enqueue err, Blocked, Start Offset, Fence context  &  seqno",
		kctx->tgid, kctx->id);
	idx = find_first_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES);

	while (idx < KBASEP_MAX_KCPU_QUEUES) {
		struct kbase_kcpu_command_queue *queue = kctx->csf.kcpu_queues.array[idx];
		int i;

		if (!queue) {
			idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
			continue;
		}

		if (!mtk_debug_trylock(&queue->lock)) {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %9lu( lock held, bypass dump )",
				kctx->tgid, kctx->id, idx);
			idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
			continue;
		}

		mtk_log_critical_exception(kbdev, true,
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

		if (queue->num_pending_cmds)
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] Queue Idx(err-mode), CMD Idx,    Wait Type, Additional info",
				kctx->tgid, kctx->id);

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
				mtk_log_critical_exception(kbdev, true,
					"[%d_%d] %9lu(  %s ), %4d(-), %12d, (unknown blocking command)",
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
				mtk_debug_csf_dump_kcpu_cmd_fence_signal(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
			case BASE_KCPU_COMMAND_TYPE_FENCE_WAIT:
				mtk_debug_csf_dump_kcpu_cmd_fence_wait(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
#endif /* IS_ENABLED(CONFIG_SYNC_FILE) */
			case BASE_KCPU_COMMAND_TYPE_CQS_WAIT:
				mtk_debug_csf_dump_kcpu_cmd_cqs_wait(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
			case BASE_KCPU_COMMAND_TYPE_CQS_SET:
				mtk_debug_csf_dump_kcpu_cmd_cqs_set(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
			case BASE_KCPU_COMMAND_TYPE_CQS_WAIT_OPERATION:
				mtk_debug_csf_dump_kcpu_cmd_cqs_wait_operation(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
			case BASE_KCPU_COMMAND_TYPE_CQS_SET_OPERATION:
				mtk_debug_csf_dump_kcpu_cmd_cqs_set_operation(kbdev, kctx, idx, queue, cmd_idx, cmd);
				break;
			default:
				mtk_log_critical_exception(kbdev, true,
					"[%d_%d] %9lu(  %s ), %4d(-), %12d, (other blocking command)",
					kctx->tgid, kctx->id,
					idx,
					queue->has_error ? "InErr" : "NoErr",
					cmd_idx,
					cmd->type);
				break;
			}
		}

		mutex_unlock(&queue->lock);
		idx = find_next_bit(kctx->csf.kcpu_queues.in_use, KBASEP_MAX_KCPU_QUEUES, idx + 1);
	}

	mutex_unlock(&kctx->csf.kcpu_queues.lock);
}

#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
#define CSHW_BASE 0x0030000
#define CSHW_CSHWIF_0 0x4000 /* () CSHWIF 0 registers */
#define CSHWIF(n) (CSHW_BASE + CSHW_CSHWIF_0 + (n)*256)
#define CSHWIF_REG(n, r) (CSHWIF(n) + r)
#define NR_HW_INTERFACES 4

static void dump_hwif_registers(struct kbase_device *kbdev)
{
	unsigned long flags;
	unsigned int i;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	for (i = 0; kbdev->pm.backend.gpu_powered && (i < NR_HW_INTERFACES); i++) {
		u64 cmd_ptr = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x0)) |
			((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x4)) << 32);

		if (!cmd_ptr)
			continue;
		mtk_log_critical_exception(kbdev, true, "Register dump of CSHWIF %d", i);
		mtk_log_critical_exception(kbdev, true, "CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x",
			cmd_ptr,
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x8)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xC)) << 32),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x24)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x34)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x60)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x64)) << 32),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x74)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x78)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x7C)));
		mtk_log_critical_exception(kbdev, true, "CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx",
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x80)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x98)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xA4)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xAC)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB0)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB8)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xBC)) << 32));
		dev_info(kbdev->dev, "\n");
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

#define CSHW_IT_COMP_REG(r) (CSHW_BASE + 0x1000 + r)
#define CSHW_IT_FRAG_REG(r) (CSHW_BASE + 0x2000 + r)
#define CSHW_IT_TILER_REG(r)(CSHW_BASE + 0x3000 + r)

static void dump_iterator_registers(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	if (kbdev->pm.backend.gpu_powered) {
		mtk_log_critical_exception(kbdev, true, "Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x80)) | ((u64)kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x84)) << 32));
		mtk_log_critical_exception(kbdev, true, "Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x80)) | ((u64)kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x84)) << 32));
		mtk_log_critical_exception(kbdev, true, "Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x SUSPEND_BUF %llx",
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x80)) | ((u64)kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x84)) << 32));
		dev_info(kbdev->dev, "\n");
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

static void mtk_debug_csf_dump_cpu_queues(struct kbase_device *kbdev, struct kbase_context *kctx)
{
	lockdep_assert_held(&kctx->csf.lock);

#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */
	if (atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_COMPLETE) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] Dump request already started!",
			kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
		mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */
		mutex_unlock(&kctx->csf.lock);
		return;
	}

	atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_ISSUED);
	init_completion(&kctx->csf.cpu_queue.dump_cmp);
	kbase_event_wakeup(kctx);
#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#else
	mutex_unlock(&kctx->csf.lock);
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */

	mtk_log_critical_exception(kbdev, true,
		"[cpu_queue] CPU Queues table (version:v%u):",
		MALI_CSF_CPU_QUEUE_DEBUGFS_VERSION);
	mtk_log_critical_exception(kbdev, true,
		"[cpu_queue] ##### Ctx %d_%d #####",
		kctx->tgid, kctx->id);

	if (!wait_for_completion_timeout(&kctx->csf.cpu_queue.dump_cmp, msecs_to_jiffies(3000))) {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] Timeout waiting for dump completion",
			kctx->tgid, kctx->id);
#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
		mutex_unlock(&kctx->csf.lock);
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */
		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
		return;
	}

#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_lock(&kctx->csf.cpu_queue.lock);
#else
	if (!mtk_debug_trylock(&kctx->csf.lock)) {
		mtk_log_critical_exception(kbdev, true, "[%d_%d] lock csf.lock failed!", kctx->tgid, kctx->id);
		atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
		return;
	}
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */
	if (kctx->csf.cpu_queue.buffer) {
		int i;
		int next_str_idx = 0;

		WARN_ON(atomic_read(&kctx->csf.cpu_queue.dump_req_status) != BASE_CSF_CPU_QUEUE_DUMP_PENDING);

		for (i = 0; i < kctx->csf.cpu_queue.buffer_size; i++) {
			if (kctx->csf.cpu_queue.buffer[i] == '\n') {
				kctx->csf.cpu_queue.buffer[i] = '\0';
				mtk_log_critical_exception(kbdev, true,
					"%s",
					&(kctx->csf.cpu_queue.buffer[next_str_idx]));
				next_str_idx = i + 1;
			}
		}

		kfree(kctx->csf.cpu_queue.buffer);
		kctx->csf.cpu_queue.buffer = NULL;
		kctx->csf.cpu_queue.buffer_size = 0;
	} else {
		mtk_log_critical_exception(kbdev, true,
			"[%d_%d] Dump error! (time out)",
			kctx->tgid, kctx->id);
	}

	atomic_set(&kctx->csf.cpu_queue.dump_req_status, BASE_CSF_CPU_QUEUE_DUMP_COMPLETE);
#if IS_ENABLED(CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY) || IS_ENABLED(CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT)
	mutex_unlock(&kctx->csf.cpu_queue.lock);
#endif /* CONFIG_MALI_MTK_CROSS_QUEUE_SYNC_RECOVERY || CONFIG_MALI_MTK_CPUQ_DUMP_ENHANCEMENT */
	mutex_unlock(&kctx->csf.lock);
}

#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid, bool check_resource)
#else
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid)
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
{
	bool dump_queue_data;
	static struct mtk_debug_cs_queue_data cs_queue_data;
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	int active_group_dump_ret = NO_ISSUE_FOUND;
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

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
	if (dump_queue_data) {
		INIT_LIST_HEAD(&cs_queue_data.queue_list);
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
		if (mtk_debug_cs_using_zlib && mtk_debug_cs_alloc_workspaces(kbdev) != 0)
			mtk_debug_cs_using_zlib = 0;
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
	}

	do {
		struct kbase_context *kctx, *kctx_dump;
		int found;
		int ret;

		mtk_log_critical_exception(kbdev, true,
			"[active_groups] MALI_CSF_CSG_DEBUGFS_VERSION: v%u", MALI_CSF_CSG_DEBUGFS_VERSION);

		/* find kctx by pid and lock kctx->csf.lock */
		if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
			mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
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
		mutex_unlock(&kbdev->kctx_list_lock);
		ret = mtk_debug_trylock(&kctx->csf.lock);
		if (!ret) {
			mtk_log_critical_exception(kbdev, true,
				"[%d_%d] %s lock csf.lock failed!", kctx->tgid, kctx->id, __func__);
			break;
		}

		/* lock csf.scheduler.lock and update active groups status */
		/* previous locks: kctx->csf.lock */
		if (!mtk_debug_trylock(&kbdev->csf.scheduler.lock)) {
			mutex_unlock(&kctx->csf.lock);
			mtk_log_critical_exception(kbdev, true, "%s lock csf.scheduler.lock failed!", __func__);
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

				mtk_log_critical_exception(kbdev, true,
					"[active_groups] ##### Ctx %d_%d #####",
					group->kctx->tgid, group->kctx->id);

				if (dump_queue_data) {
					cs_queue_data.kctx = group->kctx;
					cs_queue_data.group_type = 0;
					cs_queue_data.handle = group->handle;
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
					if(mtk_debug_csf_scheduler_dump_active_group_extra(group, &cs_queue_data, check_resource) == ISSUE_BLOCKED_IN_RESOURCE) {
						active_group_dump_ret = ISSUE_BLOCKED_IN_RESOURCE;
					}
#else
					mtk_debug_csf_scheduler_dump_active_group(group, &cs_queue_data);
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
				} else
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
					if(mtk_debug_csf_scheduler_dump_active_group_extra(group, NULL, check_resource) == ISSUE_BLOCKED_IN_RESOURCE) {
						active_group_dump_ret = ISSUE_BLOCKED_IN_RESOURCE;
					}
#else
					mtk_debug_csf_scheduler_dump_active_group(group, NULL);
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */
			}
		}

		/* dump groups */
		if (!mtk_debug_trylock(&kbdev->kctx_list_lock)) {
			mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
			mutex_unlock(&kbdev->csf.scheduler.lock);
			mutex_unlock(&kctx->csf.lock);
			break;
		}

		/* previous locks: kctx->csf.lock, csf.scheduler.lock */
		list_for_each_entry(kctx_dump, &kbdev->kctx_list, kctx_list_link) {
			u32 gr;

			if (kctx_dump != kctx) {
				if (!mtk_debug_trylock(&kctx_dump->csf.lock)) {
					mtk_log_critical_exception(kbdev, true,
						"[%d_%d] %s lock csf.lock failed, skip group dump!",
						kctx_dump->tgid, kctx_dump->id, __func__);
					continue;
				}
			}

			for (gr = 0; gr < MAX_QUEUE_GROUP_NUM; gr++) {
				struct kbase_queue_group *const group = kctx_dump->csf.queue_groups[gr];

				if (!group || kbase_csf_scheduler_group_get_slot(group) >= 0)
					continue;

				mtk_log_critical_exception(kbdev, true,
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

			if (kctx_dump != kctx)
				mutex_unlock(&kctx_dump->csf.lock);
		}
		mutex_unlock(&kbdev->kctx_list_lock);

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
				mtk_log_critical_exception(kbdev, true, "%s lock kctx_list_lock failed!", __func__);
				break;
			}
			mtk_debug_cs_queue_data_dump(kbdev, &cs_queue_data);
			mutex_unlock(&kbdev->kctx_list_lock);
		}
	} while (0);

#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
	if (active_group_dump_ret == ISSUE_BLOCKED_IN_RESOURCE) {
		mtk_log_critical_exception(kbdev, true,
		"Debug dump for resource");

		dump_hwif_registers(kbdev);
		dump_iterator_registers(kbdev);

		if (kbase_csf_firmware_ping_wait(kbdev, 10))
			mtk_log_critical_exception(kbdev, true,
			"Ping FW error");
	}
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG */

	if (dump_queue_data) {
#if USING_ZLIB_COMPRESSING && IS_ENABLED(CONFIG_ZLIB_DEFLATE)
		if (mtk_debug_cs_using_zlib)
			mtk_debug_cs_free_workspaces();
#endif /* USING_ZLIB_COMPRESSING && CONFIG_ZLIB_DEFLATE */
		mtk_debug_cs_queue_free_memory();
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

	mtk_log_critical_exception(kbdev, true,
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
		mtk_log_critical_exception(kbdev, true,
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
			mtk_log_critical_exception(kbdev, true,
				"External fence timeouts(%d ms)! Trigger GPU reset",
				timeouts);
			kbase_reset_gpu(kbdev);
		} else {
			mtk_log_critical_exception(kbdev, true,
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
