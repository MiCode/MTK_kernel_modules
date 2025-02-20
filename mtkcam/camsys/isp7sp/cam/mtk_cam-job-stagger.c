// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include "mtk_cam.h"
#include "mtk_cam-job-stagger.h"
#include "mtk_cam-job_utils.h"
#include "mtk_cam-raw_ctrl.h"

int fill_imgo_buf_to_ipi_stagger(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	int ret = 0;
	struct mtk_cam_job *job = helper->job;

	if (is_processed_raw_node(job, node)) {  /* main stream */
		struct mtkcam_ipi_frame_param *fp = helper->fp;
		struct mtkcam_ipi_img_output *out;

		out = &fp->img_outs[helper->io_idx++];

		ret = fill_img_out(helper, out, buf, node);
	} else {  /* pure raw */
		ret = fill_imgo_buf_as_working_buf(helper, buf, node);
	}

	return ret;
}

int apply_cam_mux_switch(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_camsv_device *sv_dev = dev_get_drvdata(ctx->hw_sv);
	struct mtk_mraw_device *mraw_dev;
	struct mtk_mraw_pipeline *mraw_pipe;
	struct mtk_cam_seninf_mux_param param;
	struct mtk_cam_seninf_mux_setting settings[4];
	int prev_exp = job_prev_exp_num_seamless(job);
	int cur_exp = job_exp_num(job);
	int config_exposure_num = scen_max_exp_num(&job->job_scen);
	int hw_scen = get_hw_scenario(job);
	int raw_id = get_master_raw_id(job->used_engine);
	int raw_tg_idx = raw_to_tg_idx(raw_id);
	int first_tag_idx, second_tag_idx, last_tag_idx;
	int first_tag_idx_w, last_tag_idx_w, i;
	int mraw_idx;
	bool is_dc = is_dc_mode(job) ? true : false;

	/**
	 * To identify the "max" exposure_num, we use
	 * feature_active, not job->feature.raw_feature
	 * since the latter one stores the exposure_num information,
	 * not the max one.
	 */

	if (!scen_is_normal(&job->job_scen)) {
		// config_exposure_num maybe wrong,
		// this function is expected to be used in
		// JOB_TYPE_BASIC, JOB_TYPE_STAGGER
		pr_info("%s: WARNING: scen is NOT normal", __func__);
	}

	memset(settings, 0,
		sizeof(struct mtk_cam_seninf_mux_setting) * ARRAY_SIZE(settings));

	if (config_exposure_num == 3) {
		if (cur_exp == 2) {
			first_tag_idx =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_FIRST_TAG, false);
			last_tag_idx =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_LAST_TAG, false);
			settings[0].seninf = ctx->seninf;
			settings[0].source = PAD_SRC_RAW0;
			settings[0].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
			settings[0].tag_id = first_tag_idx;
			settings[0].enable = 1;

			settings[1].seninf = ctx->seninf;
			settings[1].source = PAD_SRC_RAW1;
			settings[1].camtg  = (is_dc) ?
				mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx) :
				raw_tg_idx;
			settings[1].tag_id = (is_dc) ? last_tag_idx : -1;
			settings[1].enable = 1;

			settings[2].seninf = ctx->seninf;
			settings[2].source = PAD_SRC_RAW2;
			settings[2].camtg  = -1;
			settings[2].tag_id = -1;
			settings[2].enable = 0;

			settings[3].seninf = ctx->seninf;
			settings[3].source = PAD_SRC_RAW1;
			settings[3].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx);
			settings[3].tag_id = last_tag_idx;
			settings[3].enable = 1;
		} else if (cur_exp == 1) {
			first_tag_idx =
				get_sv_tag_idx(1, MTKCAM_IPI_ORDER_FIRST_TAG, false);
			settings[0].seninf = ctx->seninf;
			settings[0].source = PAD_SRC_RAW0;
			settings[0].camtg  = (is_dc) ?
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx) :
				raw_tg_idx;
			settings[0].tag_id = (is_dc) ? first_tag_idx : -1;
			settings[0].enable = 1;

			settings[1].seninf = ctx->seninf;
			settings[1].source = PAD_SRC_RAW1;
			settings[1].camtg  = -1;
			settings[1].tag_id = -1;
			settings[1].enable = 0;

			settings[2].seninf = ctx->seninf;
			settings[2].source = PAD_SRC_RAW2;
			settings[2].camtg  = -1;
			settings[2].tag_id = -1;
			settings[2].enable = 0;

			settings[3].seninf = ctx->seninf;
			settings[3].source = PAD_SRC_RAW0;
			settings[3].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
			settings[3].tag_id = first_tag_idx;
			settings[3].enable = 1;
		} else if (cur_exp == 3) {
			first_tag_idx =
				get_sv_tag_idx(3, MTKCAM_IPI_ORDER_FIRST_TAG, false);
			second_tag_idx =
				get_sv_tag_idx(3, MTKCAM_IPI_ORDER_NORMAL_TAG, false);
			last_tag_idx =
				get_sv_tag_idx(3, MTKCAM_IPI_ORDER_LAST_TAG, false);
			settings[0].seninf = ctx->seninf;
			settings[0].source = PAD_SRC_RAW0;
			settings[0].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
			settings[0].tag_id = first_tag_idx;
			settings[0].enable = 1;

			settings[1].seninf = ctx->seninf;
			settings[1].source = PAD_SRC_RAW1;
			settings[1].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, second_tag_idx);
			settings[1].tag_id = second_tag_idx;
			settings[1].enable = 1;

			settings[2].seninf = ctx->seninf;
			settings[2].source = PAD_SRC_RAW2;
			settings[2].camtg  = (is_dc) ?
				mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx) :
				raw_tg_idx;
			settings[2].tag_id = (is_dc) ? last_tag_idx : -1;
			settings[2].enable = 1;

			settings[3].seninf = ctx->seninf;
			settings[3].source = PAD_SRC_RAW2;
			settings[3].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx);
			settings[3].tag_id = last_tag_idx;
			settings[3].enable = 1;
		}

		param.settings = &settings[0];
		param.num = 4;
		mtk_cam_seninf_streaming_mux_change(&param);
		dev_info(ctx->cam->dev,
			"[%s] switch Req:%d pre:%d cur:%d cam_mux[0-3]:[%d/%d/%d][%d/%d/%d][%d/%d/%d][%d/%d/%d]\n",
			__func__, job->frame_seq_no, prev_exp, cur_exp,
			settings[0].source, settings[0].camtg, settings[0].enable,
			settings[1].source, settings[1].camtg, settings[1].enable,
			settings[2].source, settings[2].camtg, settings[2].enable,
			settings[3].source, settings[3].camtg, settings[3].enable);
	} else if (config_exposure_num == 2) {
		int i = 0;
		if (cur_exp == 1) {
			first_tag_idx =
				get_sv_tag_idx(1, MTKCAM_IPI_ORDER_FIRST_TAG, false);
			first_tag_idx_w =
				get_sv_tag_idx(1, MTKCAM_IPI_ORDER_FIRST_TAG, true);
			settings[i].seninf = ctx->seninf;
			settings[i].source = PAD_SRC_RAW0;
			settings[i].camtg  = (is_dc) ?
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx) :
				raw_tg_idx;
			settings[i].tag_id = (is_dc) ? first_tag_idx : -1;
			settings[i++].enable = 1;

			settings[i].seninf = ctx->seninf;
			settings[i].source = PAD_SRC_RAW1;
			settings[i].camtg  = -1;
			settings[i].tag_id = -1;
			settings[i++].enable = 0;

			if (is_rgbw(job)) {
				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW_W0;
				settings[i].camtg  =
					mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx_w);
				settings[i].tag_id = first_tag_idx_w;
				settings[i++].enable = 1;

				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW_W1;
				settings[i].camtg  = -1;
				settings[i].tag_id = -1;
				settings[i++].enable = 0;
			} else {
				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW0;
				settings[i].camtg  =
					mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
				settings[i].tag_id = first_tag_idx;
				settings[i++].enable = 1;
			}
		} else if (cur_exp == 2) {
			first_tag_idx =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_FIRST_TAG, false);
			first_tag_idx_w =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_FIRST_TAG, true);
			last_tag_idx =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_LAST_TAG, false);
			last_tag_idx_w =
				get_sv_tag_idx(2, MTKCAM_IPI_ORDER_LAST_TAG, true);

			if (hw_scen == MTKCAM_IPI_HW_PATH_OTF_STAGGER_LN_INTL) {
				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW0;
				settings[i].camtg  = raw_tg_idx;
				++raw_tg_idx; // note: next seninf pad uses next raw tg id
				settings[i].tag_id = -1;
				settings[i++].enable = 1;
			}

			settings[i].seninf = ctx->seninf;
			settings[i].source = PAD_SRC_RAW0;
			settings[i].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
			settings[i].tag_id = first_tag_idx;
			settings[i++].enable = 1;

			settings[i].seninf = ctx->seninf;
			settings[i].source = PAD_SRC_RAW1;
			settings[i].camtg  = (is_dc) ?
				mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx) :
				raw_tg_idx;
			settings[i].tag_id = (is_dc) ? last_tag_idx : -1;
			settings[i++].enable = 1;

			if (is_rgbw(job)) {
				// TODO: OTF RGBW
				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW_W0;
				settings[i].camtg  =
					mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx_w);
				settings[i].tag_id = first_tag_idx_w;
				settings[i++].enable = 1;

				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW_W1;
				settings[i].camtg  =
					mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx_w);
				settings[i].tag_id = last_tag_idx_w;
				settings[i++].enable = 1;
			} else {
				settings[i].seninf = ctx->seninf;
				settings[i].source = PAD_SRC_RAW1;
				settings[i].camtg  =
					mtk_cam_get_sv_cammux_id(sv_dev, last_tag_idx);
				settings[i].tag_id = last_tag_idx;
				settings[i++].enable = 1;
			}
		}

		if (i > ARRAY_SIZE(settings)) {
			dev_info(ctx->cam->dev,
					 "ERROR: i(%d) > array size of settings(%lu)",
					 i, ARRAY_SIZE(settings));
			return -1;
		}

		param.settings = &settings[0];
		param.num = i;
		mtk_cam_seninf_streaming_mux_change(&param);
		dev_info(ctx->cam->dev,
			"[%s] switch Req:%d pre:%d cur:%d cam_mux[0-3]:[%d/%d/%d][%d/%d/%d][%d/%d/%d][%d/%d/%d]\n",
			__func__, job->frame_seq_no, prev_exp, cur_exp,
			settings[0].source, settings[0].camtg, settings[0].enable,
			settings[1].source, settings[1].camtg, settings[1].enable,
			settings[2].source, settings[2].camtg, settings[2].enable,
			settings[3].source, settings[3].camtg, settings[3].enable);
	} else if (config_exposure_num == 1) {
		first_tag_idx =
			get_sv_tag_idx(1, MTKCAM_IPI_ORDER_FIRST_TAG, false);
		first_tag_idx_w =
			get_sv_tag_idx(1, MTKCAM_IPI_ORDER_FIRST_TAG, true);
		settings[0].seninf = ctx->seninf;
		settings[0].source = PAD_SRC_RAW0;
		settings[0].camtg  = (is_dc) ?
			mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx) :
			raw_tg_idx;
		settings[0].tag_id = (is_dc) ? first_tag_idx : -1;
		settings[0].enable = 1;

		if (is_rgbw(job)) {
			settings[1].seninf = ctx->seninf;
			settings[1].source = PAD_SRC_RAW_W0;
			settings[1].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx_w);
			settings[1].tag_id = first_tag_idx_w;
			settings[1].enable = 1;
		} else {
			settings[1].seninf = ctx->seninf;
			settings[1].source = PAD_SRC_RAW0;
			settings[1].camtg  =
				mtk_cam_get_sv_cammux_id(sv_dev, first_tag_idx);
			settings[1].tag_id = first_tag_idx;
			settings[1].enable = 1;
		}
		param.settings = &settings[0];
		param.num = 2;
		mtk_cam_seninf_streaming_mux_change(&param);
		dev_info(ctx->cam->dev,
			"[%s] switch Req:%d pre:%d cur:%d cam_mux[0-3]:[%d/%d/%d][%d/%d/%d][%d/%d/%d][%d/%d/%d]\n",
			__func__, job->frame_seq_no, prev_exp, cur_exp,
			settings[0].source, settings[0].camtg, settings[0].enable,
			settings[1].source, settings[1].camtg, settings[1].enable,
			settings[2].source, settings[2].camtg, settings[2].enable,
			settings[3].source, settings[3].camtg, settings[3].enable);
	}

	if (job->is_sensor_meta_dump)
		mtk_cam_seninf_set_camtg_camsv(
			ctx->seninf,
			PAD_SRC_GENERAL0,
			mtk_cam_get_sv_cammux_id(sv_dev, SVTAG_SENSOR_META),
			SVTAG_SENSOR_META);

	for (i = 0; i < ctx->num_sv_subdevs; i++) {
		unsigned int tag_idx = mtk_cam_get_sv_tag_index(job->tag_info,
			ctx->sv_subdev_idx[i] + MTKCAM_SUBDEV_CAMSV_START);
		unsigned int sv_cammux_id =
			mtk_cam_get_sv_cammux_id(sv_dev, tag_idx);

		mtk_cam_seninf_set_camtg_camsv(ctx->seninf,
			job->tag_info[tag_idx].seninf_padidx,
			sv_cammux_id, tag_idx);
	}

	for (i = 0; i < ctx->num_mraw_subdevs; i++) {
		mraw_idx = ctx->mraw_subdev_idx[i];
		if (cam->engines.mraw_devs[mraw_idx]) {
			mraw_dev =
				dev_get_drvdata(cam->engines.mraw_devs[mraw_idx]);
			mraw_pipe =
				&ctx->cam->pipelines.mraw[ctx->mraw_subdev_idx[i]];

			mtk_cam_seninf_set_camtg(ctx->seninf,
				mraw_pipe->seninf_padidx, mraw_dev->cammux_id);
		}
	}

	return 0;
}

