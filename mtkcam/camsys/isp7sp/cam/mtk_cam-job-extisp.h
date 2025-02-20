#ifndef __MTK_CAM_JOB_EXTISP_H
#define __MTK_CAM_JOB_EXTISP_H

#include "mtk_cam-job_utils.h"

struct mtk_cam_ctx;
struct mtk_cam_job;
struct mtk_cam_buffer;
struct mtk_cam_video_device;

int get_extisp_meta_info(struct mtk_cam_job *job, int pad_src);
int fill_sv_ext_img_buffer_to_ipi_frame_extisp(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node);
int handle_sv_tag_extisp(struct mtk_cam_job *job);

#endif //__MTK_CAM_JOB_EXTISP_H

