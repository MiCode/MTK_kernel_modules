// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2024 MediaTek Inc.


#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>

#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>

#include "mtk_cam-seninf-event-handle.h"
#include "mtk_cam-seninf-hw.h"
#include "mtk_cam-seninf-if.h"
#include "mtk_cam-seninf-utils.h"
#include "mtk_cam-seninf-sentest-ctrl.h"
#include "imgsensor-user.h"


#define PORTING_FIXME 0


/*----------------------------------------------------------------------------*/
// => sensor event/handle
//    some APIs are declared in mtk_cam-seninf-if.h
/*----------------------------------------------------------------------------*/
u8 is_reset_by_user(struct seninf_ctx *ctx)
{
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
			V4L2_CID_MTK_SENSOR_RESET_BY_USER);

	return (ctrl) ? v4l2_ctrl_g_ctrl(ctrl) : 0;
}

int reset_sensor(struct seninf_ctx *ctx)
{
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
			V4L2_CID_MTK_SENSOR_RESET);
	if (!ctrl) {
		seninf_logi(ctx, "V4L2_CID_MTK_SENSOR_RESET %s\n",
			sensor_sd->name);
		return -EINVAL;
	}

	v4l2_ctrl_s_ctrl(ctrl, 1);

	return 0;
}

bool has_multiple_expo_mode(struct seninf_ctx *ctx)
{
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct mtk_sensor_mode_config_info info;
	bool ret = false;
	int i;

	if (likely(chk_subdev_ops_command_exist(sensor_sd))) {
		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_GET_SENSOR_MODE_CONFIG_INFO, &info);

		seninf_logd(ctx, "info.cur_mode = %u, info.count = %u\n",
			 info.current_scenario_id, info.count);

		for (i = 0; i < info.count; i++) {
			seninf_logd(ctx, "mode[%d] mode id = %u, exp_num = %u\n",
				 i,
				 info.seamless_scenario_infos[i].scenario_id,
				 info.seamless_scenario_infos[i].mode_exposure_num);

			if (info.seamless_scenario_infos[i].mode_exposure_num > 1) {
				ret = true;
				break;
			}
		}
	}

	seninf_logd(ctx, "%s , ret = %d\n", __func__, ret);

	return ret;
}

void mtk_cam_sensor_get_frame_cnt(struct seninf_ctx *ctx, u32 *frame_cnt)
{
	if (!ctx)
		return;

	if (likely(chk_subdev_ops_command_exist(ctx->sensor_sd))) {
		ctx->sensor_sd->ops->core->command(ctx->sensor_sd,
						V4L2_CMD_G_SENSOR_FRAME_CNT,
						frame_cnt);
	} else {
		seninf_logi(ctx,
			"ERROR: v4l2 subdev ops core command not exist\n");
	}
}

void mtk_cam_sensor_get_glp_dt(struct seninf_ctx *ctx,
	struct seninf_glp_dt *info)
{
	u32 *glp = NULL;
	u32 cnt = 0;
	int i;

	if (!ctx || !info)
		return;
	glp = info->dt;

	if (likely(chk_subdev_ops_command_exist(ctx->sensor_sd))) {
		ctx->sensor_sd->ops->core->command(ctx->sensor_sd,
						V4L2_CMD_G_SENSOR_GLP_DT,
						glp);
	} else {
		seninf_logi(ctx,
			"ERROR: v4l2 subdev ops core command not exist\n");
	}

	for (i=0; i<SEQ_DT_MAX_CNT; i++ ){
		if(glp[i])
			cnt++;
	}

	seninf_logd(ctx,
		"glp[0/1/2/3]:0x%x/0x%x/0x%x/0x%x,cnt:%d\n",
		glp[0], glp[1], glp[2], glp[3], cnt);

	info->cnt = cnt;
}

