// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.


#include "mtk_camera-v4l2-controls.h"
#include "adaptor.h"
#include "adaptor-fsync-ctrls.h"
#include "adaptor-common-ctrl.h"
#include "adaptor-tsrec-cb-ctrl-impl.h"

#include "adaptor-command.h"


/*---------------------------------------------------------------------------*/
// define
/*---------------------------------------------------------------------------*/
#define sd_to_ctx(__sd) container_of(__sd, struct adaptor_ctx, sd)


#define REDUCE_ADAPTOR_COMMAND_LOG


/*---------------------------------------------------------------------------*/
// utilities / static functions
/*---------------------------------------------------------------------------*/
static inline int chk_input_arg(const struct adaptor_ctx *ctx, const void *arg,
	int *p_ret, const char *caller)
{
	*p_ret = 0;
	if (unlikely(ctx == NULL)) {
		*p_ret = -EINVAL;
		return *p_ret;
	}
	if (unlikely(arg == NULL)) {
		*p_ret = -EINVAL;
		adaptor_logi(ctx,
			"[%s] ERROR: idx:%d, invalid arg:(nullptr), ret:%d\n",
			caller, ctx->idx, *p_ret);
		return *p_ret;
	}
	return *p_ret;
}


static int get_sensor_mode_info(struct adaptor_ctx *ctx, u32 mode_id,
				struct mtk_sensor_mode_info *info)
{
	int ret = 0;

	/* Test arguments */
	if (unlikely(info == NULL)) {
		ret = -EINVAL;
		adaptor_logi(ctx, "ERROR: invalid argumet info is nullptr\n");
		return ret;
	}
	if (unlikely(mode_id >= SENSOR_SCENARIO_ID_MAX)) {
		ret = -EINVAL;
		adaptor_logi(ctx, "ERROR: invalid argumet scenario %u\n", mode_id);
		return ret;
	}

	info->scenario_id = mode_id;
	info->mode_exposure_num = g_scenario_exposure_cnt(ctx, mode_id);
	info->active_line_num = ctx->mode[mode_id].active_line_num;
	info->avg_linetime_in_ns = ctx->mode[mode_id].linetime_in_ns_readout;

	return 0;
}


/*---------------------------------------------------------------------------*/
// functions that called by in-kernel drivers.
/*---------------------------------------------------------------------------*/
/* GET */
static int g_cmd_sensor_mode_config_info(struct adaptor_ctx *ctx, void *arg)
{
	int i;
	int ret = 0;
	int mode_cnt = 0;
	struct mtk_sensor_mode_config_info *p_info = NULL;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	p_info = arg;

	memset(p_info, 0, sizeof(struct mtk_sensor_mode_config_info));

	p_info->current_scenario_id = ctx->cur_mode->id;
	if (!get_sensor_mode_info(ctx, ctx->cur_mode->id, p_info->seamless_scenario_infos))
		++mode_cnt;

	for (i = 0; i < SENSOR_SCENARIO_ID_MAX; i++) {
		if (ctx->seamless_scenarios[i] == SENSOR_SCENARIO_ID_NONE)
			break;
		else if (ctx->seamless_scenarios[i] == ctx->cur_mode->id)
			continue;
		else if (!get_sensor_mode_info(ctx, ctx->seamless_scenarios[i],
					       p_info->seamless_scenario_infos + mode_cnt))
			++mode_cnt;
	}

	p_info->count = mode_cnt;

	return ret;
}

static int g_cmd_send_sensor_mode_config_info(struct adaptor_ctx *ctx, void *arg)
{
	int ret = 0;
	int send_scenario_id = 0;
	struct mtk_sensor_mode_info *p_mode_info = NULL;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	p_mode_info = arg;
	send_scenario_id = p_mode_info->scenario_id;
	memset(p_mode_info, 0, sizeof(struct mtk_sensor_mode_info));

	ret = get_sensor_mode_info(ctx, send_scenario_id, p_mode_info);

	if (ret) {
		adaptor_logi(ctx,"get_sensor_mode_info fail , scenario_id:%d, ret:%d\n",
			send_scenario_id, ret);
	}

	return ret;
}

static int g_cmd_sensor_in_reset(struct adaptor_ctx *ctx, void *arg)
{
	int ret = 0;
	bool *in_reset = NULL;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	in_reset = arg;

	*in_reset = !!(ctx->is_sensor_reset_stream_off);

	return ret;
}

static int g_cmd_sensor_has_ebd_parser(struct adaptor_ctx *ctx, void *arg)
{
	int ret = 0;
	bool *has_parser = NULL;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	has_parser = arg;

	*has_parser = !!(ctx->subdrv->ops->parse_ebd_line);

	return ret;
}

