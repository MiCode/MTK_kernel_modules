// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include "mtk_cam-seninf-sentest-ctrl.h"
#include "mtk_cam-seninf_control-8.h"
#include "mtk_cam-seninf-hw.h"
#include "mtk_cam-seninf-route.h"
#include "mtk_cam-seninf-if.h"
#include "imgsensor-user.h"

/******************************************************************************/
// seninf sentest call back ctrl --- function
/******************************************************************************/

#define WATCHDOG_INTERVAL_MS 50

struct seninf_sentest_work {
	struct kthread_work work;
	struct seninf_ctx *ctx;
	union sentest_work_data {
		unsigned int seamless_scenario;
		void *data_ptr;
	} data;
};

enum SENTEST_TARGET_VSYNC {
	SENTEST_FIRST_VSYNC,
	SENTEST_LAST_VSYNC,
};

int seninf_sentest_probe_init(struct seninf_ctx *ctx)
{
	int ret = 0;

	kthread_init_worker(&ctx->sentest_worker);
	ctx->sentest_kworker_task = kthread_run(kthread_worker_fn,
			&ctx->sentest_worker, "sentest_worker");

	if (IS_ERR(ctx->sentest_kworker_task)) {
		pr_info("[%s][ERROR]: failed to start sentest kthread worker\n", __func__);
		ctx->sentest_kworker_task = NULL;
		return -EFAULT;
	}

	sched_set_fifo(ctx->sentest_kworker_task);

	ret |= seninf_sentest_flag_init(ctx);
	return ret;
}

int seninf_sentest_uninit(struct seninf_ctx *ctx)
{
	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (ctx->sentest_kworker_task)
		kthread_stop(ctx->sentest_kworker_task);

	return 0;
}

static void seninf_sentest_seamless_switch_error_handler(struct seninf_ctx *ctx)
{
	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return;
	}

	seninf_sentest_watchingdog_en(&ctx->sentest_watchdog, false);
	ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_ERR;
	ctx->sentest_seamless_ut_en = false;
}

static void seninf_sentest_reset_seamless_flag(struct seninf_ctx *ctx)
{
	ctx->sentest_seamless_ut_en = false;
	ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_IDLE;
	ctx->sentest_seamless_irq_ref = 0;
	ctx->sentest_seamless_is_set_camtg_done = 0;
	ctx->sentest_irq_counter = 0;

	memset(&ctx->sentest_seamless_cfg, 0, sizeof(struct mtk_seamless_switch_param));
}

int seninf_sentest_flag_init(struct seninf_ctx *ctx)
{
	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	seninf_sentest_reset_seamless_flag(ctx);
	ctx->sentest_adjust_isp_en = false;
	ctx->sentest_mipi_measure_en = false;

	return 0;
}


