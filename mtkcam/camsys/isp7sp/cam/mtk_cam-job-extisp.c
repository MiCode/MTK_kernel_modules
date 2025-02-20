#include "mtk_cam.h"
#include "mtk_cam-job-extisp.h"

int handle_sv_tag_extisp(struct mtk_cam_job *job)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct mtk_raw_sink_data *raw_sink;
	struct mtk_camsv_pipeline *sv_pipe;
	struct mtk_camsv_sink_data *sv_sink;
	struct mtk_camsv_tag_param img_tag_param[SVTAG_IMG_END];
	struct mtk_camsv_tag_param meta_tag_param;
	struct v4l2_format *img_fmt;
	struct mtk_raw_sink_data preisp_sink_data;
	u32 vdev_id;
	unsigned int tag_idx, sv_pipe_idx, hw_scen;
	unsigned int exp_no, req_amount;
	int ret = 0, i;

	/* reset tag info */
	mtk_cam_sv_reset_tag_info(job);

	/* img tag(s) */
	exp_no = req_amount = 1;
	hw_scen = ((1 << MTKCAM_SV_SPECIAL_SCENARIO_EXT_ISP));
	pr_info("[%s] hw_scen:%d exp_no:%d req_amount:%d",
			__func__, hw_scen, exp_no, req_amount);
	if (mtk_cam_sv_get_tag_param(img_tag_param, hw_scen, exp_no, req_amount))
		return 1;

	// raw_sink = get_raw_sink_data(job);
	vdev_id = MTK_RAW_META_SV_OUT_0 - MTK_RAW_SINK_NUM;
	img_fmt = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx]
				.vdev_nodes[vdev_id].active_fmt;
	raw_sink = &preisp_sink_data;
	raw_sink->width = img_fmt->fmt.pix_mp.width;
	raw_sink->height = img_fmt->fmt.pix_mp.height;
	raw_sink->mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8;

	for (i = 0; i < req_amount; i++) {
		/* pd's sof comes earlier */
		if (ctx->num_sv_subdevs)
			img_tag_param[i].tag_order = MTKCAM_IPI_ORDER_LAST_TAG;
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&img_tag_param[i], hw_scen, 3,
			job->sub_ratio,
			raw_sink->width, raw_sink->height,
			raw_sink->mbus_code, NULL);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << img_tag_param[i].tag_idx);

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			img_tag_param[i].tag_idx,
			img_tag_param[i].seninf_padidx,
			img_tag_param[i].tag_order,
			raw_sink->width,
			raw_sink->height,
			raw_sink->mbus_code);
	}

	/* meta tag(s) */
	tag_idx = SVTAG_META_START;
	for (i = 0; i < ctx->num_sv_subdevs; i++) {
		if (tag_idx >= SVTAG_END)
			return 1;
		sv_pipe_idx = ctx->sv_subdev_idx[i];
		if (sv_pipe_idx >= ctx->cam->pipelines.num_camsv)
			return 1;

		sv_pipe = &ctx->cam->pipelines.camsv[sv_pipe_idx];
		sv_sink = &job->req->sv_data[sv_pipe_idx].sink;
		meta_tag_param.tag_idx = tag_idx;
		meta_tag_param.seninf_padidx = sv_pipe->seninf_padidx;
		meta_tag_param.tag_order = mtk_cam_seninf_get_tag_order(
			job->seninf, sv_sink->mbus_code, sv_pipe->seninf_padidx);
		mtk_cam_sv_fill_tag_info(job->tag_info,
			&job->ipi_config,
			&meta_tag_param, 1, 3, job->sub_ratio,
			sv_sink->width, sv_sink->height,
			sv_sink->mbus_code, sv_pipe);

		job->used_tag_cnt++;
		job->enabled_tags |= (1 << tag_idx);
		tag_idx++;

		pr_info("[%s] tag_idx:%d seninf_padidx:%d tag_order:%d width/height/mbus_code:0x%x_0x%x_0x%x\n",
			__func__,
			meta_tag_param.tag_idx,
			meta_tag_param.seninf_padidx,
			meta_tag_param.tag_order,
			sv_sink->width,
			sv_sink->height,
			sv_sink->mbus_code);
	}

	ctx->is_sensor_meta_dump = job->is_sensor_meta_dump = false;
	ctx->used_tag_cnt = job->used_tag_cnt;
	ctx->enabled_tags = job->enabled_tags;
	memcpy(ctx->tag_info, job->tag_info,
		sizeof(struct mtk_camsv_tag_info) * CAMSV_MAX_TAGS);

	return ret;
}

int fill_sv_ext_img_buffer_to_ipi_frame_extisp(
	struct req_buffer_helper *helper, struct mtk_cam_buffer *buf,
	struct mtk_cam_video_device *node)
{
	struct mtk_cam_ctx *ctx = helper->job->src_ctx;
	struct mtkcam_ipi_frame_param *fp = helper->fp;
	struct mtkcam_ipi_img_output *out;
	struct mtk_camsv_device *sv_dev;
	unsigned int tag_idx;
	int ret = -1;

	if (ctx->hw_sv == NULL)
		return ret;

	sv_dev = dev_get_drvdata(ctx->hw_sv);
	tag_idx = SVTAG_3;

