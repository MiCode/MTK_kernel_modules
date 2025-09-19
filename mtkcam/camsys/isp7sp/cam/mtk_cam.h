/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_H
#define __MTK_CAM_H

#include <linux/list.h>
#include <linux/of.h>
#include <linux/rpmsg.h>
#include <linux/version.h>
#include <media/media-device.h>
#include <media/media-request.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>

/* for seninf pad enum... */
#include "imgsensor-user.h"

#include "mtk_cam-ctrl.h"
#include "mtk_cam-hsf-def.h"
#include "mtk_cam-ipi.h"
#include "mtk_cam-job.h"
#include "mtk_cam-pool.h"
#include "mtk_cam-request.h"

#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-larb.h"
#include "mtk_cam-mraw.h"
#include "mtk_cam-raw.h"
#include "mtk_cam-seninf-drv.h"
#include "mtk_cam-seninf-if.h"
#include "mtk_cam-sv.h"

#include "mtk_cam-bit_mapping.h"

#include "mtk_cam-debug_option.h"
#include "mtk_cam-debug.h"

#include "mtk_cam-hsf-def.h"

#define CCD_READY 1
#define NO_CHECK_RETURN(ret) (void) ret

struct mtk_cam_debug_fs;
struct mtk_cam_request;
struct mtk_raw_pipeline;
struct mtk_sv_pipeline;
struct mtk_mraw_pipeline;

struct mtk_cam_device;
struct mtk_rpmsg_device;

#define CQ_BUF_SIZE  0x10000
#define CAM_CQ_BUF_NUM \
			max(JOB_NUM_PER_STREAM * 2, JOB_NUM_PER_STREAM_DISPLAY_IC) /* 2 for mstream */

#define SENSOR_META_BUF_SIZE 0x8000
#define SENSOR_META_BUF_NUM 8
#define RUN_ADL_FRAME_MODE_FROM_RAWI

struct mtk_cam_adl_work {
	struct work_struct work;
	struct mtk_raw_device *raw_dev;
	bool is_dc;
};

struct mtk_cam_buf_fmt_desc *get_fmt_desc(
		struct mtk_cam_driver_buf_desc *buf_desc);
int update_buf_fmt_desc(struct mtk_cam_driver_buf_desc *desc,
		struct v4l2_mbus_framefmt *mf);

struct mtk_cam_ctx {
	struct mtk_cam_device *cam;
	unsigned int stream_id;

	/* v4l2 related */
	unsigned int enabled_node_cnt;
	unsigned int streaming_node_cnt;
	int has_raw_subdev;
	u32 enable_hsf_raw;

	struct media_pipeline pipeline;
	struct v4l2_subdev *sensor;
	//struct v4l2_subdev *prev_sensor;
	struct v4l2_subdev *seninf;
	//struct v4l2_subdev *prev_seninf;
	struct v4l2_subdev *pipe_subdevs[MAX_PIPES_PER_STREAM];

	/* TODO */
	int raw_subdev_idx;
	int sv_subdev_idx[MAX_SV_PIPES_PER_STREAM];
	int num_sv_subdevs;
	int mraw_subdev_idx[MAX_MRAW_PIPES_PER_STREAM];
	int num_mraw_subdevs;
	/* stored raw data for switch exp case : prev : 1exp , next: 2exp */
	bool ctrldata_stored;
	struct mtk_raw_ctrl_data ctrldata;
	/* job pool */
	struct mtk_cam_job_data jobs[MAX_JOB_NUM_PER_STREAM];
	struct mtk_cam_pool job_pool;
	int available_jobs; /* cached value for enque */

	/* rpmsg related */
	struct rpmsg_channel_info rpmsg_channel;
	struct mtk_rpmsg_device *rpmsg_dev;
	struct work_struct session_work;
	int ipi_id;
	bool session_created;
	struct completion session_complete;
	struct completion session_flush;

	struct task_struct *sensor_worker_task;
	struct kthread_worker sensor_worker;
	struct task_struct *flow_task;
	struct kthread_worker flow_worker;
	struct task_struct *done_task;
	struct kthread_worker done_worker;
	char str_ae_data[512];

	struct mtk_cam_device_buf cq_buffer;
	struct mtk_cam_device_buf ipi_buffer;

	struct mtk_cam_pool	cq_pool;
	struct mtk_cam_pool	ipi_pool;

	struct mtk_cam_driver_buf_desc img_work_buf_desc;

	/* cached for pack job */
	struct mtk_cam_pool_wrapper *pack_job_img_wbuf_pool_wrapper;
	/*
	 * scenario dependent
	 */
	bool scenario_init;
	struct mtk_cam_device_refcnt_buf *w_caci_buf;

