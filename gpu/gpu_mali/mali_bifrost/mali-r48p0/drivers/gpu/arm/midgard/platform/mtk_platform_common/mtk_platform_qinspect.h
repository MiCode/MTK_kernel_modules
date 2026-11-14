/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_QINSPECT_H__
#define __MTK_PLATFORM_QINSPECT_H__

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)

/*
 * mtk wait_operation/queue/fence/cqs/command extensions
 */

/* basep_cqs_wait_operation_op extension */
enum mtk_qinspect_cqs_wait_operation_op {
	MTK_BASEP_CQS_WAIT_OPERATION_LE = 0,
	MTK_BASEP_CQS_WAIT_OPERATION_GT = 1,
	MTK_BASEP_CQS_WAIT_OPERATION_GE = 5
};

/* queue extension */
enum mtk_qinspect_queue_type {
	QINSPECT_CPU_QUEUE,
	QINSPECT_KCPU_QUEUE,
	QINSPECT_GPU_QUEUE,
	QINSPECT_NONE_QUEUE
};

/* fence extension */
struct mtk_qinspect_fence_info {
	u64 context;
	u64 seqno;
};

/* cqs extension */
struct mtk_qinspect_cqs_wait_obj {
	u64 addr;
	u64 val;
	u8 operation;
	u8 data_type;
};

/* cpu command extension */
union mtk_qinspect_cpu_command_buf;

/* co-define with enum basep_cqs_type at ubase */
enum mtk_qinspect_basep_cqs_type {
	/** Event type which is backed by a memory allocation. It can have many
	 *  waiters and/or setters. The value in the memory indicates its
	 *  current state:
	 *    0: Not signaled.
	 *    1: signaled.
	 */
	MTK_BASE_CQS_TYPE_EVENT,
	/** Event type which is backed by a memory allocation. It is a counting
	 * semaphore which allows one or more operations to be executed before
	 * becoming blocked and needing a related operation to be completed.
	 * See @ref base_cqs_counting_semaphore_init for more details.
	 */
	MTK_BASE_CQS_TYPE_COUNTING,
	/** Event type which is instantiated from indirect event CQS.
	 */
	MTK_BASE_CQS_TYPE_INSTANTIATED,
	/** Event type which is backed by a memory allocation. Specifies a Timeline
	 * CQS object.
	 *
	 * It is similar to @ref MTK_BASE_CQS_TYPE_COUNTING, in that it is a 'counting'
	 * semaphore.
	 *
	 * However, the wait on such an object is defined not by
	 * base_cqs_object::basep::metadata but by separate wait operation objects,
	 * see @ref base_cqs_object_wait_operation. Multiple wait operation
	 * objects may be used on the same Timeline CQS Object.
	 *
	 * See base_cqs_u64_timeline_init() for more details.
	 */
	MTK_BASE_CQS_TYPE_TIMELINE,
	/** Event type which is backed by a memory allocation. It has a batch size
	 * specified at creation time and allows to wait until its live value
	 * reaches the batch size.
	 * See @ref base_cqs_fixed_size_batch_init for more details.
	 */
	MTK_BASE_CQS_TYPE_FIXED_SIZE_BATCH,
};

struct mtk_qinspect_cpu_object {
	u64 address;
	u64 value;
	u32 obj_no;
	u8 cqs_type;
	u8 inherit_error;
	u8 data_operation;
	u8 data_type;
	union {
		u64 pending;
		u64 batch_size;
		u64 data_value;
	};
};

/* co-define with enum mtk_qinspect_cpu_queue_work_type at ubase */
enum mtk_qinspect_cpu_queue_work_type {
	MTK_BASE_CPU_QUEUE_WORK_WAIT = 0,
	MTK_BASE_CPU_QUEUE_WORK_SET,
	MTK_BASE_CPU_QUEUE_WORK_WAIT_OP,
	MTK_BASE_CPU_QUEUE_WORK_SET_OP,
	MTK_BASE_CPU_QUEUE_WORK_COUNT
};

struct mtk_qinspect_cpu_command {
	u8 work_type;
	u32 num_objs;
	u32 cmd_idx;

	/* variables used only at kernel side */
	union mtk_qinspect_cpu_command_buf *objs;
	struct kbase_context *kctx;
};

#define MTK_QINSPECT_CPU_QUEUE_TAG 0x54434550534e4951

struct mtk_qinspect_cpu_command_queue {
	u64 tag;	/* MTK_QINSPECT_CPU_QUEUE_TAG */
	u64 queue;
	u32 thread_id;
	u32 flags;
	u16 status;
	u16 num_cmds;	/* number of commands */
	u16 num_bufs;	/* number of commands + objects */
	u8 completed;
};

union mtk_qinspect_cpu_command_buf {
	struct mtk_qinspect_cpu_object object;
	struct mtk_qinspect_cpu_command command;
	struct mtk_qinspect_cpu_command_queue queue;
};

