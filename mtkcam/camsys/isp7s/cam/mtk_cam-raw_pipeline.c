// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/pm_runtime.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>

#include "mtk_cam.h"
#include "mtk_cam-dvfs_qos.h"
#include "mtk_cam-raw_pipeline.h"
#include "mtk_cam-resource_calc.h"
#include "mtk_cam-plat.h"
#include "mtk_cam-fmt_utils.h"

static unsigned int debug_raw;
module_param(debug_raw, uint, 0644);
MODULE_PARM_DESC(debug_raw, "activates debug info");

static int debug_raw_num = -1;
module_param(debug_raw_num, int, 0644);
MODULE_PARM_DESC(debug_raw_num, "debug: num of used raw devices");

static int debug_pixel_mode = -1;
module_param(debug_pixel_mode, int, 0644);
MODULE_PARM_DESC(debug_pixel_mode, "debug: pixel mode");

static int debug_clk_idx = -1;
module_param(debug_clk_idx, int, 0644);
MODULE_PARM_DESC(debug_clk_idx, "debug: clk idx");

static int debug_user_raws_must[3] = {-1, -1, -1};
module_param_array(debug_user_raws_must, int, NULL, 0644);
MODULE_PARM_DESC(debug_user_raws_must, "debug: user raws must");

static int debug_user_raws[3] = {-1, -1, -1};
module_param_array(debug_user_raws, int, NULL, 0644);
MODULE_PARM_DESC(debug_user_raws, "debug: user raws");

static int debug_disable_twin;
module_param(debug_disable_twin, int, 0644);
MODULE_PARM_DESC(debug_disable_twin, "debug: disable twin");

#define RAW_PIPELINE_NUM 3

#define MTK_CAMSYS_RES_IDXMASK		0xF0
#define MTK_CAMSYS_RES_BIN_TAG		0x10
#define MTK_CAMSYS_RES_FRZ_TAG		0x20
#define MTK_CAMSYS_RES_HWN_TAG		0x30
#define MTK_CAMSYS_RES_CLK_TAG		0x40

#define MTK_CAMSYS_RES_PLAN_NUM		10
#define TGO_MAX_PXLMODE		8
#define FRZ_PXLMODE_THRES		71
#define MHz		1000000
#define MTK_CAMSYS_PROC_DEFAULT_PIXELMODE	2
#define DC_DEFAULT_CAMSV_PIXELMODE			8

#define DC_SUPPORT_RAW_FEATURE_MASK	\
	(STAGGER_2_EXPOSURE_LE_SE | STAGGER_2_EXPOSURE_SE_LE)

#define sizeof_u32(__struct__) ((sizeof(__struct__) + sizeof(u32) - 1)/ \
				sizeof(u32))

struct mbus_config_getter {
	struct v4l2_mbus_framefmt *(*get_fmt)(struct v4l2_subdev *sd,
					      struct v4l2_subdev_state *state,
					      unsigned int pad);
	struct v4l2_rect *(*get_crop)(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *state,
				      unsigned int pad);
};

static inline struct v4l2_rect fullsize_as_crop(unsigned int w, unsigned int h)
{
	return (struct v4l2_rect) {
		.left = 0, .top = 0, .width = w, .height = h,
	};
}

#define USE_CTRL_PIXEL_RATE 0
#define DC_MODE_VB_MARGIN 100
static int res_calc_fill_sensor(struct mtk_cam_res_calc *c,
				const struct mtk_cam_resource_sensor_v2 *s,
				const struct mtk_cam_resource_raw_v2 *r)
{
	u32 vb = (r->hw_mode != HW_MODE_DIRECT_COUPLED) ?
			s->vblank : DC_MODE_VB_MARGIN;

#if USE_CTRL_PIXEL_RATE
	c->mipi_pixel_rate = s->pixel_rate;
#else
	c->mipi_pixel_rate =
		(u64)(s->width + s->hblank)
		* (s->height + s->vblank)
		* s->interval.denominator / max(s->interval.numerator, 1U);
#endif
	c->line_time =
		1000000000L
		* s->interval.numerator / s->interval.denominator
		/ max(s->height + vb, 1U);
	c->width = s->width;
	c->height = s->height;
	return 0;
}

struct raw_resource_stepper {
	/* pixel mode */
	int pixel_mode_min;
	int pixel_mode_max;

	/* raw num */
	int num_raw_min;
	int num_raw_max;

	/* freq */
	int opp_num;
	const struct camsys_opp_table *tbl;
	int min_opp_idx;

	int pixel_mode;
	int num_raw;
	int opp_idx;
};

static int step_next_raw_num(struct raw_resource_stepper *stepper)
{
	if (stepper->num_raw < stepper->num_raw_max) {
		++stepper->num_raw;
		return 0;
	}
	stepper->num_raw = stepper->num_raw_min;
	return -1;
}

static int step_next_opp(struct raw_resource_stepper *stepper)
{
	++stepper->opp_idx;
	if (stepper->opp_idx < stepper->opp_num)
		return 0;

	stepper->opp_idx = stepper->min_opp_idx;
	return -1;
}

static int step_next_pixel_mode(struct raw_resource_stepper *stepper)
{
	if (stepper->pixel_mode < stepper->pixel_mode_max) {
		++stepper->pixel_mode;
		return 0;
	}
	stepper->pixel_mode = stepper->pixel_mode_min;
	return -1;
}

typedef int (*step_fn_t)(struct raw_resource_stepper *stepper);

static bool valid_resouce_set(struct raw_resource_stepper *s)
{
	/* invalid sets */
	bool invalid =
		(s->pixel_mode == 1 && s->opp_idx != s->min_opp_idx) ||
		(s->pixel_mode == 1 && s->num_raw > 1);

	return !invalid;
}

static int loop_resource_till_valid(struct mtk_cam_res_calc *c,
				    struct raw_resource_stepper *stepper,
				    const step_fn_t *arr_step, int arr_size)
{
	const bool enable_log = false;
	int i;
	int ret = -1;

	do {
		bool pass;

		i = 0;
		if (valid_resouce_set(stepper)) {

			c->raw_pixel_mode = stepper->pixel_mode;
			c->raw_num = stepper->num_raw;
			c->clk = stepper->tbl[stepper->opp_idx].freq_hz;

			pass = mtk_cam_check_mipi_pixel_rate(c, enable_log) &&
				mtk_cam_raw_check_line_buffer(c, enable_log) &&
				mtk_cam_raw_check_throughput(c, enable_log);
			if (pass) {
				ret = 0;
				break;
			}
		}

		if (arr_step[0](stepper)) {

			i = 1;
			while (i < arr_size && arr_step[i](stepper))
				++i;
		}
	} while (i < arr_size);

	return ret;
}

static inline int _get_min_opp_idx(struct raw_resource_stepper *stepper)
{
	int min_idx = 0;

	while (min_idx < stepper->opp_num &&
	       stepper->tbl[stepper->opp_idx].freq_hz == 0)
		++min_idx;

	return (min_idx == stepper->opp_num) ? 0 : min_idx;
}

static inline int mtk_raw_find_combination(struct mtk_cam_res_calc *c,
				    struct raw_resource_stepper *stepper)
{
	static const step_fn_t policy[] = {
		step_next_pixel_mode,
		step_next_raw_num,
		step_next_opp
	};

	stepper->min_opp_idx = _get_min_opp_idx(stepper);

	/* initial value */
	stepper->pixel_mode = stepper->pixel_mode_min;
	stepper->num_raw = stepper->num_raw_min;
	stepper->opp_idx = stepper->min_opp_idx;

	return loop_resource_till_valid(c, stepper,
					policy, ARRAY_SIZE(policy));
}

static void mtk_cam_get_work_buf_num(struct mtk_cam_resource_v2 *user_ctrl)
{
	struct mtk_cam_resource_sensor_v2 *s = &user_ctrl->sensor_res;
	struct mtk_cam_resource_raw_v2 *r = &user_ctrl->raw_res;
	struct mtk_cam_scen *scen = &r->scen;
	int exp_num = 0, buf_require = 0;
	struct mtk_cam_driver_buf_desc desc;
	struct v4l2_mbus_framefmt mf;

	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
		exp_num = (scen->scen.normal.max_exp_num == 0) ?
					1 : scen->scen.normal.max_exp_num;
		buf_require = (r->hw_mode == HW_MODE_DIRECT_COUPLED) ?
					exp_num : exp_num - 1;
		buf_require = !!(scen->scen.normal.w_chn_supported) ?
					buf_require * 2 : buf_require;
		break;
	case MTK_CAM_SCEN_MSTREAM:
		buf_require = (r->hw_mode == HW_MODE_DIRECT_COUPLED) ?
					2 : 1;
		break;
	case MTK_CAM_SCEN_EXT_ISP:
		/* TODO */
		buf_require = 1;
		break;
	default:
		break;
	}

	/* TODO: check pure raw fmt for DC */
	mf.code = s->code;
	mf.width = s->width;
	mf.height = s->height;
	update_buf_fmt_desc(&desc, &mf);
	r->img_wbuf_size = desc.max_size;

	r->img_wbuf_num = buf_require;
}

static int mtk_raw_calc_raw_mask_chk(struct device *dev,
				     const int num_raws, u8 *raws,
				     u8 *raws_must, u8 *raws_max_num)
{
	int all_raw_mask;

	/* consider raw numbers in runtime */
	all_raw_mask = (1 << num_raws) - 1;

	/**
	 * That raws with 0 is a special case, it means all raws can be selected by driver.
	 */
	if (!(*raws)) {
		if (CAM_DEBUG_ENABLED(V4L2))
			dev_info(dev, "warning! raws can't be 0, reset it to 0x%x\n",
				 all_raw_mask);
		*raws = all_raw_mask;
	}

	if ((*raws & all_raw_mask) != *raws ||
	    (*raws_must & all_raw_mask) != *raws_must) {
		dev_info(dev,
			 "%s: raws_must(0x%x)/raws(0x%x) have invalid raws. valid(0x%x)\n",
			 __func__, *raws_must, *raws, all_raw_mask);
		return -1;
	}

	/* Error can't be recovered since raws_must has bits which is not enabled in raws */
	if ((*raws & *raws_must) != *raws_must) {
		dev_info(dev,
			 "%s: error! raws_must(0x%x) has unavailable raw. raws(0x%x)\n",
			 __func__, *raws_must, *raws);
		return -1;
	}

	if (*raws_max_num > num_raws) {
		dev_info(dev,
			 "%s: invalid raws_max_num(%d) reset it to (%d)\n",
			 __func__, *raws_max_num, num_raws);
		*raws_max_num = num_raws;
	}

	/* TODO: dc mode's raws check, we don't support twin with DC in ISP7S */

	return 0;
}

