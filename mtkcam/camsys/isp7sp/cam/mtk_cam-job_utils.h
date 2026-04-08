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
struct mtk_cam_ufbc_header_entry;
struct mtk_cam_ufbc_header;

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

	/* cached metadata buffer va */
	void *meta_cfg_buf_va;
	void *meta_stats0_buf_va;
	void *meta_stats1_buf_va;

	/* for stagger case */
	bool filled_hdr_buffer;

	struct mtk_cam_ufbc_header *ufbc_header;
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

int job_prev_exp_num_seamless(struct mtk_cam_job *job);
int job_prev_exp_num(struct mtk_cam_job *job);
int job_exp_num(struct mtk_cam_job *job);
int scen_max_exp_num(struct mtk_cam_scen *scen);
int get_subsample_ratio(struct mtk_cam_scen *scen);
u64 infer_i2c_deadline_ns(struct mtk_cam_job *job, u64 frame_interval_ns);
u64 infer_cq_trigger_deadline_ns(struct mtk_cam_job *job, u64 frame_interval_ns);

int get_raw_subdev_idx(unsigned long used_pipe);
int get_sv_subdev_idx(unsigned long used_pipe);
unsigned int get_master_engines(unsigned int used_engine);
unsigned int get_master_raw_id(unsigned int used_engine);
unsigned int get_master_sv_id(unsigned int used_engine);

int fill_img_in(struct mtkcam_ipi_img_input *ii,
		struct mtk_cam_buffer *buf,
		struct mtk_cam_video_device *node,
		int id_overwite);
int fill_img_out(struct req_buffer_helper *helper,
		 struct mtkcam_ipi_img_output *io,
		 struct mtk_cam_buffer *buf,
		 struct mtk_cam_video_device *node);
int fill_img_out_w(struct req_buffer_helper *helper,
		   struct mtkcam_ipi_img_output *io,
		   struct mtk_cam_buffer *buf,
		   struct mtk_cam_video_device *node);
int get_buf_plane(int exp_order_ipi, int exp_seq_num);
int get_plane_per_exp(bool is_rgbw);
int get_plane_buf_offset(bool w_path);
int get_buf_offset_idx(int plane, int plane_per_exp, int plane_buf_offset,
		       bool is_valid_mp_buf);
int fill_mp_img_out_hdr(struct req_buffer_helper *helper,
			struct mtkcam_ipi_img_output *io,
			struct mtk_cam_buffer *buf,
			struct mtk_cam_video_device *node, int id,
			unsigned int plane,
			unsigned int plane_per_exp,
			unsigned int plane_buf_offset);
int fill_mp_img_in_hdr(struct mtkcam_ipi_img_input *ii,
		       struct mtk_cam_buffer *buf,
		       struct mtk_cam_video_device *node, int id,
		       unsigned int plane,
		       unsigned int plane_per_exp,
		       unsigned int plane_buf_offset);
int fill_img_in_by_exposure(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int fill_m2m_rawi_to_img_in_ipi(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int fill_imgo_out_subsample(struct req_buffer_helper *helper,
			    struct mtkcam_ipi_img_output *io,
			    struct mtk_cam_buffer *buf,
			    struct mtk_cam_video_device *node,
			    int sub_ratio);
int fill_yuvo_out_subsample(struct req_buffer_helper *helper,
			    struct mtkcam_ipi_img_output *io,
			    struct mtk_cam_buffer *buf,
			    struct mtk_cam_video_device *node,
			    int sub_ratio);
int fill_sv_img_fp(struct req_buffer_helper *helper,
	struct mtk_cam_buffer *buf, struct mtk_cam_video_device *node);
int fill_imgo_buf_as_working_buf(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int update_work_buffer_to_ipi_frame(struct req_buffer_helper *helper);
int update_sensor_meta_buffer_to_ipi_frame(struct mtk_cam_job *job,
	struct mtkcam_ipi_frame_param *fp);

int write_ufbc_header_to_buf(struct mtk_cam_ufbc_header *ufbc_header);
int add_ufbc_header_entry(struct req_buffer_helper *helper,
		unsigned int pixelformat, int ipi_video_id,
		struct mtk_cam_buffer *buf, int plane, unsigned int offset);

struct mtkcam_ipi_crop v4l2_rect_to_ipi_crop(const struct v4l2_rect *r);
bool ipi_crop_eq(const struct mtkcam_ipi_crop *s,
				 const struct mtkcam_ipi_crop *d);
int get_sv_tag_idx(unsigned int exp_no, unsigned int tag_order, bool is_w);

int get_hw_scenario(struct mtk_cam_job *job);
int get_sw_feature(struct mtk_cam_job *job);
int get_exp_order(struct mtk_cam_scen *scen);

bool is_vhdr(struct mtk_cam_job *job);
bool is_dc_mode(struct mtk_cam_job *job);
bool is_sv_pure_raw(struct mtk_cam_job *job);
bool is_rgbw(struct mtk_cam_job *job);
bool is_extisp(struct mtk_cam_job *job);
bool is_dcg_sensor_merge(struct mtk_cam_job *job);
bool is_dcg_ap_merge(struct mtk_cam_job *job);
bool is_m2m(struct mtk_cam_job *job);
bool is_m2m_apu(struct mtk_cam_job *job);
bool is_m2m_apu_dc(struct mtk_cam_job *job);
bool is_stagger_lbmf(struct mtk_cam_job *job);
bool is_camsv_16p(struct mtk_cam_job *job);
int raw_video_id_w_port(int rawi_id);
void get_stagger_rawi_table(struct mtk_cam_job *job,
	const int **rawi_table, int *cnt);

int map_ipi_vpu_point(int vpu_point);
int map_ipi_imgo_path(int v4l2_raw_path);

bool find_video_node(struct mtk_cam_job *job, int node_id);
bool is_pure_raw_node(struct mtk_cam_job *job,
		      struct mtk_cam_video_device *node);
bool is_processed_raw_node(struct mtk_cam_job *job,
			   struct mtk_cam_video_device *node);

struct mtk_raw_ctrl_data *get_raw_ctrl_data(struct mtk_cam_job *job);
struct mtk_raw_sink_data *get_raw_sink_data(struct mtk_cam_job *job);
struct mtk_camsv_sink_data *get_sv_sink_data(struct mtk_cam_job *job);

bool has_valid_mstream_exp(struct mtk_cam_job *job);

u32 get_used_raw_num(struct mtk_cam_job *job);
u64 get_line_time(struct mtk_cam_job *job);
u32 get_sensor_h(struct mtk_cam_job *job);
u32 get_sensor_vb(struct mtk_cam_job *job);
u32 get_sensor_fps(struct mtk_cam_job *job);
u32 get_sensor_interval_us(struct mtk_cam_job *job);
u8 get_sensor_data_pattern(struct mtk_cam_job *job);
void mtk_cam_sv_reset_tag_info(struct mtk_cam_job *job);
int handle_sv_tag(struct mtk_cam_job *job);
int handle_sv_tag_display_ic(struct mtk_cam_job *job);
int handle_sv_tag_only_sv(struct mtk_cam_job *job);
bool is_sv_img_tag_used(struct mtk_cam_job *job);

bool belong_to_current_ctx(struct mtk_cam_job *job, int ipi_pipe_id);

void fill_hdr_timestamp(struct mtk_cam_job *job,
			struct mtk_cam_ctrl_runtime_info *info);
#endif //__MTK_CAM_JOB_UTILS_H

