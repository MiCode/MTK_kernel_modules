/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_RESOURCE_H
#define __MTK_CAM_RAW_RESOURCE_H

#include <linux/align.h>
#include <linux/minmax.h>

struct mtk_cam_res_calc {
	s64 mipi_pixel_rate;
	long line_time;		/* ns */
	long raw_line_time;	/* ns, for raw's dc/m2m */
	int width;
	int height;

	int raw_num;		/* [1..3] */
	int raw_pixel_mode;	/* [1,2] */
	int frontal_pixel_mode;	/* [8,16] */
	int clk;		/* Hz */

	int cbn_type : 4; /* 0: disable, 1: 2x2, 2: 3x3 3: 4x4 */
	int qbnd_en : 4;
	int qbn_type : 4; /* 0: disable, 1: w/2, 2: w/4 */
	int bin_en : 4;

	unsigned int slb_size;
};

static inline int _bin_ratio(struct mtk_cam_res_calc *c)
{
	/* note: qbn is not used yet */
	return (c->cbn_type || c->qbnd_en || c->bin_en) ? 4 : 1;
}

static inline int mtk_raw_overall_pixel_mode(struct mtk_cam_res_calc *c)
{
	return c->raw_num * _bin_ratio(c) * c->raw_pixel_mode;
}

static inline int _hor_twin_loss(struct mtk_cam_res_calc *c)
{
	static const int loss[] = {0, 108, 148};
	int idx = c->raw_num - 1;

	if (idx < 0 || idx >= ARRAY_SIZE(loss))
		return 0;

	return loss[idx];
}

static inline int _twin_loss(struct mtk_cam_res_calc *c)
{
	int loss;

	loss = _hor_twin_loss(c);
	return loss;
}

static inline int _hor_overhead(struct mtk_cam_res_calc *c)
{
	int oh;
	const int HOR_BUBBLE = 25;

	oh = HOR_BUBBLE * c->raw_num * c->raw_pixel_mode
		+ c->raw_num * _twin_loss(c);

	if (c->bin_en)
		oh = oh * 2;
	if (c->cbn_type)
		oh = oh * (1 + c->cbn_type);
	if (c->qbnd_en)
		oh = oh * 2;
	if (c->qbn_type)
		oh = oh * (1 + c->qbn_type);

	return max(mtk_raw_overall_pixel_mode(c) == 8 ? 999 : 0, oh);

}

static inline int _twin_overhead(struct mtk_cam_res_calc *c)
{
	return _hor_overhead(c) * 100 / c->width;
}

static inline int _clk_hopping(void)
{
	return 2; /* percent */
}

static inline int _process_pxl_per_line(struct mtk_cam_res_calc *c,
					long line_time,
					bool is_raw, bool enable_log)
{
	int twin_overhead = 0;
	int freq_Mhz = c->clk / 1000000;
	int w_processed;

	if (is_raw) {
		twin_overhead = _twin_overhead(c);
		w_processed = line_time * freq_Mhz * mtk_raw_overall_pixel_mode(c)
			* (100 - _clk_hopping()) / 100
			* (100 - twin_overhead) / 100
			/ 1000;
	} else
		w_processed = line_time * freq_Mhz * 8
			* (100 - _clk_hopping()) / 100
			/ 1000;

	if (enable_log)
		pr_info("%s: wp %d lt %ld ov %d clk %d pxl %d num %d\n",
			__func__, w_processed, line_time, twin_overhead,
			freq_Mhz, c->raw_pixel_mode, c->raw_num);

	return w_processed;
}

static inline int process_pxl_per_line(struct mtk_cam_res_calc *c,
				       bool is_raw, bool enable_log)
{
	return _process_pxl_per_line(c,
				     is_raw ? c->raw_line_time : c->line_time,
				     is_raw, enable_log);
}

static inline int process_pxl_per_sensor_line(struct mtk_cam_res_calc *c,
					      bool is_raw, bool enable_log)
{
	return _process_pxl_per_line(c, c->line_time, is_raw, enable_log);
}

static inline bool _valid_cbn_type(int cbn_type)
{
	return cbn_type >= 0 && cbn_type < 4;
}