static int max_num_conti_bits(u8 bits)
{
	int count = 0;

	while (bits) {
		bits &= bits << 1;
		count++;
	}
	return count;
}

static int mtk_raw_calc_raw_mask_chk_scen(struct mtk_cam_device *cam,
					  struct mtk_cam_resource_raw_v2 *r)
{
	// RGBW requires a minimum of 2 raw
	if (r->scen.scen.normal.w_chn_enabled) {
		int raws_must_cnt = max_num_conti_bits(r->raws_must);

		if (r->raws_must && raws_must_cnt != 2) {
			dev_info(cam->dev, "w_chn_enabled, wrong raws_must 0x%x\n",
				 r->raws_must);
			return -1;
		}

		r->raws_max_num = 2;
	}

	return 0;
}

/**
 * Pre-condition:
 * 1. raws can't be 0
 * 2. raws_max_num must <= real raw's number
 */
static void mtk_raw_calc_num_raw_max_min(struct mtk_cam_resource_raw_v2 *r,
					 int *n_min, int *n_max)
{
	bool is_rgbw = r->scen.scen.normal.w_chn_enabled;

	if (is_rgbw) {
		*n_min = 2;
		*n_max = 2;
		return;
	}

	*n_min = max(1, max_num_conti_bits(r->raws_must));
	*n_max = min3(1, (int)r->raws_max_num, max_num_conti_bits(r->raws));

	if (debug_disable_twin)
		*n_max = 1;
}

static unsigned int mtk_raw_choose_raws(const int raw_count, const int num_raws,
					const u8 raws, const u8 raws_must)
{
	u8 raws_test;
	int i;

	if (raw_count <= 0)
		return 0;

	raws_test = (u8)((1 << raw_count) - 1);

	for (i = 0; i < num_raws - raw_count + 1; i++, raws_test <<= 1) {

		/* must cover raws_must and raws*/
		if ((raws_test & raws_must) == raws_must &&
		    (raws_test & raws) == raws_test) {
			return raws_test;
		}
	}

	return 0;
}

#define M2M_PROCESS_MARGIN_N 11550
#define M2M_PROCESS_MARGIN_D 10000
static void res_sensor_info_validate(
		struct mtk_cam_resource_sensor_v2 *s,
		struct mtk_cam_resource_raw_v2 *r)
{
	struct mtk_cam_scen *scen = &r->scen;
	u64 prate = 0;

	if (scen->id == MTK_CAM_SCEN_M2M_NORMAL ||
		scen->id == MTK_CAM_SCEN_ODT_NORMAL) {

		if (s->interval.numerator == 0 ||
		    s->interval.denominator == 0) {
			s->interval.denominator = 300;
			s->interval.numerator = 10;
		}

		prate = (u64)s->width * s->height *
				s->interval.denominator * M2M_PROCESS_MARGIN_N;
		do_div(prate, s->interval.numerator * M2M_PROCESS_MARGIN_D);
		s->pixel_rate = prate;

		pr_info("%s:width:%u height:%u fps:%u/%u prate:%lld\n",
			__func__,
			s->width, s->height,
			s->interval.denominator,
			s->interval.numerator, prate);
	} else {
		if (s->interval.numerator == 0 ||
		    s->interval.denominator == 0) {
			pr_info("%s: wrong fps(%u/%u) use (300/10) instead\n",
				__func__,
				s->interval.denominator,
				s->interval.numerator);

			s->interval.denominator = 300;
			s->interval.numerator = 10;
		}
	}
}

static inline int mtk_pixelmode_val(int pxl_mode)
{
	int val = 0;

	switch (pxl_mode) {
	case 1:
		val = 0;
		break;
	case 2:
		val = 1;
		break;
	case 4:
		val = 2;
		break;
	case 8:
		val = 3;
		break;
	default:
		break;
	}

	return val;
}

/* 0: disable, 1: 2x2, 2: 3x3 3: 4x4 */
static inline int mtk_cbn_type(int cbn)
{
	if (cbn == MTK_CAM_CBN_2X2_ON)
		return 1;
	else if (cbn == MTK_CAM_CBN_3X3_ON)
		return 2;
	else if (cbn == MTK_CAM_CBN_4X4_ON)
		return 3;
	else
		return 0;
}

#define PIX_MODE_SIZE_CONSTRAIN 1920
static int mtk_raw_calc_raw_resource(struct mtk_raw_pipeline *pipeline,
				     struct mtk_cam_resource_v2 *user_ctrl,
				     struct mtk_cam_resource_driver *drv_data)
{
	struct mtk_cam_device *cam = subdev_to_cam_device(&pipeline->subdev);
	struct mtk_cam_resource_sensor_v2 *s = &user_ctrl->sensor_res;
	struct mtk_cam_resource_raw_v2 *r = &user_ctrl->raw_res;
	struct mtk_cam_res_calc c;
	struct raw_resource_stepper stepper;
	int ret, final_raw_num; /* consider debug setting and calc result */
	unsigned int raws_driver_selected;

	memset(&c, 0, sizeof(c));
	memset(&stepper, 0, sizeof(stepper));

	if (debug_user_raws_must[pipeline->id] != -1) {
		dev_info(cam->dev,
			 "debug:pipe(%d):replace raws_must, 0x%x--> 0x%x\n",
			 pipeline->id, r->raws_must, debug_user_raws_must[pipeline->id]);
		r->raws_must = debug_user_raws_must[pipeline->id];
	}

	if (debug_user_raws[pipeline->id] != -1) {
		dev_info(cam->dev,
			 "debug:pipe(%d):replace raws, 0x%x--> 0x%x\n",
			 pipeline->id, r->raws, debug_user_raws_must[pipeline->id]);
		r->raws = debug_user_raws_must[pipeline->id];
	}

	ret = mtk_raw_calc_raw_mask_chk(cam->dev, cam->engines.num_raw_devices,
				       &r->raws, &r->raws_must, &r->raws_max_num)
		|| mtk_raw_calc_raw_mask_chk_scen(cam, r);

	if (ret)
		return -EINVAL;

	res_sensor_info_validate(s, r);
	res_calc_fill_sensor(&c, s, r);
	c.cbn_type = mtk_cbn_type(user_ctrl->raw_res.bin);
	c.qbnd_en = (user_ctrl->raw_res.bin == MTK_CAM_QBND_ON) ? 1 : 0;
	c.qbn_type = 0; /* 0: disable, 1: w/2, 2: w/4 */
	c.bin_en = (user_ctrl->raw_res.bin == MTK_CAM_BIN_ON) ? 1 : 0;

	/* constraints */
	/* always 2 pixel mode, beside sensor size <= 1920 */
	stepper.pixel_mode_min = (s->width <= PIX_MODE_SIZE_CONSTRAIN) ? 1 : 2;
	stepper.pixel_mode_max = 2;

	mtk_raw_calc_num_raw_max_min(r,
				     &stepper.num_raw_min,
				     &stepper.num_raw_max);

	stepper.opp_num = mtk_cam_dvfs_get_opp_table(&cam->dvfs, &stepper.tbl);

	ret = mtk_raw_find_combination(&c, &stepper);
	if (ret) {
		dev_info(cam->dev, "failed to find valid resource\n");
		ret = -EBUSY;
		goto EXIT;
	}

	r->raw_pixel_mode = c.raw_pixel_mode;

	mtk_cam_get_work_buf_num(user_ctrl);

	final_raw_num = (debug_raw_num == -1) ? c.raw_num : debug_raw_num;

	raws_driver_selected = mtk_raw_choose_raws(final_raw_num,
						   cam->engines.num_raw_devices,
						   r->raws, r->raws_must);
	if (!raws_driver_selected) {
		dev_info(cam->dev,
			 "%s: failed to choose raws: raws(0x%x)/raws_must(0x%x)/raw_num(%d)\n",
			 __func__, r->raws, r->raws_must, c.raw_num);
		ret = -EINVAL;
	}

	r->raws = raws_driver_selected;
	if (drv_data) {
		drv_data->user_data = *user_ctrl;
		drv_data->clk_target = c.clk;
		drv_data->raw_num = final_raw_num;
		drv_data->tgo_pxl_mode =
			mtk_pixelmode_val(mtk_raw_overall_pixel_mode(&c));
		drv_data->tgo_pxl_mode_before_raw = mtk_pixelmode_val(8);
	}

EXIT:
	if (drv_data || ret == -EBUSY || CAM_DEBUG_ENABLED(V4L2_TRY))
		dev_info(cam->dev,
			 "calc_resource: sensor fps %u/%u %dx%d blank %u/%u linet %ld prate %llu clk %d pxlmode %d num %d bin %d img_wbuf %dx%d raw(0x%x,0x%x,%d)\n",
			 s->interval.denominator, s->interval.numerator,
			 s->width, s->height, s->hblank, s->vblank, c.line_time,
			 s->pixel_rate,
			 user_ctrl->raw_res.bin,
			 c.clk, c.raw_pixel_mode, c.raw_num,
			 r->img_wbuf_num, r->img_wbuf_size,
			 r->raws, r->raws_must, r->raws_max_num);

	return ret;
}

static int mtk_raw_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_raw_pipeline *pipeline;
	struct device *dev;

	pipeline = mtk_cam_ctrl_handler_to_raw_pipeline(ctrl->handler);
	dev = subdev_to_cam_dev(&pipeline->subdev);

	if (ctrl->id == V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL)
		*((struct mtk_cam_internal_mem *)ctrl->p_new.p) = pipeline->ctrl_data.pre_alloc_mem;
	else
		dev_info(dev, "%s: error. ctrl(\"%s\", id:0x%x) not supported yet\n",
			 __func__, ctrl->name, ctrl->id);
	return 0;
}