int mtk_cam_seninf_get_active_line_info(struct v4l2_subdev *sd,
	unsigned int mbus_code, struct mtk_seninf_active_line_info *result)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	struct v4l2_subdev *sensor_sd = NULL;
	struct mtk_sensor_mode_info mode_info;
	struct mtk_seninf_pad_data_info pad_info;
	unsigned int scenario_id;

	if (!result)
		return -1;

	scenario_id = (mbus_code >> 16) & 0xff;
	memset(result, 0, sizeof(*result));

	if (ctx)
		sensor_sd = ctx->sensor_sd;

	mode_info.scenario_id = scenario_id;

	if (likely(chk_subdev_ops_command_exist(sensor_sd))) {
		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_GET_SEND_SENSOR_MODE_CONFIG_INFO, &mode_info);

		result->active_line_num = mode_info.active_line_num;
		result->avg_linetime_in_ns = mode_info.avg_linetime_in_ns;

		if (!result->active_line_num) {
			if (mtk_cam_seninf_get_pad_data_info(sd,
				PAD_SRC_RAW0, &pad_info) != -1) {
				result->active_line_num = pad_info.exp_vsize;
			} else if (mtk_cam_seninf_get_pad_data_info(sd,
				PAD_SRC_RAW_EXT0, &pad_info) != -1) {
				result->active_line_num = pad_info.exp_vsize;
			}
		}
		seninf_logi(ctx, "modeid=%u, active_line=%u/%u, avg_tline_ns=%llu\n",
					mode_info.scenario_id,
					mode_info.active_line_num,
					result->active_line_num,
					mode_info.avg_linetime_in_ns);
	}

	return -1;
}

bool has_embedded_parser(struct v4l2_subdev *sd)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	bool ret = false;

	if (likely(chk_subdev_ops_command_exist(sensor_sd))) {
		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_SENSOR_HAS_EBD_PARSER, &ret);
	}

	seninf_logd(ctx, "ret = %d\n", ret);

	return ret;
}

int mtk_cam_seninf_get_ebd_info_by_scenario(struct v4l2_subdev *sd,
	u32 scenario_mbus_code, struct mtk_seninf_pad_data_info *result)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct mtk_sensor_ebd_info_by_scenario ebd_info;
	int ret = -1;

	memset(result, 0, sizeof(*result));

	if (likely(chk_subdev_ops_command_exist(sensor_sd))) {
		ebd_info.input_scenario_id = get_scenario_from_fmt_code(scenario_mbus_code);

		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_GET_SENSOR_EBD_INFO_BY_SCENARIO, &ebd_info);

		result->feature = VC_GENERAL_EMBEDDED;
		result->exp_hsize = ebd_info.exp_hsize;
		result->exp_vsize = ebd_info.exp_vsize;
		result->mbus_code = get_mbus_format_by_dt(ebd_info.data_type,
						ebd_info.dt_remap_to_type);

		if (ebd_info.data_type >= 0x10 && ebd_info.data_type <= 0x17) {
			result->mbus_code = MEDIA_BUS_FMT_SBGGR14_1X14;
			result->exp_hsize = conv_ebd_hsize_raw14(result->exp_hsize,
					ebd_info.ebd_parsing_type);
		}

		seninf_logd(ctx, "mode = %u, result(%u,%u,%u,0x%x)\n",
			    ebd_info.input_scenario_id,
			    result->feature,
			    result->exp_hsize,
			    result->exp_vsize,
			    result->mbus_code);

		if (result->exp_hsize && result->exp_vsize) // has ebd info
			ret = 0;
	}

	seninf_logd(ctx, "ret = %d\n", ret);

	return ret;
}

