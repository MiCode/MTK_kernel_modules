/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_UTILS_H
#define __MTK_CAM_JOB_UTILS_H

#include "mtk_cam-ipi.h"

struct mtk_cam_job;
struct mtk_cam_buffer;
struct mtk_cam_video_device;

/*For state analysis and controlling for request*/
enum MTK_CAMSYS_STATE_IDX {
	E_STATE_READY = 0x0,
	E_STATE_SENINF,
	E_STATE_SENSOR,
	E_STATE_CQ,
	E_STATE_OUTER,
	E_STATE_CAMMUX_OUTER_CFG,
	E_STATE_CAMMUX_OUTER,
	E_STATE_INNER,
	E_STATE_DONE_NORMAL,
	E_STATE_CQ_SCQ_DELAY,
	E_STATE_CAMMUX_OUTER_CFG_DELAY,
	E_STATE_OUTER_HW_DELAY,
	E_STATE_INNER_HW_DELAY,
	E_STATE_DONE_MISMATCH,
	E_STATE_SUBSPL_READY = 0x10,
	E_STATE_SUBSPL_SCQ,
	E_STATE_SUBSPL_OUTER,
	E_STATE_SUBSPL_SENSOR,
	E_STATE_SUBSPL_INNER,
	E_STATE_SUBSPL_DONE_NORMAL,
	E_STATE_SUBSPL_SCQ_DELAY,
	E_STATE_TS_READY = 0x20,
	E_STATE_TS_SENSOR,
	E_STATE_TS_SV,
	E_STATE_TS_MEM,
	E_STATE_TS_CQ,
	E_STATE_TS_INNER,
	E_STATE_TS_DONE_NORMAL,
	E_STATE_EXTISP_READY = 0x30,
	E_STATE_EXTISP_SENSOR,
	E_STATE_EXTISP_SV_OUTER,
	E_STATE_EXTISP_SV_INNER,
	E_STATE_EXTISP_CQ,
	E_STATE_EXTISP_OUTER,
	E_STATE_EXTISP_INNER,
	E_STATE_EXTISP_DONE_NORMAL,
};

struct req_buffer_helper {
	struct mtk_cam_job *job;
	struct mtkcam_ipi_frame_param *fp;

	int ii_idx; /* image in */
	int io_idx; /* imgae out */
	int mi_idx; /* meta in */
	int mo_idx; /* meta out */

	/* cached metadata buffer for later */
	struct mtk_cam_buffer *meta_cfg_buf;
	struct mtk_cam_buffer *meta_stats0_buf;
	struct mtk_cam_buffer *meta_stats1_buf;

	/* for stagger case */
	bool filled_hdr_buffer;
};
struct pack_job_ops_helper {
	/* specific init for job */
	int (*job_init)(struct mtk_cam_job *job);

	/* pack job ops */
	int (*pack_job)(struct mtk_cam_job *job,
			struct pack_job_ops_helper *job_helper);
	int (*update_raw_bufs_to_ipi)(struct req_buffer_helper *helper,
				      struct mtk_cam_buffer *buf,
				      struct mtk_cam_video_device *node);
	/* special flow */
	int (*update_raw_rawi_to_ipi)(struct req_buffer_helper *helper,
				      struct mtk_cam_buffer *buf,
				      struct mtk_cam_video_device *node);
	int (*update_raw_imgo_to_ipi)(struct req_buffer_helper *helper,
				      struct mtk_cam_buffer *buf,
				      struct mtk_cam_video_device *node);
	int (*update_raw_yuvo_to_ipi)(struct req_buffer_helper *helper,
				      struct mtk_cam_buffer *buf,
				      struct mtk_cam_video_device *node);
	int (*append_work_buf_to_ipi)(struct req_buffer_helper *helper);
};
void _set_timestamp(struct mtk_cam_job *job,
	u64 time_boot, u64 time_mono);