static int mtk_raw_try_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_raw_pipeline *pipeline;
	struct device *dev;
	int ret = 0;

	pipeline = mtk_cam_ctrl_handler_to_raw_pipeline(ctrl->handler);
	dev = subdev_to_cam_dev(&pipeline->subdev);

	switch (ctrl->id) {
	case V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC:
		{
			struct mtk_cam_resource_v2 *user_ctrl =
				(struct mtk_cam_resource_v2 *)ctrl->p_new.p;

			ret = mtk_raw_calc_raw_resource(pipeline,
							user_ctrl, NULL);
		}
		break;
	case V4L2_CID_MTK_CAM_TG_FLASH_CFG:
		ret = mtk_cam_tg_flash_try_ctrl(ctrl);
		break;
	/* skip control value checks */
	case V4L2_CID_MTK_CAM_RAW_RESOURCE_UPDATE:
	case V4L2_CID_MTK_CAM_MSTREAM_EXPOSURE:
	case V4L2_CID_MTK_CAM_APU_INFO:
	case V4L2_CID_MTK_CAM_RAW_PATH_SELECT:
	case V4L2_CID_MTK_CAM_SYNC_ID:
	case V4L2_CID_MTK_CAM_HSF_EN:
	case V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL:
		ret = 0;
		break;
	default:
		dev_info(dev, "%s: error. ctrl(\"%s\", id:0x%x) not supported yet\n",
			 __func__, ctrl->name, ctrl->id);
		break;
	}

	return ret;
}

static void reset_internal_mem(struct mtk_raw_pipeline *pipeline)
{
	struct mtk_raw_ctrl_data *ctrl_data;
	struct mtk_cam_internal_mem *mem;

	ctrl_data = &pipeline->ctrl_data;
	mem = &ctrl_data->pre_alloc_mem;

	if (ctrl_data->pre_alloc_dbuf)
		dma_buf_put(ctrl_data->pre_alloc_dbuf);

	ctrl_data->pre_alloc_dbuf = NULL;
	mem->num = 0;
}

static int mtk_raw_set_internal_mem(struct mtk_raw_pipeline *pipeline,
				    struct mtk_cam_internal_mem *ctrl_mem)
{
	struct mtk_raw_ctrl_data *ctrl_data;
	struct device *dev;

	ctrl_data = &pipeline->ctrl_data;
	dev = subdev_to_cam_dev(&pipeline->subdev);

	reset_internal_mem(pipeline);

	if (ctrl_mem->num > 0) {
		int buf_fd;

		buf_fd = ctrl_mem->bufs[0].fd;
		ctrl_data->pre_alloc_dbuf = dma_buf_get(buf_fd);
		if (!ctrl_data->pre_alloc_dbuf) {
			dev_info(dev, "%s: pipe(%d): failed to get dbuf from fd %d\n",
				 __func__, pipeline->id, buf_fd);
			return -1;
		}

		ctrl_data->pre_alloc_mem = *ctrl_mem;
	}

	dev_info(dev,
		 "%s:pipe(%d): pre_alloc_mem(%d,%d,%d)\n",
		 __func__, pipeline->id,
		 ctrl_mem->num,
		 ctrl_mem->bufs[0].fd,
		 ctrl_mem->bufs[0].length);
	return 0;
}

static int mtk_raw_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_raw_pipeline *pipeline;
	struct mtk_raw_ctrl_data *ctrl_data;
	struct device *dev;
	int ret = 0;

	pipeline = mtk_cam_ctrl_handler_to_raw_pipeline(ctrl->handler);
	ctrl_data = &pipeline->ctrl_data;
	dev = subdev_to_cam_dev(&pipeline->subdev);

	switch (ctrl->id) {
	case V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC:
		{
			struct mtk_cam_resource_v2 *user_ctrl =
				(struct mtk_cam_resource_v2 *)ctrl->p_new.p;
			struct mtk_cam_resource_driver *drv_res =
				&ctrl_data->resource;

			ret = mtk_raw_calc_raw_resource(pipeline,
							user_ctrl, drv_res);
		}
		break;
	case V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL:
		ret = mtk_raw_set_internal_mem(pipeline, ctrl->p_new.p);
		break;
	case V4L2_CID_MTK_CAM_RAW_PATH_SELECT:
		{
			if (ctrl->val > V4L2_MTK_CAM_RAW_PATH_SELECT_LTM ||
			    ctrl->val < 0) {
				dev_info(dev,
					 "%s:pipe(%d): invalid raw_path(%d) from user\n",
					 __func__, pipeline->id,
					 ctrl->val);
				ret = -EINVAL;
			} else {
				pipeline->ctrl_data.raw_path = ctrl->val;
				ret = 0;
			}
		}
		break;
	case V4L2_CID_MTK_CAM_MSTREAM_EXPOSURE:
		ctrl_data->mstream_exp =
			*(struct mtk_cam_mstream_exposure *)ctrl->p_new.p;
		break;
	case V4L2_CID_MTK_CAM_APU_INFO:
		{
			struct mtk_cam_apu_info *apu_info = &ctrl_data->apu_info;

			*apu_info = *(struct mtk_cam_apu_info *)ctrl->p_new.p;

			if (1 || CAM_DEBUG_ENABLED(V4L2))
				dev_info(dev, "%s: apu_info: path %d i,o=%d,%d sysram %d opp_idx=%d blk_y_size %d\n",
					 __func__,
					 apu_info->apu_path,
					 apu_info->vpu_i_point,
					 apu_info->vpu_o_point,
					 apu_info->sysram_en,
					 apu_info->opp_index,
					 apu_info->block_y_size);
		}
		break;
	case V4L2_CID_MTK_CAM_RAW_RESOURCE_UPDATE:
		ctrl_data->rc_data.sensor_mode_update = ctrl->val;
		dev_info(dev, "%s:pipe(%d):sensor_mode_update(%d)\n",
			 __func__, pipeline->id, ctrl_data->rc_data.sensor_mode_update);
		break;
#ifdef NOT_READY
	case V4L2_CID_MTK_CAM_TG_FLASH_CFG:
		ret = mtk_cam_tg_flash_s_ctrl(ctrl);
		break;
#endif
	case V4L2_CID_MTK_CAM_SYNC_ID:
		break;
	default:
		dev_info_ratelimited(dev, "%s: error. ctrl(\"%s\", id:0x%x) not supported yet\n",
				     __func__, ctrl->name, ctrl->id);
		break;
	}
	return ret;
}

static const struct v4l2_ctrl_ops cam_ctrl_ops = {
	.g_volatile_ctrl = mtk_raw_get_ctrl,
	.s_ctrl = mtk_raw_set_ctrl,
	.try_ctrl = mtk_raw_try_ctrl,
};

static int mtk_raw_pde_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_raw_pipeline *pipeline;
	struct mtk_raw_pde_config *pde_cfg;
	struct mtk_cam_pde_info *pde_info_p;
	struct device *dev = mtk_cam_root_dev();
	int ret = 0;

	pipeline = mtk_cam_ctrl_handler_to_raw_pipeline(ctrl->handler);
	pde_cfg = &pipeline->pde_config;
	pde_info_p = ctrl->p_new.p;

	switch (ctrl->id) {
	case V4L2_CID_MTK_CAM_PDE_INFO:
		pde_info_p->pdo_max_size = pde_cfg->pde_info.pdo_max_size;
		pde_info_p->pdi_max_size = pde_cfg->pde_info.pdi_max_size;
		pde_info_p->pd_table_offset = pde_cfg->pde_info.pd_table_offset;
		break;
	default:
		dev_info(dev, "%s(id:0x%x,val:%d) is not handled\n",
			 __func__, ctrl->id, ctrl->val);
		ret = -EINVAL;
	}

	return ret;
}

static int mtk_raw_pde_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_raw_pipeline *pipeline;
	struct mtk_raw_pde_config *pde_cfg;
	struct mtk_cam_pde_info *pde_info_p;
	struct mtk_cam_video_device *node;
	struct mtk_cam_dev_node_desc *desc;
	const struct v4l2_format *default_fmt;
	int ret = 0;
	struct device *dev = mtk_cam_root_dev();

	pipeline = mtk_cam_ctrl_handler_to_raw_pipeline(ctrl->handler);
	pde_cfg = &pipeline->pde_config;
	pde_info_p = ctrl->p_new.p;

	node = &pipeline->vdev_nodes[MTK_RAW_META_IN - MTK_RAW_SINK_NUM];
	desc = &node->desc;
	default_fmt = &desc->fmts[desc->default_fmt_idx].vfmt;

	switch (ctrl->id) {
	case V4L2_CID_MTK_CAM_PDE_INFO:
		if (!pde_info_p->pdo_max_size || !pde_info_p->pdi_max_size) {
			dev_info(dev,
				 "%s:pdo_max_sz(%d)/pdi_max_sz(%d) cannot be 0\n",
				 __func__, pde_info_p->pdo_max_size,
				 pde_info_p->pdi_max_size);
			ret = -EINVAL;
			break;
		}

		pde_cfg->pde_info.pdo_max_size = pde_info_p->pdo_max_size;
		pde_cfg->pde_info.pdi_max_size = pde_info_p->pdi_max_size;
		pde_cfg->pde_info.pd_table_offset =
			default_fmt->fmt.meta.buffersize;
		break;
	default:
		dev_info(dev, "%s(id:0x%x,val:%d) is not handled\n",
			 __func__, ctrl->id, ctrl->val);
		ret = -EINVAL;
	}

	return ret;
}

static const struct v4l2_ctrl_ops cam_pde_ctrl_ops = {
	.g_volatile_ctrl = mtk_raw_pde_get_ctrl,
	.s_ctrl = mtk_raw_pde_set_ctrl,
	.try_ctrl = mtk_raw_pde_set_ctrl,
};