static inline bool _valid_qbn_type(int qbn_type)
{
	return qbn_type >= 0 && qbn_type < 3;
}

static inline bool mtk_cam_raw_check_xbin_valid(struct mtk_cam_res_calc *c,
						bool enable_log)
{
	bool cbn_en = !!(c->cbn_type);
	bool invalid =
		!_valid_cbn_type(c->cbn_type) ||
		!_valid_qbn_type(c->qbn_type) ||
		(cbn_en + c->qbnd_en + c->bin_en > 1) ||
		(c->qbn_type && (c->qbnd_en || c->bin_en)); /* QBN w. CBN only */

	if (invalid && enable_log)
		pr_info("%s: cbn %d qbnd %d qbn %d bin %d failed\n",
			__func__, c->cbn_type, c->qbnd_en, c->qbn_type,
			c->bin_en);

	return !invalid;
}

static inline int _xbin_scale_ratio(struct mtk_cam_res_calc *c)
{
	int ratio = 1;

	ratio *= c->bin_en ? 2 : 1;
	ratio *= c->qbn_type ? (1 << c->qbn_type) : 1;
	ratio *= c->qbnd_en ? 2 : 1;
	ratio *= c->cbn_type ? (c->cbn_type + 1) : 1;
	return ratio;
}

static inline bool mtk_cam_check_mipi_pixel_rate(struct mtk_cam_res_calc *c,
						 bool enable_log)
{
	s64 frontal_throughput = (s64)c->clk * (100 - _clk_hopping()) / 100 * c->frontal_pixel_mode;
	bool valid = frontal_throughput > c->mipi_pixel_rate;

	if (!valid && enable_log)
		pr_info("%s: clk %d is not enough for mipi pixel rate %lld, pixel mode %d\n",
			__func__,  c->clk, c->mipi_pixel_rate, c->frontal_pixel_mode);

	return valid;
}

static inline bool mtk_cam_raw_check_line_buffer(struct mtk_cam_res_calc *c,
						 bool enable_log)
{
	const int max_main_pipe_w = GET_PLAT_HW(max_main_pipe_w);
	const int max_main_pipe_twin_w = GET_PLAT_HW(max_main_pipe_twin_w);
	int max_width;
	bool valid;

	if (!mtk_cam_raw_check_xbin_valid(c, enable_log))
		return false;

	max_width = c->raw_num > 1 ?
		(c->raw_num * max_main_pipe_twin_w) : max_main_pipe_w;
	max_width *= _xbin_scale_ratio(c);

	valid = (max_width >= c->width);
	if (!valid && enable_log)
		pr_info("%s: w %d max_w %d n %d cbn %d qbnd %d qbn %d bin %d failed\n",
			__func__, c->width, max_width,
			c->raw_num,
			c->cbn_type, c->qbnd_en, c->qbn_type, c->bin_en);

	return valid;
}

static inline bool mtk_cam_raw_check_throughput(struct mtk_cam_res_calc *c,
						bool enable_log)
{
	int processed_w = process_pxl_per_line(c, 1, enable_log);

	return c->width <= processed_w;
}

static inline bool mtk_cam_sv_check_throughput(struct mtk_cam_res_calc *c,
					       bool enable_log)
{
	int processed_w = process_pxl_per_line(c, 0, enable_log);

	return c->width <= processed_w;
}

static inline bool mtk_cam_raw_check_slb_size(struct mtk_cam_res_calc *c,
					      bool enable_log)
{
	int processed_w;
	size_t max_pending;
	bool valid;

	if (!c->slb_size)
		return true;

	processed_w = process_pxl_per_sensor_line(c, 1, enable_log);

	/* todo: calculate with bpp */
	max_pending = (size_t)max(c->width - processed_w, 0) * c->height * 10 / 8;

	/* 5% as margin */
	valid = (max_pending * 100 < (size_t)c->slb_size * 95);
	if (!valid && enable_log)
		pr_info("%s: processed_w %d max_pending %zu slb_size %u\n", __func__,
			processed_w, max_pending, c->slb_size);
	return valid;
}

#endif //__MTK_CAM_RAW_RESOURCE_H