/* gpu command extension */
union mtk_qinspect_csf_instruction {
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
	struct {
		u64 ep: 1;
		u64 s: 2;
		u64 res0: 13;
		u64 sbm: 16;
		u64 src1: 8;
		u64 src0: 8;
		u64 sb: 4;
		u64 dm: 1;
		u64 res1: 3;
		u64 opcode: 8;
	} sync_set;	/* SYNC_ADD32, SYNC_SET32, SYNC_ADD64, SYNC_SET64 */
	struct {
		u64 res0: 16;
		u64 sbm: 16;
		u64 pi: 1;
		u64 res1: 15;
		u64 se: 4;
		u64 dm: 1;
		u64 res2: 3;
		u64 opcode: 8;
	} shared_sb_inc;
	struct {
		u64 res0: 32;
		u64 pi: 1;
		u64 res1: 15;
		u64 se: 4;
		u64 res2: 4;
		u64 opcode: 8;
	} shared_sb_dec;
};

enum mtk_qinspect_gpu_command_type {
	GPU_COMMAND_TYPE_SYNC_WAIT,
	GPU_COMMAND_TYPE_SYNC_SET,
	GPU_COMMAND_TYPE_SHARED_SB_WAIT,
	GPU_COMMAND_TYPE_OTHERS,
};

struct mtk_qinspect_gpu_sync_info {
	union mtk_qinspect_csf_instruction inst;
	u64 cqs_addr;
	u64 value;
	u8 cond;
	u8 size;
};

struct mtk_qinspect_gpu_shared_sb_info {
	union mtk_qinspect_csf_instruction inst;
	u8 shared_entry;
};

struct mtk_qinspect_gpu_command {
	enum mtk_qinspect_gpu_command_type type;
	union {
		struct mtk_qinspect_gpu_sync_info gpu_sync_info;
		struct mtk_qinspect_gpu_shared_sb_info gpu_shared_sb_info;
	};
};

/* API: helper functions */
int mtk_qinspect_get_cqs_value(struct kbase_context *kctx, u64 addr, u32 *val);

/*
 * cpuq helper
 */

/* API: cpuq helper */
union mtk_qinspect_cpu_command_buf *mtk_qinspect_cpuq_internal_get_buf(struct kbase_context *kctx);
int mtk_qinspect_cpuq_internal_load_cpuq(struct kbase_context *kctx);
void mtk_qinspect_cpuq_internal_unload_cpuq(struct kbase_context *kctx);
union mtk_qinspect_cpu_command_buf *mtk_qinspect_search_cpuq_internal_by_queue_addr(struct kbase_context *kctx,
	u64 queue_addr);

/*
 * top_wait_cmd query operations
 */

/* wait_cmd type definition */
enum mtk_qinspect_wait_cmd_type {
	MTK_QINSPECT_NONE,
	MTK_QINSPECT_FENCE_WAIT,
	MTK_QINSPECT_CQS_WAIT,
	MTK_QINSPECT_SHARED_SB_WAIT,
};

/* API: top_wait_cmd query operations */
struct mtk_qinspect_cpu_command *mtk_qinspect_query_cpuq_internal_top_wait_cmd(
	union mtk_qinspect_cpu_command_buf *queue_buf, int *completed);
struct kbase_kcpu_command *mtk_qinspect_query_kcpuq_internal_top_wait_cmd(struct kbase_kcpu_command_queue *queue,
	int *blocked);
struct mtk_qinspect_gpu_command *mtk_qinspect_query_gpuq_internal_top_wait_cmd(struct kbase_queue *queue,
	u32 *blocked_reason, struct mtk_qinspect_gpu_command *gpu_cmd);

/*
 * wait_cmd build operations
 */

/* API: fence/wait object build operations */
void mtk_qinspect_build_fence_info(struct mtk_qinspect_fence_info *fence_info, u64 context, u64 seqno);
void mtk_qinspect_build_cqs_wait_obj(struct mtk_qinspect_cqs_wait_obj *cmd,
	u64 addr, u64 val, u8 operation, u8 data_type);

/*
 * fence iteration
 */

#if IS_ENABLED(CONFIG_SYNC_FILE)

struct mtk_qinspect_fence_wait_on {
	struct kbase_context *kctx;
	struct kbase_kcpu_command_queue *kcpu_queue;
};

struct mtk_qinspect_fence_wait_it {
	struct kbase_device *kbdev;
	struct kbase_context *kctx;
	struct kbase_kcpu_command_fence_info *fence_info;

	/* current wait info */
	u64 context;
	u64 seqno;

	/* kctx search record */
	struct kbase_context *kctx_next;

	/* kcpu_queue search record */
	unsigned long kcpu_queue_idx;

	/* return info */
	struct mtk_qinspect_fence_wait_on wait_on;
	bool wait_on_found;
};