int job_prev_exp_num(struct mtk_cam_job *job);
int job_exp_num(struct mtk_cam_job *job);
int scen_max_exp_num(struct mtk_cam_scen *scen);
int get_subsample_ratio(struct mtk_cam_scen *scen);
u64 infer_i2c_deadline_ns(struct mtk_cam_scen *scen, u64 frame_interval_ns);

int get_raw_subdev_idx(unsigned long used_pipe);
int get_sv_subdev_idx(unsigned long used_pipe);
unsigned int _get_master_engines(unsigned int used_engine);
unsigned int _get_master_raw_id(unsigned int used_engine);
unsigned int _get_master_sv_id(unsigned int used_engine);

int fill_img_in(struct mtkcam_ipi_img_input *ii,
		struct mtk_cam_buffer *buf,
		struct mtk_cam_video_device *node,
		int id_overwite);
int fill_img_out(struct mtkcam_ipi_img_output *io,
		 struct mtk_cam_buffer *buf,
		 struct mtk_cam_video_device *node);
int fill_img_out_w(struct mtkcam_ipi_img_output *io,
		 struct mtk_cam_buffer *buf,
		 struct mtk_cam_video_device *node);
int fill_img_out_hdr(struct mtkcam_ipi_img_output *io,
		     struct mtk_cam_buffer *buf,
		     struct mtk_cam_video_device *node,
		     int index, int id);
int fill_img_in_hdr(struct mtkcam_ipi_img_input *ii,
		    struct mtk_cam_buffer *buf,
		    struct mtk_cam_video_device *node, int index, int id);
int fill_img_in_by_exposure(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int fill_m2m_rawi_to_img_in_ipi(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int fill_imgo_out_subsample(struct mtkcam_ipi_img_output *io,
			    struct mtk_cam_buffer *buf,
			    struct mtk_cam_video_device *node,
			    int sub_ratio);
int fill_yuvo_out_subsample(struct mtkcam_ipi_img_output *io,
			    struct mtk_cam_buffer *buf,
			    struct mtk_cam_video_device *node,
			    int sub_ratio);
int fill_sv_img_fp(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf, struct mtk_cam_video_device *node);
int fill_imgo_buf_as_working_buf(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);

int update_work_buffer_to_ipi_frame(struct req_buffer_helper *helper);

struct mtkcam_ipi_crop v4l2_rect_to_ipi_crop(const struct v4l2_rect *r);
bool ipi_crop_eq(const struct mtkcam_ipi_crop *s,
				 const struct mtkcam_ipi_crop *d);
int get_sv_tag_idx(unsigned int exp_no, unsigned int tag_order, bool is_w);

int get_hw_scenario(struct mtk_cam_job *job);
int get_sw_feature(struct mtk_cam_job *job);
bool is_vhdr(struct mtk_cam_job *job);
bool is_dc_mode(struct mtk_cam_job *job);
bool is_sv_pure_raw(struct mtk_cam_job *job);
bool is_vhdr(struct mtk_cam_job *job);
bool is_rgbw(struct mtk_cam_job *job);
bool is_m2m(struct mtk_cam_job *job);
bool is_m2m_apu(struct mtk_cam_job *job);
bool is_hw_offline(struct mtk_cam_job *job);
int raw_video_id_w_port(int rawi_id);
void get_stagger_rawi_table(struct mtk_cam_job *job,
	const int **rawi_table, int *cnt);

int map_ipi_vpu_point(int vpu_point);
int map_ipi_imgo_path(int v4l2_raw_path);

bool require_imgo(struct mtk_cam_job *job);
bool require_pure_raw(struct mtk_cam_job *job);
bool require_proccessed_raw(struct mtk_cam_job *job);

struct mtk_raw_ctrl_data *get_raw_ctrl_data(struct mtk_cam_job *job);
struct mtk_raw_sink_data *get_raw_sink_data(struct mtk_cam_job *job);

#endif //__MTK_CAM_JOB_UTILS_H