void mtk_cam_seninf_parse_ebd_line(struct v4l2_subdev *sd,
	unsigned int req_id, char *req_fd_desc,
	char *buf, u32 buf_sz, u32 stride, u32 scenario_mbus_code)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct mtk_recv_sensor_ebd_line ebd_line;
	struct mtk_sensor_ebd_info_by_scenario ebd_info;

	seninf_logd(ctx, "req_id = %u, req_fd_desc = %s, mbus = 0x%x\n",
		    req_id, req_fd_desc, scenario_mbus_code);

	if (likely(chk_subdev_ops_command_exist(sensor_sd))) {
		ebd_info.input_scenario_id = get_scenario_from_fmt_code(scenario_mbus_code);

		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_GET_SENSOR_EBD_INFO_BY_SCENARIO, &ebd_info);

		ebd_line.req_id = req_id;
		ebd_line.req_fd_desc = req_fd_desc;
		ebd_line.stride = stride;
		ebd_line.buf_sz = buf_sz;
		ebd_line.buf = buf;
		ebd_line.ebd_parsing_type = ebd_info.ebd_parsing_type;
		ebd_line.mbus_code = get_mbus_format_by_dt(ebd_info.data_type,
						ebd_info.dt_remap_to_type);

		sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_SENSOR_PARSE_EBD, &ebd_line);
	}
}


/*----------------------------------------------------------------------------*/
// => seninf event/handle
/*----------------------------------------------------------------------------*/
int seninf_get_fmeter_clk(struct seninf_core *core,
	int clk_fmeter_idx, unsigned int *out_clk)
{
	struct clk_fmeter_info *fmeter;

	if (!core || !out_clk || clk_fmeter_idx < 0 || clk_fmeter_idx >= CLK_FMETER_MAX)
		return -EINVAL;

	fmeter = &core->fmeter[clk_fmeter_idx];

#ifndef REDUCE_KO_DEPENDANCY_FOR_SMT
	if (fmeter->fmeter_no) {
		*out_clk = mt_get_fmeter_freq(fmeter->fmeter_no, fmeter->fmeter_type);
		return 0;
	}
#endif

	*out_clk = 0;

	return -EPERM;
}



/*----------------------------------------------------------------------------*/
// => fsync event/handle
/*----------------------------------------------------------------------------*/
static int cammux_tag_2_fsync_target_id(struct seninf_ctx *ctx,
	const int cammux, const int tag)
{
#if PORTING_FIXME
	unsigned int const raw_cammux_factor = 2;
	int cammux_factor = 8;
	int fsync_camsv_start_id = 5;
	int fsync_pdp_start_id = 56;
	struct seninf_core *core = ctx->core;
	enum CAM_TYPE_ENUM type = outmux2camtype(ctx, cammux);
	int ret = 0xff;

	if (cammux < 0 || cammux >= 0xff) {
		ret = 0xff;
	} else if (type == TYPE_CAMSV_SAT) {
		ret = fsync_camsv_start_id
			+ (cammux - core->cammux_range[TYPE_CAMSV_SAT].first);
	} else if (type == TYPE_CAMSV_NORMAL) {
		ret = ((cammux - core->cammux_range[TYPE_CAMSV_NORMAL].first) * cammux_factor)
			+ core->cammux_range[TYPE_CAMSV_NORMAL].first
			+ fsync_camsv_start_id + tag;
	} else if (type == TYPE_RAW) {
		ret = 1 +
			((cammux - core->cammux_range[TYPE_RAW].first) / raw_cammux_factor);
	} else if (type == TYPE_PDP) {
		ret = fsync_pdp_start_id + (cammux - core->cammux_range[TYPE_PDP].first);
	}

	seninf_logd(ctx, "cammux = %d, tag = %d, target_id = %d\n",
		 cammux, tag, ret);

	return ret;
#else
	return 0;
#endif
}

