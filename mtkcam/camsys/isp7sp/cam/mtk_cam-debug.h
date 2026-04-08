/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_CAM_DEBUG__
#define __MTK_CAM_DEBUG__

#include <linux/mutex.h>
#include <linux/types.h>

#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <aee.h>
#endif

struct proc_dir_entry;
struct mtk_cam_device;
struct mtk_cam_debug_fs;

struct mtk_cam_dump_param {
	/* Common Debug Information*/
	char desc[64];
	__u32 request_fd;
	__u32 stream_id;
	__u64 timestamp;
	__u32 sequence;

	/* CQ dump */
	void *cq_cpu_addr;
	__u32 cq_size;
	__u64 cq_iova;
	__u32 cq_desc_offset;
	__u32 cq_desc_size;
	__u32 sub_cq_desc_offset;
	__u32 sub_cq_desc_size;

	/* meta in */
	void *meta_in_cpu_addr;
	__u32 meta_in_dump_buf_size;
	__u64 meta_in_iova;

	/* meta out 0 */
	void *meta_out_0_cpu_addr;
	__u32 meta_out_0_dump_buf_size;
	__u64 meta_out_0_iova;

	/* meta out 1 */
	void *meta_out_1_cpu_addr;
	__u32 meta_out_1_dump_buf_size;
	__u64 meta_out_1_iova;

	/* meta out 2 */
	void *meta_out_2_cpu_addr;
	__u32 meta_out_2_dump_buf_size;
	__u64 meta_out_2_iova;

	/* ipi frame param */
	struct mtkcam_ipi_frame_param *frame_params;
	__u32 frame_param_size;

	/* ipi config param */
	struct mtkcam_ipi_config_param *config_params;
	__u32 config_param_size;
};

/* exception dump */
struct mtk_cam_exception {
	struct proc_dir_entry *dump_entry;
	atomic_t ready;
	struct mutex lock;
	size_t buf_size;
	void *buf;
};

/* normal dump */
struct dump_ctrl;
struct mtk_cam_normal_dump {
	struct proc_dir_entry *dbg_entry;
	int num_ctrls;
	struct dump_ctrl *ctrls;

	atomic_long_t enabled;
};

#ifdef MAYBE_LATER
struct mtk_cam_debug_ops {
	int (*init)(struct mtk_cam_debug *dbg, struct mtk_cam_device *cam);
	void (*deinit)(struct mtk_cam_debug *dbg);

	//int (*debug_dump)(struct mtk_cam_debug *dbg,
	//  struct mtk_cam_dump_param *param);
	int (*exp_dump)(struct mtk_cam_debug *dbg,
			struct mtk_cam_dump_param *param);
};
#endif

struct mtk_cam_debug {
	struct mtk_cam_device *cam;

	struct mtk_cam_exception exp;
	struct mtk_cam_normal_dump dump;
};

int mtk_cam_debug_init(struct mtk_cam_debug *dbg, struct mtk_cam_device *cam);
void mtk_cam_debug_deinit(struct mtk_cam_debug *dbg);

/* normal dump */
bool mtk_cam_debug_dump_enabled(struct mtk_cam_debug *dbg, int pipe_id);

int mtk_cam_debug_dump(struct mtk_cam_debug *dbg,
		       int raw_pipe_id, struct mtk_cam_dump_param *param);

/* exception dump */
void mtk_cam_debug_exp_reset(struct mtk_cam_debug *dbg);
int mtk_cam_debug_exp_dump(struct mtk_cam_debug *dbg,
			   struct mtk_cam_dump_param *param);

/* AEE */
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#define WRAP_AEE_EXCEPTION(module, msg)					\
	aee_kernel_exception_api(__FILE__, __LINE__,			\
				 DB_OPT_DEFAULT | DB_OPT_FTRACE,	\
				 module, msg)

#else
#define WRAP_AEE_EXCEPTION(module, msg)	\
	WARN_ON(1, "<%s:%d> %s: %s\n", __FILE__, __LINE__, module, msg)

#endif //IS_ENABLED(CONFIG_MTK_AEE_FEATURE)

#define MSG_VSYNC_TIMEOUT	"Camsys: Vsync timeout"
#define MSG_COMPOSE_ERROR	"Camsys: compose error"
#define MSG_STREAM_ON_ERROR	"Camsys: 1st CQ done timeout"
#define MSG_DEQUE_ERROR		"Camsys: No P1 done"
#define MSG_TG_OVERRUN		"Camsys: TG Overrun Err"
#define MSG_TG_GRAB_ERROR	"Camsys: TG Grab Err"
#define MSG_M4U_TF		"Camsys: M4U TF"
#define MSG_SWITCH_FAILURE	"Camsys: switch error"
#define MSG_NORMAL_DUMP		"Camsys: normal dump"
#define MSG_CAMSV_ERROR		"Camsys: camsv error"
#define MSG_CAMSV_SEAMLESS_ERROR	"Camsys: camsv seamless error"
#define MSG_RINGBUFFER_OFL	"Camsys: dcif ringbuffer ofl"
#define MSG_DC_SKIP_FRAME	"Camsys: dc mode skip frame"

#endif /* __MTK_CAM_DEBUG__ */