static const struct v4l2_ctrl_config hsf = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_HSF_EN,
	.name = "HSF raw",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config raw_path = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_RAW_PATH_SELECT,
	.name = "Raw Path",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 7,
	.step = 1,
	.def = 1,
};

static const struct v4l2_ctrl_config frame_sync_id = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_SYNC_ID,
	.name = "Frame sync id",
	.type = V4L2_CTRL_TYPE_INTEGER64,
	.min = -1,
	.max = 0x7FFFFFFF,
	.step = 1,
	.def = -1,
};

static struct v4l2_ctrl_config cfg_pre_alloc_mem_ctrl = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_INTERNAL_MEM_CTRL,
	.name = "pre alloc memory",
	.type = V4L2_CTRL_COMPOUND_TYPES, /* V4L2_CTRL_TYPE_U32,*/
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 0xffffffff,
	.step = 1,
	.dims = {sizeof(struct mtk_cam_internal_mem)},
};

static struct v4l2_ctrl_config cfg_res_ctrl = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_RAW_RESOURCE_CALC,
	.name = "resource ctrl",
	.type = V4L2_CTRL_COMPOUND_TYPES,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 0xFFFFFFFF,
	.step = 1,
	.dims = {sizeof(struct mtk_cam_resource_v2)},
};

static struct v4l2_ctrl_config cfg_res_update = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_RAW_RESOURCE_UPDATE,
	.name = "resource update",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 0xf,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mstream_exposure = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_MSTREAM_EXPOSURE,
	.name = "mstream exposure",
	.type = V4L2_CTRL_TYPE_U32,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 0xFFFFFFFF,
	.step = 1,
	.dims = {sizeof_u32(struct mtk_cam_mstream_exposure)},
};

static const struct v4l2_ctrl_config cfg_apu_info = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_APU_INFO,
	.name = "apu information",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 0x1fffffff,
	.step = 1,
	.def = 0,
	.dims = {sizeof_u32(struct mtk_cam_apu_info)},
};

static const struct v4l2_ctrl_config mtk_cam_tg_flash_enable = {
	.ops = &cam_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_TG_FLASH_CFG,
	.name = "Mediatek camsys tg flash",
	.type = V4L2_CTRL_COMPOUND_TYPES,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 0xffffffff,
	.step = 1,
	.dims = {sizeof(struct mtk_cam_tg_flash_config)},
};

static const struct v4l2_ctrl_config cfg_pde_info = {
	.ops = &cam_pde_ctrl_ops,
	.id = V4L2_CID_MTK_CAM_PDE_INFO,
	.name = "pde information",
	.type = V4L2_CTRL_COMPOUND_TYPES,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 0,
	.max = 0x1fffffff,
	.step = 1,
	.def = 0,
	.dims = {sizeof_u32(struct mtk_cam_pde_info)},
};

struct v4l2_subdev *mtk_cam_find_sensor(struct mtk_cam_ctx *ctx,
					struct media_entity *entity)
{
	struct v4l2_subdev *sensor = NULL;
	struct media_pipeline_pad *ppad;

	mutex_lock(&ctx->cam->v4l2_dev.mdev->graph_mutex);

	list_for_each_entry(ppad, &ctx->pipeline.pads, list) {
		entity = ppad->pad->entity;
		dev_dbg(ctx->cam->dev, "linked entity: %s\n", entity->name);
		sensor = NULL;

		switch (entity->function) {
		case MEDIA_ENT_F_CAM_SENSOR:
			sensor = media_entity_to_v4l2_subdev(entity);
			break;
		default:
			break;
		}

		if (sensor)
			break;
	}

	mutex_unlock(&ctx->cam->v4l2_dev.mdev->graph_mutex);

	return sensor;
}

static int mtk_raw_sd_subscribe_event(struct v4l2_subdev *subdev,
				      struct v4l2_fh *fh,
				      struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_FRAME_SYNC:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_REQUEST_DRAINED:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_REQUEST_DUMPED:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_ESD_RECOVERY:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_REQUEST_SENSOR_TRIGGER:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	case V4L2_EVENT_ERROR:
		return v4l2_event_subscribe(fh, sub, 0, NULL);

	default:
		return -EINVAL;
	}
}

static int mtk_raw_sd_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct device *dev = sd->v4l2_dev->dev;
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);

	dev_info(dev, "%s: %d\n", __func__, enable);

	if (!enable)
		reset_internal_mem(pipe);

	return 0;
}

static int mtk_raw_init_cfg(struct v4l2_subdev *sd,
			    struct v4l2_subdev_state *state)
{
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);
	struct v4l2_mbus_framefmt *mf;
	struct v4l2_rect *crop;
	unsigned int i;

	for (i = 0; i < sd->entity.num_pads; i++) {
		mf = v4l2_subdev_get_try_format(sd, state, i);
		crop = v4l2_subdev_get_try_crop(sd, state, i);

		*mf = pipe->pad_cfg[i].mbus_fmt;
		*crop = pipe->pad_cfg[i].crop;
	}
	return 0;
}

#ifdef NOT_READY
unsigned int mtk_cam_get_rawi_sensor_pixel_fmt(unsigned int fmt)
{
	// return V4L2_PIX_FMT_MTISP for matching
	// length returned by mtk_cam_get_pixel_bits()
	// with ipi_fmt returned by mtk_cam_get_sensor_fmt()

	switch (fmt & SENSOR_FMT_MASK) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		return V4L2_PIX_FMT_SBGGR8;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		return V4L2_PIX_FMT_SGBRG8;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		return V4L2_PIX_FMT_SGRBG8;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		return V4L2_PIX_FMT_SRGGB8;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		return V4L2_PIX_FMT_MTISP_SBGGR10;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		return V4L2_PIX_FMT_MTISP_SGBRG10;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		return V4L2_PIX_FMT_MTISP_SGRBG10;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		return V4L2_PIX_FMT_MTISP_SGRBG10;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		return V4L2_PIX_FMT_MTISP_SBGGR12;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		return V4L2_PIX_FMT_MTISP_SGBRG12;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		return V4L2_PIX_FMT_MTISP_SGRBG12;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		return V4L2_PIX_FMT_MTISP_SRGGB12;
	case MEDIA_BUS_FMT_SBGGR14_1X14:
		return V4L2_PIX_FMT_MTISP_SBGGR14;
	case MEDIA_BUS_FMT_SGBRG14_1X14:
		return V4L2_PIX_FMT_MTISP_SGBRG14;
	case MEDIA_BUS_FMT_SGRBG14_1X14:
		return V4L2_PIX_FMT_MTISP_SGRBG14;
	case MEDIA_BUS_FMT_SRGGB14_1X14:
		return V4L2_PIX_FMT_MTISP_SRGGB14;
	default:
		break;
	}
	return V4L2_PIX_FMT_MTISP_SBGGR14;
}
#endif

static struct v4l2_mbus_framefmt *
raw_try_fmt_getter(struct v4l2_subdev *sd,
		   struct v4l2_subdev_state *state,
		   unsigned int pad)
{
	return v4l2_subdev_get_try_format(sd, state, pad);
}

static struct v4l2_rect *
raw_try_crop_getter(struct v4l2_subdev *sd,
		    struct v4l2_subdev_state *state,
		    unsigned int pad)
{
	return v4l2_subdev_get_try_crop(sd, state, pad);
}

static struct v4l2_mbus_framefmt *
raw_active_fmt_getter(struct v4l2_subdev *sd,
		      struct v4l2_subdev_state *state,
		      unsigned int pad)
{
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);

	if (WARN_ON(!raw_is_valid_pad(pad)))
		pad = 0;
	return &pipe->pad_cfg[pad].mbus_fmt;
}

static struct v4l2_rect *
raw_active_crop_getter(struct v4l2_subdev *sd,
		       struct v4l2_subdev_state *state,
		       unsigned int pad)
{
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);

	if (WARN_ON(!raw_is_valid_pad(pad)))
		pad = 0;
	return &pipe->pad_cfg[pad].crop;
}

static bool mbus_framefmt_eq(const struct v4l2_mbus_framefmt *a,
			     const struct v4l2_mbus_framefmt *b)
{
	return a->width == b->width &&
		a->height == b->height &&
		a->code == b->code;
}

static void disable_other_sink_fmt(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *state,
				   struct mbus_config_getter *config_getter,
				   unsigned int pad)
{
	struct v4l2_mbus_framefmt *fmt;
	unsigned int i;

	for (i = MTK_RAW_SINK; i < MTK_RAW_META_OUT_BEGIN; i++) {
		if (i == MTK_RAW_META_IN || i == pad)
			continue;

		fmt = config_getter->get_fmt(sd, state, i);
		mtk_cam_pad_fmt_enable(fmt, false);
	}
}

static struct v4l2_rect *
get_enabled_sink_crop(struct v4l2_subdev *sd,
		      struct v4l2_subdev_state *state,
		      struct mbus_config_getter *config_getter)
{
	struct v4l2_mbus_framefmt *fmt;
	unsigned int i;

	for (i = MTK_RAW_SINK; i < MTK_RAW_META_OUT_BEGIN; i++) {
		if (i == MTK_RAW_META_IN)
			continue;

		fmt = config_getter->get_fmt(sd, state, i);
		if (mtk_cam_is_pad_fmt_enable(fmt))
			return config_getter->get_crop(sd, state, i);
	}

	return NULL;
}

/* note: ret > 0 if changed */
static int set_sink_pad_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *dst_fmt,
			    struct v4l2_mbus_framefmt *fmt)
{
	struct device *dev = subdev_to_cam_dev(sd);
	int changed = 0;

	/* remove redundant mtk's sensor mode info? */
	// fmt->code = fmt->code & SENSOR_FMT_MASK;

	/* test if pattern is supported */
	if (MTKCAM_IPI_IMG_FMT_UNKNOWN ==
	    sensor_mbus_to_ipi_fmt(fmt->code)) {

		dev_info(dev, "%s: warning: %s: unsupported code: 0x%x\n",
			 __func__, sd->name, fmt->code);

		fmt->code = MEDIA_BUS_FMT_SBGGR10_1X10;
	}