static int g_cmd_sensor_ebd_info_by_scenario(struct adaptor_ctx *ctx, void *arg)
{
	int i;
	int ret = 0;
	struct mtk_sensor_ebd_info_by_scenario *p_info = NULL;
	u32 scenario_id;
	struct mtk_mbus_frame_desc fd_tmp = {0};
	struct mtk_mbus_frame_desc_entry_csi2 *entry;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	p_info = arg;

	scenario_id = p_info->input_scenario_id;

	memset(p_info, 0, sizeof(struct mtk_sensor_ebd_info_by_scenario));
	p_info->input_scenario_id = scenario_id;

	ret = subdrv_call(ctx, get_frame_desc, scenario_id, &fd_tmp);
	if (!ret) {
		for (i = 0; i < fd_tmp.num_entries; ++i) {
			entry = &fd_tmp.entry[i].bus.csi2;
			if (entry->user_data_desc == VC_GENERAL_EMBEDDED &&
			    entry->ebd_parsing_type != MTK_EBD_PARSING_TYPE_MIPI_RAW_NA) {

				p_info->exp_hsize = entry->hsize;
				p_info->exp_vsize = entry->vsize;
				p_info->dt_remap_to_type = entry->dt_remap_to_type;
				p_info->data_type = entry->data_type;
				p_info->ebd_parsing_type = entry->ebd_parsing_type;

				adaptor_logd(ctx,
					"[%s] scenario %u desc %u/%u/%u/0x%x/%u\n",
					__func__,
					scenario_id,
					p_info->exp_hsize,
					p_info->exp_vsize,
					p_info->dt_remap_to_type,
					p_info->data_type,
					p_info->ebd_parsing_type);

				break;
			}

		}
	}

	return ret;
}

static int g_cmd_sensor_frame_cnt(struct adaptor_ctx *ctx, void *arg)
{
	u32 *frame_cnt = arg;
	union feature_para para;
	u32 len;
	int ret = 0;

	para.u64[0] = 0;

	/* error handling (unexpected case) */
	if (unlikely(arg == NULL)) {
		ret = -ENOIOCTLCMD;
		adaptor_logi(ctx,
			"ERROR: V4L2_CMD_G_SENSOR_FRAME_CNT, idx:%d, input arg is nullptr, return:%d\n",
			ctx->idx, ret);
		return ret;
	}

	subdrv_call(ctx, feature_control,
		SENSOR_FEATURE_GET_FRAME_CNT,
		para.u8, &len);

	*frame_cnt = para.u64[0];

	return ret;
}

static int g_cmd_sensor_get_lbmf_type_by_secnario(struct adaptor_ctx *ctx, void *arg)
{
	int ret = 0;
	struct mtk_seninf_lbmf_info *info = NULL;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	info = arg;
	info->is_lbmf = 0;

	if (unlikely(ctx->subctx.s_ctx.mode == NULL))
		return -EINVAL;


	info->is_lbmf =
			(ctx->subctx.s_ctx.mode[info->scenario].hdr_mode == HDR_RAW_LBMF) ?
				true : false;

	return ret;
}

static int g_cmd_sensor_glp_dt(struct adaptor_ctx *ctx, void *arg)
{
	u32 *buf = NULL;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	buf = (u32 *)arg;
	memcpy(buf, ctx->subctx.s_ctx.glp_dt, sizeof(ctx->subctx.s_ctx.glp_dt));

	return ret;
}

static int g_cmd_sensor_vc_info_by_scenario(struct adaptor_ctx *ctx, void *arg)
{
	int ret = 0;
	struct mtk_sensor_vc_info_by_scenario *p_info = NULL;
	struct mtk_mbus_frame_desc fd_tmp = {0};

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	p_info = arg;
	ret = subdrv_call(ctx, get_frame_desc, p_info->scenario_id, &fd_tmp);
	memcpy(&p_info->fd.entry, &fd_tmp.entry,
				sizeof(struct mtk_mbus_frame_desc_entry)*fd_tmp.num_entries);
	p_info->fd.num_entries = fd_tmp.num_entries;

	return ret;
}

/* SET */
static int s_cmd_fsync_sync_frame_start_end(struct adaptor_ctx *ctx, void *arg)
{
	int *p_flag = NULL;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	/* casting arg to int-pointer for using */
	p_flag = arg;

	adaptor_logd(ctx,
		"V4L2_CMD_FSYNC_SYNC_FRAME_START_END, idx:%d, flag:%d\n",
		ctx->idx, *p_flag);

	notify_fsync_mgr_sync_frame(ctx, *p_flag);

	return ret;
}