int seninf_sentest_get_debug_reg_result(struct seninf_ctx *ctx, void *arg)
{
	int i;
	struct seninf_core *core;
	struct seninf_ctx *ctx_;
	struct mtk_cam_seninf_vcinfo_debug *vcinfo_debug;
	struct outmux_debug_result *outmux_result;
	struct mtk_cam_seninf_debug debug_result;
	static __u16 last_pkCnt;
	struct mtk_seninf_debug_result *result =
			kmalloc(sizeof(struct mtk_seninf_debug_result), GFP_KERNEL);

	if (unlikely(result == NULL)) {
		pr_info("[%s][ERROR] result is NULL\n", __func__);
		return -EFAULT;
	}

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		goto SENTEST_GET_DBG_ERR_EXIT;
	}

	core = ctx->core;
	if (unlikely(core == NULL)) {
		pr_info("[%s][ERROR] core is NULL\n", __func__);
		goto SENTEST_GET_DBG_ERR_EXIT;
	}

	if (copy_from_user(result, arg, sizeof(struct mtk_seninf_debug_result))) {
		pr_info("[%s][ERROR] copy_from_user return failed\n", __func__);
		goto SENTEST_GET_DBG_ERR_EXIT;
	}

	memset(&debug_result, 0, sizeof(struct mtk_cam_seninf_debug));

	list_for_each_entry(ctx_, &core->list, list) {
		if (unlikely(ctx_ == NULL)) {
			pr_info("[%s][ERROR] ctx_ is NULL\n", __func__);
			goto SENTEST_GET_DBG_ERR_EXIT;
		}

		if (!ctx_->streaming)
			continue;

		g_seninf_ops->get_seninf_debug_core_dump(ctx_, &debug_result);

		result->is_cphy = ctx_->is_cphy;
		result->csi_port = ctx_->port;
		result->seninfAsyncIdx = ctx_->seninfAsyncIdx;
		result->data_lanes = ctx_->num_data_lanes;
		result->valid_result_cnt = debug_result.valid_result_cnt;
		result->seninf_async_irq = debug_result.seninf_async_irq;
		result->csi_mac_irq_status = debug_result.csi_mac_irq_status;

		if (last_pkCnt == debug_result.packet_cnt_status) {

			/* Need fix this logic */
			last_pkCnt = debug_result.packet_cnt_status;
			result->packet_status_err = 0;
		} else {
			result->packet_status_err = 1;
		}

		for (i = 0; i <= debug_result.valid_result_cnt; i++) {
			vcinfo_debug = &debug_result.vcinfo_debug[i];
			outmux_result = &result->outmux_result[i];

			if (unlikely(vcinfo_debug == NULL)) {
				pr_info("[%s][ERROR] vcinfo_debug is NULL\n", __func__);
				goto SENTEST_GET_DBG_ERR_EXIT;
			}

			if (unlikely(outmux_result == NULL)) {
				pr_info("[%s][ERROR] outmux_result is NULL\n", __func__);
				goto SENTEST_GET_DBG_ERR_EXIT;
			}

			outmux_result->vc_feature	= vcinfo_debug->vc_feature;
			outmux_result->tag_id	= vcinfo_debug->tag_id;
			outmux_result->vc		= vcinfo_debug->vc;
			outmux_result->dt		= vcinfo_debug->dt;
			outmux_result->exp_size_h	= vcinfo_debug->exp_size_h;
			outmux_result->exp_size_v	= vcinfo_debug->exp_size_v;
			outmux_result->outmux_id	= vcinfo_debug->outmux_id;

			outmux_result->done_irq_status		= vcinfo_debug->done_irq_status;
			outmux_result->oversize_irq_status	= vcinfo_debug->oversize_irq_status;
			outmux_result->incomplete_frame_status	= vcinfo_debug->incomplete_frame_status;
			outmux_result->ref_vsync_irq_status	= vcinfo_debug->ref_vsync_irq_status;
		}

		if (copy_to_user(arg, result, sizeof(struct mtk_seninf_debug_result))) {
			pr_info("[%s][ERROR] copy_to_user return failed\n", __func__);
			goto SENTEST_GET_DBG_ERR_EXIT;
		}
	}
	kfree(result);
	return 0;

SENTEST_GET_DBG_ERR_EXIT:
	kfree(result);
	return -EFAULT;
}

static void seninf_sentest_watchdog_timer_callback(struct timer_list *t)
{
	struct mtk_cam_sentest_watchdog *wd = from_timer(wd, t, timer);
	struct seninf_ctx *ctx =
		container_of(wd, struct seninf_ctx, sentest_watchdog);

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return;
	}

	pr_info("[%s] watchog timeout tirggered", __func__);

	ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_TIMEOUT;
	ctx->sentest_seamless_ut_en = false;
	del_timer_sync(&wd->timer);
}

