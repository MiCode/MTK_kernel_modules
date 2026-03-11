// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>

#include "custom_hw_sync.h"
#include "frame_sync_log.h"
#include "frame_sync_util.h"
#include "frame_sync_aee.h"

#define PFX "CustomHwSync"

#undef EN_CUSTOM_DBG_LOG

#define MCSS_TH 16
#define MAX_FL_LC (0xFFFC - MCSS_TH)


/* flicker table */
#define CUSTOM_FLK_TABLE_SIZE 8
static unsigned int custom_fs_flicker_table[CUSTOM_FLK_TABLE_SIZE][2] = {
	/* 14.6 ~ 15.3 */
	{68493, 65359},

	/* 24.6 ~ 25.3 */
	{40650, 39525},

	/* 29.6 ~ 30.5 */
	{33783, 32786},

	/* 59.2 ~ 60.7 */
	{16891, 16474},

	/* END */
	{0, 0}
};

static unsigned int get_anti_flicker_fl(unsigned int framelength)
{
	unsigned int i = 0;


	for (i = 0; i < CUSTOM_FLK_TABLE_SIZE; ++i) {
		if (custom_fs_flicker_table[i][0] == 0)
			break;

		if ((custom_fs_flicker_table[i][0] > framelength) &&
			(framelength >= custom_fs_flicker_table[i][1])) {

			framelength = custom_fs_flicker_table[i][0];
			break;
		}
	}


	return framelength;
}


/*
 * Sample code for normal single exposure sensor
 * Set all sensor's frame time to the max frame time of sensors
 *
 * This function is custom-able if need
 */
int custom_frame_time_calculator(
	struct SyncSensorPara sensor_paras[], unsigned int len)
{
	int i;
	unsigned int tmp;
	unsigned int min_fl;
	unsigned int max_frame_time = 0;
	struct SyncSensorPara *para;

	/* Test parameter */
	if (!sensor_paras) {
		LOG_PR_WARN("The parameter sensor_pars is invalid\n");
		return 1;
	}

	/* calculate min frame time for all */
	for (i = 0; i < len; ++i) {
		para = &sensor_paras[i];

#ifdef EN_CUSTOM_DBG_LOG
		LOG_MUST(
			"sensor_idx(%u), #%u, sensor_type(%u), hw_sync:%u(N:0/M:1/S:2), line_time_ns(%u), shutter_lc(%u), min_fl_lc(%u), margin_lc(%u)\n",
			para->sensor_idx,
			para->magic_num,
			para->sensor_type,
			para->sync_mode,
			para->line_time_in_ns,
			para->shutter_lc,
			para->min_fl_lc,
			para->sensor_margin_lc);
#endif // EN_CUSTOM_DBG_LOG

		min_fl = max((para->shutter_lc + para->sensor_margin_lc),
			     para->min_fl_lc);
		tmp = convert2TotalTime(para->line_time_in_ns, min_fl);

		if (para->flicker_en)
			tmp = get_anti_flicker_fl(tmp);

		if (tmp > max_frame_time)
			max_frame_time = tmp;
	}

	/* calculate min frame time for all */
	for (i = 0; i < len; ++i) {
		para = &sensor_paras[i];

		para->out_fl_lc = convert2LineCount(para->line_time_in_ns, max_frame_time);

		/* make master sensor slightly larger than slave */
		if (para->sync_mode == SENSOR_SYNC_MASTER)
			++(para->out_fl_lc);

#ifdef EN_CUSTOM_DBG_LOG
		LOG_MUST("sensor_idx(%u), #%u, out_fl:%u(%u), hw_sync:%u(N:0/M:1/S:2)\n",
			para->sensor_idx,
			para->magic_num,
			convert2TotalTime(
				para->line_time_in_ns,
				para->out_fl_lc),
			para->out_fl_lc,
			para->sync_mode);
#endif // EN_CUSTOM_DBG_LOG

	}

	return 0;
}


static unsigned int global_fl_line_time_in_ns;