	/* slb */
	int slb_uid;
	void __iomem *slb_addr;
	dma_addr_t slb_iova;
	unsigned int slb_size;
	unsigned int slb_used_size;
	unsigned int ring_start_offset;

	unsigned int set_adl_aid;

	/* cmdq */
	bool cmdq_enabled;

	struct mtk_cam_adl_work adl_work;

	/* TODO:
	 * life-cycle of work buffer during switch
	 * e.g., PDI/camsv's imgo
	 */

	atomic_t streaming;
	unsigned int used_pipe;
	int used_engine;

	bool not_first_job;
	bool configured;
	struct mtk_raw_sink_data configured_raw_sink;
	struct mtkcam_ipi_config_param ipi_config;

	struct device *hw_raw[MAX_RAW_PER_STREAM];
	struct device *hw_sv;
	struct device *hw_mraw[MAX_MRAW_PIPES_PER_STREAM];
	//struct mtk_raw_pipeline *pipe;
	//struct mtk_camsv_pipeline *sv_pipe[MAX_SV_PIPES_PER_STREAM];
	//struct mtk_mraw_pipeline *mraw_pipe[MAX_MRAW_PIPES_PER_STREAM];
	struct mtk_cam_ctrl cam_ctrl;
	/* list for struct mtk_cam_job */
	struct mtk_cam_hsf_ctrl *hsf;

	/* sv tag control */
	unsigned int used_tag_cnt;
	unsigned int enabled_tags;
	struct mtk_camsv_tag_info tag_info[CAMSV_MAX_TAGS];

	/* for mmqos usage */
	struct mtk_seninf_active_line_info act_line_info;

	/* sensor meta dump */
	bool enable_sensor_meta_dump;
	bool is_sensor_meta_dump;
	struct mtk_cam_device_buf sensor_meta_buffer;
	struct mtk_cam_driver_buf_desc seninf_meta_buf_desc;
	struct mtk_cam_pool sensor_meta_pool;

	u64 sw_recovery_ts;
};

struct mtk_cam_v4l2_pipelines {
	int num_raw;
	struct mtk_raw_pipeline *raw;

	int num_camsv;
	struct mtk_camsv_pipeline *camsv;

	int num_mraw;
	struct mtk_mraw_pipeline *mraw;
};

int ctx_stream_on_seninf_sensor(struct mtk_cam_job *job,
			int seninf_pad, int raw_tg_idx);
int ctx_stream_off_seninf_sensor(struct mtk_cam_ctx *ctx);

struct mtk_cam_engines {
	int num_seninf_devices;

	int num_raw_devices;
	int num_camsv_devices;
	int num_mraw_devices;
	int num_larb_devices;

	/* raw */
	struct device **raw_devs;
	struct device **yuv_devs;
	struct device **rms_devs;

	/* camsv */
	struct device **sv_devs;

	/* mraw */
	struct device **mraw_devs;

	/* larb */
	struct device **larb_devs;

	unsigned long full_set;
	unsigned long occupied_engine;
};

struct cmdq_client;
struct mtk_cam_device {
	struct device *dev;
	struct device *smmu_dev;
	void __iomem *base;
	void __iomem *adl_base;

	struct v4l2_device v4l2_dev;
	struct v4l2_async_notifier notifier;
	struct media_device media_dev;

	atomic_t initialize_cnt;

	//TODO: for real SCP
	//struct device *smem_dev;
	//struct platform_device *scp_pdev; /* only for scp case? */
	phandle rproc_phandle;
	struct rproc *rproc_handle;

	struct mutex ccu_lock;
	int ccu_use_cnt;
	phandle rproc_ccu_phandle;
	struct rproc *rproc_ccu_handle;
	struct platform_device *ccu_pdev;

	struct cmdq_client *cmdq_clt;

	struct mtk_cam_v4l2_pipelines	pipelines;
	struct mtk_cam_engines		engines;

	/* to guarantee the enque sequence */
	atomic_t is_queuing;

	unsigned int max_stream_num;
	struct mtk_cam_ctx *ctxs;

	spinlock_t streaming_lock;
	int streaming_ctx;
	//int streaming_pipe;

	/* request related */
	struct list_head pending_job_list;
	spinlock_t pending_job_lock;

	struct list_head running_job_list;
	unsigned int running_job_count;
	spinlock_t running_job_lock;

	struct mtk_camsys_dvfs dvfs;

	struct mtk_cam_debug dbg;

	/* shutdown flow */
	wait_queue_head_t shutdown_wq;
};

static inline struct device *subdev_to_cam_dev(struct v4l2_subdev *sd)
{
	return sd->v4l2_dev->dev;
}