static int s_cmd_tsrec_notify_vsync(struct adaptor_ctx *ctx, void *arg)
{
	struct mtk_cam_seninf_tsrec_vsync_info *buf = NULL;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	buf = (struct mtk_cam_seninf_tsrec_vsync_info *)arg;

	memcpy(&ctx->vsync_info, buf, sizeof(ctx->vsync_info));


#ifndef REDUCE_ADAPTOR_COMMAND_LOG
	adaptor_logd(ctx,
		"V4L2_CMD_TSREC_NOTIFY_VSYNC, idx:%d, vsync_info(tsrec_no:%u, seninf_idx:%u, sys_ts%llu(ns), tsrec_ts:%llu(us))\n",
		ctx->idx,
		ctx->vsync_info.tsrec_no,
		ctx->vsync_info.seninf_idx,
		ctx->vsync_info.irq_sys_time_ns,
		ctx->vsync_info.irq_tsrec_ts_us);
#endif


	/* tsrec notify vsync, call all APIs that needed this info */
	notify_fsync_mgr_vsync_by_tsrec(ctx);

	return ret;
}


static int s_cmd_tsrec_notify_sensor_hw_pre_latch(
	struct adaptor_ctx *ctx, void *arg)
{
	struct mtk_cam_seninf_tsrec_timestamp_info *ts_info = NULL;
	// unsigned long long sys_ts;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	ts_info = (struct mtk_cam_seninf_tsrec_timestamp_info *)arg;
	// sys_ts = ktime_get_boottime_ns();

#ifndef REDUCE_ADAPTOR_COMMAND_LOG
	adaptor_logd(ctx,
		"V4L2_CMD_TSREC_NOTIFY_SENSOR_HW_PRE_LATCH, idx:%d, ts_info(tsrec_no:%u, seninf_idx:%u, tick_factor:%u, sys_ts:%llu(ns), tsrec_ts:%llu(us), tick:%llu, ts(0:(%llu/%llu/%llu/%llu), 1:(%llu/%llu/%llu/%llu), 2:(%llu/%llu/%llu/%llu)), curr_sys_ts:%llu(ns)\n",
		ctx->idx,
		ts_info->tsrec_no,
		ts_info->seninf_idx,
		ts_info->tick_factor,
		ts_info->irq_sys_time_ns,
		ts_info->irq_tsrec_ts_us,
		ts_info->tsrec_curr_tick,
		ts_info->exp_recs[0].ts_us[0],
		ts_info->exp_recs[0].ts_us[1],
		ts_info->exp_recs[0].ts_us[2],
		ts_info->exp_recs[0].ts_us[3],
		ts_info->exp_recs[1].ts_us[0],
		ts_info->exp_recs[1].ts_us[1],
		ts_info->exp_recs[1].ts_us[2],
		ts_info->exp_recs[1].ts_us[3],
		ts_info->exp_recs[2].ts_us[0],
		ts_info->exp_recs[2].ts_us[1],
		ts_info->exp_recs[2].ts_us[2],
		ts_info->exp_recs[2].ts_us[3],
		sys_ts);
#endif

	/* tsrec notify sensor hw pre-latch, call all APIs that needed this info */
	notify_fsync_mgr_sensor_hw_pre_latch_by_tsrec(ctx, ts_info);

	return ret;
}


static int s_cmd_tsrec_send_timestamp_info(struct adaptor_ctx *ctx, void *arg)
{
	struct mtk_cam_seninf_tsrec_timestamp_info *buf = NULL;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	buf = (struct mtk_cam_seninf_tsrec_timestamp_info *)arg;
	memcpy(&ctx->ts_info, buf, sizeof(ctx->ts_info));

	return ret;
}

static int s_cmd_sensor_parse_ebd(struct adaptor_ctx *ctx, void *arg)
{
	struct mtk_recv_sensor_ebd_line *recv_ebd;
	struct mtk_ebd_dump obj;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	recv_ebd = (struct mtk_recv_sensor_ebd_line *)arg;

	memset(&obj, 0, sizeof(obj));
	ret = subdrv_call(ctx, parse_ebd_line, recv_ebd, &obj);

	mutex_lock(&ctx->ebd_lock);
	ctx->latest_ebd.recv_ts = ktime_get_boottime_ns();
	ctx->latest_ebd.req_no = recv_ebd->req_id;
	memset(ctx->latest_ebd.req_fd_desc, 0, sizeof(ctx->latest_ebd.req_fd_desc));
	strncpy(ctx->latest_ebd.req_fd_desc, recv_ebd->req_fd_desc,
		sizeof(ctx->latest_ebd.req_fd_desc) - 1);
	memcpy(&ctx->latest_ebd.record, &obj, sizeof(struct mtk_ebd_dump));
	mutex_unlock(&ctx->ebd_lock);

	return ret;
}