	if (!mbus_framefmt_eq(dst_fmt, fmt)) {

		if (CAM_DEBUG_ENABLED(V4L2))
			dev_info(dev, "%s: %ux%u(0x%x)->%ux%u(0x%x)\n",
				 __func__,
				 dst_fmt->width, dst_fmt->height, dst_fmt->code,
				 fmt->width, fmt->height, fmt->code);

		*dst_fmt = *fmt;
		changed = 1;
	}

	mtk_cam_pad_fmt_enable(dst_fmt, true);
	return changed;
}

static int set_src_pad_fmt(struct mtk_cam_video_device *node,
			   struct v4l2_mbus_framefmt *fmt,
			   struct v4l2_rect *crop,
			   unsigned int sink_w, unsigned int sink_h)
{
	struct mtk_cam_dev_node_desc *desc = &node->desc;
	unsigned int w, h;

	/* note: currently, meta has no frmsizes */
	w = min(sink_w,
		desc->frmsizes ? desc->frmsizes->stepwise.max_width : UINT_MAX);
	h = min(sink_h,
		desc->frmsizes ? desc->frmsizes->stepwise.max_height : UINT_MAX);

	fmt->width = w;
	fmt->height = h;

	*crop = fullsize_as_crop(w, h);

	if (CAM_DEBUG_ENABLED(V4L2))
		pr_info("%s: %s: %ux%u\n", __func__, desc->name, w, h);
	return 0;
}

static int reset_all_src_pad(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *state,
			     struct mbus_config_getter *config_getter,
			     const struct v4l2_rect *sink_crop)
{
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);
	struct mtk_cam_video_device *node;
	struct v4l2_mbus_framefmt *fmt;
	struct v4l2_rect *crop;
	unsigned int i;

	for (i = 0; i < sd->entity.num_pads; i++) {

		if (raw_is_sink_pad(i))
			continue;

		fmt = config_getter->get_fmt(sd, state, i);
		crop = config_getter->get_crop(sd, state, i);
		node = mtk_raw_get_node(pipe, i);

		if (!fmt || !crop || !node)
			continue;

		set_src_pad_fmt(node, fmt, crop,
				sink_crop->width, sink_crop->height);
	}
	return 0;
}

static int mtk_raw_set_sink_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct mbus_config_getter *config_getter,
				struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *mfmt;
	struct v4l2_rect *crop;
	int ret;
	int reset = fmt->pad != MTK_RAW_META_IN;

	mfmt = config_getter->get_fmt(sd, state, fmt->pad);
	crop = config_getter->get_crop(sd, state, fmt->pad);
	if (!mfmt || !crop) {
		pr_info("%s: error: pad %d\n", __func__, fmt->pad);
		return -EINVAL;
	}
	/* disable all sink fmt's enable flag */
	/* set MTK_RAW_META_IN sink pad will disable flags of MTK_RAW_SINK */
	if (reset)
		disable_other_sink_fmt(sd, state, config_getter, fmt->pad);

	ret = set_sink_pad_fmt(sd, mfmt, &fmt->format);
	if (ret > 0 && reset) { /* if sink changed */

		/* reset crop */
		*crop = fullsize_as_crop(mfmt->width, mfmt->height);
		reset_all_src_pad(sd, state, config_getter, crop);
		ret = 0;
	}

	return 0;
}

static int mtk_raw_set_src_fmt(struct v4l2_subdev *sd,
			       struct v4l2_subdev_state *state,
			       struct mbus_config_getter *config_getter,
			       struct v4l2_subdev_format *fmt)
{
	struct mtk_raw_pipeline *pipe =
		container_of(sd, struct mtk_raw_pipeline, subdev);
	struct mtk_cam_video_device *node;
	struct v4l2_mbus_framefmt *mfmt;
	struct v4l2_rect *crop;
	struct v4l2_rect *sink_crop;

	mfmt = config_getter->get_fmt(sd, state, fmt->pad);
	crop = config_getter->get_crop(sd, state, fmt->pad);
	if (!mfmt || !crop) {
		pr_info("%s: error: pad %d\n", __func__, fmt->pad);
		return -EINVAL;
	}

	node = mtk_raw_get_node(pipe, fmt->pad);
	if (!node)
		return -EINVAL;

	/* find enabled sink pad */
	sink_crop = get_enabled_sink_crop(sd, state, config_getter);
	if (!sink_crop) {
		pr_info("%s: error. no sink fmt is set already.\n",
			__func__);
		return -EINVAL;
	}

	/* set fmt with constraint */
	set_src_pad_fmt(node, mfmt, crop, sink_crop->width, sink_crop->height);
	return 0;
}

static struct mbus_config_getter *fetch_mbus_config_getter(unsigned int which)
{
	static struct mbus_config_getter try_config_getter = {
		.get_fmt = raw_try_fmt_getter,
		.get_crop = raw_try_crop_getter,
	};
	static struct mbus_config_getter active_config_getter = {
		.get_fmt = raw_active_fmt_getter,
		.get_crop = raw_active_crop_getter,
	};
	struct mbus_config_getter *config_getter = NULL;

	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		config_getter = &try_config_getter;
		break;
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		config_getter = &active_config_getter;
		break;
	default:
		pr_info("%s: invalid which(%u)\n", __func__, which);
		break;
	}

	return config_getter;
}

static void fill_fixed_mfmt_values(struct v4l2_mbus_framefmt *fmt)
{
	/* fixed values */
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	fmt->quantization = V4L2_QUANTIZATION_DEFAULT;
	fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	fmt->flags = 0;
}

static inline void log_raw_subdev_format(struct v4l2_subdev *sd,
					 struct v4l2_subdev_format *fmt,
					 const char *caller)
{
	struct device *dev = subdev_to_cam_dev(sd);

	dev_info(dev, "%s: sd(%s) active %d, pad %u(%s) fmt %ux%u code %u\n",
		 caller, sd->name,
		 fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE,
		 fmt->pad, raw_get_pad_name(fmt->pad),
		 fmt->format.width, fmt->format.height, fmt->format.code);
}

static inline void log_raw_subdev_selection(struct v4l2_subdev *sd,
					    struct v4l2_subdev_selection *sel,
					    const char *caller)
{
	struct device *dev = subdev_to_cam_dev(sd);

	dev_info(dev, "%s: sd(%s) active %d pad %u(%s) sel (%d,%d %ux%u)\n",
		 caller, sd->name,
		 sel->which == V4L2_SUBDEV_FORMAT_ACTIVE,
		 sel->pad, raw_get_pad_name(sel->pad),
		 sel->r.left, sel->r.top, sel->r.width, sel->r.height);
}

static int mtk_raw_set_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_state *state,
			   struct v4l2_subdev_format *fmt)
{
	struct device *dev = subdev_to_cam_dev(sd);
	struct mbus_config_getter *config_getter;
	int ret;

	if (CAM_DEBUG_ENABLED(V4L2_TRY) ||
	    fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		log_raw_subdev_format(sd, fmt, __func__);

	config_getter = fetch_mbus_config_getter(fmt->which);
	if (!config_getter)
		return -EINVAL;

	fill_fixed_mfmt_values(&fmt->format);

	ret = raw_is_sink_pad(fmt->pad) ?
		mtk_raw_set_sink_fmt(sd, state, config_getter, fmt) :
		mtk_raw_set_src_fmt(sd, state, config_getter, fmt);

	if (ret)
		dev_info(dev, "%s: failed ret = %d\n", __func__, ret);

	return ret;
}

static int mtk_raw_get_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct mbus_config_getter *config_getter;
	struct v4l2_mbus_framefmt *mf;

	config_getter = fetch_mbus_config_getter(fmt->which);
	if (!config_getter)
		return -EINVAL;

	mf = config_getter->get_fmt(sd, state, fmt->pad);
	fmt->format = *mf;
	fmt->format.flags = 0; /* to remove internal enabled flag */

	if (CAM_DEBUG_ENABLED(V4L2))
		log_raw_subdev_format(sd, fmt, __func__);

	return 0;
}

static int adjust_crop(struct v4l2_rect *r, const struct v4l2_rect *sink)
{
	bool invalid =
		(r->left + r->width > sink->width) ||
		(r->top + r->height > sink->height) ||
		r->left < 0 ||
		r->top < 0;

	if (invalid) {
		pr_info("warn: crop(%d,%d,%ux%u)->(0,0,%u,%u)\n",
			r->left, r->top, r->width, r->height,
			sink->width, sink->height);

		*r = fullsize_as_crop(sink->width, sink->height);
	}
	return invalid ? -1 : 0;
}

static int mtk_raw_set_pad_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_state *state,
					  struct v4l2_subdev_selection *sel)
{
	struct mbus_config_getter *config_getter;
	struct v4l2_rect *crop;
	struct v4l2_rect *sink_crop;

	if (CAM_DEBUG_ENABLED(V4L2))
		log_raw_subdev_selection(sd, sel, __func__);

	config_getter = fetch_mbus_config_getter(sel->which);
	if (!config_getter)
		return -EINVAL;

	if (raw_is_sink_pad(sel->pad)) {
		pr_info("%s: error: not support sink corp yet\n", __func__);
		return -EINVAL;
	}

	sink_crop = get_enabled_sink_crop(sd, state, config_getter);
	if (!sink_crop) {
		pr_info("%s: error: no sink fmt is set already.\n",
			__func__);
		return -EINVAL;
	}

	adjust_crop(&sel->r, sink_crop);

	crop = config_getter->get_crop(sd, state, sel->pad);
	*crop = sel->r;

	return 0;
}

static int mtk_raw_get_pad_selection(struct v4l2_subdev *sd,
					  struct v4l2_subdev_state *state,
					  struct v4l2_subdev_selection *sel)
{
	struct mbus_config_getter *config_getter;
	struct v4l2_rect *crop;

	config_getter = fetch_mbus_config_getter(sel->which);
	if (!config_getter)
		return -EINVAL;

	crop = config_getter->get_crop(sd, state, sel->pad);
	sel->r = *crop;

	if (CAM_DEBUG_ENABLED(V4L2))
		log_raw_subdev_selection(sd, sel, __func__);

