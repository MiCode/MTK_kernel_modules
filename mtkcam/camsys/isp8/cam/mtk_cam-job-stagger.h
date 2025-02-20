/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_JOB_STAGGER_H
#define __MTK_CAM_JOB_STAGGER_H


#include "mtk_cam-job_utils.h"
#include "mtk_cam-fmt_utils.h"

struct mtk_cam_job_event_info;
struct mtk_cam_job;
struct mtk_cam_buffer;
struct mtk_cam_video_device;
int get_hard_scenario_stagger(struct mtk_cam_job *job);
int wakeup_apply_sensor(struct mtk_cam_job *job);
void update_event_setting_done_stagger(struct mtk_cam_job *job,
		struct mtk_cam_job_event_info *event_info, int *action);
void update_event_sensor_try_set_stagger(struct mtk_cam_job *job,
		struct mtk_cam_job_event_info *event_info, int *action);
void update_frame_start_event_stagger(struct mtk_cam_job *job,
		struct mtk_cam_job_event_info *event_info, int *action);
int apply_cam_mux_switch(struct mtk_cam_job *job);
int wait_apply_sensor_stagger(struct mtk_cam_job *job);

int fill_imgo_buf_to_ipi_stagger(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);

#endif //__MTK_CAM_JOB_STAGGER_H