int seninf_sentest_watchingdog_en(struct mtk_cam_sentest_watchdog *wd, bool en)
{
	struct seninf_ctx *ctx =
		container_of(wd, struct seninf_ctx, sentest_watchdog);
	u64 shutter_for_timeout = 0;

	if (unlikely(wd == NULL)) {
		pr_info("[Error][%s] wd is NULL", __func__);
		return -EFAULT;
	}

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	if (en) {

		if (ctx->sentest_seamless_cfg.ae_ctrl[0].exposure.arr[0]) {
			shutter_for_timeout =
				ctx->sentest_seamless_cfg.ae_ctrl[0].exposure.arr[0] > WATCHDOG_INTERVAL_MS ?
				ctx->sentest_seamless_cfg.ae_ctrl[0].exposure.arr[0] :
				WATCHDOG_INTERVAL_MS;
		}


		seninf_sentest_watchdog_init(wd);
		// setup timer
		wd->timer.expires = jiffies + msecs_to_jiffies(shutter_for_timeout);
		add_timer(&wd->timer);

	} else {
		// del timer
		del_timer_sync(&wd->timer);

		ctx->sentest_seamless_ut_status = SENTEST_SEAMLESS_IS_IDLE;
	}

	pr_info("[%s] setup sentest swatchdong timeout %llu en: %d done",
			__func__, shutter_for_timeout, en);

	return 0;
}

int seninf_sentest_watchdog_init(struct mtk_cam_sentest_watchdog *wd)
{
	if (unlikely(wd == NULL)) {
		pr_info("[Error][%s] wd is NULL", __func__);
		return -EFAULT;
	}

	timer_setup(&wd->timer, seninf_sentest_watchdog_timer_callback, 0);
	return 0;
}

void seninf_sentest_seamless_ut_disable_outmux(struct seninf_ctx *ctx)
{
	mtk_cam_seninf_release_outmux(ctx);

}



static u32 compose_format_code_by_scenario(u32 target_scenario)
{
	return (target_scenario << 16) & 0xFF0000;
}

static int get_lastest_outmux_id_by_vc_cnt(struct seninf_ctx *ctx, u32 target_count, u32 *outmux)
{
	struct seninf_core *core = ctx->core;
	struct seninf_outmux *ent;
	int count = 0;

	list_for_each_entry(ent, &core->list_outmux, list) {
		if (count == target_count) {
			*outmux = ent->idx;
			return 0;
		}
		count++;
	}

	return -EINVAL;
}

static int seninf_sentest_disable_old_camtg(struct seninf_ctx *ctx)
{
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	int i, out_pad;

	if (ctx == NULL) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	for (i = 0; i < vcinfo->cnt; i++) {
		out_pad = vcinfo->vc[i].out_pad;
		mtk_cam_seninf_set_camtg_camsv(
				&ctx->subdev,
				out_pad,
				0xff,
				ctx->pad_tag_id[out_pad][0]);
	}

	mtk_cam_seninf_apply_disable_mux(&ctx->subdev);
	return 0;
}

static int seninf_sentest_set_fmt(struct seninf_ctx *ctx)
{
	int i;
	u32 code;
	struct seninf_vcinfo *cur_vcinfo = &ctx->cur_vcinfo;

	if (ctx == NULL) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	mtk_cam_seninf_get_sensor_usage(&ctx->subdev);
	code = compose_format_code_by_scenario(ctx->sentest_seamless_cfg.target_scenario_id);
	ctx->fmt[PAD_SRC_RAW0].format.code = code;
	mtk_cam_sensor_get_vc_info_by_scenario(ctx, code);

	for (i = 0; i < cur_vcinfo->cnt; i++) {
		if (cur_vcinfo->vc[i].out_pad != PAD_SRC_RAW0)
			continue;

		switch (cur_vcinfo->vc[i].dt) {
		case 0x2c:
			ctx->fmt[PAD_SRC_RAW0].format.code |= MEDIA_BUS_FMT_SBGGR12_1X12;
			break;
		case 0x2d:
			ctx->fmt[PAD_SRC_RAW0].format.code |= MEDIA_BUS_FMT_SBGGR14_1X14;
			break;
		default:
			ctx->fmt[PAD_SRC_RAW0].format.code |= MEDIA_BUS_FMT_SBGGR10_1X10;
			break;
		}
	}
	return 0;
}