static inline struct mtk_cam_device *subdev_to_cam_device(struct v4l2_subdev *sd)
{
	return dev_get_drvdata(subdev_to_cam_dev(sd));
}

struct device *mtk_cam_root_dev(void);

int mtk_cam_set_dev_raw(struct device *dev, int idx,
			struct device *raw, struct device *yuv,
			struct device *rms);
int mtk_cam_set_dev_sv(struct device *dev, int idx, struct device *sv);
int mtk_cam_set_dev_mraw(struct device *dev, int idx, struct device *mraw);
 /* special case: larb dev is push back into array */
int mtk_cam_set_dev_larb(struct device *dev, struct device *larb);
struct device *mtk_cam_get_larb(struct device *dev, int larb_id);

bool mtk_cam_is_any_streaming(struct mtk_cam_device *cam);
bool mtk_cam_are_all_streaming(struct mtk_cam_device *cam,
			       unsigned long stream_mask);

int mtk_cam_get_available_engine(struct mtk_cam_device *cam);
int mtk_cam_update_engine_status(struct mtk_cam_device *cam,
				 unsigned long engine_mask, bool available);
static inline int mtk_cam_release_engine(struct mtk_cam_device *cam,
					 unsigned long engines)
{
	return mtk_cam_update_engine_status(cam, engines, true);
}

static inline int mtk_cam_occupy_engine(struct mtk_cam_device *cam,
					unsigned long engines)
{
	return mtk_cam_update_engine_status(cam, engines, false);
}

int mtk_cam_pm_runtime_engines(struct mtk_cam_engines *eng,
			       unsigned long engine_mask, int enable);

/* note: flag V4L2_MBUS_FRAMEFMT_PAD_ENABLE is defined by mtk internally */
static inline void
mtk_cam_pad_fmt_enable(struct v4l2_mbus_framefmt *framefmt, bool enable)
{
	if (enable)
		framefmt->flags |= V4L2_MBUS_FRAMEFMT_PAD_ENABLE;
	else
		framefmt->flags &= ~V4L2_MBUS_FRAMEFMT_PAD_ENABLE;
}

static inline bool
mtk_cam_is_pad_fmt_enable(struct v4l2_mbus_framefmt *framefmt)
{
	return framefmt->flags & V4L2_MBUS_FRAMEFMT_PAD_ENABLE;
}

struct mtk_cam_ctx *mtk_cam_find_ctx(struct mtk_cam_device *cam,
				     struct media_entity *entity);
struct mtk_cam_ctx *mtk_cam_start_ctx(struct mtk_cam_device *cam,
				      struct mtk_cam_video_device *node);
void mtk_cam_stop_ctx(struct mtk_cam_ctx *ctx, struct media_entity *entity);

static inline bool mtk_cam_ctx_is_adl_flow(struct mtk_cam_ctx *ctx)
{
	return !!ctx->adl_work.raw_dev;
}

static inline void mtk_cam_ctx_flush_adl_work(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_adl_work *adl_work = &ctx->adl_work;

	if (adl_work->raw_dev)
		flush_work(&adl_work->work);
}

int mtk_cam_power_ctrl_ccu(struct device *dev, int on_off);

int mtk_cam_ctx_init_scenario(struct mtk_cam_ctx *ctx);
int mtk_cam_ctx_all_nodes_streaming(struct mtk_cam_ctx *ctx);
int mtk_cam_ctx_all_nodes_idle(struct mtk_cam_ctx *ctx);
int mtk_cam_ctx_stream_on(struct mtk_cam_ctx *ctx);
int mtk_cam_ctx_stream_off(struct mtk_cam_ctx *ctx);
void mtk_cam_ctx_engine_off(struct mtk_cam_ctx *ctx);
void mtk_cam_ctx_engine_enable_irq(struct mtk_cam_ctx *ctx);
void mtk_cam_ctx_engine_disable_irq(struct mtk_cam_ctx *ctx);
void mtk_cam_ctx_engine_reset(struct mtk_cam_ctx *ctx);
void mtk_cam_ctx_engine_dc_sw_recovery(struct mtk_cam_ctx *ctx);
int mtk_cam_ctx_send_raw_event(struct mtk_cam_ctx *ctx,
			       struct v4l2_event *event);
int mtk_cam_ctx_send_sv_event(struct mtk_cam_ctx *ctx,
			       struct v4l2_event *event);

int mtk_cam_ctx_queue_sensor_worker(struct mtk_cam_ctx *ctx,
				    struct kthread_work *work);
int mtk_cam_ctx_queue_flow_worker(struct mtk_cam_ctx *ctx,
				  struct kthread_work *work);
int mtk_cam_ctx_queue_done_worker(struct mtk_cam_ctx *ctx,
				  struct kthread_work *work);