/* API: fence iteration */
void mtk_qinspect_query_internal_fence_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *fence_info, struct mtk_qinspect_fence_wait_it *wait_it);
struct mtk_qinspect_fence_wait_on *mtk_qinspect_query_internal_fence_wait_it(
	struct mtk_qinspect_fence_wait_it *wait_it);
void mtk_qinspect_query_internal_fence_wait_it_done(struct mtk_qinspect_fence_wait_it *wait_it);

#endif /* IS_ENABLED(CONFIG_SYNC_FILE) */

/*
 * cqs iteration
 */

struct mtk_qinspect_cqs_wait_on {
	enum mtk_qinspect_queue_type queue_type;
	union {
		struct {	/* QINSPECT_CPU_QUEUE */
			union mtk_qinspect_cpu_command_buf *cpu_queue_buf;
		};
		struct {	/* QINSPECT_KCPU_QUEUE */
			struct kbase_kcpu_command_queue *kcpu_queue;
		};
		struct {	/* QINSPECT_GPU_QUEUE */
			struct kbase_queue *gpu_queue;
		};
	};
};

struct mtk_qinspect_cqs_wait_it {
	struct kbase_context *kctx;
	union {
		struct {	/* QINSPECT_CPU_QUEUE */
			union mtk_qinspect_cpu_command_buf *cpu_queue_buf;
			struct mtk_qinspect_cpu_command *cpu_command;
		};
		struct {	/* QINSPECT_KCPU_QUEUE */
			struct kbase_kcpu_command_queue *kcpu_queue;
			struct kbase_kcpu_command *kcpu_cmd;
		};
		struct {	/* QINSPECT_GPU_QUEUE */
			struct kbase_queue *gpu_queue;
			struct mtk_qinspect_gpu_sync_info *gpu_sync_info_wait;
		};
		struct {	/* QINSPECT_NONE_QUEUE */
			struct mtk_qinspect_cqs_wait_obj *cqs_wait;
		};
	};
	enum mtk_qinspect_queue_type queue_type;
	unsigned int wait_obj_nr;
	u64 objs_signaled_map;
	u64 objs_failure_map;
	u64 objs_match_map;
	u64 objs_deadlock_map;
	u64 objs_map_mask;

	/* current wait info */
	u64 cqs_addr;
	u64 cqs_val;
	u8 cqs_operation;
	u8 cqs_data_type;

	/* cpu_queue search record */
	union mtk_qinspect_cpu_command_buf *cpuq_next;

	/* kcpu_queue search record */
	unsigned long kcpu_queue_idx;

	/* gpu_queue search record */
	struct kbase_queue_group *queue_group;
	u32 queue_group_idx;
	unsigned int gpu_queue_idx;

	/* return info */
	struct mtk_qinspect_gpu_sync_info gpu_sync_info_ret;
	struct mtk_qinspect_cqs_wait_on wait_on;
};

/* API: cqs iteration */
void mtk_qinspect_query_internal_cqs_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *cmd, struct mtk_qinspect_cqs_wait_it *wait_it);
struct mtk_qinspect_cqs_wait_on *mtk_qinspect_query_internal_cqs_wait_it(struct mtk_qinspect_cqs_wait_it *wait_it);
int mtk_qinspect_query_internal_cqs_wait_obj(struct mtk_qinspect_cqs_wait_it *wait_it, unsigned int obj_nr,
	struct mtk_qinspect_cqs_wait_obj *wait_obj);
void mtk_qinspect_query_internal_cqs_wait_it_update_deadlock_map(struct mtk_qinspect_cqs_wait_it *wait_it);

/*
 * shared scoreboard iteration
 */

struct mtk_qinspect_shared_sb_wait_on {
	enum mtk_qinspect_queue_type queue_type;
	struct kbase_queue *gpu_queue;
};

struct mtk_qinspect_shared_sb_wait_it {
	enum mtk_qinspect_queue_type queue_type;
	struct kbase_context *kctx;
	struct kbase_queue_group *queue_group;
	struct kbase_queue *gpu_queue;

	/* current wait info */
	struct mtk_qinspect_gpu_shared_sb_info *gpu_shared_sb_wait;

	/* gpu_queue search record */
	unsigned int gpu_queue_idx;

	/* return info */
	struct mtk_qinspect_shared_sb_wait_on wait_on;
	bool wait_on_found;
};

/* API: shared scoreboard iteration */
void mtk_qinspect_query_internal_shared_sb_wait_it_init(enum mtk_qinspect_queue_type queue_type,
	void *queue, void *cmd, struct mtk_qinspect_shared_sb_wait_it *wait_it);
struct mtk_qinspect_shared_sb_wait_on *mtk_qinspect_query_internal_shared_sb_wait_it(
	struct mtk_qinspect_shared_sb_wait_it *wait_it);

#endif /* CONFIG_MALI_CSF_SUPPORT */

#endif /* __MTK_PLATFORM_QINSPECT_H__ */