static int seninf_sentest_set_camtg_for_seamless(struct seninf_ctx *ctx)
{
	int i, ret = 0;
	int outmux_id = 0;
	struct seninf_vcinfo *cur_vcinfo = &ctx->cur_vcinfo;
	struct mtk_cam_seninf_mux_param param;
	struct mtk_cam_seninf_mux_setting settings[12];
	struct v4l2_ctrl *ctrl;

	memset(&param, 0, sizeof(struct mtk_cam_seninf_mux_param));

	memset(settings, 0, sizeof(struct mtk_cam_seninf_mux_setting) * ARRAY_SIZE(settings));

	if (seninf_sentest_disable_old_camtg(ctx)) {
		pr_info("[Error][%s] seninf_sentest_disable_old_camtg return failed", __func__);
		return -EFAULT;
	}

	if (seninf_sentest_set_fmt(ctx)) {
		pr_info("[Error][%s] seninf_sentest_set_fmt return failed", __func__);
		return -EFAULT;
	}

	ctrl = v4l2_ctrl_find(ctx->sensor_sd->ctrl_handler,
			V4L2_CID_START_SEAMLESS_SWITCH);

	if (!ctrl) {
		pr_info("[%s][ERROR], no V4L2_CID_START_SEAMLESS_SWITCH cid found in %s\n",
			__func__,
			ctx->sensor_sd->name);
		return -EFAULT;
	}

	v4l2_ctrl_s_ctrl_compound(ctrl, V4L2_CTRL_TYPE_U32, &ctx->sentest_seamless_cfg);
	seninf_sentest_watchingdog_en(&ctx->sentest_watchdog, true);

	for (i = 0; i < cur_vcinfo->cnt; i++) {

		if (get_lastest_outmux_id_by_vc_cnt(ctx, i, &outmux_id)) {
			pr_info("[Error][%s] get_lastest_outmux_id_by_vc_cnt return failed", __func__);
			return -EFAULT;
		}

		settings[i].seninf = &ctx->subdev;
		settings[i].source = cur_vcinfo->vc[i].out_pad;
		settings[i].camtg = i;
		settings[i].enable = 1;
		settings[i].tag_id = 0;
		param.num++;

		pr_info("[%s]pad %d, camtg %d, en %d tag %d num %d",
				__func__,
				settings[i].source,
				settings[i].camtg,
				settings[i].enable,
				settings[i].tag_id,
				param.num);
	}

	param.settings = settings;
	ret |= mtk_cam_seninf_streaming_mux_change(&param, false);
	return ret;
}

static int seninf_sentest_ops_before_sensor_seamless(struct seninf_ctx *ctx)
{

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	if (seninf_sentest_set_camtg_for_seamless(ctx)) {
		pr_info("[Error][%s] seninf_sentest_set_camtg_for_seamless returned false",
				__func__);
		seninf_sentest_seamless_switch_error_handler(ctx);
		return -EFAULT;
	}

	return 0;
}

static int seninf_sentest_seamless_ut_start(struct seninf_ctx *ctx)
{
	int ret;
	struct seninf_sentest_work *sentest_work = NULL;

	sentest_work = kmalloc(sizeof(struct seninf_sentest_work), GFP_ATOMIC);

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		goto SENTEST_SEAMLESS_UT_START_ERR_EXIT;
	}

	if (unlikely(sentest_work == NULL)) {
		pr_info("[Error][%s] sentest_work is NULL", __func__);
		goto SENTEST_SEAMLESS_UT_START_ERR_EXIT;
	}

	ret = seninf_sentest_ops_before_sensor_seamless(ctx);

	ctx->sentest_seamless_is_set_camtg_done = true;

	kfree(sentest_work);
	return ret;

SENTEST_SEAMLESS_UT_START_ERR_EXIT:
	kfree(sentest_work);
	return -EFAULT;
}

static int is_target_vsync(struct seninf_ctx *ctx,
	const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info,
	enum SENTEST_TARGET_VSYNC vsync_type)
{
	bool ret = 0;
	int i = 0 , mask_shift_cnt = 0, mask = 0x01;
	struct seninf_vcinfo *vcinfo = &ctx->cur_vcinfo;
	struct seninf_vc *vc;

	if (unlikely(p_info == NULL)) {
		pr_info("[Error][%s] p_info is NULL", __func__);
		return 0;
	}