int mtk_cam_ctx_fetch_devices(struct mtk_cam_ctx *ctx, unsigned long engines);
void mtk_cam_ctx_clean_img_pool(struct mtk_cam_ctx *ctx);

int mtk_cam_ctx_alloc_rgbw_caci_buf(struct mtk_cam_ctx *ctx, int w, int h);
void mtk_cam_ctx_clean_rgbw_caci_buf(struct mtk_cam_ctx *ctx);

int mtk_cam_ctx_flush_session(struct mtk_cam_ctx *ctx);

int isp_composer_create_session(struct mtk_cam_ctx *ctx);
void isp_composer_destroy_session(struct mtk_cam_ctx *ctx);
void isp_composer_flush_session(struct mtk_cam_ctx *ctx);

int mtk_cam_call_seninf_set_pixelmode(struct mtk_cam_ctx *ctx,
				      struct v4l2_subdev *sd,
				      int pad_id, int pixel_mode);

int mtk_cam_dev_req_enqueue(struct mtk_cam_device *cam,
			    struct mtk_cam_request *req);

/* use ipi pipe/node id */
void mtk_cam_req_buffer_done(struct mtk_cam_job *job,
			     int pipe_id, int buf_state, int node_id,
			     bool is_proc);
//void mtk_cam_dev_req_cleanup(struct mtk_cam_ctx *ctx, int pipe_id, int buf_state);
//void mtk_cam_dev_req_clean_pending(struct mtk_cam_device *cam, int pipe_id,
//				   int buf_state);

int mtk_cam_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt);
int mtk_cam_seninf_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt);
int mtk_cam_sv_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt);
int mtk_cam_mraw_link_validate(struct v4l2_subdev *sd,
			  struct media_link *link,
			  struct v4l2_subdev_format *source_fmt,
			  struct v4l2_subdev_format *sink_fmt);

void mtk_engine_dump_debug_status(struct mtk_cam_device *cam,
				  unsigned long engines, bool is_srt);

struct v4l2_subdev
*mtk_cam_find_sensor_seninf(struct v4l2_subdev *subdev, int media_func);

bool mtk_cam_check_img_pool_need(struct device *dev_to_attach,
				 struct mtk_raw_ctrl_data *ctrl_data,
				 struct v4l2_mbus_framefmt *mf,
				 struct mtk_cam_driver_buf_desc *desc);
int mtk_cam_alloc_img_pool(struct device *dev_to_attach,
			   struct mtk_raw_ctrl_data *ctrl_data,
			   struct v4l2_mbus_framefmt *mf,
			   struct mtk_cam_driver_buf_desc *desc,
			   struct mtk_cam_device_buf *img_work_buffer,
			   struct mtk_cam_pool *img_work_pool);
int mtk_cam_ctx_alloc_img_pool(struct mtk_cam_ctx *ctx, struct mtk_raw_ctrl_data *ctrl_data);

void mtk_cam_destroy_img_pool(struct mtk_cam_pool_wrapper *pool_wrapper);

static inline void mtk_cam_ctx_set_raw_sink(struct mtk_cam_ctx *ctx,
					    struct mtk_raw_sink_data *sink)
{
	if (!sink) {
		memset(&ctx->configured_raw_sink, 0, sizeof(*sink));
		return;
	}
	ctx->configured_raw_sink = *sink;
}

bool mtk_cam_ctx_is_raw_sink_changed(struct mtk_cam_ctx *ctx,
				     struct mtk_raw_sink_data *sink);

bool mtk_cam_is_dcif_slb_supported(void);
void
mtk_cam_pool_wrapper_get(struct mtk_cam_pool_wrapper *wrapper);

void
mtk_cam_pool_wrapper_put(struct mtk_cam_pool_wrapper *wrapper);
struct mtk_cam_pool_wrapper*
mtk_cam_pool_wrapper_create(struct device *dev_to_attach,
			    struct mtk_raw_ctrl_data *ctrl_data,
			    struct v4l2_mbus_framefmt *mf,
			    struct mtk_cam_driver_buf_desc *desc);

void mtk_cam_pool_wrapper_destroy(struct kref *ref);

struct mtk_cam_device_refcnt_buf*
mtk_cam_device_refcnt_buf_create(struct device *dev_to_attach, size_t caci_size);
void mtk_cam_device_refcnt_buf_destroy(struct kref *ref);
void mtk_cam_device_refcnt_buf_get(struct mtk_cam_device_refcnt_buf *buf);
void mtk_cam_device_refcnt_buf_put(struct mtk_cam_device_refcnt_buf *buf);

#endif /*__MTK_CAM_H*/