void setup_fsync_vsync_src_pad(struct seninf_ctx *ctx,
	const u64 fsync_ext_vsync_pad_code)
{
	const unsigned int has_processed_data = (unsigned int)
		((fsync_ext_vsync_pad_code >> PAD_SRC_RAW_EXT0) & (u64)1);
	const unsigned int has_general_embedded = (unsigned int)
		((fsync_ext_vsync_pad_code >> PAD_SRC_GENERAL0) & (u64)1);
	const unsigned int has_pdaf_0 = (unsigned int)
		((fsync_ext_vsync_pad_code >> PAD_SRC_PDAF0) & (u64)1);
	const unsigned int has_pdaf_1 = (unsigned int)
		((fsync_ext_vsync_pad_code >> PAD_SRC_PDAF1) & (u64)1);
	const unsigned int has_pdaf_2 = (unsigned int)
		((fsync_ext_vsync_pad_code >> PAD_SRC_PDAF2) & (u64)1);
	const bool has_multi_expo = has_multiple_expo_mode(ctx);

	/* default using raw0 vsync signal */
	ctx->fsync_vsync_src_pad = PAD_SRC_RAW0;

	/* check case to overwrite */
	/* --- if pre-isp case */
	if (has_processed_data) {
		if (has_multi_expo && has_pdaf_0)
			ctx->fsync_vsync_src_pad = PAD_SRC_PDAF0;
		else if (has_multi_expo && has_pdaf_1)
			ctx->fsync_vsync_src_pad = PAD_SRC_PDAF1;
		else if (has_multi_expo && has_pdaf_2)
			ctx->fsync_vsync_src_pad = PAD_SRC_PDAF2;
		else if (has_general_embedded)
			ctx->fsync_vsync_src_pad = PAD_SRC_GENERAL0;
		else {
			ctx->fsync_vsync_src_pad = PAD_SRC_RAW0;

			seninf_logi(ctx,
				"WARNING: fsync_ext_vsync_pad_code:%#llx, has processed_data:%u, but pdaf(0:%u/1:%u/2:%u), general_embedded:%u, force set fsync_vsync_src_pad:%d(RAW0:%d/pdaf(0:%d/1:%d/2:%d)/GENERAL0:%d)\n",
				fsync_ext_vsync_pad_code,
				has_processed_data,
				has_pdaf_0, has_pdaf_1, has_pdaf_2,
				has_general_embedded,
				ctx->fsync_vsync_src_pad,
				PAD_SRC_RAW0,
				PAD_SRC_PDAF0, PAD_SRC_PDAF1, PAD_SRC_PDAF2,
				PAD_SRC_GENERAL0);

			return;
		}

		seninf_logi(ctx,
			"NOTICE: set fsync_vsync_src_pad:%d(RAW0:%d/pdaf(0:%d/1:%d/2:%d)/GENERAL0:%d), fsync_ext_vsync_pad_code:%#llx(processed_data:%u/pdaf(0:%u/1:%u/2:%u)/general_embedded:%u)\n",
			ctx->fsync_vsync_src_pad,
			PAD_SRC_RAW0,
			PAD_SRC_PDAF0, PAD_SRC_PDAF1, PAD_SRC_PDAF2,
			PAD_SRC_GENERAL0,
			fsync_ext_vsync_pad_code,
			has_processed_data,
			has_pdaf_0, has_pdaf_1, has_pdaf_2,
			has_general_embedded);
	}
}

void chk_is_fsync_vsync_src(struct seninf_ctx *ctx, const int pad_id)
{
	const int vsync_src_pad = ctx->fsync_vsync_src_pad;

	if (vsync_src_pad != pad_id)
		return;

	if (vsync_src_pad == PAD_SRC_RAW0) {
		// notify vc->cam
		notify_fsync_listen_target_with_kthread(ctx, 0);
	} else if (vsync_src_pad == PAD_SRC_PDAF0
		|| vsync_src_pad == PAD_SRC_PDAF1
		|| vsync_src_pad == PAD_SRC_PDAF2
		|| vsync_src_pad == PAD_SRC_GENERAL0) {

		seninf_logi(ctx,
			"NOTICE: pad_id:%d, fsync_vsync_src_pad:%d(RAW0:%d/pdaf(0:%d/1:%d/2:%d)/GENERAL0:%d), fsync listen extra vsync signal\n",
			pad_id,
			vsync_src_pad,
			PAD_SRC_RAW0,
			PAD_SRC_PDAF0, PAD_SRC_PDAF1, PAD_SRC_PDAF2,
			PAD_SRC_GENERAL0);

		notify_fsync_listen_target_with_kthread(ctx, 0);
	} else {
		/* unexpected case */
		seninf_logi(ctx,
			"ERROR: unknown fsync_vsync_src_pad:%d(RAW0:%d/pdaf(0:%d/1:%d/2:%d)/GENERAL0:%d) type, pad_id:%d\n",
			vsync_src_pad,
			PAD_SRC_RAW0,
			PAD_SRC_PDAF0, PAD_SRC_PDAF1, PAD_SRC_PDAF2,
			PAD_SRC_GENERAL0,
			pad_id);
	}
}

