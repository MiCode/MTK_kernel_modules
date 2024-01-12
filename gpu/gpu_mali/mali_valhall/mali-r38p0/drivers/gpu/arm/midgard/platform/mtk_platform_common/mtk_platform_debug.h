// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
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
 * memory dump for command stream buffers and mem_view
 */
#define MTK_DEBUG_MEM_DUMP_DISABLE		0
#define MTK_DEBUG_MEM_DUMP_CS_BUFFER	0b001			/* dump command stream buffers */
#define MTK_DEBUG_MEM_DUMP_FULL_DUMP	0b010			/* dump full kctx memory */
#define MTK_DEBUG_MEM_DUMP_MASK			0b011

#define MTK_DEBUG_MEM_DUMP_HEADER	0x574549565f4d454d	/* magic number for packet header */
#define MTK_DEBUG_MEM_DUMP_FAIL		0x4c4941465f50414d	/* magic number for map fail */

#define MTK_DEBUG_MEM_DUMP_OVERFLOW	0x0001

/* keep size of packet_header == PAGE_SIZE * N */
#define MTK_DEBUG_MEM_DUMP_NODE_PAGES	1
#define MTK_DEBUG_MEM_DUMP_NODE_NUM	((MTK_DEBUG_MEM_DUMP_NODE_PAGES * PAGE_SIZE - 32) / 8)

struct mtk_debug_mem_view_node_info {
	u64 addr: 48;
	u64 nr_pages: 16;
};

struct mtk_debug_mem_view_packet_header {
	u64 tag;
	u32 tgid;
	u32 id;
	u32 nr_nodes;
	u32 nr_pages;
	u64 flags;
	struct mtk_debug_mem_view_node_info nodes[MTK_DEBUG_MEM_DUMP_NODE_NUM];
};

struct mtk_debug_mem_view_node {
	struct kbase_mem_phy_alloc *alloc;
	u64 start_pfn;
	unsigned long flags;
	int nr_pages;
};

struct mtk_debug_mem_view_dump_data {
	struct kbase_device *kbdev;
	struct kbase_context *kctx_prev;/* previous used kctx for dump */
	struct kbase_context *kctx;	/* current used kctx for dump */

	struct mtk_debug_mem_view_packet_header packet_header;
	int node_count;			/* total region nodes of the kctx */
	int page_count;			/* total pages of the kctx */

	int node_idx;			/* used to control packet header or node output index */
	int page_offset;		/* used to control page offset in a node */
	struct mtk_debug_mem_view_node mem_view_nodes[MTK_DEBUG_MEM_DUMP_NODE_NUM];
};

/*
 * Dump command stream buffers that associated with command queue groups.
 */
struct mtk_debug_cs_queue_mem_data {
	struct list_head node;

	struct kbase_context *kctx;
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
};

struct mtk_debug_cs_queue_dump_record_kctx {
	struct list_head list_node;
	struct list_head record_list;
	struct kbase_context *kctx;
};

struct mtk_debug_cs_queue_dump_record {
	struct list_head record_list;
};

/* Value of CsfSourceEncoding.register_index must be less than or equal to 95 */
#define MTK_DEBUG_CSF_REG_NUM 96
union mtk_debug_csf_register_file {
	u32 reg32[MTK_DEBUG_CSF_REG_NUM];
	u64 reg64[MTK_DEBUG_CSF_REG_NUM / 2];
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
void mtk_debug_csf_dump_groups_and_queues(struct kbase_device *kbdev, int pid);

#endif /* __MTK_PLATFORM_DEBUG_H__ */