	out = &fp->camsv_param[0][tag_idx].camsv_img_outputs[0];
	// fake param for ipi frame
	buf->image_info.width = node->active_fmt.fmt.pix_mp.width;
	buf->image_info.height = node->active_fmt.fmt.pix_mp.height;
	buf->image_info.bytesperline[0] = node->active_fmt.fmt.pix_mp.width;
	buf->image_info.v4l2_pixelformat = node->active_fmt.fmt.pix_mp.pixelformat;
	buf->image_info.size[0] = node->active_fmt.fmt.pix_mp.width *
		node->active_fmt.fmt.pix_mp.height;
	buf->image_info.crop.left = 0;
	buf->image_info.crop.top = 0;
	buf->image_info.crop.width = node->active_fmt.fmt.pix_mp.width;
	buf->image_info.crop.height = node->active_fmt.fmt.pix_mp.height;
	ret = fill_img_out(helper, out, buf, node);

	fp->camsv_param[0][tag_idx].pipe_id =
		sv_dev->id + MTKCAM_SUBDEV_CAMSV_START;
	fp->camsv_param[0][tag_idx].tag_id = tag_idx;
	fp->camsv_param[0][tag_idx].hardware_scenario = 0;
	out->uid.id = MTKCAM_IPI_CAMSV_MAIN_OUT;
	out->uid.pipe_id =
		sv_dev->id + MTKCAM_SUBDEV_CAMSV_START;
	out->buf[0][0].iova = buf->daddr;

	return ret;
}

int get_extisp_meta_info(struct mtk_cam_job *job, int pad_src)
{
	struct mtk_cam_ctx *ctx = job->src_ctx;
	struct v4l2_format *img_fmt;
	struct mtk_seninf_pad_data_info result;
	u32 vdev_id;

	if (ctx->seninf) {
		mtk_cam_seninf_get_pad_data_info(ctx->seninf,
			pad_src, &result);
		dev_info(ctx->cam->dev, "[%s] pad_src:%d, hsize/vsize:%d/%d\n",
			__func__, pad_src, result.exp_hsize, result.exp_vsize);
		if (pad_src == PAD_SRC_GENERAL0) {
			vdev_id = MTK_RAW_META_SV_OUT_0 - MTK_RAW_SINK_NUM;
			img_fmt = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx]
				.vdev_nodes[vdev_id].active_fmt;
			if (result.exp_hsize != 0 && result.exp_vsize != 0) {
				//img_fmt->fmt.meta.dataformat = V4L2_META_FMT_MTISP_PARAMS;
				//img_fmt->fmt.meta.buffersize = result.exp_hsize * result.exp_vsize;
				img_fmt->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR8;
				img_fmt->fmt.pix_mp.width = result.exp_hsize;
				img_fmt->fmt.pix_mp.height = result.exp_vsize;
				img_fmt->fmt.pix_mp.num_planes = 1;
				img_fmt->fmt.pix_mp.plane_fmt[0].sizeimage = result.exp_hsize * result.exp_vsize * 1;
				img_fmt->fmt.pix_mp.plane_fmt[0].bytesperline = result.exp_hsize * 1;
				job->extisp_data |= BIT(EXTISP_DATA_META);
			}
			dev_info(ctx->cam->dev, "[%s:PAD_SRC_GENERAL0] vdev_nodes:%d, w/h/size:%d/%d/%d\n",
				__func__, vdev_id,
				result.exp_hsize, result.exp_vsize,
				img_fmt->fmt.meta.buffersize);
		}
		if (pad_src == PAD_SRC_RAW0) {
			vdev_id = MTK_RAW_MAIN_STREAM_SV_1_OUT - MTK_RAW_SINK_NUM;
			img_fmt = &ctx->cam->pipelines.raw[ctx->raw_subdev_idx]
				.vdev_nodes[vdev_id].active_fmt;
			if (img_fmt->fmt.pix_mp.width != result.exp_hsize ||
				img_fmt->fmt.pix_mp.height != result.exp_vsize) {
				result.exp_hsize = 0;
				result.exp_vsize = 0;
			}
			dev_info(ctx->cam->dev, "[%s:PAD_SRC_RAW0] vdev_nodes:%d, w/h/size:%d/%d/%d\n",
				__func__, vdev_id,
				img_fmt->fmt.pix_mp.width, img_fmt->fmt.pix_mp.height,
				img_fmt->fmt.pix_mp.width * img_fmt->fmt.pix_mp.height);
		}
		if (pad_src == PAD_SRC_RAW_EXT0) {
			vdev_id = MTK_RAW_MAIN_STREAM_SV_1_OUT - MTK_RAW_SINK_NUM;
			dev_info(ctx->cam->dev, "[%s:PAD_SRC_RAW_EXT0] vdev_nodes:%d, w/h/size:%d/%d/%d\n",
				__func__, vdev_id,
				result.exp_hsize, result.exp_vsize,
				result.exp_hsize * result.exp_vsize);
			job->extisp_data |= BIT(EXTISP_DATA_PROCRAW);
		}
		return result.exp_hsize * result.exp_vsize;
	}
	return CAMSV_EXT_META_0_WIDTH * CAMSV_EXT_META_0_HEIGHT;
}