bool is_fsync_listening_on_pd(struct v4l2_subdev *sd)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	bool ret = false;

	if (ctx->fsync_vsync_src_pad >= PAD_SRC_PDAF0 &&
	    ctx->fsync_vsync_src_pad <= PAD_SRC_PDAF6)
		ret = true;

	seninf_logi(ctx, "%s , ret = %d\n", __func__, ret);

	return ret;
}

static int mtk_cam_seninf_get_fsync_vsync_src_cam_info(struct seninf_ctx *ctx)
{
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	struct seninf_vc *vc;
	int i;
	int target_id = -1;

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];

		if (vc->out_pad == ctx->fsync_vsync_src_pad) {
			/* vsync_src_pad must be first-raw or NE PDAF type or general-embedded */
			target_id = cammux_tag_2_fsync_target_id(ctx,
					vc->dest[0].outmux, vc->dest[0].tag);

			seninf_logi(ctx,
				"fsync_vsync_src_pad:%d(RAW0:%d/pdaf(0:%d/1:%d/2:%d)/GENERAL0:%d) => vc->outmux:%d, vc->tag:%d => target_id:%d\n",
				ctx->fsync_vsync_src_pad,
				PAD_SRC_RAW0,
				PAD_SRC_PDAF0, PAD_SRC_PDAF1, PAD_SRC_PDAF2,
				PAD_SRC_GENERAL0,
				vc->dest[0].outmux,
				vc->dest[0].tag,
				target_id);

			return target_id;
		}
	}

	seninf_logi(ctx, "%s: no raw data in vc channel\n", __func__);
	return -1;
}

static void mtk_notify_listen_target_fn(struct kthread_work *work)
{
	struct mtk_seninf_work *seninf_work = NULL;
	struct seninf_ctx *ctx = NULL;

	// --- change to use kthread_delayed_work.
	// seninf_work = container_of(work, struct mtk_seninf_work, work);
	seninf_work = container_of(work, struct mtk_seninf_work, dwork.work);

	if (seninf_work) {
		ctx = seninf_work->ctx;
		if (ctx)
			notify_fsync_listen_target(ctx);

		kfree(seninf_work);
	}
}

int notify_fsync_listen_target(struct seninf_ctx *ctx)
{
	int cam_idx = mtk_cam_seninf_get_fsync_vsync_src_cam_info(ctx);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;

	if (cam_idx < 0 || cam_idx >= 0xff)
		return -EINVAL;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
			V4L2_CID_FSYNC_LISTEN_TARGET);
	if (!ctrl) {
		seninf_logi(ctx, "no fsync listen target in %s\n",
			sensor_sd->name);
		return -EINVAL;
	}

	seninf_logd(ctx, "raw cammux usage = %d\n", cam_idx);

	v4l2_ctrl_s_ctrl(ctrl, cam_idx);

	return 0;
}

void notify_fsync_listen_target_with_kthread(struct seninf_ctx *ctx,
	const unsigned int mdelay)
{
	struct mtk_seninf_work *seninf_work = NULL;

	if (ctx->streaming) {
		seninf_work = kmalloc(sizeof(struct mtk_seninf_work),
					GFP_ATOMIC);
		if (seninf_work) {
			// --- change to use kthread_delayed_work.
			// kthread_init_work(&seninf_work->work,
			//		mtk_notify_listen_target_fn);
			kthread_init_delayed_work(&seninf_work->dwork,
					mtk_notify_listen_target_fn);

			seninf_work->ctx = ctx;

			// --- change to use kthread_delayed_work.
			// kthread_queue_work(&ctx->core->seninf_worker,
			//		&seninf_work->work);
			kthread_queue_delayed_work(&ctx->core->seninf_worker,
					&seninf_work->dwork,
					msecs_to_jiffies(mdelay));
		}
	}
}