	for (i = 0; i < vcinfo->cnt; i++) {
		vc = &vcinfo->vc[i];
		if ((vc->out_pad == PAD_SRC_RAW1) ||
			(vc->out_pad == PAD_SRC_RAW2) ||
			(vc->out_pad == PAD_SRC_RAW_W1) ||
			(vc->out_pad == PAD_SRC_RAW_W2))
			mask_shift_cnt++;
	}

	if (vsync_type == SENTEST_FIRST_VSYNC) {
		ret = (p_info->vsync_status & 0x01)? true : false;
	} else {
		mask |= (mask << mask_shift_cnt);

		ret = (p_info->vsync_status & mask)? true : false;
	}

	pr_info("[%s], vsync_status 0x%x, mask_shift_cnt %d is_last_vsync %d ret %d",
			__func__,
			p_info->vsync_status,
			mask_shift_cnt,
			vsync_type,
			ret);

	return ret;
}

static int seninf_sentest_ops_after_sensor_seamless(struct seninf_ctx *ctx)
{
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	unsigned int sof_cnt = ctx->sentest_irq_counter;

	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	if (!ctx->sentest_seamless_is_set_camtg_done) {
		pr_info("[Error][%s] sentest_seamless_camtg hasn't been processed", __func__);
		seninf_sentest_seamless_switch_error_handler(ctx);
		return -EFAULT;
	}

	/* notify sensor drv Vsync */
	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
				V4L2_CID_VSYNC_NOTIFY);

	if (!ctrl) {
		pr_info("[%s][ERROR], no V4L2_CID_VSYNC_NOTIFY %s\n",
			__func__,
			sensor_sd->name);
		return -EFAULT;
	}
	v4l2_ctrl_s_ctrl(ctrl, sof_cnt);
	seninf_sentest_watchingdog_en(&ctx->sentest_watchdog, false);

	ctx->sentest_seamless_ut_en = false;

	pr_info("[%s] -", __func__);
	return 0;
}

int notify_sentest_irq(struct seninf_ctx *ctx,
					const struct mtk_cam_seninf_tsrec_irq_notify_info *p_info)
{
	if (unlikely(ctx == NULL)) {
		pr_info("[Error][%s] ctx is NULL", __func__);
		return -EFAULT;
	}

	if (!ctx->sentest_seamless_ut_en)
		return -EINVAL;

	if (is_target_vsync(ctx, p_info , SENTEST_FIRST_VSYNC))
		ctx->sentest_irq_counter++;

	pr_info("[%s] sentest_seamless_irq_ref %llu, sentest_irq_counter %llu\n",
			__func__,
			ctx->sentest_seamless_irq_ref,
			ctx->sentest_irq_counter);

	if ((ctx->sentest_seamless_irq_ref + 1) == ctx->sentest_irq_counter) {

		if (!is_target_vsync(ctx, p_info , SENTEST_LAST_VSYNC))
			return 0;

		seninf_sentest_seamless_ut_start(ctx);
	} else {
		seninf_sentest_ops_after_sensor_seamless(ctx);
	}

	return 0;
}

int seninf_sentest_get_csi_mipi_measure_result(struct seninf_ctx *ctx,
		struct mtk_cam_seninf_meter_info *info)
{
	struct v4l2_mbus_framefmt *format;
	int i, valid_measure_req = 0;

	if (unlikely(ctx == NULL)) {
		pr_info("[%s][ERROR] ctx is NULL\n", __func__);
		return -EINVAL;
	}

	if (unlikely(info == NULL)) {
		pr_info("[%s][ERROR] info is NULL\n", __func__);
		return -EINVAL;
	}

	format = &ctx->fmt[PAD_SRC_RAW0].format;

	/* check if measure size is invalid */
	for (i = 0; i < CSIMAC_MEASURE_MAX_NUM; i++) {
		if ((info->probes[i].measure_line >= (format->height - 1)) ||
			(info->probes[i].measure_line == 0))
			continue;

		valid_measure_req++;
	}

	info->valid_measure_line = valid_measure_req;
	if (g_seninf_ops->_get_csi_HV_HB_meter(ctx, info, valid_measure_req)) {
		pr_info("[%s][ERROR] _get_csi_HV_HB_meter return failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}