static int s_cmd_tsrec_setup_cb_info(struct adaptor_ctx *ctx, void *arg)
{
	struct mtk_cam_seninf_tsrec_cb_info *tsrec_cb_info = NULL;
	int ret = 0;

	/* unexpected case, arg is nullptr */
	if (unlikely((chk_input_arg(ctx, arg, &ret, __func__)) != 0))
		return ret;

	tsrec_cb_info = (struct mtk_cam_seninf_tsrec_cb_info *)arg;

	adaptor_tsrec_cb_ctrl_info_setup(ctx,
		tsrec_cb_info, tsrec_cb_info->is_connected_to_tsrec);

	// adaptor_tsrec_cb_ctrl_dbg_info_dump(ctx, __func__);

	return ret;
}

/*---------------------------------------------------------------------------*/
// adaptor command framework/entry
/*---------------------------------------------------------------------------*/

struct command_entry {
	unsigned int cmd;
	int (*func)(struct adaptor_ctx *ctx, void *arg);
};

static const struct command_entry command_list[] = {
	/* GET */
	{V4L2_CMD_GET_SENSOR_MODE_CONFIG_INFO, g_cmd_sensor_mode_config_info},
	{V4L2_CMD_GET_SEND_SENSOR_MODE_CONFIG_INFO, g_cmd_send_sensor_mode_config_info},
	{V4L2_CMD_SENSOR_IN_RESET, g_cmd_sensor_in_reset},
	{V4L2_CMD_SENSOR_HAS_EBD_PARSER, g_cmd_sensor_has_ebd_parser},
	{V4L2_CMD_GET_SENSOR_EBD_INFO_BY_SCENARIO, g_cmd_sensor_ebd_info_by_scenario},
	{V4L2_CMD_SENSOR_GET_LBMF_TYPE_BY_SCENARIO, g_cmd_sensor_get_lbmf_type_by_secnario},
	{V4L2_CMD_G_SENSOR_FRAME_CNT, g_cmd_sensor_frame_cnt},
	{V4L2_CMD_G_SENSOR_GLP_DT, g_cmd_sensor_glp_dt},
	{V4L2_CMD_G_SENSOR_VC_INFO_BY_SCENARIO, g_cmd_sensor_vc_info_by_scenario},

	/* SET */
	{V4L2_CMD_FSYNC_SYNC_FRAME_START_END, s_cmd_fsync_sync_frame_start_end},
	{V4L2_CMD_TSREC_NOTIFY_VSYNC, s_cmd_tsrec_notify_vsync},
	{V4L2_CMD_TSREC_NOTIFY_SENSOR_HW_PRE_LATCH,
		s_cmd_tsrec_notify_sensor_hw_pre_latch},
	{V4L2_CMD_TSREC_SEND_TIMESTAMP_INFO, s_cmd_tsrec_send_timestamp_info},
	{V4L2_CMD_SENSOR_PARSE_EBD, s_cmd_sensor_parse_ebd},
	{V4L2_CMD_TSREC_SETUP_CB_FUNC_OF_SENSOR, s_cmd_tsrec_setup_cb_info},
};

long adaptor_command(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct adaptor_ctx *ctx = NULL;
	int i, ret = -ENOIOCTLCMD, is_cmd_found = 0;

	/* error handling (unexpected case) */
	if (unlikely(sd == NULL)) {
		ret = -ENOIOCTLCMD;
		pr_info(
			"[%s] ERROR: get nullptr of v4l2_subdev (sd), return:%d   [cmd id:%#x]\n",
			__func__, ret, cmd);
		return ret;
	}

	ctx = sd_to_ctx(sd);

#ifndef REDUCE_ADAPTOR_COMMAND_LOG
	adaptor_logd(ctx,
		"dispatch command request, idx:%d, cmd id:%#x\n",
		ctx->idx, cmd);
#endif

	/* dispatch command request */
	for (i = 0; i < ARRAY_SIZE(command_list); i++) {
		if (command_list[i].cmd == cmd) {
			is_cmd_found = 1;
			ret = command_list[i].func(ctx, arg);
			break;
		}
	}
	if (unlikely(is_cmd_found == 0)) {
		ret = -ENOIOCTLCMD;
		adaptor_logi(ctx,
			"ERROR: get unknown command request, idx:%d, cmd:%u, return:%d\n",
			ctx->idx, cmd, ret);
		return ret;
	}

	return ret;
}

