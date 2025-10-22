// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_DEBUG_H__
#define __MTK_PLATFORM_DEBUG_H__

#define KBASE_PLATFORM_TAG  "[KBASE/PLATFORM]"
#define KBASE_PLATFORM_LOGD(fmt, args...) \
	do { if (KBASE_PLATFORM_DEBUG_ENABLE) \
            {pr_info(KBASE_PLATFORM_TAG"[DEBUG]@%s: "fmt"\n", __func__, ##args);} \
        else \
            {pr_debug(KBASE_PLATFORM_TAG"[DEBUG]@%s: "fmt"\n", __func__, ##args);} \
        } while (0)
#define KBASE_PLATFORM_LOGE(fmt, args...) \
	pr_info(KBASE_PLATFORM_TAG"[ERROR]@%s: "fmt"\n", __func__, ##args)
#define KBASE_PLATFORM_LOGI(fmt, args...) \
	pr_info(KBASE_PLATFORM_TAG"[INFO]@%s: "fmt"\n", __func__, ##args)

/*
 * memory dump mode control for command stream buffers
 */
#define MTK_DEBUG_MEM_DUMP_DISABLE	0
#define MTK_DEBUG_MEM_DUMP_CS_BUFFER	0b001			/* dump command stream buffers */
#define MTK_DEBUG_MEM_DUMP_MASK		0b001

/*
 * Dump command stream buffers that associated with command queue groups.
 */
struct mtk_debug_cs_queue_mem_data {
	struct list_head node;

	struct kbase_context *kctx;
	pid_t tgid;
	u32 id;
	int group_type;		/* 0: active groups, 1: groups */
	u8 handle;
	s8 csi_index;
	u32 size;
	u64 base_addr;
	u64 cs_extract;
	u64 cs_insert;
};

struct mtk_debug_cs_queue_data {
	struct list_head queue_list;
	struct kbase_context *kctx;
	int group_type;		/* 0: active groups, 1: groups */
	u8 handle;
};

/* record the visited pages to speedup cs_queue_mem dump */
struct mtk_debug_cs_queue_dump_record_gpu_addr {
	struct list_head list_node;
	u64 gpu_addr;
	void *cpu_addr;
	u8 bitmap[(PAGE_SIZE / 64) / 8];	/* cache line size as dump unit */
};

struct mtk_debug_cs_queue_dump_record_kctx {
	struct list_head list_node;
	struct list_head record_list;
	struct kbase_context *kctx;
};

struct mtk_debug_cs_queue_dump_record {
	struct list_head record_list;
};

union mtk_debug_csf_instruction {
	struct {
		u64 pad: 56;
		u64 opcode: 8;
	} inst;
	struct {
		u64 imm: 48;
		u64 dest: 8;
		u64 opcode: 8;
	} move;
	struct {
		u64 imm: 32;
		u64 res0: 16;
		u64 dest: 8;
		u64 opcode: 8;
	} move32;
	struct {
		u64 res0: 32;
		u64 src1: 8;
		u64 src0: 8;
		u64 res1: 8;
		u64 opcode: 8;
	} call;
};

int mtk_debug_init(struct kbase_device *kbdev);
int mtk_debug_term(struct kbase_device *kbdev);
int mtk_debug_csf_debugfs_init(struct kbase_device *kbdev);

void mtk_debug_dump_pm_status(struct kbase_device *kbdev);
#if IS_ENABLED(CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG)
// check if active cag queue 3 blocked in resource, if yes do more dump for debug
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid, bool check_resource);
#else
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid);
#endif /* CONFIG_MALI_MTK_BLOCKED_RESOURCE_DEBUG*/
#endif /* __MTK_PLATFORM_DEBUG_H__ */