int mcss_global_fl_calculator(
	struct SyncSensorPara sensor_paras[], unsigned int len)
{
	int i;
	unsigned int tmp;
	unsigned int min_fl;
	unsigned int max_frame_time = 0;
	unsigned int boundary = 0;
	unsigned int min_boundary = 0xffffffff; // UINT_MAX
	unsigned int min_line_time_in_ns = 0xffffffff; // UINT_MAX
	unsigned int max_line_time_in_ns = 0;
	struct SyncSensorPara *para;

	/* Test parameter */
	if (!sensor_paras) {
		LOG_PR_WARN("The parameter sensor_pars is invalid\n");
		return 1;
	}

	/* calculate min frame time for all */
	for (i = 0; i < len; ++i) {
		para = &sensor_paras[i];

#ifdef EN_CUSTOM_DBG_LOG
		LOG_MUST(
			"sensor_idx(%u), #%u, sensor_type(%u), hw_sync:%u(N:0/M:1/S:2), line_time_ns(%u), shutter_lc(%u), min_fl_lc(%u), margin_lc(%u)\n",
			para->sensor_idx,
			para->magic_num,
			para->sensor_type,
			para->sync_mode,
			para->line_time_in_ns,
			para->shutter_lc,
			para->min_fl_lc,
			para->sensor_margin_lc);
#endif // EN_CUSTOM_DBG_LOG

		min_fl = para->cal_min_fl_lc;
		tmp = convert2TotalTime(para->line_time_in_ns, min_fl);

		boundary = convert2TotalTime(para->line_time_in_ns, MAX_FL_LC);

		if (tmp > max_frame_time)
			max_frame_time = tmp;

		if ((boundary < min_boundary) && (boundary > 0))
			min_boundary = boundary;

		if ((para->line_time_in_ns < min_line_time_in_ns) && (para->line_time_in_ns > 0))
			min_line_time_in_ns = para->line_time_in_ns;

		if (para->line_time_in_ns > max_line_time_in_ns)
			max_line_time_in_ns = para->line_time_in_ns;
	}

	if (min_boundary < max_frame_time) {
		LOG_MUST(
				"!!! ASSERT IT !!! touch frame_length_max(%u > %u)\n",
					max_frame_time, min_boundary);
		FS_WRAP_AEE_EXCEPTION("[AEE] MCSS exposure make FL touch boundary", "Err");
		return 22; // EINVAL
	}

	if (global_fl_line_time_in_ns == max_frame_time) {
		LOG_PF_INF(
				"update no need, global_fl_line_time_in_ns:%u\n",
					global_fl_line_time_in_ns);

		// return 1;
	} else {
		global_fl_line_time_in_ns = max_frame_time;
		LOG_PF_INF(
				"update global_fl_line_time_in_ns:%u\n",
					global_fl_line_time_in_ns);
	}

	/* calculate min frame time for all */
	for (i = 0; i < len; ++i) {
		para = &sensor_paras[i];

		(para->out_fl_lc) = convert2LineCount(para->line_time_in_ns, global_fl_line_time_in_ns);

		/* make master sensor slightly larger than slave */
		if (para->sync_mode == SENSOR_SYNC_MASTER) {
			unsigned int TH = (MCSS_TH * max_line_time_in_ns)/min_line_time_in_ns;
			(para->out_fl_lc) = (para->out_fl_lc) + TH;
			LOG_PF_INF(
				"MCSS_TH(%u), min_line_time_in_ns:%u max_line_time_in_ns:%u para->line_time_in_ns:%u TH:%u\n",
			MCSS_TH, min_line_time_in_ns, max_line_time_in_ns, para->line_time_in_ns, TH);
		}

#ifdef EN_CUSTOM_DBG_LOG
		LOG_MUST("sensor_idx(%u), #%u, out_fl:%u(%u), hw_sync:%u(N:0/M:1/S:2)\n",
			para->sensor_idx,
			para->magic_num,
			convert2TotalTime(
				para->line_time_in_ns,
				para->out_fl_lc),
			para->out_fl_lc,
			para->sync_mode);
#endif // EN_CUSTOM_DBG_LOG

	}

	return 0;
}