	return 0;
}

static int mtk_cam_media_link_setup(struct media_entity *entity,
				    const struct media_pad *local,
				    const struct media_pad *remote, u32 flags)
{
	struct mtk_raw_pipeline *pipe =
		container_of(entity, struct mtk_raw_pipeline, subdev.entity);
	struct device *dev = pipe->subdev.v4l2_dev->dev;

	if (CAM_DEBUG_ENABLED(V4L2))
		dev_info(dev, "%s: raw %d: remote:%s:%d->local:%s:%d flags:0x%x\n",
			 __func__, pipe->id,
			 remote->entity->name, remote->index,
			 local->entity->name, local->index, flags);

	return 0;
}

static int
mtk_raw_s_frame_interval(struct v4l2_subdev *sd,
			 struct v4l2_subdev_frame_interval *interval)
{
	return 0;
}

static const struct v4l2_subdev_core_ops mtk_raw_subdev_core_ops = {
	.subscribe_event = mtk_raw_sd_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops mtk_raw_subdev_video_ops = {
	.s_stream =  mtk_raw_sd_s_stream,
	.s_frame_interval = mtk_raw_s_frame_interval,
};

static const struct v4l2_subdev_pad_ops mtk_raw_subdev_pad_ops = {
	.link_validate = mtk_cam_link_validate,
	.init_cfg = mtk_raw_init_cfg,
	.set_fmt = mtk_raw_set_fmt,
	.get_fmt = mtk_raw_get_fmt,
	.set_selection = mtk_raw_set_pad_selection,
	.get_selection = mtk_raw_get_pad_selection,
};

static const struct v4l2_subdev_ops mtk_raw_subdev_ops = {
	.core = &mtk_raw_subdev_core_ops,
	.video = &mtk_raw_subdev_video_ops,
	.pad = &mtk_raw_subdev_pad_ops,
};

static const struct media_entity_operations mtk_cam_media_entity_ops = {
	.link_setup = mtk_cam_media_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_ioctl_ops mtk_cam_v4l2_vout_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_framesizes = mtk_cam_vidioc_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = mtk_cam_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_out_mplane = mtk_cam_vidioc_g_fmt,
	.vidioc_s_fmt_vid_out_mplane = mtk_cam_vidioc_s_fmt,
	.vidioc_try_fmt_vid_out_mplane = mtk_cam_vidioc_try_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_ioctl_ops mtk_cam_v4l2_vcap_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_framesizes = mtk_cam_vidioc_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = mtk_cam_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_cap_mplane = mtk_cam_vidioc_g_fmt,
	.vidioc_s_fmt_vid_cap_mplane = mtk_cam_vidioc_s_fmt,
	.vidioc_try_fmt_vid_cap_mplane = mtk_cam_vidioc_try_fmt,
	.vidioc_s_selection = mtk_cam_vidioc_s_selection,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_ioctl_ops mtk_cam_v4l2_meta_cap_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_fmt_meta_cap = mtk_cam_vidioc_meta_enum_fmt,
	.vidioc_g_fmt_meta_cap = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_s_fmt_meta_cap = mtk_cam_vidioc_s_meta_fmt,
	.vidioc_try_fmt_meta_cap = mtk_cam_vidioc_try_meta_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
};

static const struct v4l2_ioctl_ops mtk_cam_v4l2_meta_out_ioctl_ops = {
	.vidioc_querycap = mtk_cam_vidioc_querycap,
	.vidioc_enum_fmt_meta_out = mtk_cam_vidioc_meta_enum_fmt,
	.vidioc_g_fmt_meta_out = mtk_cam_vidioc_g_meta_fmt,
	.vidioc_s_fmt_meta_out = mtk_cam_vidioc_s_meta_fmt,
	.vidioc_try_fmt_meta_out = mtk_cam_vidioc_try_meta_fmt,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = mtk_cam_vidioc_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_expbuf = vb2_ioctl_expbuf,
};

static struct mtk_cam_format_desc meta_cfg_fmts[] = {
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_PARAMS,
			.buffersize = 0,
		},
	},
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_PARAMS_RGBW,
			.buffersize = 0,
		},
	},
};

static struct mtk_cam_format_desc meta_stats0_fmts[] = {
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_3A,
			.buffersize = 0,
		},
	},
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_3A_RGBW,
			.buffersize = 0,
		},
	},
};

static struct mtk_cam_format_desc meta_stats1_fmts[] = {
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_AF,
			.buffersize = 0,
		},
	},
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_AF_RGBW,
			.buffersize = 0,
		},
	},
};

static struct mtk_cam_format_desc meta_ext_fmts[] = {
	{
		.vfmt.fmt.meta = {
			.dataformat = V4L2_META_FMT_MTISP_3A,
			.buffersize = 0,
		},
	},
};

static struct mtk_cam_format_desc stream_out_fmts[] = {
	/* This is a default image format */
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR14,
			 .num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG10,
			.num_planes = 1,
		},

	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG12,
			.num_planes = 1,
		},

	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG12,
			.num_planes = 1,
		},

	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB8,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB10,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB10P,
		},
	},

	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER8_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER10_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER12_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_BAYER14_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_YUYV,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_YVYU,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_UYVY,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = IMG_MAX_WIDTH,
			.height = IMG_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_VYUY,
		},
	},
};

static const struct mtk_cam_format_desc yuv_out_group1_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_12P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_12P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_YUV420,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_10_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_10_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_12_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_12_UFBC,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRB8F,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRB10F,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP1_MAX_WIDTH,
			.height = YUV_GROUP1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRB12F,
		},
	}
};

static const struct mtk_cam_format_desc yuv_out_group2_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_12P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = YUV_GROUP2_MAX_WIDTH,
			.height = YUV_GROUP2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_12P,
		},
	}
};

static const struct mtk_cam_format_desc rzh1n2to1_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO1_MAX_WIDTH,
			.height = RZH1N2TO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO1_MAX_WIDTH,
			.height = RZH1N2TO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO1_MAX_WIDTH,
			.height = RZH1N2TO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV16,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO1_MAX_WIDTH,
			.height = RZH1N2TO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV61,
		},
	}
};

static const struct mtk_cam_format_desc rzh1n2to2_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO2_MAX_WIDTH,
			.height = RZH1N2TO2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_YUYV,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO2_MAX_WIDTH,
			.height = RZH1N2TO2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_YVYU,
		},
	}
};

static const struct mtk_cam_format_desc rzh1n2to3_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO3_MAX_WIDTH,
			.height = RZH1N2TO3_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO3_MAX_WIDTH,
			.height = RZH1N2TO3_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO3_MAX_WIDTH,
			.height = RZH1N2TO3_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV16,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = RZH1N2TO3_MAX_WIDTH,
			.height = RZH1N2TO3_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV61,
		},
	}
};

static const struct mtk_cam_format_desc drzs4no1_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_GREY,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV16,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV61,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV16_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV61_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV16_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV61_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV12_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_NV21_10,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV12_10P,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO1_MAX_WIDTH,
			.height = DRZS4NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_NV21_10P,
		},
	},
};

static const struct mtk_cam_format_desc drzs4no2_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO2_MAX_WIDTH,
			.height = DRZS4NO2_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_GREY,
		},
	}
};

static const struct mtk_cam_format_desc drzs4no3_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZS4NO3_MAX_WIDTH,
			.height = DRZS4NO3_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_GREY,
		},
	}
};

static const struct mtk_cam_format_desc drzb2no1_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG12,
			.num_planes = 1,
		},

	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR14,
			 .num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SBGGR16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGBRG16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SGRBG16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_SRGGB16,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG12,
			.num_planes = 1,
		},

	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB12,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SBGGR14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGBRG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SGRBG14,
			.num_planes = 1,
		},
	},
	{
		.vfmt.fmt.pix_mp = {
			.width = DRZB2NO1_MAX_WIDTH,
			.height = DRZB2NO1_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_MTISP_SRGGB14,
			.num_planes = 1,
		},
	},
};

static const struct mtk_cam_format_desc ipu_out_fmts[] = {
	{
		.vfmt.fmt.pix_mp = {
			.width = IPUO_MAX_WIDTH,
			.height = IPUO_MAX_HEIGHT,
			.pixelformat = V4L2_PIX_FMT_GREY,
		},
	}
};

#define MTK_RAW_TOTAL_OUTPUT_QUEUES 2