int notify_mipi_err_detect_handler(struct seninf_ctx *ctx,
		const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info)
{
	struct mtk_cam_seninf_vsync_info vsync_info = {0};

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	if (ctx->core->vsync_irq_en_flag || ctx->core->csi_irq_en_flag)
		g_seninf_ops->_seninf_dump_mipi_err(ctx->core, &vsync_info);

	return 0;
}


/*----------------------------------------------------------------------------*/
// => tsrec event/handle
/*----------------------------------------------------------------------------*/
void mtk_cam_seninf_tsrec_irq_notify(
	const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info)
{
	/* Please add your handler function here carefully */
	notify_sentest_irq(p_info->inf_ctx, p_info);
	notify_mipi_err_detect_handler(p_info->inf_ctx, p_info);
}


/*----------------------------------------------------------------------------*/
// => camsys event/handle
//    some APIs are declared in mtk_cam-seninf-if.h
/*----------------------------------------------------------------------------*/
static void mtk_notify_vsync_fn(struct kthread_work *work)
{
	struct mtk_seninf_work *seninf_work =
		container_of(work, struct mtk_seninf_work, work);
	struct seninf_ctx *ctx = seninf_work->ctx;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	unsigned int sof_cnt = seninf_work->data.sof;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
				V4L2_CID_VSYNC_NOTIFY);
	if (!ctrl) {
		seninf_logi(ctx, "no V4L2_CID_VSYNC_NOTIFY %s\n",
			sensor_sd->name);
		return;
	}

//	seninf_logi(ctx, "sof %s cnt %d\n",
//		sensor_sd->name,
//		sof_cnt);
	v4l2_ctrl_s_ctrl(ctrl, sof_cnt);

	kfree(seninf_work);
}


void mtk_cam_seninf_sof_notify(struct mtk_seninf_sof_notify_param *param)
{
	struct v4l2_subdev *sd = param->sd;
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);
	struct mtk_seninf_work *seninf_work = NULL;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;

	if (ctx->is_test_model) {
		seninf_logi(ctx, "test model mode, skip sof notify\n");
		return;
	}

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
				V4L2_CID_UPDATE_SOF_CNT);
	if (!ctrl) {
		seninf_logi(ctx, "no V4L2_CID_UPDATE_SOF_CNT %s\n",
			sensor_sd->name);
		return;
	}

//	seninf_logi(ctx, "sof %s cnt %d\n",
//		sensor_sd->name,
//		param->sof_cnt);
	v4l2_ctrl_s_ctrl(ctrl, param->sof_cnt);

	if (ctx->streaming) {
		seninf_work = kmalloc(sizeof(struct mtk_seninf_work),
				GFP_ATOMIC);
		if (seninf_work) {
			kthread_init_work(&seninf_work->work,
					mtk_notify_vsync_fn);
			seninf_work->ctx = ctx;
			seninf_work->data.sof = param->sof_cnt;
			kthread_queue_work(&ctx->core->seninf_worker,
					&seninf_work->work);
		}
	}
}

void notify_sensor_set_fl_prolong(struct v4l2_subdev *sd,
	unsigned int action)
{
	struct seninf_ctx *ctx = container_of(sd, struct seninf_ctx, subdev);

	if (!ctx)
		return;

	if (likely(chk_subdev_ops_command_exist(ctx->sensor_sd))) {
		ctx->sensor_sd->ops->core->command(ctx->sensor_sd,
						V4L2_CMD_SET_SENSOR_FL_PROLONG,
						&action);
	} else {
		seninf_logi(ctx,
			"ERROR: v4l2 subdev ops core command not exist\n");
	}
}