static const struct
mtk_cam_dev_node_desc output_queues[] = {
	{
		.id = MTK_RAW_META_IN,
		.name = "meta input",
		.cap = V4L2_CAP_META_OUTPUT,
		.buf_type = V4L2_BUF_TYPE_META_OUTPUT,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
#ifdef CONFIG_MTK_SCP
		.smem_alloc = true,
#else
		.smem_alloc = false,
#endif
		.dma_port = MTKCAM_IPI_RAW_META_STATS_CFG,
		.fmts = meta_cfg_fmts,
		.num_fmts = ARRAY_SIZE(meta_cfg_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_out_ioctl_ops,
	},
	{
		.id = MTK_RAW_RAWI_2_IN,
		.name = "rawi 2",
		.cap = V4L2_CAP_VIDEO_OUTPUT_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_RAWI_2,
		.fmts = stream_out_fmts,
		.num_fmts = ARRAY_SIZE(stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vout_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = IMG_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = IMG_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
};

static const char *output_queue_names[RAW_PIPELINE_NUM][MTK_RAW_TOTAL_OUTPUT_QUEUES] = {
	{"mtk-cam raw-0 meta-input", "mtk-cam raw-0 rawi-2"},
	{"mtk-cam raw-1 meta-input", "mtk-cam raw-1 rawi-2"},
	{"mtk-cam raw-2 meta-input", "mtk-cam raw-2 rawi-2"},
};

#ifndef PREISP
#define MTK_RAW_TOTAL_CAPTURE_QUEUES 15 //todo :check backend node size
#else
#define MTK_RAW_TOTAL_CAPTURE_QUEUES 20 //todo :check backend node size
#endif
static const struct
mtk_cam_dev_node_desc capture_queues[] = {
	{
		.id = MTK_RAW_MAIN_STREAM_OUT,
		.name = "imgo",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_IMGO,
		.fmts = stream_out_fmts,
		.num_fmts = ARRAY_SIZE(stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = IMG_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = IMG_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_YUVO_1_OUT,
		.name = "yuvo 1",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_YUVO_1,
		.fmts = yuv_out_group1_fmts,
		.num_fmts = ARRAY_SIZE(yuv_out_group1_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = YUV_GROUP1_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = YUV_GROUP1_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_YUVO_2_OUT,
		.name = "yuvo 2",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_YUVO_2,
		.fmts = yuv_out_group2_fmts,
		.num_fmts = ARRAY_SIZE(yuv_out_group2_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = YUV_GROUP2_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = YUV_GROUP2_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_YUVO_3_OUT,
		.name = "yuvo 3",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_YUVO_3,
		.fmts = yuv_out_group1_fmts,
		.num_fmts = ARRAY_SIZE(yuv_out_group1_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = YUV_GROUP1_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = YUV_GROUP1_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_YUVO_4_OUT,
		.name = "yuvo 4",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_YUVO_4,
		.fmts = yuv_out_group2_fmts,
		.num_fmts = ARRAY_SIZE(yuv_out_group2_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = YUV_GROUP2_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = YUV_GROUP2_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_YUVO_5_OUT,
		.name = "yuvo 5",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_YUVO_5,
		.fmts = yuv_out_group2_fmts,
		.num_fmts = ARRAY_SIZE(yuv_out_group2_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = YUV_GROUP2_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = YUV_GROUP2_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_DRZS4NO_1_OUT,
		.name = "drzs4no 1",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_DRZS4NO_1,
		.fmts = drzs4no1_out_fmts,
		.num_fmts = ARRAY_SIZE(drzs4no1_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = DRZS4NO1_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = DRZS4NO1_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_DRZS4NO_3_OUT,
		.name = "drzs4no 3",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_DRZS4NO_3,
		.fmts = drzs4no3_out_fmts,
		.num_fmts = ARRAY_SIZE(drzs4no3_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = DRZS4NO3_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = DRZS4NO3_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_RZH1N2TO_1_OUT,
		.name = "rzh1n2to 1",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_RZH1N2TO_1,
		.fmts = rzh1n2to1_out_fmts,
		.num_fmts = ARRAY_SIZE(rzh1n2to1_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = RZH1N2TO1_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = RZH1N2TO1_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_RZH1N2TO_2_OUT,
		.name = "rzh1n2to 2",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_RZH1N2TO_2,
		.fmts = rzh1n2to2_out_fmts,
		.num_fmts = ARRAY_SIZE(rzh1n2to2_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = RZH1N2TO2_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = RZH1N2TO2_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_RZH1N2TO_3_OUT,
		.name = "rzh1n2to 3",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_RZH1N2TO_3,
		.fmts = rzh1n2to3_out_fmts,
		.num_fmts = ARRAY_SIZE(rzh1n2to3_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = RZH1N2TO3_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = RZH1N2TO3_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_DRZB2NO_1_OUT,
		.name = "drzb2no 1",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_DRZB2NO_1,
		.fmts = drzb2no1_out_fmts,
		.num_fmts = ARRAY_SIZE(drzb2no1_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = DRZB2NO1_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = DRZB2NO1_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_IPU_OUT,
		.name = "ipuo",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_IPUO,
		.fmts = ipu_out_fmts,
		.num_fmts = ARRAY_SIZE(ipu_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = IPUO_MAX_WIDTH,
				.min_width = IMG_MIN_HEIGHT,
				.max_height = IPUO_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_MAIN_STREAM_SV_1_OUT,
		.name = "sv imgo 1",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = stream_out_fmts,
		.num_fmts = ARRAY_SIZE(stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = IMG_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = IMG_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_MAIN_STREAM_SV_2_OUT,
		.name = "sv imgo 2",
		.cap = V4L2_CAP_VIDEO_CAPTURE_MPLANE,
		.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = true,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = stream_out_fmts,
		.num_fmts = ARRAY_SIZE(stream_out_fmts),
		.default_fmt_idx = 0,
		.ioctl_ops = &mtk_cam_v4l2_vcap_ioctl_ops,
		.frmsizes = &(struct v4l2_frmsizeenum) {
			.index = 0,
			.type = V4L2_FRMSIZE_TYPE_CONTINUOUS,
			.stepwise = {
				.max_width = IMG_MAX_WIDTH,
				.min_width = IMG_MIN_WIDTH,
				.max_height = IMG_MAX_HEIGHT,
				.min_height = IMG_MIN_HEIGHT,
				.step_height = 1,
				.step_width = 1,
			},
		},
	},
	{
		.id = MTK_RAW_META_OUT_0,
		.name = "partial meta 0",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_META_STATS_0,
		.fmts = meta_stats0_fmts,
		.num_fmts = ARRAY_SIZE(meta_stats0_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_cap_ioctl_ops,
	},
	{
		.id = MTK_RAW_META_OUT_1,
		.name = "partial meta 1",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_RAW_META_STATS_1,
		.fmts = meta_stats1_fmts,
		.num_fmts = ARRAY_SIZE(meta_stats1_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_cap_ioctl_ops,
	},
	{
		.id = MTK_RAW_META_SV_OUT_0,
		.name = "external meta 0",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = meta_ext_fmts,
		.num_fmts = ARRAY_SIZE(meta_ext_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_cap_ioctl_ops,
	},
	{
		.id = MTK_RAW_META_SV_OUT_1,
		.name = "external meta 1",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = meta_ext_fmts,
		.num_fmts = ARRAY_SIZE(meta_ext_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_cap_ioctl_ops,
	},
	{
		.id = MTK_RAW_META_SV_OUT_2,
		.name = "external meta 2",
		.cap = V4L2_CAP_META_CAPTURE,
		.buf_type = V4L2_BUF_TYPE_META_CAPTURE,
		.link_flags = MEDIA_LNK_FL_ENABLED |  MEDIA_LNK_FL_IMMUTABLE,
		.image = false,
		.smem_alloc = false,
		.dma_port = MTKCAM_IPI_CAMSV_MAIN_OUT,
		.fmts = meta_ext_fmts,
		.num_fmts = ARRAY_SIZE(meta_ext_fmts),
		.default_fmt_idx = 0,
		.max_buf_count = 16,
		.ioctl_ops = &mtk_cam_v4l2_meta_cap_ioctl_ops,
	},
};

static const char *capture_queue_names[RAW_PIPELINE_NUM][MTK_RAW_TOTAL_CAPTURE_QUEUES] = {
	{"mtk-cam raw-0 main-stream",
	 "mtk-cam raw-0 yuvo-1", "mtk-cam raw-0 yuvo-2",
	 "mtk-cam raw-0 yuvo-3", "mtk-cam raw-0 yuvo-4",
	 "mtk-cam raw-0 yuvo-5",
	 "mtk-cam raw-0 drzs4no-1", "mtk-cam raw-0 drzs4no-3",
	 "mtk-cam raw-0 rzh1n2to-1", "mtk-cam raw-0 rzh1n2to-2", "mtk-cam raw-0 rzh1n2to-3",
	 "mtk-cam raw-0 drzb2no-1",
	 "mtk-cam raw-0 ipuo",
	 "mtk-cam raw-0 sv-imgo-1", "mtk-cam raw-0 sv-imgo-2",
	 "mtk-cam raw-0 partial-meta-0", "mtk-cam raw-0 partial-meta-1",
	 "mtk-cam raw-0 ext-meta-0", "mtk-cam raw-0 ext-meta-1",
	 "mtk-cam raw-0 ext-meta-2"},

	{"mtk-cam raw-1 main-stream",
	 "mtk-cam raw-1 yuvo-1", "mtk-cam raw-1 yuvo-2",
	 "mtk-cam raw-1 yuvo-3", "mtk-cam raw-1 yuvo-4",
	 "mtk-cam raw-1 yuvo-5",
	 "mtk-cam raw-1 drzs4no-1", "mtk-cam raw-1 drzs4no-3",
	 "mtk-cam raw-1 rzh1n2to-1", "mtk-cam raw-1 rzh1n2to-2", "mtk-cam raw-1 rzh1n2to-3",
	 "mtk-cam raw-1 drzb2no-1",
	 "mtk-cam raw-1 ipuo",
	 "mtk-cam raw-1 sv-imgo-1", "mtk-cam raw-1 sv-imgo-2",
	 "mtk-cam raw-1 partial-meta-0", "mtk-cam raw-1 partial-meta-1",
	 "mtk-cam raw-1 ext-meta-0", "mtk-cam raw-1 ext-meta-1",
	 "mtk-cam raw-1 ext-meta-2"},

	{"mtk-cam raw-2 main-stream",
	 "mtk-cam raw-2 yuvo-1", "mtk-cam raw-2 yuvo-2",
	 "mtk-cam raw-2 yuvo-3", "mtk-cam raw-2 yuvo-4",
	 "mtk-cam raw-2 yuvo-5",
	 "mtk-cam raw-2 drzs4no-1", "mtk-cam raw-2 drzs4no-3",
	 "mtk-cam raw-2 rzh1n2to-1", "mtk-cam raw-2 rzh1n2to-2", "mtk-cam raw-2 rzh1n2to-3",
	 "mtk-cam raw-2 drzb2no-1",
	 "mtk-cam raw-2 ipuo",
	 "mtk-cam raw-2 sv-imgo-1", "mtk-cam raw-2 sv-imgo-2",
	 "mtk-cam raw-2 partial-meta-0", "mtk-cam raw-2 partial-meta-1",
	 "mtk-cam raw-2 ext-meta-0", "mtk-cam raw-2 ext-meta-1",
	 "mtk-cam raw-2 ext-meta-2"},
};

static void update_platform_meta_size(struct mtk_cam_format_desc *fmts,
				      int fmt_num, int sv)
{
	int i, size;

	for (i = 0; i < fmt_num; i++) {

		switch (fmts[i].vfmt.fmt.meta.dataformat) {
		case  V4L2_META_FMT_MTISP_PARAMS:
			size = GET_PLAT_V4L2(meta_cfg_size);
			break;
		case  V4L2_META_FMT_MTISP_PARAMS_RGBW:
			size = GET_PLAT_V4L2(meta_cfg_size_rgbw);
			break;
		case  V4L2_META_FMT_MTISP_3A:
			/* workaround */
			size = !sv ? GET_PLAT_V4L2(meta_stats0_size) :
				GET_PLAT_V4L2(meta_sv_ext_size);
			break;
		case  V4L2_META_FMT_MTISP_3A_RGBW:
			/* workaround */
			size = !sv ? GET_PLAT_V4L2(meta_stats0_size_rgbw) :
				GET_PLAT_V4L2(meta_sv_ext_size);
			break;
		case  V4L2_META_FMT_MTISP_AF:
			size = GET_PLAT_V4L2(meta_stats1_size);
			break;
		case  V4L2_META_FMT_MTISP_AF_RGBW:
			size = GET_PLAT_V4L2(meta_stats1_size_rgbw);
			break;
		default:
			size = 0;
			break;
		}

		if (size)
			fmts[i].vfmt.fmt.meta.buffersize = size;
	}
}

/* The helper to configure the device context */
static void mtk_raw_pipeline_queue_setup(struct mtk_raw_pipeline *pipe)
{
	struct mtk_cam_video_device *vdev;
	unsigned int node_idx, i;

	/* TODO: platform-based */
	if (WARN_ON(MTK_RAW_TOTAL_OUTPUT_QUEUES + MTK_RAW_TOTAL_CAPTURE_QUEUES
	    != MTK_RAW_TOTAL_NODES))
		return;

	node_idx = 0;

	/* update platform specific */
	update_platform_meta_size(meta_cfg_fmts,
				  ARRAY_SIZE(meta_cfg_fmts), 0);
	update_platform_meta_size(meta_stats0_fmts,
				  ARRAY_SIZE(meta_stats0_fmts), 0);
	update_platform_meta_size(meta_stats1_fmts,
				  ARRAY_SIZE(meta_stats1_fmts), 0);
	//update_platform_meta_size(meta_stats2_fmts,
	//			  ARRAY_SIZE(meta_stats2_fmts), 0);
	update_platform_meta_size(meta_ext_fmts,
				  ARRAY_SIZE(meta_ext_fmts), 1);

	/* Setup the output queue */
	for (i = 0; i < MTK_RAW_TOTAL_OUTPUT_QUEUES; i++) {
		vdev = &pipe->vdev_nodes[node_idx];

		vdev->desc = output_queues[i];
		vdev->desc.name = output_queue_names[pipe->id][i];

		++node_idx;
	}

	/* Setup the capture queue */
	for (i = 0; i < MTK_RAW_TOTAL_CAPTURE_QUEUES; i++) {
		vdev = &pipe->vdev_nodes[node_idx];

		vdev->desc = capture_queues[i];
		vdev->desc.name = capture_queue_names[pipe->id][i];

		++node_idx;
	}
}

static void mtk_raw_pipeline_ctrl_setup(struct mtk_raw_pipeline *pipe)
{
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	ctrl_hdlr = &pipe->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 8);
	if (ret) {
		pr_info("%s: v4l2_ctrl_handler init failed\n", __func__);
		return;
	}
	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &hsf, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &frame_sync_id, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE |
			       V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &raw_path, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE |
			       V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	/* TODO: remove pde later */
	// PDE
	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &cfg_pde_info, NULL);

	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &cfg_res_update, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &cfg_res_ctrl, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &cfg_pre_alloc_mem_ctrl, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;
	/* TG flash ctrls */
	ctrl = v4l2_ctrl_new_custom(ctrl_hdlr, &mtk_cam_tg_flash_enable, NULL);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_EXECUTE_ON_WRITE;

	v4l2_ctrl_new_custom(ctrl_hdlr, &mstream_exposure, NULL);

	/* APU */
	v4l2_ctrl_new_custom(ctrl_hdlr, &cfg_apu_info, NULL);

	pipe->subdev.ctrl_handler = ctrl_hdlr;

	/* TODO: properly set default values */
	memset(&pipe->ctrl_data, 0, sizeof(pipe->ctrl_data));
	memset(&pipe->pde_config, 0, sizeof(pipe->pde_config));
}

static int mtk_raw_pipeline_register(const char *str, unsigned int id,
				     struct mtk_raw_pipeline *pipe,
				     struct v4l2_device *v4l2_dev)
{
	struct v4l2_subdev *sd = &pipe->subdev;
	struct mtk_cam_video_device *video;
	int i;
	int ret;

	pipe->id = id;

	/* Initialize subdev */
	v4l2_subdev_init(sd, &mtk_raw_subdev_ops);
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	sd->entity.ops = &mtk_cam_media_entity_ops;
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	(void)snprintf(sd->name, sizeof(sd->name), "%s-%d", str, pipe->id);
	v4l2_set_subdevdata(sd, pipe);
	mtk_raw_pipeline_ctrl_setup(pipe);
	pr_info("%s: %s\n", __func__, sd->name);

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0) {
		pr_info("Failed to register subdev: %d\n", ret);
		return ret;
	}

	mtk_raw_pipeline_queue_setup(pipe);

	/* setup pads of raw pipeline */
	for (i = 0; i < ARRAY_SIZE(pipe->pads); i++) {
		pipe->pads[i].flags = i < MTK_RAW_SOURCE_BEGIN ?
			MEDIA_PAD_FL_SINK : MEDIA_PAD_FL_SOURCE;
	}

	media_entity_pads_init(&sd->entity, ARRAY_SIZE(pipe->pads), pipe->pads);

	/* setup video node */
	for (i = 0; i < ARRAY_SIZE(pipe->vdev_nodes); i++) {
		video = pipe->vdev_nodes + i;

		video->uid.pipe_id = pipe->id;
		video->uid.id = video->desc.dma_port;

		ret = mtk_cam_video_register(video, v4l2_dev);
		if (ret)
			goto fail_unregister_video;

		if (V4L2_TYPE_IS_OUTPUT(video->desc.buf_type))
			ret = media_create_pad_link(&video->vdev.entity, 0,
						    &sd->entity,
						    video->desc.id,
						    video->desc.link_flags);
		else
			ret = media_create_pad_link(&sd->entity,
						    video->desc.id,
						    &video->vdev.entity, 0,
						    video->desc.link_flags);

		if (ret)
			goto fail_unregister_video;
	}

	return 0;

fail_unregister_video:
	for (i = i - 1; i >= 0; i--)
		mtk_cam_video_unregister(pipe->vdev_nodes + i);

	return ret;
}

static void mtk_raw_pipeline_unregister(struct mtk_raw_pipeline *pipe)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pipe->vdev_nodes); i++)
		mtk_cam_video_unregister(pipe->vdev_nodes + i);
	v4l2_ctrl_handler_free(&pipe->ctrl_handler);

	v4l2_device_unregister_subdev(&pipe->subdev);
	media_entity_cleanup(&pipe->subdev.entity);
}

struct mtk_raw_pipeline *mtk_raw_pipeline_create(struct device *dev, int n)
{
	if (n <= 0)
		return NULL;
	return devm_kcalloc(dev, n, sizeof(struct mtk_raw_pipeline),
			    GFP_KERNEL);
}

int mtk_raw_setup_dependencies(struct mtk_cam_engines *eng)
{
	struct device *consumer, *supplier;
	struct device_link *link;
	struct mtk_raw_device *raw_dev;
	struct mtk_yuv_device *yuv_dev;
	int i;

	for (i = 0; i < eng->num_raw_devices; i++) {
		consumer = eng->raw_devs[i];
		supplier = eng->yuv_devs[i];
		if (!consumer || !supplier) {
			pr_info("failed to get raw/yuv dev for id %d\n", i);
			continue;
		}

		raw_dev = dev_get_drvdata(consumer);
		yuv_dev = dev_get_drvdata(supplier);
		raw_dev->yuv_base = yuv_dev->base;
		raw_dev->yuv_base_inner = yuv_dev->base_inner;

		link = device_link_add(consumer, supplier,
				       DL_FLAG_AUTOREMOVE_CONSUMER |
				       DL_FLAG_PM_RUNTIME);
		if (!link) {
			pr_info("Unable to create link between %s and %s\n",
				 dev_name(consumer), dev_name(supplier));
			return -ENODEV;
		}
	}

	return 0;
}

static void mtk_raw_pad_init_cfg(struct mtk_raw_pipeline *pipe)
{
	/* TODO: should config according to limitation */
	static const struct v4l2_mbus_framefmt mfmt_default = {
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.width = 1600,
		.height = 1200,
		.field = V4L2_FIELD_NONE,
		.colorspace = V4L2_COLORSPACE_SRGB,
		.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_DEFAULT,
		.xfer_func = V4L2_XFER_FUNC_DEFAULT,
		.flags = 0,
	};
	struct mtk_raw_pad_config *cfg;
	int i;

	for (i = 0, cfg = pipe->pad_cfg; i < ARRAY_SIZE(pipe->pad_cfg);
	     i++, cfg++) {
		cfg->mbus_fmt = mfmt_default;
		cfg->crop = (struct v4l2_rect) {
			.left = 0,
			.top = 0,
			.width = cfg->mbus_fmt.width,
			.height = cfg->mbus_fmt.height,
		};
	}
}

int mtk_raw_register_entities(struct mtk_raw_pipeline *arr_pipe, int num,
			      struct v4l2_device *v4l2_dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < num; i++) {
		struct mtk_raw_pipeline *pipe = arr_pipe + i;

		mtk_raw_pad_init_cfg(pipe);

		ret = mtk_raw_pipeline_register("mtk-cam raw",
						MTKCAM_SUBDEV_RAW_0 + i,
						pipe, v4l2_dev);
		if (ret)
			return ret;
	}
	return 0;
}

void mtk_raw_unregister_entities(struct mtk_raw_pipeline *arr_pipe, int num)
{
	unsigned int i;

	for (i = 0; i < num; i++)
		mtk_raw_pipeline_unregister(arr_pipe + i);
}
