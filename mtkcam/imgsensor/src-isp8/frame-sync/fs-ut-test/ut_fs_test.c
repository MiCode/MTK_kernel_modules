// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>    /* Needed by sleep() function */

/* ut fs headers */
#include "ut_fs_test.h"
#include "ut_fs_streaming_info.h"
#include "ut_fs_perframe_ctrl_info.h"
#include "ut_fs_test_config.h"
#include "ut_fs_perframe_ctrl_sample_main_0.h"
#include "ut_fs_perframe_ctrl_sample_main_1.h"
#include "ut_fs_perframe_ctrl_sample_main_2.h"

#include "ut_fs_tsrec.h"

/* frame-sync headers */
#include "../frame_sync_util.h"
#include "../frame_monitor.h"


#define REDUCE_UT_DEBUG_PRINTF
#define USING_PRIVATE_CTRL_ORDER

#define UT_SENSOR_TRIGGER_BIAS 13000

#define WAITING_FOR_REMOVE_CODE 0


/******************************************************************************/
#define SHOW_TEXT() \
({ \
	printf(LIGHT_CYAN \
		">=============================================================================<\n"\
		NONE); \
	printf(LIGHT_CYAN \
		"!!!                        FrameSync UT Test Program                        !!!\n"\
		NONE); \
	printf(LIGHT_CYAN \
		">=============================================================================<\n"\
		NONE); \
})
/******************************************************************************/


static struct FrameSync *frameSync;


/******************************************************************************/
// about sensor data
/******************************************************************************/
static unsigned int g_set_synced_sensors[SENSOR_MAX_NUM] = {0};

static struct ut_fs_streaming_sensor_list
	g_streaming_sensors[SENSOR_MAX_NUM] = {0};

static struct ut_fs_perframe_sensor_mode_list
	g_streaming_sensors_modes_list[SENSOR_MAX_NUM] = {0};

static unsigned int g_sensor_mode[SENSOR_MAX_NUM] = {0};


/* EXT CTRL config */
static struct ut_fs_test_ext_ctrl_cfg g_ext_ctrls[SENSOR_MAX_NUM] = {0};
/******************************************************************************/


/******************************************************************************/
// about timestamp
/******************************************************************************/
struct UT_Timestamp {
	unsigned int idx;   // idx: last data in timestamp[VSYNC_MAX] array
	unsigned long long timestamp[VSYNCS_MAX];
	unsigned long long target_ts[VSYNCS_MAX];

	unsigned long long ts_diff[SENSOR_MAX_NUM];

	unsigned int curr_bias;
	unsigned int next_bias;

	/* for UT cross frame trigger frame-sync */
	unsigned long long should_be_triggered_before;
	unsigned long long be_triggered_at_ts;
	unsigned int should_trigger;
};
static struct UT_Timestamp g_ut_vts[SENSOR_MAX_NUM] = {0};

static long long g_ut_ts_diff_table[TS_DIFF_TABLE_LEN] = {-1};

#define VDIFF_SYNC_SUCCESS_TH 1000
static unsigned int g_vdiff_sync_success_th;

// #define MANUALLY_SET_CURRENT_TIMESTAMP

static struct vsync_rec g_v_rec = {0};

static unsigned int g_query_tg_cnt;
/******************************************************************************/


/******************************************************************************/
// about per-frame ctrl and AE shutter
/******************************************************************************/
/* 1: CTRL Pair / 2: SA(StandAlone) */
static unsigned int g_user_alg_method;
static unsigned int g_alg_method;

static unsigned int g_auto_run;
static unsigned int g_run_times;
static unsigned int g_counter;
static unsigned int g_broke_at_counter;

// static unsigned int g_trigger_ext_fl;
// static unsigned int g_trigger_seamless_switch;


/* auto-run shutter */
static unsigned int g_shutter;
static unsigned int g_hdr_shutter[FS_HDR_MAX] = {0};
static const unsigned int g_hdr_divided_ratio[FS_HDR_MAX] = {
	1,
	2,
	4,
	5,
	6,
};

static unsigned int g_sensor_exp_triggered_cnt[SENSOR_MAX_NUM] = {0};
static unsigned int g_sensor_request_new_exp[SENSOR_MAX_NUM] = {0};

static unsigned int force_lock_exp;
static unsigned int lock_exp;

#define EXP_TABLE_SIZE 6
static unsigned int exp_table_idx;
static const unsigned int exp_table[EXP_TABLE_SIZE] = {
	10002,
	19997,
	29996,
	40005,
	50004,
	60002,
};


/* flicker enable table */
static unsigned int lock_flk;
static const unsigned int flk_en_table[10] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned int flk_en_table_idx;


static unsigned int simulation_passed_vsyncs;
static unsigned int passed_vsyncs_ratio;
static unsigned int max_pass_cnt;
/******************************************************************************/


/******************************************************************************/
// about N:1 FrameSync mode
/******************************************************************************/
static struct ut_fs_test_n_1_mode_cfg n_1_cfg[SENSOR_MAX_NUM] = {0};
static unsigned int g_n_1_status[SENSOR_MAX_NUM] = {0};
static unsigned int g_n_1_min_fl_us[SENSOR_MAX_NUM] = {0};
static unsigned int g_n_1_f_cell_size[SENSOR_MAX_NUM] = {0};
/******************************************************************************/


/******************************************************************************/
// about FrameSync Algorithm stability test
/******************************************************************************/
static unsigned int g_fs_alg_stability_test_flag;

/**
 * sync type config,
 * e.g., sync point set to 'readout center'
 */
static unsigned int g_sync_type_en_rout_center;
/******************************************************************************/





/******************************************************************************/
// FrameSync UT basic CMD interactive functions
/******************************************************************************/
static inline void ut_select_frame_sync_algorithm(void)
{
	printf(GREEN
		"\n\n\n>>> Please choose FrameSync algorithm when testing! (1: CTRL Pair / 2: SA(StandAlone)) <<<\n"
		NONE);
	printf(LIGHT_PURPLE
		">>> (Input 1 integer) \"select a algorithm\" : "
		NONE);
	scanf("%u", &g_user_alg_method);
}


static int print_and_select_sensor(
	const struct ut_fs_streaming_sensor_list s_list[])
{
	unsigned int i = 0;
	int select = 0;

	/* print out sensors */
	for (i = 0; s_list[i].sensor != NULL; ++i) {
		printf(GREEN "[%d] %s\t" NONE,
				i, s_list[i].sensor_name);
		if ((i >= 10) && (i % 10 == 0))
			printf("\n");
	}
	printf(GREEN "[-1] END\n" NONE);


	/* select a sensor */
	printf(LIGHT_PURPLE
		">>> (Input 1 integer) \"select a sensor\" : "
		NONE);
	scanf("%d", &select);

	while (select > 0 && select >= i) { // no such idx's sensor in array
		printf(RED
			">>> NO such idx's sensor in list, please select again !\n"
			NONE);

		/* select a sensor */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"select a sensor\" : "
			NONE);
		scanf("%d", &select);
	}

	return select;
}


static unsigned int print_and_select_s_mode(const int idx)
{
	unsigned int i = 0, select = 0;

	/* print out sensor's mode list */
	for (i = 0;
		g_streaming_sensors_modes_list[idx]
			.mode_list[i].sensor_id != 0; ++i) {

		printf(GREEN
			"[%d] ID:%#x (sidx:%u), margin_lc:%3u, lineTimeInNs:%7u, pclk:%10llu, linelength:%6u, hdr_exp.mode_exp_cnt:%u\n"
			NONE,
			i,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].sensor_id,
			g_streaming_sensors_modes_list[idx]
					.sensor_idx,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].margin_lc,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].lineTimeInNs,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].pclk,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].linelength,
			g_streaming_sensors_modes_list[idx]
					.mode_list[i].hdr_exp.mode_exp_cnt);
	}

	/* select sensor mode */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) \"select a sensor mode\" : "
		NONE);
	scanf("%d", &select);

	while (select < 0 || select >= i) { // no such idx's sensor mode in above list
		printf(RED
			">>> NO such idx's sensor in list, please select again !\n"
			NONE);

		/* select a sensor */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"select a sensor mode\" : "
			NONE);
		scanf("%d", &select);
	}

	return select;
}


/******************************************************************************/





/******************************************************************************/
// FrameSync UT basic / utility functions
/******************************************************************************/
int cb_func_ut_fsync_mgr_set_fl_lc(void *p_ctx, const unsigned int cmd_id,
	const void *pf_info, const unsigned int fl_lc,
	const unsigned int fl_lc_arr[], const unsigned int arr_len)
{
	struct fs_perframe_st pf_ctrl;

	/* copy full structure info */
	memcpy(&pf_ctrl, pf_info, sizeof(pf_ctrl));

	/* overwrite frame length info */
	pf_ctrl.out_fl_lc = fl_lc;
	memcpy(pf_ctrl.hdr_exp.fl_lc, fl_lc_arr,
		sizeof(unsigned int) * arr_len);

	printf(GREEN
		"[%s] p_ctx:%p, cmd_id:%u, &pf_info:%p, fl_lc:%u, fl_lc_arr:(%u/%u/%u/%u/%u), arr_len:%u\n"
		NONE,
		__func__,
		p_ctx, cmd_id,
		pf_info, fl_lc,
		fl_lc_arr[0],
		fl_lc_arr[1],
		fl_lc_arr[2],
		fl_lc_arr[3],
		fl_lc_arr[4],
		arr_len);
	printf(LIGHT_GREEN
		"[%s] call fs update shutter ...\n"
		NONE,
		__func__);

	/* call for update info */
	frameSync->fs_update_shutter(&pf_ctrl);

	return 0;
}


static inline unsigned int ut_auto_choose_fs_alg_method(void)
{
	return (g_alg_method == 0)
		? (g_user_alg_method == 2) ? 1 : 0
		: (g_alg_method == 2) ? 1 : 0;
}


static inline int ut_get_sidx_by_sensor_id(
	const unsigned int id, unsigned int *iidx)
{
	unsigned int i = 0;

	for (i = 0; ; ++i) {
		if (g_streaming_sensors[i].sensor == NULL)
			break;

		if (g_streaming_sensors[i].sensor->sensor_id == id) {
			*iidx = i;
			return g_streaming_sensors[i].sensor_idx;
		}
	}

	*iidx = -1;
	return -1;
}


static inline int ut_get_sidx_by_sensor_idx(
	const unsigned int idx, unsigned int *iidx)
{
	unsigned int i = 0;

	for (i = 0; ; ++i) {
		if (g_streaming_sensors[i].sensor == NULL)
			break;

		if (g_streaming_sensors[i].sensor_idx == idx) {
			*iidx = i;
			return i;
		}
	}

	*iidx = -1;
	return -1;
}


static inline int ut_get_sid_by_sensor_idx(
	const unsigned int idx, unsigned int *iidx)
{
	unsigned int i = 0;

	for (i = 0; ; ++i) {
		if (g_streaming_sensors[i].sensor == NULL)
			break;

		if (g_streaming_sensors[i].sensor_idx == idx) {
			*iidx = i;
			return g_streaming_sensors[i].sensor->sensor_id;
		}
	}

	*iidx = -1;
	return -1;
}


static inline void set_pf_ctrl_sensor_ident(
	const unsigned int idx,
	struct fs_perframe_st *p_pf_ctrl)
{
	p_pf_ctrl->sensor_id =
		g_streaming_sensors[idx].sensor->sensor_id;
	p_pf_ctrl->sensor_idx =
		g_streaming_sensors[idx].sensor_idx;
}


/*
 * for faster setup pf_ctrl data structure
 *
 * input:
 *     idx: idx of g_streaming_sensors[]
 *     mode: mode number from user selected
 *
 * input/output:
 *     p_pf_ctrl: copy needed variable out
 */
static inline void set_pf_ctrl_s_mode(
	const unsigned int idx, const unsigned int mode,
	struct fs_perframe_st *p_pf_ctrl)
{
	*p_pf_ctrl = g_streaming_sensors_modes_list[idx]
				.mode_list[mode];

	/* bellow item may be change during copy mode info, */
	/* so overwrite these data back (idx & id) */
	set_pf_ctrl_sensor_ident(idx, p_pf_ctrl);
}


static inline void ut_dump_vsync_rec(const struct vsync_rec (*pData))
{
	unsigned int i = 0;

	printf(GREEN
#if defined(TS_TICK_64_BITS)
		"[UT dump_vsync_rec] buf->ids:%u, buf->cur_tick:%llu, buf->tick_factor:%u\n"
#else
		"[UT dump_vsync_rec] buf->ids:%u, buf->cur_tick:%u, buf->tick_factor:%u\n"
#endif
		NONE,
		pData->ids,
		pData->cur_tick,
		pData->tick_factor);

	for (i = 0; i < pData->ids; ++i) {
		printf(GREEN
#if defined(TS_TICK_64_BITS)
			"[UT dump_vsync_rec] buf->recs[%u]: id:%u (TG), vsyncs:%u, vts:(%llu/%llu/%llu/%llu)\n"
#else
			"[UT dump_vsync_rec] buf->recs[%u]: id:%u (TG), vsyncs:%u, vts:(%u/%u/%u/%u)\n"
#endif
			NONE,
			i,
			pData->recs[i].id,
			pData->recs[i].vsyncs,
			pData->recs[i].timestamps[0],
			pData->recs[i].timestamps[1],
			pData->recs[i].timestamps[2],
			pData->recs[i].timestamps[3]);
	}
}


static inline void ut_dump_ut_vts_data(const unsigned int idx)
{
	printf(GREEN
		"[UT dump_ut_vts_data] [%u] vts:(last idx:%u, ts:%llu/%llu/%llu/%llu, bias(c:%u, n:%u), target_ts:(%llu/%llu/%llu/%llu), trigger:(%u, at:%llu, before:%llu)), ts_diff:(%llu/%llu/%llu/%llu)\n"
		NONE,
		idx,
		g_ut_vts[idx].idx,
		g_ut_vts[idx].timestamp[0],
		g_ut_vts[idx].timestamp[1],
		g_ut_vts[idx].timestamp[2],
		g_ut_vts[idx].timestamp[3],
		g_ut_vts[idx].curr_bias,
		g_ut_vts[idx].next_bias,
		g_ut_vts[idx].target_ts[0],
		g_ut_vts[idx].target_ts[1],
		g_ut_vts[idx].target_ts[2],
		g_ut_vts[idx].target_ts[3],
		g_ut_vts[idx].should_trigger,
		g_ut_vts[idx].be_triggered_at_ts,
		g_ut_vts[idx].should_be_triggered_before,
		g_ut_vts[idx].ts_diff[0],
		g_ut_vts[idx].ts_diff[1],
		g_ut_vts[idx].ts_diff[2],
		g_ut_vts[idx].ts_diff[3]);
}


static inline void ut_dump_ts_diff_table_results(void)
{
	unsigned int i;

	printf(CYAN
		"[UT dump_ts_diff_table_results] "
		NONE);

	for (i = 0; i < TS_DIFF_TABLE_LEN; ++i) {
		printf(CYAN
			"[%u]:%lld   "
			NONE,
			i, g_ut_ts_diff_table[i]);
	}

	printf("\n");
}
/******************************************************************************/





/******************************************************************************/
//  Frame-Sync UT => case 3: run fs control test
/******************************************************************************/
static void test_fs_register_sensor_behavior(void)
{
	struct SensorInfo *sensor_info;
	const unsigned int already_used = 3;

	sensor_info = calloc(SENSOR_MAX_NUM, sizeof(*sensor_info));

	printf(
		"/*************************************************************/\n"
		);
	printf("/* --- Test fs register sensor function behavior. --- */\n");
	printf("\n\n\n");
	printf(">>> Scenario : already registered %u sensors. (sidx:0/2/4)\n",
		already_used);

	sensor_info[0].sensor_id = main0.sensor->sensor_id;
	sensor_info[0].sensor_idx = main0.sensor->sensor_idx;
	sensor_info[0].seninf_idx = 2;
	frameSync->fs_register_sensor(&sensor_info[0], REGISTER_METHOD);
	frameSync->fs_unregister_sensor(&sensor_info[0], REGISTER_METHOD);

	sensor_info[1].sensor_id = main1.sensor->sensor_id;
	sensor_info[1].sensor_idx = main1.sensor->sensor_idx;
	sensor_info[1].seninf_idx = 0;
	frameSync->fs_register_sensor(&sensor_info[1], REGISTER_METHOD);
	frameSync->fs_unregister_sensor(&sensor_info[1], REGISTER_METHOD);

	sensor_info[2].sensor_id = main2.sensor->sensor_id;
	sensor_info[2].sensor_idx = main2.sensor->sensor_idx;
	sensor_info[3].seninf_idx = 4;
	frameSync->fs_register_sensor(&sensor_info[2], REGISTER_METHOD);
	frameSync->fs_unregister_sensor(&sensor_info[2], REGISTER_METHOD);

	printf("\n\n\n");

	/* !!! start testing here !!! */
	for (int run = 0; run < 2; ++run) {
		printf(
			">>>>>>>>>>>>>>>>>>>> Test Run %d (chk first & second run behavior) <<<<<<<<<<<<<<<<<<<<\n",
			run+1);
		printf(
			"[--- Already registered/streaming ON %d sensors, register more... ---]\n\n",
			already_used);

		/* Test fs register sensor */
		for (unsigned int i = 0; i < SENSOR_MAX_NUM; ++i) {
			sensor_info[i].sensor_id = i * 2 + 1;
			sensor_info[i].sensor_idx = i * 2 + 1;
			sensor_info[i].seninf_idx = i * 2 + 1;

			printf("\n=== Test register more sensor:%2d - %#x(sidx:%u/inf:%u) ===\n",
				already_used + i + 1,
				sensor_info[i].sensor_id,
				sensor_info[i].sensor_idx,
				sensor_info[i].seninf_idx);

			frameSync->fs_register_sensor(
				&sensor_info[i], REGISTER_METHOD);
			frameSync->fs_unregister_sensor(
				&sensor_info[i], REGISTER_METHOD);
		}

		printf("\n\n");
	}

	free(sensor_info);

	printf("//--- Test fs register sensor function behavior END! ---//\n");
}


static void test_streaming_behavior(void)
{
	struct SensorInfo reg_info;
	struct fs_streaming_st sensor_info = {0};

	printf("\n");
	printf(
		"/*************************************************************/\n"
		);
	printf("//--- Test streaming behavior. ---//\n");

	/* first, register sensor */
	sensor_info.sensor_id = 0x30D5;
	sensor_info.sensor_idx = 2;
	fs_setup_sensor_info_st_by_fs_streaming_st(&sensor_info, &reg_info);
	frameSync->fs_register_sensor(&reg_info, REGISTER_METHOD);

	/* Test streaming */
	printf("\n--- Test streaming ON behavior. ---\n");
	frameSync->fs_streaming(1, &sensor_info);
	printf("\n--- Test streaming OFF behavior. ---\n");
	frameSync->fs_streaming(0, &sensor_info);
	printf("\n--- Test streaming ON behavior. (using config data) ---\n");
	frameSync->fs_streaming(1, main1.sensor);

	/* Test set sync */
	printf("\n--- Test set sync behavior. (ON & OFF) ---\n");
	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		frameSync->fs_set_sync(sensor_info.sensor_id, 1);
		frameSync->fs_set_sync(sensor_info.sensor_id, 0);
		frameSync->fs_set_sync(sensor_info.sensor_id, 1);
		break;
	case BY_SENSOR_IDX:
	default:
		frameSync->fs_set_sync(sensor_info.sensor_idx, 1);
		frameSync->fs_set_sync(sensor_info.sensor_idx, 0);
		frameSync->fs_set_sync(sensor_info.sensor_idx, 1);
		break;
	}

	printf("\n--- Test streaming OFF w/ enable sync behavior. ---\n");
	frameSync->fs_streaming(0, &sensor_info);

	frameSync->fs_unregister_sensor(&reg_info, REGISTER_METHOD);

	printf("//--- Test streaming behavior END! ---//\n");
}


static void test_syncFrame_behavior(void)
{
	/* Test fs_sync_frame behavior. */
	printf("\n");
	printf(
		"/*************************************************************/\n"
		);
	printf("//--- Test fs_sync_frame behavior. ---//\n");

	printf("\n//=== Not sync start, call sync end. ===//\n");
	frameSync->fs_sync_frame(0);
	printf("\n\n");

	printf("//=== unset some sensor sync ===//\n");


	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
	{
		frameSync->fs_set_sync(0x481, 0);
		printf("\n");
		frameSync->fs_sync_frame(1);
		printf("\n");
		frameSync->fs_set_sync(0x481, 1);
		frameSync->fs_set_sync(0x481, 0);
		frameSync->fs_set_sync(0x586, 0);
		printf("\n");
		frameSync->fs_sync_frame(1);
		printf("\n");
		frameSync->fs_set_sync(0x481, 1);
		frameSync->fs_set_sync(0x586, 1);
	}
		break;
	case BY_SENSOR_IDX:
	default:
	{
		frameSync->fs_set_sync(4, 0);
		printf("\n");
		frameSync->fs_sync_frame(1);
		printf("\n");
		frameSync->fs_set_sync(4, 1);
		frameSync->fs_set_sync(4, 0);
		frameSync->fs_set_sync(0, 0);
		printf("\n");
		frameSync->fs_sync_frame(1);
		printf("\n");
		frameSync->fs_set_sync(4, 1);
		frameSync->fs_set_sync(0, 1);
	}
		break;
	}


	printf("//--- Test fs_sync_frame behavior END! ---//\n");
}


static void run_fs_control_test(void)
{
	unsigned int ret = 0;

	ret = FrameSyncInit(&frameSync);
	if (ret != 0) {
		printf(RED ">>> frameSync Init failed !\n" NONE);
		return;
	}


	printf("\n");
	printf(
		"/*************************************************************/\n"
		);
	printf(
		"/* ---         Run FrameSync Driver Control Test         --- */\n"
		);

	test_fs_register_sensor_behavior();
	test_streaming_behavior();
	test_syncFrame_behavior();

	printf("\n\n\n");
	printf(
		"/* ---         END FrameSync Driver Control Test         --- */\n"
		);
	printf(
		"/*************************************************************/\n"
		);
	printf("\n");
}
/******************************************************************************/





/******************************************************************************/
//  Frame-Sync UT => case 4: run fs data racing test
/******************************************************************************/
static void *ut_test_data_racing(void *ut_fs_test_sensor_cfg)
{
	struct ut_fs_test_sensor_cfg *arg =
		(struct ut_fs_test_sensor_cfg *) ut_fs_test_sensor_cfg;
	struct SensorInfo reg_info;
	unsigned int ret;

	ret = FrameSyncInit(&frameSync);

	printf("***### frameSync addr = %p (sensor_idx=%2d) ###***\n",
		frameSync,
		arg->sensor_idx);

	fs_setup_sensor_info_st_by_fs_streaming_st(arg->sensor, &reg_info);
	frameSync->fs_register_sensor(&reg_info, REGISTER_METHOD);
	frameSync->fs_unregister_sensor(&reg_info, REGISTER_METHOD);

	frameSync->fs_streaming(1, arg->sensor);

	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		ret = frameSync->fs_is_set_sync(arg->sensor->sensor_id);
		if (ret == 0)
			frameSync->fs_set_sync(arg->sensor->sensor_id, 1);

		ret = frameSync->fs_is_set_sync(arg->sensor->sensor_id);
		break;
	case BY_SENSOR_IDX:
	default:
		ret = frameSync->fs_is_set_sync(arg->sensor->sensor_idx);
		if (ret == 0)
			frameSync->fs_set_sync(arg->sensor->sensor_idx, 1);

		ret = frameSync->fs_is_set_sync(arg->sensor->sensor_idx);
		break;
	}

	pthread_exit(NULL);
}


static void run_fs_data_racing_test(void)
{
	pthread_t thread[3];

	pthread_create(&thread[0], NULL, ut_test_data_racing, (void *)&main0);
	pthread_create(&thread[1], NULL, ut_test_data_racing, (void *)&main1);
	pthread_create(&thread[2], NULL, ut_test_data_racing, (void *)&main2);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);

	/* for triggering print out newest status */
	frameSync->fs_sync_frame(1);
}
/******************************************************************************/





/******************************************************************************/
//  Frame-Sync UT => case 1 & 2:
//      run fs algorithm stability test
//      test fs processing
/******************************************************************************/
/**
 * called by exe fs alg stability test item API (using multi-thread)
 */
static void *ut_set_fs_streaming_and_synced(void *ut_fs_test_sensor_cfg)
{
	const struct ut_fs_test_sensor_cfg *sensor_cfg =
		(struct ut_fs_test_sensor_cfg *)ut_fs_test_sensor_cfg;
	struct fs_streaming_st s_sensor = {0};
	struct SensorInfo reg_info;
	unsigned int alg_method = 0, sync_type_cfg;
	unsigned int ret;

	ret = FrameSyncInit(&frameSync);
	if (ret != 0) {
		printf(RED ">>> frameSync Init failed !\n" NONE);
		pthread_exit(NULL);
	}
	printf(GREEN
		">>> frameSync addr = %p (sensor_idx=%2d)\n"
		NONE,
		frameSync,
		sensor_cfg->sensor_idx);


	/* setup ut streaming data */
	s_sensor = *sensor_cfg->sensor;
	/* --- overwrite some data */
	s_sensor.sensor_idx = sensor_cfg->sensor_idx;
	s_sensor.cammux_id = sensor_cfg->tg;
	s_sensor.target_tg = CAMMUX_ID_INVALID;
	s_sensor.func_ptr = &cb_func_ut_fsync_mgr_set_fl_lc;

	/* call fs register sensor */
	fs_setup_sensor_info_st_by_fs_streaming_st(&s_sensor, &reg_info);
	frameSync->fs_register_sensor(&reg_info, REGISTER_METHOD);

	/* call fs streaming... */
	frameSync->fs_streaming(1, &s_sensor);


	/* call fs set_sync... */
	/* setup FrameSync algorithm for running */
	alg_method = ut_auto_choose_fs_alg_method();
	frameSync->fs_set_using_sa_mode(alg_method);

	g_set_synced_sensors[sensor_cfg->sensor_idx] = 1;

	sync_type_cfg = sensor_cfg->sync_type;
	if (g_sync_type_en_rout_center)
		sync_type_cfg |= FS_SYNC_TYPE_READOUT_CENTER;

	printf(GREEN
		"[UT set_fs_streaming_and_synced] sensor_idx:%u, sync_type:%#x\n"
		NONE,
		s_sensor.sensor_idx,
		sync_type_cfg);

	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		frameSync->fs_set_sync(s_sensor.sensor_id, sync_type_cfg);
		// ret = frameSync->fs_is_set_sync(s_sensor.sensor_id);
		break;
	case BY_SENSOR_IDX:
	default:
		frameSync->fs_set_sync(s_sensor.sensor_idx, sync_type_cfg);
		// ret = frameSync->fs_is_set_sync(s_sensor.sensor_idx);
		break;
	}

	pthread_exit(NULL);
}


/**
 * currently no one use this api
 */
void test_sync_2_sensors(void)
{
	// TODO: run times should not be hardcode.
	for (unsigned int i = 0; i < 4; ++i) {
		struct vsync_rec rec = {0};

		rec.ids = 2;
		rec.tick_factor = TICK_FACTOR;
		rec.recs[0].id = main0.sensor->tg;
		rec.recs[0].vsyncs = pf_settings_main0[i].vsyncs;
		rec.recs[0].timestamps[0] =
			pf_settings_main0[i].last_vsync_timestamp;

		rec.recs[1].id = main1.sensor->tg;
		rec.recs[1].vsyncs = pf_settings_main1[i].vsyncs;
		rec.recs[1].timestamps[0] =
			pf_settings_main1[i].last_vsync_timestamp;

		rec.cur_tick = (
			rec.recs[0].timestamps[0] > rec.recs[1].timestamps[0])
			? rec.recs[0].timestamps[0] + 100
			: rec.recs[1].timestamps[0] + 100;
		rec.cur_tick *= rec.tick_factor;

		frm_debug_set_last_vsync_data(&rec);

		printf("//--- fs_sync_frame:%d ---//\n", i+1);
		frameSync->fs_sync_frame(1);
		frameSync->fs_set_shutter(pf_settings_main0[i].pf_settings);
		frameSync->fs_set_shutter(pf_settings_main1[i].pf_settings);
		frameSync->fs_set_shutter(&frameCtrl_main2[0]);
		frameSync->fs_sync_frame(0);
		printf("//--- fs_sync_frame:%d END!!! ---//\n", i+1);
		printf("\n\n\n");
	}
}


/*
 * only update data query by the TG requested
 *
 * input:
 *     idx: idx of vsync_rec->recs[] (0 ~ TG_MAX_NUM)
 *     vsyncs: how many vsync want to let it passed (pf => 1)
 *
 * output:
 *     p_v_rec: pointer of output data structure
 */
static void ut_gen_timestamps_data(
	unsigned int idx, unsigned int vsyncs,
	struct vsync_rec *(p_v_rec))
{
	unsigned long long cur_tick = 0;
	unsigned int i = 0, index = 0, choose = 0;
	unsigned int fl_us[2] = {0}, sensor_curr_fl_us = 0;
	unsigned int ref_idx = 0, ref_idx_next = 0;
	unsigned int trigger_timing_bias = UT_SENSOR_TRIGGER_BIAS;


	if (p_v_rec->recs[idx].id == 0)
		return;

	index = frm_get_instance_idx_by_tg(p_v_rec->recs[idx].id);

	if (index < 0)
		return;

	if (!g_ut_vts[idx].should_trigger)
		return;


	/* fl_us[0] => curr predicteed, fl_us[1] => next predicted */
	/* sensor_curr_fl_us => output fl set to sensor driver */
	frm_get_predicted_fl_us(index, fl_us, &sensor_curr_fl_us);


	g_ut_vts[idx].curr_bias = g_ut_vts[idx].next_bias;
	frm_get_next_vts_bias_us(index, &g_ut_vts[idx].next_bias);


	/* generate each timestamp according to pass vsyncs count */
	for (i = 0; i < vsyncs; ++i) {
		ref_idx = g_ut_vts[idx].idx;
		ref_idx_next = (ref_idx + 1) % VSYNCS_MAX;

		if (i < 2) {
			choose = (i > 0) ? 1 : 0;
			g_ut_vts[idx].timestamp[ref_idx_next] =
				g_ut_vts[idx].timestamp[ref_idx] +
				fl_us[choose];
		} else {
			g_ut_vts[idx].timestamp[ref_idx_next] =
				g_ut_vts[idx].timestamp[ref_idx] +
				sensor_curr_fl_us;
		}

		g_ut_vts[idx].target_ts[ref_idx_next] =
			g_ut_vts[idx].timestamp[ref_idx_next]
			+ g_ut_vts[idx].curr_bias;

		g_ut_vts[idx].idx = ref_idx_next;
	}


	/* calculate trigger timimg for UT cross trigger frame-sync using */
	g_ut_vts[idx].be_triggered_at_ts =
		g_ut_vts[idx].timestamp[g_ut_vts[idx].idx] +
		trigger_timing_bias;

	if (vsyncs > 1) {
		g_ut_vts[idx].should_be_triggered_before =
			g_ut_vts[idx].timestamp[g_ut_vts[idx].idx] +
			sensor_curr_fl_us;

	} else {
		g_ut_vts[idx].should_be_triggered_before =
			g_ut_vts[idx].timestamp[g_ut_vts[idx].idx] +
			fl_us[1];
	}


	/* update cur_tick for a query only query "1" TG data */
	/* UT default set to (SOF + 100) us */
	cur_tick =
		(g_ut_vts[idx].timestamp[g_ut_vts[idx].idx] + 100) *
		p_v_rec->tick_factor;

	p_v_rec->cur_tick =
		check_tick_b_after_a(cur_tick, p_v_rec->cur_tick)
		? p_v_rec->cur_tick
		: cur_tick;



#if defined(REDUCE_UT_DEBUG_PRINTF)
	printf(LIGHT_CYAN
#if defined(TS_TICK_64_BITS)
		"[UT][gen_timestamps_data] [%u] FRM pr_fl(c:%u, n:%u)/vts_bias(c:%u, n:%u), g_ut_vts[%u]:(idx:%u, ts:(%llu/%llu/%llu/%llu), target_ts:(%llu/%llu/%llu/%llu)), vsyncs:%u, cur_tick:%llu, be_triggered:(at:%llu, before:%llu, bias:%u)  [p_v_rec->recs[%u].id:%u (TG)]\n"
#else
		"[UT][gen_timestamps_data] [%u] FRM pr_fl(c:%u, n:%u)/vts_bias(c:%u, n:%u), g_ut_vts[%u]:(idx:%u, ts:(%llu/%llu/%llu/%llu), target_ts:(%llu/%llu/%llu/%llu)), vsyncs:%u, cur_tick:%u, be_triggered:(at:%llu, before:%llu, bias:%u)  [p_v_rec->recs[%u].id:%u (TG)]\n"
#endif
		NONE,
		idx,
		fl_us[0], fl_us[1],
		g_ut_vts[idx].curr_bias, g_ut_vts[idx].next_bias,
		idx,
		g_ut_vts[idx].idx,
		g_ut_vts[idx].timestamp[0],
		g_ut_vts[idx].timestamp[1],
		g_ut_vts[idx].timestamp[2],
		g_ut_vts[idx].timestamp[3],
		g_ut_vts[idx].target_ts[0],
		g_ut_vts[idx].target_ts[1],
		g_ut_vts[idx].target_ts[2],
		g_ut_vts[idx].target_ts[3],
		vsyncs,
		p_v_rec->cur_tick,
		g_ut_vts[idx].be_triggered_at_ts,
		g_ut_vts[idx].should_be_triggered_before,
		trigger_timing_bias,
		idx,
		p_v_rec->recs[idx].id);
#endif


	/* copy data out */
	ref_idx = g_ut_vts[idx].idx;
	p_v_rec->recs[idx].vsyncs = vsyncs;
	for (i = 0; i < VSYNCS_MAX; ++i) {
		p_v_rec->recs[idx].timestamps[i] =
			g_ut_vts[idx].timestamp[ref_idx];

		ref_idx = (ref_idx + (VSYNCS_MAX - 1)) % VSYNCS_MAX;
	}


#if !defined(REDUCE_UT_DEBUG_PRINTF)
	/* for monitor global static struct data */
	ut_dump_vsync_rec(p_v_rec);
#endif
}


static unsigned int ut_setup_next_pf_ctrl_trigger_timing(void)
{
	unsigned long long trigger_timing = 0;
	unsigned int i = 0;


	/* find the earlist be triggered timing */
	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		if (g_ut_vts[i].be_triggered_at_ts == 0)
			continue;

		/* take first non-zero timing for init */
		if (trigger_timing == 0)
			trigger_timing = g_ut_vts[i].be_triggered_at_ts;

		// if (g_ut_vts[i].be_triggered_at_ts < trigger_timing)
		if (check_timestamp_b_after_a(
				g_ut_vts[i].be_triggered_at_ts,
				trigger_timing,
				TICK_FACTOR))
			trigger_timing = g_ut_vts[i].be_triggered_at_ts;
	}
	trigger_timing++;


	/* check should be trigger set ctrl or not */
	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		if (g_ut_vts[i].be_triggered_at_ts == 0)
			continue;

		g_ut_vts[i].should_trigger =
			// (g_ut_vts[i].timestamp[g_ut_vts[i].idx] < trigger_timing)
			// && (trigger_timing >= g_ut_vts[i].be_triggered_at_ts)
			((check_timestamp_b_after_a(
					g_ut_vts[i].timestamp[g_ut_vts[i].idx],
					trigger_timing,
					TICK_FACTOR))
				&& (check_timestamp_b_after_a(
					g_ut_vts[i].be_triggered_at_ts,
					trigger_timing,
					TICK_FACTOR)))
			? 1 : 0;
	}


	printf(GREEN
		"[UT setup_next_pf_ctrl_trigger_timing] trigger timing:%llu\n"
		NONE,
		trigger_timing);

	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		if (g_ut_vts[i].be_triggered_at_ts == 0)
			continue;

		printf(GREEN
			"[UT setup_next_pf_ctrl_trigger_timing] g_ut_vts[%u]: ts(at:%u):(%llu/%llu/%llu/%llu), be_triggered:(at:%llu, before:%llu), should be trigger:%u [target_ts(at:%u):(%llu/%llu/%llu/%llu)]\n"
			NONE,
			i,
			g_ut_vts[i].idx,
			g_ut_vts[i].timestamp[0],
			g_ut_vts[i].timestamp[1],
			g_ut_vts[i].timestamp[2],
			g_ut_vts[i].timestamp[3],
			g_ut_vts[i].be_triggered_at_ts,
			g_ut_vts[i].should_be_triggered_before,
			g_ut_vts[i].should_trigger,
			g_ut_vts[i].idx,
			g_ut_vts[i].target_ts[0],
			g_ut_vts[i].target_ts[1],
			g_ut_vts[i].target_ts[2],
			g_ut_vts[i].target_ts[3]);
	}


	return trigger_timing;
}


static void ut_gen_shutter_data(void)
{
	/* 0 => idx-- ; 1 => idx++ ; 2 => idx+0 */
	unsigned int exp_table_idx_dir = 0;
	unsigned int i = 0;


	if (lock_exp)
		goto SETUP_AUTO_TEST_EXP;


	exp_table_idx_dir = rand() % 3;


	/* update exp_table_idx value */
	if (exp_table_idx_dir == 1) { // idx++
		// boundary check
		if (exp_table_idx < (EXP_TABLE_SIZE - 1))
			exp_table_idx++;
	} else if (exp_table_idx_dir == 0) { // idx--
		// boundary check
		if (exp_table_idx > 0)
			exp_table_idx--;
	}


SETUP_AUTO_TEST_EXP:
	/* for normal sensor auto-run shutter value*/
	g_shutter = exp_table[exp_table_idx];


	/* for hdr auto-run shutter values */
	for (i = 0; i < FS_HDR_MAX; ++i) {
		g_hdr_shutter[i] =
			exp_table[exp_table_idx] / g_hdr_divided_ratio[i];
	}


	printf(LIGHT_BLUE
		"[UT gen_shutter_data] g_shutter:%u, g_hdr_shutter(%u/%u/%u/%u/%u), exp_table_idx_dir:%u(0:idx--/1:idx++/2:idx+0), exp_table_idx:%u, lock_exp:%u\n"
		NONE,
		g_shutter,
		g_hdr_shutter[0],
		g_hdr_shutter[1],
		g_hdr_shutter[2],
		g_hdr_shutter[3],
		g_hdr_shutter[4],
		exp_table_idx_dir,
		exp_table_idx,
		lock_exp
		);
}


static void ut_try_gen_new_shutter_data(const unsigned int idx)
{
	int ret = 0;
	unsigned int i = 0, sidx = -1;
	unsigned int should_trigger = 1;
	unsigned int valid_sync_bits = 0, request_gen_new_exp_bits = 0;


	/* error handling for input param boundary check */
	if (idx >= SENSOR_MAX_NUM) {
		printf(RED
			"[UT try_gen_new_shutter_data] ERROR: idx(%u) >= SENSOR_MAX_NUM(%u), return\n"
			NONE,
			idx, SENSOR_MAX_NUM);

		return;
	}


	/* check if new shutter data should be triggered */
	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		ret = ut_get_sidx_by_sensor_idx(i, &sidx);

		if (ret < 0) {

#if !defined(REDUCE_UT_DEBUG_PRINTF)
			printf(RED
				"[UT try_gen_new_shutter_data] ERROR: sensor idx:%u to sidx fail, continue\n"
				NONE,
				idx);
#endif // REDUCE_UT_DEBUG_PRINTF

			continue;
		}


		if (g_set_synced_sensors[i])
			valid_sync_bits |= 1U << i;

		if (g_n_1_f_cell_size[sidx] == 0) {
			if (g_sensor_request_new_exp[i])
				request_gen_new_exp_bits |= 1U << i;
		} else {
			if (g_sensor_request_new_exp[i] == g_n_1_f_cell_size[sidx])
				request_gen_new_exp_bits |= 1U << i;
		}
	}

	should_trigger = (valid_sync_bits == request_gen_new_exp_bits) ? 1 : 0;
	if (should_trigger) {
		printf(LIGHT_CYAN
			"[UT try_gen_new_shutter_data] should_trigger:%u, set_sync(%u/%u/%u/%u/%u), req_new_exp(%u/%u/%u/%u/%u), exp_triggered_cnt(%u/%u/%u/%u/%u)\n"
			NONE,
			should_trigger,
			g_set_synced_sensors[0],
			g_set_synced_sensors[1],
			g_set_synced_sensors[2],
			g_set_synced_sensors[3],
			g_set_synced_sensors[4],
			g_sensor_request_new_exp[0],
			g_sensor_request_new_exp[1],
			g_sensor_request_new_exp[2],
			g_sensor_request_new_exp[3],
			g_sensor_request_new_exp[4],
			g_sensor_exp_triggered_cnt[0],
			g_sensor_exp_triggered_cnt[1],
			g_sensor_exp_triggered_cnt[2],
			g_sensor_exp_triggered_cnt[3],
			g_sensor_exp_triggered_cnt[4]
			);


		/* clear flag of all sensors */
		for (i = 0; i < SENSOR_MAX_NUM; ++i)
			g_sensor_request_new_exp[i] = 0;

		/* call for generating new shutter data */
		ut_gen_shutter_data();
	}


	/* case checking */
	if (g_sensor_request_new_exp[idx]) {
		printf(YELLOW
			"[UT try_gen_new_shutter_data] NOTICE: request gen new exp data again! User should manually check current case/situation, idx:%u\n"
			NONE,
			idx);
	}


	/* update info */
	g_sensor_exp_triggered_cnt[idx]++;
	g_sensor_request_new_exp[idx]++;


	printf(LIGHT_CYAN
		"[UT try_gen_new_shutter_data] set_sync(%u/%u/%u/%u/%u) bits:%u, req_new_exp(%u/%u/%u/%u/%u) bits:%u, f_cell(%u/%u/%u/%u/%u), exp_triggered_cnt(%u/%u/%u/%u/%u)\n"
		NONE,
		g_set_synced_sensors[0],
		g_set_synced_sensors[1],
		g_set_synced_sensors[2],
		g_set_synced_sensors[3],
		g_set_synced_sensors[4],
		valid_sync_bits,
		g_sensor_request_new_exp[0],
		g_sensor_request_new_exp[1],
		g_sensor_request_new_exp[2],
		g_sensor_request_new_exp[3],
		g_sensor_request_new_exp[4],
		request_gen_new_exp_bits,
		g_n_1_f_cell_size[0],
		g_n_1_f_cell_size[1],
		g_n_1_f_cell_size[2],
		g_n_1_f_cell_size[3],
		g_n_1_f_cell_size[4],
		g_sensor_exp_triggered_cnt[0],
		g_sensor_exp_triggered_cnt[1],
		g_sensor_exp_triggered_cnt[2],
		g_sensor_exp_triggered_cnt[3],
		g_sensor_exp_triggered_cnt[4]
		);
}


/**
 * called by test_frame_sync_proc(), exe_fs_alg_stability_test()
 * before all processing going
 */
static inline void reset_ut_test_variables(void)
{
	unsigned int i = 0, j = 0;

	struct ut_fs_streaming_sensor_list s_sensor_clear_st = {0};
	struct ut_fs_perframe_sensor_mode_list s_sensor_mode_clear_st = {0};
	struct ut_fs_test_n_1_mode_cfg n_1_cfg_clear_st = {0};
	struct UT_Timestamp ut_vts_clear_st = {0};


	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		g_set_synced_sensors[i] = 0;

		g_streaming_sensors[i] = s_sensor_clear_st;
		g_streaming_sensors_modes_list[i] = s_sensor_mode_clear_st;

		g_sensor_mode[i] = 0;


		g_ut_vts[i].idx = 0;
		for (j = 0; j < VSYNCS_MAX; ++j) {
			g_ut_vts[i].timestamp[j] = 0;
			g_ut_vts[i].target_ts[j] = 0;
		}
		g_ut_vts[i].curr_bias = 0;
		g_ut_vts[i].next_bias = 0;


		n_1_cfg[i] = n_1_cfg_clear_st;
		g_n_1_status[i] = 0;
		g_n_1_min_fl_us[i] = 0;
		g_n_1_f_cell_size[i] = 0;


		g_sensor_exp_triggered_cnt[i] = 0;
		g_sensor_request_new_exp[i] = 0;


		g_ut_vts[i] = ut_vts_clear_st;
	}


	for (i = 0; i < TS_DIFF_TABLE_LEN; ++i)
		g_ut_ts_diff_table[i] = -1;


	exp_table_idx = 0;
	flk_en_table_idx = 0;
}


/*static*/ bool ut_check_pf_sync_result(struct vsync_rec *(p_v_rec))
{
	unsigned long long biggest_vts = 0, ts_last_idx = 0;
	unsigned int i = 0;
	unsigned int vdiff[SENSOR_MAX_NUM] = {0};
	bool is_success = true;


	/* 0. prepare vsync timestamp data for using */
	for (i = 0; i < TG_MAX_NUM; ++i) {
		ts_last_idx = g_ut_vts[i].idx;

		g_ut_vts[i].target_ts[ts_last_idx] =
			p_v_rec->recs[i].timestamps[0] +
			// p_v_rec->recs[i].timestamps[1] +
			g_ut_vts[i].curr_bias;
	}


	/* 1. find out lastest vsync timestamp */
	for (i = 0; i < TG_MAX_NUM; ++i) {
		ts_last_idx = g_ut_vts[i].idx;

		if (g_ut_vts[i].target_ts[ts_last_idx] > biggest_vts)
			biggest_vts = g_ut_vts[i].target_ts[ts_last_idx];
	}


	/* 2. check each vsync timestamp diff and */
	/*    check if sync successfully */
	for (i = 0; i < TG_MAX_NUM; ++i) {

#if defined(REDUCE_UT_DEBUG_PRINTF)
		ut_dump_ut_vts_data(i);
#endif // REDUCE_UT_DEBUG_PRINTF


		ts_last_idx = g_ut_vts[i].idx;

		if (g_ut_vts[i].target_ts[ts_last_idx] == 0) {

#if !defined(REDUCE_UT_DEBUG_PRINTF)
			printf(RED
				"[UT check_pf_sync_result] [%u] target_vts(idx:%u):(%llu/%llu/%llu/%llu)\n"
				NONE,
				i,
				ts_last_idx,
				g_ut_vts[i].target_ts[0],
				g_ut_vts[i].target_ts[1],
				g_ut_vts[i].target_ts[2],
				g_ut_vts[i].target_ts[3]);
#endif // REDUCE_UT_DEBUG_PRINTF

			continue;
		}

		vdiff[i] = biggest_vts - g_ut_vts[i].target_ts[ts_last_idx];

#if !defined(REDUCE_UT_DEBUG_PRINTF)
		printf(RED
#if defined(TS_TICK_64_BITS)
			">>> UT: [%u] vdiff:%u, biggest_vts:%llu, vts:%llu, bias(c:%u, n:%u)\n\n\n"
#else
			">>> UT: [%u] vdiff:%u, biggest_vts:%llu, vts:%u, bias(c:%u, n:%u)\n\n\n"
#endif
			NONE,
			i, vdiff[i], biggest_vts,
			p_v_rec->recs[i].timestamps[0],
			g_ut_vts[i].curr_bias,
			g_ut_vts[i].next_bias);
#endif // REDUCE_UT_DEBUG_PRINTF

		if (vdiff[i] > g_vdiff_sync_success_th)
			is_success = false;
	}


	/* 3. highlight result for checking if sync failed */
	if (is_success)
		printf(GREEN ">>> UT: PF sync PASS\n\n\n" NONE);
	else {
		printf(RED ">>> UT: PF sync FAIL\n" NONE);

		for (i = 0; i < TG_MAX_NUM; ++i) {
			printf(RED
#if defined(TS_TICK_64_BITS)
				">>> UT: [%u] tg:%u, vsyncs:%u, vdiff:%u, vts:%llu, bias(c:%u, n:%u)\n"
#else
				">>> UT: [%u] tg:%u, vsyncs:%u, vdiff:%u, vts:%u, bias(c:%u, n:%u)\n"
#endif
				NONE,
				i,
				p_v_rec->recs[i].id,
				p_v_rec->recs[i].vsyncs,
				vdiff[i],
				p_v_rec->recs[i].timestamps[0],
				g_ut_vts[i].curr_bias,
				g_ut_vts[i].next_bias);
		}
		printf("\n\n\n");
	}


	return is_success;
}


#if (WAITING_FOR_REMOVE_CODE)
static void ut_try_update_ts_diff(unsigned int idx)
{
	unsigned int i = 0, ts_cnt;
	unsigned int ts_idx, ts_idx_prev, ts_cmp_idx, ts_cmp_prev_idx;
	unsigned int ts, ts_prev, ts_cmp, ts_cmp_prev;
	unsigned int diff_l, diff_r;

	unsigned int ts_a_ordered[VSYNCS_MAX], ts_b_ordered[VSYNCS_MAX];
	int diff_new_ver;


	for (i = 0; i < SENSOR_MAX_NUM; ++i) {
		printf("[UT try_update_ts_diff] idx = %u, i = %u\n", idx, i);
#if !defined(REDUCE_UT_DEBUG_PRINTF)
		ut_dump_ut_vts_data(i);
#endif // REDUCE_UT_DEBUG_PRINTF

		if (i == idx) {
			g_ut_vts[idx].ts_diff[i] = 0;
			continue;
		}

		ts_cmp_idx =
			(g_ut_vts[i].idx + (VSYNCS_MAX-1))
			% VSYNCS_MAX;
		ts_cmp_prev_idx =
			(g_ut_vts[i].idx + (VSYNCS_MAX-2))
			% VSYNCS_MAX;

		ts_cmp = g_ut_vts[i].timestamp[ts_cmp_idx];
		ts_cmp_prev = g_ut_vts[i].timestamp[ts_cmp_prev_idx];

		if (ts_cmp == 0)
			continue;

		printf("[UT try_update_ts_diff] 2... idx = %u, i = %u\n", idx, i);
		for (ts_cnt = 1; ts_cnt < VSYNCS_MAX-1; ++ts_cnt) {
			ts_idx =
				(g_ut_vts[idx].idx + (VSYNCS_MAX-ts_cnt))
				% VSYNCS_MAX;
			ts_idx_prev =
				(g_ut_vts[idx].idx + (VSYNCS_MAX-ts_cnt-1))
				% VSYNCS_MAX;

			ts = g_ut_vts[idx].timestamp[ts_idx];
			ts_prev = g_ut_vts[idx].timestamp[ts_idx_prev];

			if (ts == 0)
				break;

			if ((check_tick_b_after_a(
					convert_timestamp_2_tick(ts_cmp, TICK_FACTOR),
					convert_timestamp_2_tick(ts, TICK_FACTOR))
				&&
				check_tick_b_after_a(
					convert_timestamp_2_tick(ts_prev, TICK_FACTOR),
					convert_timestamp_2_tick(ts_cmp_prev, TICK_FACTOR))
			)) {
				printf(RED
					"[UT try_update_ts_diff] [%u] TS cross vsync/sof [(ts_cmp:%u < ts:%u) && (ts_cmp_prev:%u > ts_prev:%u)], break. (point:[%u] idx:%u, %llu/%llu/%llu/%llu), (ref:[%u] idx:%u, %llu/%llu/%llu/%llu)\n"
					NONE,
					idx,
					ts_cmp, ts, ts_cmp_prev, ts_prev,
					idx, ts_idx,
					g_ut_vts[idx].timestamp[0],
					g_ut_vts[idx].timestamp[1],
					g_ut_vts[idx].timestamp[2],
					g_ut_vts[idx].timestamp[3],
					i, g_ut_vts[i].idx,
					g_ut_vts[i].timestamp[0],
					g_ut_vts[i].timestamp[1],
					g_ut_vts[i].timestamp[2],
					g_ut_vts[i].timestamp[3]);

				break;
			}


			if ((check_tick_b_after_a(
					convert_timestamp_2_tick(ts_cmp, TICK_FACTOR),
					convert_timestamp_2_tick(ts, TICK_FACTOR))
				&&
				check_tick_b_after_a(
					convert_timestamp_2_tick(ts_prev, TICK_FACTOR),
					convert_timestamp_2_tick(ts_cmp, TICK_FACTOR))
			)) {
				diff_l = ts_cmp - ts_prev;
				diff_r = ts - ts_cmp;

				g_ut_vts[idx].ts_diff[i] =
					(diff_r < diff_l) ? diff_r : diff_l;

				printf(LIGHT_GREEN
					"[UT try_update_ts_diff] [%u] diff:%llu(%u/%u), ts:(c:%u, t:%u, p:%u) (point:[%u] idx:%u/%u, %llu/%llu/%llu/%llu), (ref:[%u] idx:%u, %llu/%llu/%llu/%llu)\n"
					NONE,
					idx,
					g_ut_vts[idx].ts_diff[i],
					diff_l, diff_r,
					ts, ts_cmp, ts_prev,
					idx,
					ts_idx, ts_idx_prev,
					g_ut_vts[idx].timestamp[0],
					g_ut_vts[idx].timestamp[1],
					g_ut_vts[idx].timestamp[2],
					g_ut_vts[idx].timestamp[3],
					i, g_ut_vts[i].idx,
					g_ut_vts[i].timestamp[0],
					g_ut_vts[i].timestamp[1],
					g_ut_vts[i].timestamp[2],
					g_ut_vts[i].timestamp[3]);

				// found => done => terminate loop
				break;
			}

			printf(PURPLE
				"[UT try_update_ts_diff] [%u] diff:%llu(%u/%u), ts:(c:%u, t:%u, p:%u) (point:[%u] idx:%u/%u, %llu/%llu/%llu/%llu), (ref:[%u] idx:%u, %llu/%llu/%llu/%llu)\n"
				NONE,
				idx,
				g_ut_vts[idx].ts_diff[i],
				diff_l, diff_r,
				ts, ts_cmp, ts_prev,
				idx,
				ts_idx, ts_idx_prev,
				g_ut_vts[idx].timestamp[0],
				g_ut_vts[idx].timestamp[1],
				g_ut_vts[idx].timestamp[2],
				g_ut_vts[idx].timestamp[3],
				i, g_ut_vts[i].idx,
				g_ut_vts[i].timestamp[0],
				g_ut_vts[i].timestamp[1],
				g_ut_vts[i].timestamp[2],
				g_ut_vts[i].timestamp[3]);


			get_array_data_from_new_to_old(
				g_ut_vts[idx].target_ts, g_ut_vts[idx].idx-1,
				VSYNCS_MAX, ts_a_ordered);
			get_array_data_from_new_to_old(
				g_ut_vts[i].target_ts, g_ut_vts[i].idx-1,
				VSYNCS_MAX, ts_b_ordered);

			diff_new_ver = find_two_sensor_timestamp_diff(
					ts_a_ordered, ts_b_ordered,
					VSYNCS_MAX, TICK_FACTOR);

			printf(PURPLE
				"[UT try_update_ts_diff] [%u/%u] diff_new_ver:%d\n"
				NONE,
				idx, i,
				diff_new_ver);
		}
	}
}
#endif // WAITING_FOR_REMOVE_CODE


/*static*/ bool ut_check_pf_sync_result_v2(const struct vsync_rec *(p_v_rec))
{
	unsigned long long ts_a_ordered[VSYNCS_MAX], ts_b_ordered[VSYNCS_MAX];
	unsigned long long trigger_timing = (unsigned long long)(0-1);
	long long ts_diff = -1;
	unsigned int i = 0, j = 0;
	unsigned int ts_a_from_idx = 0, ts_b_from_idx = 0;
	int ret = 0, map_idx = -1;

	/* 1. try to update timestamp diff of all sensors combination */
	for (i = 0; i < SENSOR_MAX_NUM-1; ++i) {
		for (j = i+1; j < SENSOR_MAX_NUM; ++j) {
			map_idx = get_ts_diff_table_idx(i, j);

			/* for error idx checking */
			if (map_idx < 0)
				continue;
			if (map_idx >= TS_DIFF_TABLE_LEN) {
				printf(RED
					"ERROR: something error happened, map_idx:%d (bigger than TS_DIFF_TABLE_LEN:%u), abort auto continue to next run\n"
					NONE,
					map_idx, TS_DIFF_TABLE_LEN);
				continue;
			}

			/* find out current pair min be triggered timing */
			trigger_timing =
				(check_timestamp_b_after_a(
					g_ut_vts[i].be_triggered_at_ts,
					g_ut_vts[j].be_triggered_at_ts,
					TICK_FACTOR))
				? g_ut_vts[i].be_triggered_at_ts
				: g_ut_vts[j].be_triggered_at_ts;

			/* using trigger timing to check */
			/* which timestamp should be took account of */
			ts_a_from_idx =
				(check_timestamp_b_after_a(
					g_ut_vts[i].timestamp[g_ut_vts[i].idx],
					trigger_timing,
					TICK_FACTOR))
				// (trigger_timing
				//	> g_ut_vts[i].timestamp[g_ut_vts[i].idx])
				?
					// (g_ut_vts[i].idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
					g_ut_vts[i].idx % VSYNCS_MAX
				:
					(g_ut_vts[i].idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
					// g_ut_vts[i].idx % VSYNCS_MAX;

			get_array_data_from_new_to_old(
				g_ut_vts[i].target_ts,
				ts_a_from_idx,
				VSYNCS_MAX, ts_a_ordered);

			ts_b_from_idx =
				(trigger_timing
					> g_ut_vts[j].timestamp[g_ut_vts[j].idx])
				?
					// (g_ut_vts[j].idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
					g_ut_vts[j].idx % VSYNCS_MAX
				:
					(g_ut_vts[j].idx + (VSYNCS_MAX-1)) % VSYNCS_MAX;
					// g_ut_vts[j].idx % VSYNCS_MAX;

			get_array_data_from_new_to_old(
				g_ut_vts[j].target_ts,
				ts_b_from_idx,
				VSYNCS_MAX, ts_b_ordered);

#if defined(REDUCE_UT_DEBUG_PRINTF)
			printf(PURPLE
				"ts_a_ordered:(%llu/%llu/%llu/%llu, %llu/%llu/%llu/%llu, from:%u(at:%u)), ts_b_ordered:(%llu/%llu/%llu/%llu, %llu/%llu/%llu/%llu, from:%u(at:%u))\n"
				NONE,
				ts_a_ordered[0],
				ts_a_ordered[1],
				ts_a_ordered[2],
				ts_a_ordered[3],
				g_ut_vts[i].target_ts[0],
				g_ut_vts[i].target_ts[1],
				g_ut_vts[i].target_ts[2],
				g_ut_vts[i].target_ts[3],
				ts_a_from_idx,
				g_ut_vts[i].idx,
				ts_b_ordered[0],
				ts_b_ordered[1],
				ts_b_ordered[2],
				ts_b_ordered[3],
				g_ut_vts[j].target_ts[0],
				g_ut_vts[j].target_ts[1],
				g_ut_vts[j].target_ts[2],
				g_ut_vts[j].target_ts[3],
				ts_b_from_idx,
				g_ut_vts[j].idx);
#endif // REDUCE_UT_DEBUG_PRINTF

			ts_diff = find_two_sensor_timestamp_diff(
				ts_a_ordered, ts_b_ordered,
				VSYNCS_MAX, TICK_FACTOR);

			g_ut_ts_diff_table[map_idx] = (ts_diff >= 0)
				? ts_diff
				: g_ut_ts_diff_table[map_idx];

#if !defined(REDUCE_UT_DEBUG_PRINTF)
			printf(PURPLE
				"(%u,%u) => %d, :%lld, g_ut_ts_diff_table[%d]:%d\n"
				NONE,
				i, j, map_idx, ts_diff,
				map_idx, g_ut_ts_diff_table[map_idx]);
#endif // REDUCE_UT_DEBUG_PRINTF
		}
	}

	ut_dump_ts_diff_table_results();

	ret = check_sync_result(
		g_ut_ts_diff_table, 0x3FF,
		TS_DIFF_TABLE_LEN, g_vdiff_sync_success_th);

	return ret == 1;
}


static void
ut_generate_vsync_data_pf_auto(struct vsync_rec *(p_v_rec))
{
	unsigned long long biggest_vts = 0;
	unsigned int i = 0, passed_vsyncs = 0;


	passed_vsyncs = rand() % 100;
	passed_vsyncs = (passed_vsyncs < passed_vsyncs_ratio) ? 1 : 0;
	if (passed_vsyncs)
		passed_vsyncs = rand() % max_pass_cnt;
	passed_vsyncs++; // to 1~2


	/* TODO: TG_MAX_NUM here is only a number for loop running, "fix me" */
	for (i = 0; i < TG_MAX_NUM; ++i) {
		ut_gen_timestamps_data(i, passed_vsyncs, p_v_rec);

		// if (p_v_rec->recs[i].timestamps[0] > biggest_vts)
		if (check_tick_b_after_a(
			biggest_vts, p_v_rec->recs[i].timestamps[0]))
			biggest_vts = p_v_rec->recs[i].timestamps[0];
	}

	// p_v_rec->cur_tick = (biggest_vts + 100) * p_v_rec->tick_factor;
	p_v_rec->cur_tick =
		ut_setup_next_pf_ctrl_trigger_timing() * p_v_rec->tick_factor;


	printf(GREEN
#if defined(TS_TICK_64_BITS)
		"[UT generate_vsync_data_pf_auto] query ids:%u, p_v_rec->cur_tick:%llu, tick_factor:%u\n"
#else
		"[UT generate_vsync_data_pf_auto] query ids:%u, p_v_rec->cur_tick:%u, tick_factor:%u\n"
#endif
		NONE,
		p_v_rec->ids,
		p_v_rec->cur_tick,
		p_v_rec->tick_factor);


#if !defined(REDUCE_UT_DEBUG_PRINTF)
	/* for monitor global static struct data */
	ut_dump_vsync_rec(p_v_rec);
#endif


	frm_debug_set_last_vsync_data(p_v_rec);

	// printf("\n\n\n");
}


static struct vsync_rec
ut_generate_vsync_data_manually(
	struct ut_fs_streaming_sensor_list     s_list[],
	struct ut_fs_perframe_sensor_mode_list m_list[])
{
	unsigned long long biggest_vts = 0, input_ts = 0;
	unsigned int i = 0, j = 0;
	unsigned int idx = 0;
	int input = 0;

	struct vsync_rec v_rec = {0};

	v_rec.tick_factor = TICK_FACTOR;


	printf(GREEN
		"\n\n\n>>> UT Generate Vsync Data \"Manually\" <<<\n"
		NONE);
	printf(LIGHT_RED
		">>> !!! Please set Vsync data carefully !!!\n\n\n"
		NONE);


	for (i = 0; ; ++i) {
		/* print sensor which you can select */
		printf(GREEN
			"Please select bellow sensors for \"setting vsync data\"\n"
			NONE);

		/* using CLI for choise */
		input = print_and_select_sensor(s_list);

		if (input < 0) {
			printf("\n\n\n");
			break;
		}

		idx = input;



		/* 1. set tg */
		v_rec.ids++;
		// v_rec.recs[idx].id = s_list[idx].sensor->tg;
		v_rec.recs[idx].id = g_streaming_sensors[i].tg;


		/* 2. set passed Vsyncs count */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set passed vsync count (min:1)\" : "
			NONE);
		scanf("%d", &input);

		if (input <= 0) {
			printf(RED
				">>> You set vsync passed count less than 1, so auto force to 1 !\n"
				NONE);

			// force vsync count to 1.
			input = 1;
		}

		v_rec.recs[idx].vsyncs = input;


		/* 3. set lastest Vsync timestamp value */
		// for (unsigned int i = 0; i < VSYNCS_MAX; ++i) {
		for (j = 0; j < 1; ++j) {
			printf(LIGHT_PURPLE
				">>> (Input 1 integer) \"timestamps[%d]\" : "
				NONE, j);
			scanf("%llu", &input_ts);
			v_rec.recs[idx].timestamps[j] = input_ts;
			g_ut_vts[idx].timestamp[j] = input_ts;
			g_ut_vts[idx].target_ts[j] = input_ts;
			g_ut_vts[idx].be_triggered_at_ts =
				g_ut_vts[idx].timestamp[0] + UT_SENSOR_TRIGGER_BIAS;

			//if (input > biggest_vts)
			if (check_tick_b_after_a(biggest_vts, input_ts))
				biggest_vts = input_ts;
		}


		printf("\n\n\n");
	}


#ifdef MANUALLY_SET_CURRENT_TIMESTAMP

	printf(GREEN
		"Please reference bellow timestamp to \"determine and input current timestamp\"\n"
		NONE);
	for (j = 0; s_list[j].sensor != NULL; ++j) {
		printf(GREEN
			"[%d] %s, last Vsync timestamp:%llu\n"
			NONE,
			j, s_list[j].sensor_name, v_rec.recs[j].timestamps[0]);
	}

	printf(LIGHT_PURPLE
		">>> (Input 1 integer) \"set current timestamp\" : "
		NONE);
	scanf("%llu", &input_ts);

	v_rec.cur_tick = input_ts * v_rec.tick_factor;

#else // END manual set current timestamp

	/* (biggest_vts + 1) => prevent overflow */
	v_rec.cur_tick =
		// (biggest_vts + 1) * v_rec.tick_factor;
		ut_setup_next_pf_ctrl_trigger_timing() * v_rec.tick_factor;

#endif // MANUALLY_SET_CURRENT_TIMESTAMP

	// printf("[0] tg:%u, [1] tg:%u, [2] tg:%u\n",
	//	v_rec.recs[0].id,
	//	v_rec.recs[1].id,
	//	v_rec.recs[2].id);

	printf(GREEN
		">>> frame monitor set vsync data, call frm_debug_set_last_vsync_data()...\n\n\n"
		NONE);

	frm_debug_set_last_vsync_data(&v_rec);


	printf("\n\n\n");

	return v_rec;
}


static int ut_set_fs_set_shutter_select_sensor_manually(int *select);
static void ut_preset_fs_update_shutter(void)
{
	struct fs_perframe_st pf_ctrl = {0};
	unsigned int ret = 0;
	unsigned int mode_idx;
	unsigned int hdr_mode = 0;
	unsigned int ae_exp_cnt = 0;
	unsigned int exp_i = 0;
	int select = 2147483647;
	int input;


	/* set preset perframe data */
	ret = ut_set_fs_set_shutter_select_sensor_manually(
			&select);
	if (ret != 0)
		return;


	/* print sensor mode which you can select */
	printf(GREEN
		"Please select a sensor mode bellow :\n"
		NONE);

	/* using CLI for choise */
	mode_idx = print_and_select_s_mode(select);
	set_pf_ctrl_s_mode(select, mode_idx, &pf_ctrl);

	printf(LIGHT_PURPLE
		">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"flicker_en\" : "
		NONE,
		select, pf_ctrl.sensor_id, pf_ctrl.sensor_idx);
	scanf("%d", &input);

	pf_ctrl.flicker_en = input;

	printf(LIGHT_PURPLE
		">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"min_fl (us)\" : "
		NONE,
		select, pf_ctrl.sensor_id, pf_ctrl.sensor_idx);
	scanf("%d", &input);

	pf_ctrl.min_fl_lc =
		US_TO_LC(input, pf_ctrl.lineTimeInNs);

	printf(LIGHT_PURPLE
		">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"shutter (us)\" : "
		NONE,
		select, pf_ctrl.sensor_id, pf_ctrl.sensor_idx);
	scanf("%d", &input);

	pf_ctrl.shutter_lc =
		US_TO_LC(input, pf_ctrl.lineTimeInNs);

	hdr_mode = g_streaming_sensors_modes_list[select]
			.mode_list[mode_idx].hdr_exp.mode_exp_cnt;

	printf(GREEN
		">>> HDR:exp[] => LE:[0] / ME:[1] / SE:[2] / SSE:[3] / SSSE:[4], mode_exp_cnt:%u\n"
		NONE,
		hdr_mode);

	printf(LIGHT_PURPLE
		">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"AE exp cnt\" : "
		NONE,
		select, pf_ctrl.sensor_id, pf_ctrl.sensor_idx);
	scanf("%u", &ae_exp_cnt);

	for (exp_i = 0; exp_i < ae_exp_cnt; ++exp_i) {
		int hdr_idx = 0;

		hdr_idx = hdr_exp_idx_map[ae_exp_cnt][exp_i];

		printf(LIGHT_PURPLE
			">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"HDR exp[%u] (us)\" : "
			NONE,
			select, pf_ctrl.sensor_id,
			pf_ctrl.sensor_idx, exp_i);
		scanf("%d", &input);

		pf_ctrl.hdr_exp.exp_lc[hdr_idx] =
			US_TO_LC(input, pf_ctrl.lineTimeInNs);
	}

	frameSync->fs_update_shutter(&pf_ctrl);
}

/**
 * called by test_frame_sync_proc()
 * for step by step setting to test frame-sync
 */
static void ut_set_fs_streaming(void)
{
	int select = 2147483647, input;

	unsigned int idx = 0, tg = 0;
	unsigned int i = 0, j = 0;


	printf("\n\n\n");

	/* for fs streaming..., streaming on sensors */
	for (i = 0; ; ++i) {
		struct SensorInfo reg_info;

		/* print sensor info which in ut_fs_streaming_info.h */
		printf(LIGHT_CYAN ">>> fs_streaming() ...\n" NONE);
		printf(GREEN
			"Please select bellow sensors for \"streaming on\"\n"
			NONE);

		/* using CLI for choise */
		select = print_and_select_sensor(ut_fs_s_list);

		if (select < 0) { // => END streaming on sensor
			g_streaming_sensors[i].sensor_name = "NULL";
			g_streaming_sensors[i].sensor = NULL;

			g_streaming_sensors_modes_list[i]
						.sensor_name = "NULL";
			g_streaming_sensors_modes_list[i]
						.mode_list = NULL;

			printf("\n\n\n");

			break;
		}


		/* set sensor idx which you select on bellow step */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set sensor_idx\" : "
			NONE);
		scanf("%d", &input);
		idx = input;

		g_streaming_sensors[i].sensor_name =
					ut_fs_s_list[select].sensor_name;

		g_streaming_sensors[i].sensor =
					ut_fs_s_list[select].sensor;

		g_streaming_sensors[i].sensor->sensor_idx = idx;

		/* !!! Becareful !!! */
		/* prevent choose same sensor with different sensor idx */
		/* fs_set_sync() will choose wrong sensor idx */
		/* because "fs_streaming_st" memory address is the same */
		g_streaming_sensors[i].sensor_idx = idx;

		fs_setup_sensor_info_st_by_fs_streaming_st(
			g_streaming_sensors[i].sensor, &reg_info);
		frameSync->fs_register_sensor(&reg_info, REGISTER_METHOD);


		/* set sensor tg */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set TG\" : "
			NONE);
		scanf("%d", &input);
		tg = input;

		// g_streaming_sensors[i].sensor->tg = tg;
		g_streaming_sensors[i].sensor->cammux_id = tg;
		g_streaming_sensors[i].sensor->target_tg = CAMMUX_ID_INVALID;

		/* !!! Becareful !!! */
		/* prevent choose same sensor with different tg */
		/* fs aglo, fs monitor will be error */
		/* because "fs_streaming_st" memory address is the same */
		/* for UT frm set timestamp data, sync process to convert TG */
		g_streaming_sensors[i].tg =
			frm_convert_cammux_id_to_ccu_tg_id(tg);



		/* query sensor mode info for ut test using */
		/* (in ut_fs_perframe_ctrl_info.h file) */
		for (j = 0; ut_fs_pf_s_mode_list[j].mode_list != NULL; ++j) {
			if (strcmp(
				g_streaming_sensors[i].sensor_name,
				ut_fs_pf_s_mode_list[j].sensor_name) == 0) {

				g_streaming_sensors_modes_list[i].sensor_name =
					ut_fs_pf_s_mode_list[j].sensor_name;

				g_streaming_sensors_modes_list[i].sensor_idx = idx;

				g_streaming_sensors_modes_list[i].mode_list =
					ut_fs_pf_s_mode_list[j].mode_list;
			}
		}


		/* set sensor sync mode (SW:NONE / HW Master / HW Slave) */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set HW sync mode(NONE:0/HW:Master:1/HW:Slave:2)\" : "
			NONE);
		scanf("%d", &input);
		g_streaming_sensors[i].sensor->sync_mode = input;


		/* set sensor sync group ID */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set HW sync group ID (MIN:%d/MAX:%d)\" : "
			NONE,
			FS_HW_SYNC_GROUP_ID_MIN,
			FS_HW_SYNC_GROUP_ID_MAX);
		scanf("%d", &input);
		g_streaming_sensors[i].sensor->hw_sync_group_id = input;


		/* set preset perframe data */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set preset perframe data, Yes:1 / NO:0\" : "
			NONE);
		scanf("%d", &input);
		if (input)
			ut_preset_fs_update_shutter();


		/* set whether streaming on or not */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set streaming ON:1 / OFF:0\" : "
			NONE);
		scanf("%d", &input);

		if (input > 0) {
			printf(GREEN
				">>> framesync streaming ON, call fs_streaming(1, )...\n\n\n"
				NONE);

			frameSync->fs_streaming(1, g_streaming_sensors[i].sensor);
		} else {
			printf(GREEN
				">>> framesync streaming OFF, call fs_streaming(0, )...\n\n\n"
				NONE);

			frameSync->fs_streaming(0, g_streaming_sensors[i].sensor);
		}
		printf("\n\n\n");
	}
	/* END for fs streaming..., streaming on sensors */
}


/**
 * called by test_frame_sync_proc()
 * for step by step setting to test frame-sync
 */
static void ut_set_fs_set_sync(void)
{
	int select = 2147483647, input;

	unsigned int i = 0, alg_method = 0;


	alg_method = ut_auto_choose_fs_alg_method();
	frameSync->fs_set_using_sa_mode(alg_method);

	/* for fs_set_sync(), set sensor for doing frame sync */
	for (i = 0; ; ++i) {
		/* print sensor which you can select */
		printf(LIGHT_CYAN ">>> fs_set_sync() ...\n" NONE);
		printf(GREEN
			"Please select bellow sensors for \"doing frame sync\"\n"
			NONE);

		/* using CLI for choise */
		select = print_and_select_sensor(g_streaming_sensors);

		if (select < 0) { // => END set sensor sync
			printf("\n\n\n");
			break;
		}


		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"set sync (Yes:1 / No:0)\" : "
			NONE);
		scanf("%d", &input);
		g_set_synced_sensors[select] = input;

		if (input > 0) {
			printf(GREEN
				">>> framesync set sync, call fs_set_sync( ,1)...\n\n\n"
				NONE);
		} else {
			printf(GREEN
				">>> framesync unset sync, call fs_set_sync( ,0)...\n\n\n"
				NONE);
		}


		switch (REGISTER_METHOD) {
		case BY_SENSOR_ID:
			frameSync->fs_set_sync(
				g_streaming_sensors[select].sensor->sensor_id,
				g_set_synced_sensors[select]);

			break;

		case BY_SENSOR_IDX:
			frameSync->fs_set_sync(
				g_streaming_sensors[select].sensor_idx,
				g_set_synced_sensors[select]);

			break;

		default:
			frameSync->fs_set_sync(
				g_streaming_sensors[select].sensor_idx,
				g_set_synced_sensors[select]);

			break;
		}

		printf("\n\n\n");
	}
	/* END for fs_set_sync(), set sensor for doing frame sync */
}


static inline void
ut_set_fs_update_auto_flicker_mode(struct fs_perframe_st *pf_ctrl)
{
	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		frameSync->fs_update_auto_flicker_mode(
				pf_ctrl->sensor_id,
				pf_ctrl->flicker_en);

		break;

	case BY_SENSOR_IDX:
		frameSync->fs_update_auto_flicker_mode(
				pf_ctrl->sensor_idx,
				pf_ctrl->flicker_en);

		break;

	default:
		frameSync->fs_update_auto_flicker_mode(
				pf_ctrl->sensor_idx,
				pf_ctrl->flicker_en);

		break;
	}
}


static void ut_set_fs_update_min_fl_lc(const struct fs_perframe_st *pf_ctrl)
{
	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		frameSync->fs_update_min_fl_lc(
			pf_ctrl->sensor_id, pf_ctrl->min_fl_lc,
			pf_ctrl->min_fl_lc,
			pf_ctrl->hdr_exp.fl_lc, FS_HDR_MAX);

		break;
	case BY_SENSOR_IDX:
	default:
		frameSync->fs_update_min_fl_lc(
			pf_ctrl->sensor_idx, pf_ctrl->min_fl_lc,
			pf_ctrl->min_fl_lc,
			pf_ctrl->hdr_exp.fl_lc, FS_HDR_MAX);

		break;
	}
}


static inline void
ut_auto_set_anti_flicker(struct fs_perframe_st *pf_ctrl)
{
	if ((pf_ctrl->sensor_idx == 0) && (!lock_flk)) {
		// only main cam
		pf_ctrl->flicker_en =
			flk_en_table[flk_en_table_idx];

		flk_en_table_idx = (flk_en_table_idx + 1) % 10;
	} else
		pf_ctrl->flicker_en = 0;
}


static inline void ut_setup_n_1_cfg(
	struct ut_fs_test_n_1_mode_cfg *n_1_cfg_arr)
{
	unsigned int i = 0;
	struct ut_fs_test_n_1_mode_cfg clear_st = {0};


	if (n_1_cfg_arr == NULL) {
		for (i = 0; i < SENSOR_MAX_NUM; ++i)
			n_1_cfg[i] = clear_st;

		return;
	}

	for (i = 0;
		(n_1_cfg_arr[i].turn_on_at_n_run != 0
			&& n_1_cfg_arr[i].turn_off_at_n_run != 0); ++i) {

		n_1_cfg[i] = n_1_cfg_arr[i];
	}
}


static void ut_chk_and_set_async_master(const unsigned int sidx,
	const unsigned int en)
{
	if (sidx > 0)
		frameSync->fs_sa_set_user_async_master(sidx-1, en);
}


static inline void ut_check_turn_on_off_n_1_mode(unsigned int run_times)
{
	unsigned int i = 0, iidx = -1;
	int sensor_id = -1;


	for (i = 0;
		((i < SENSOR_MAX_NUM) &&
		((n_1_cfg[i].turn_on_at_n_run != 0)
		&& (n_1_cfg[i].turn_off_at_n_run != 0)));
		++i) {

		sensor_id = ut_get_sid_by_sensor_idx(
				n_1_cfg[i].sensor_idx, &iidx);

		if (sensor_id < 0) {
			printf(RED
				"ERROR: get sensor_id failed, i:%u, sensor_idx:%u\n"
				NONE,
				i, n_1_cfg[i].sensor_idx
			);

			continue;
		}


#if defined(REDUCE_UT_DEBUG_PRINTF)
		printf(GREEN
			"[UT check_turn_on_off_n_1_mode] run_times:%u, i:%u[sidx:%u/ON:%u/OFF:%u], ID:%#x/iidx:%u\n"
			NONE,
			run_times, i,
			n_1_cfg[i].sensor_idx,
			n_1_cfg[i].turn_on_at_n_run,
			n_1_cfg[i].turn_off_at_n_run,
			sensor_id,
			iidx
		);
#endif


		switch (REGISTER_METHOD) {
		case BY_SENSOR_ID:
			if (n_1_cfg[i].turn_on_at_n_run == run_times) {
				frameSync->fs_n_1_en(sensor_id,
						n_1_cfg[i].n, 1);


				g_n_1_status[iidx] = 1; // 1 for ON
				g_n_1_min_fl_us[iidx] = n_1_cfg[i].on_min_fl_us;
				g_n_1_f_cell_size[iidx] = n_1_cfg[i].n;

			} else if (n_1_cfg[i].turn_off_at_n_run == run_times) {
				frameSync->fs_n_1_en(sensor_id,
						n_1_cfg[i].n, 0);


				g_n_1_status[iidx] = 2; // 2 for OFF
				g_n_1_min_fl_us[iidx] = n_1_cfg[i].off_min_fl_us;
				g_n_1_f_cell_size[iidx] = 0;
			}

			break;

		case BY_SENSOR_IDX:
		default:
			if (n_1_cfg[i].turn_on_at_n_run == run_times) {
				frameSync->fs_n_1_en(n_1_cfg[i].sensor_idx,
						n_1_cfg[i].n, 1);


				g_n_1_status[iidx] = 1; // 1 for ON
				g_n_1_min_fl_us[iidx] = n_1_cfg[i].on_min_fl_us;
				g_n_1_f_cell_size[iidx] = n_1_cfg[i].n;

			} else if (n_1_cfg[i].turn_off_at_n_run == run_times) {
				frameSync->fs_n_1_en(n_1_cfg[i].sensor_idx,
						n_1_cfg[i].n, 0);


				g_n_1_status[iidx] = 2; // 2 for OFF
				g_n_1_min_fl_us[iidx] = n_1_cfg[i].off_min_fl_us;
				g_n_1_f_cell_size[iidx] = 0;
			}

			break;
		}
	}
}


/******************************************************************************/
// return:
//     1: continue
//    -1: break
//     0: do nothing
/******************************************************************************/
static int ut_set_fs_set_shutter_select_sensor_manually(int *select)
{
	unsigned int input = 0;


	/* select a sensor which you will set it in bellow flow */
	/* print sensor which you can select */
	printf(GREEN
		"Please select bellow sensors for \"setting shutter (perframe_st)\"\n"
		NONE);

	/* using CLI for choise */
	*select = print_and_select_sensor(g_streaming_sensors);

	/* all fs_shutter() has been set, call fs_sync_frame(0) */
	if (*select < 0) {
		printf(LIGHT_PURPLE
			"FrameSync per-frame ctrl process, framesync sync frame(0)...\n\n\n"
			NONE);

		frameSync->fs_sync_frame(0);

		printf("\n\n");



		/* check if you want to go next run */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) Next run? (Yes:1 / No:0) : "
			NONE);
		scanf("%d", &input);

		/* need not to go for next run, so break the for loop */
		if (input == 0)
			return -1;


		/* generate vsync data */
		printf(LIGHT_PURPLE
			">>> (Input 1 integer) generate vsync data \"pf auto:1\" OR \"manually:0\" : "
			NONE);
		scanf("%d", &input);

		if (input > 0) {
			ut_generate_vsync_data_pf_auto(
				&g_v_rec);
		} else {
			g_v_rec = ut_generate_vsync_data_manually(
				g_streaming_sensors,
				g_streaming_sensors_modes_list);
		}

		printf(GREEN
			">>> FrameSync per-frame ctrl process, framesync sync frame(1)...\n\n\n"
			NONE);

		frameSync->fs_sync_frame(1);

		printf("\n\n");

		return 1;
	}

	printf("\n\n");
	return 0;
}


static int ut_fs_ctrl_request_setup_basic_pf_ctrl_data(
	const unsigned int idx,
	int *p_user_select_idx,	struct fs_perframe_st *p_pf_ctrl)
{
	int ret = 0, input_s_mode = 0;

	if (!g_auto_run) {
		ret = ut_set_fs_set_shutter_select_sensor_manually(
			p_user_select_idx);

		if (ret != 0)
			return ret;


		/* print sensor mode which you can select */
		printf(GREEN
			"Please select a sensor mode bellow :\n"
			NONE);

		/* using CLI for choise */
		input_s_mode = print_and_select_s_mode(*p_user_select_idx);

		set_pf_ctrl_s_mode(
			*p_user_select_idx, input_s_mode, p_pf_ctrl);

	} else {
		/* auto run will use mode data have been prepared */
		ret = 0;
		set_pf_ctrl_s_mode(idx, g_sensor_mode[idx], p_pf_ctrl);
	}

	return ret;
}


static void ut_notify_vsync(void)
{
	int user_select_idx = 2147483647, /*input = 0,*/ ret = 0;
	unsigned int i = 0;


	for (i = 0; ; ++i) {
		if (g_auto_run && g_streaming_sensors[i].sensor == NULL)
			break;

		if (g_auto_run && !g_ut_vts[i].should_trigger)
			continue;

		struct fs_perframe_st pf_ctrl = {0};


		/* 0. setup basic sensor info */
		/*    => setup some perframe_st data for below ctrl using */
		ret = ut_fs_ctrl_request_setup_basic_pf_ctrl_data(
			i, &user_select_idx, &pf_ctrl);

		if (ret == 1)
			continue;
		else if (ret == -1)
			break;


		printf("\n\n\n");
		printf(GREEN
			"[UT notify vsync] i:%u, sensor_id:%#x, sensor_idx:%u\n"
			NONE,
			i, pf_ctrl.sensor_id, pf_ctrl.sensor_idx);


		/* UT test: update debug info --- sof cnt */
		switch (REGISTER_METHOD) {
		case BY_SENSOR_ID:
			frameSync->fs_notify_vsync(pf_ctrl.sensor_id);
			break;

		case BY_SENSOR_IDX:
			frameSync->fs_notify_vsync(pf_ctrl.sensor_idx);
			break;

		default:
			printf(
				"\n=== Run in defalut case, not assign register method ===\n");
			break;
		}

		if (!g_auto_run)
			printf("\n\n");
	}
}


static void ut_fs_ctrl_request_setup_anti_flicker(
	const int user_select_idx,
	struct fs_perframe_st *p_pf_ctrl)
{
	int input = 0;

	if (!g_auto_run) {
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"flicker_en\" : "
			NONE,
			user_select_idx, p_pf_ctrl->sensor_id, p_pf_ctrl->sensor_idx);
		scanf("%d", &input);

		p_pf_ctrl->flicker_en = input;

	} else {
		/* TODO: let the behavior of auto set anti flicker like ISP7 */
		ut_auto_set_anti_flicker(p_pf_ctrl);
	}
}


static void ut_fs_ctrl_request_setup_max_frame_rate(
	const unsigned int idx, const int user_select_idx,
	struct fs_perframe_st *p_pf_ctrl)
{
	int input = 0;

	if (!g_auto_run) {
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"min_fl (us)\" : "
			NONE,
			user_select_idx, p_pf_ctrl->sensor_id, p_pf_ctrl->sensor_idx);
		scanf("%d", &input);

		p_pf_ctrl->min_fl_lc =
			US_TO_LC(input, p_pf_ctrl->lineTimeInNs);

		/* default using ctx->subctx.frame_length => sensor current fl_lc */
		p_pf_ctrl->out_fl_lc = p_pf_ctrl->min_fl_lc;

	} else {
		/* TODO: change max frame rate => is it well for checking at this API */
		if ((g_n_1_status[idx] == 1) || (g_n_1_status[idx] == 2)) {
			p_pf_ctrl->min_fl_lc =
				US_TO_LC(g_n_1_min_fl_us[idx],
					p_pf_ctrl->lineTimeInNs);

			/* default using ctx->subctx.frame_length => sensor current fl_lc */
			p_pf_ctrl->out_fl_lc = p_pf_ctrl->min_fl_lc;
		} else {
			p_pf_ctrl->min_fl_lc =
				g_streaming_sensors_modes_list[idx]
					.mode_list[g_sensor_mode[idx]]
					.min_fl_lc;

			/* default using ctx->subctx.frame_length => sensor current fl_lc */
			p_pf_ctrl->out_fl_lc = p_pf_ctrl->min_fl_lc;
		}
	}
}


static void ut_fs_ctrl_request_setup_do_ae_ctrl(
	const unsigned int idx, const int user_select_idx,
	struct fs_perframe_st *p_pf_ctrl)
{
	int input = 0;
	unsigned int hdr_mode = 0, ae_exp_cnt = 0, exp_i = 0;

	/* setup normal shutter time (us)*/
	if (!g_auto_run) {
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"shutter (us)\" : "
			NONE,
			user_select_idx, p_pf_ctrl->sensor_id, p_pf_ctrl->sensor_idx);
		scanf("%d", &input);

		p_pf_ctrl->shutter_lc =
			US_TO_LC(input, p_pf_ctrl->lineTimeInNs);

	} else {
		p_pf_ctrl->shutter_lc =
			US_TO_LC(g_shutter, p_pf_ctrl->lineTimeInNs);
	}

	/* setup hdr shutter time (us) */
	if (!g_auto_run) {
		hdr_mode =
			g_streaming_sensors_modes_list[user_select_idx]
				.mode_list->hdr_exp.mode_exp_cnt;

		printf(GREEN
			">>> HDR:exp[] => LE:[0] / ME:[1] / SE:[2] / SSE:[3] / SSSE:[4], mode_exp_cnt:%u\n"
			NONE,
			hdr_mode);

		printf(LIGHT_PURPLE
			">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"AE exp cnt\" : "
			NONE,
			user_select_idx, p_pf_ctrl->sensor_id, p_pf_ctrl->sensor_idx);
		scanf("%u", &ae_exp_cnt);

	} else {
		hdr_mode =
			g_streaming_sensors_modes_list[idx]
				.mode_list->hdr_exp.mode_exp_cnt;

		ae_exp_cnt =
			g_streaming_sensors_modes_list[idx]
				.mode_list->hdr_exp.ae_exp_cnt;
	}

	for (exp_i = 0; exp_i < ae_exp_cnt; ++exp_i) {
		int hdr_idx = 0;

		hdr_idx = hdr_exp_idx_map[ae_exp_cnt][exp_i];

		if (!g_auto_run) {
			printf(LIGHT_PURPLE
				">>> (Input 1 integers) [%d] ID:%#x (sidx:%u), set \"HDR exp[%u] (us)\" : "
				NONE,
				user_select_idx, p_pf_ctrl->sensor_id,
				p_pf_ctrl->sensor_idx, exp_i);
			scanf("%d", &input);

			p_pf_ctrl->hdr_exp.exp_lc[hdr_idx] =
				US_TO_LC(input, p_pf_ctrl->lineTimeInNs);

		} else {
			p_pf_ctrl->hdr_exp.exp_lc[hdr_idx] =
				US_TO_LC(
					g_hdr_shutter[hdr_idx],
					p_pf_ctrl->lineTimeInNs);
		}
		// printf("hdr_idx:%u, exp:%u\n", hdr_idx, p_pf_ctrl->hdr_exp.exp_lc[hdr_idx]);
	}
}


static void ut_fs_set_debug_info_sof_cnt(struct fs_perframe_st *p_pf_ctrl)
{
	/* UT test: update debug info --- sof cnt */
	switch (REGISTER_METHOD) {
	case BY_SENSOR_ID:
		frameSync->fs_set_debug_info_sof_cnt(
			p_pf_ctrl->sensor_id, g_counter);
		break;

	case BY_SENSOR_IDX:
		frameSync->fs_set_debug_info_sof_cnt(
			p_pf_ctrl->sensor_idx, g_counter);
		break;

	default:
		printf(
			"\n=== Run in defalut case, not assign register method ===\n");
		break;
	}
}


static void ut_trigger_ext_ctrl(struct fs_perframe_st *p_pf_ctrl)
{
	struct fs_seamless_st seamless_info = {0};
	unsigned int i = 0;

	for (i = 0;
		(g_ext_ctrls[i].do_ext_fl_at_n_run != 0 ||
		g_ext_ctrls[i].do_seamless_switch_at_n_run != 0); ++i) {

		printf(GREEN
			"Do ext_ctrl at => g_ext_ctrls.ext_at:%u/seamless_at:%u, g_counter:%u\n"
			NONE,
			g_ext_ctrls[i].do_ext_fl_at_n_run,
			g_ext_ctrls[i].do_seamless_switch_at_n_run,
			g_counter);

		switch (REGISTER_METHOD) {
		case BY_SENSOR_IDX:
		default:
			if (g_ext_ctrls[i].do_ext_fl_at_n_run == g_counter) {
				frameSync->fs_set_extend_framelength(
						g_ext_ctrls[i].sensor_idx,
						0,
						g_ext_ctrls[i].ext_fl_us);
			}


			if (g_ext_ctrls[i].do_seamless_switch_at_n_run == g_counter) {
				printf(GREEN
					"[UT seamless ctrl] call seamless switch, g_counter:%u\n"
					NONE,
					g_counter);

				seamless_info.seamless_pf_ctrl = *p_pf_ctrl;
				seamless_info.fl_active_delay = 2;
				seamless_info.prop.orig_readout_time_us =
					p_pf_ctrl->readout_time_us;
				seamless_info.prop.prsh_length_lc =
					p_pf_ctrl->hdr_exp.exp_lc[0] + 100;

				/* test seamless switch procedure */
				switch (REGISTER_METHOD) {
				case BY_SENSOR_ID:
					frameSync->fs_seamless_switch(p_pf_ctrl->sensor_id,
							&seamless_info, g_counter);

					break;
				case BY_SENSOR_IDX:
					frameSync->fs_seamless_switch(p_pf_ctrl->sensor_idx,
							&seamless_info, g_counter);

					break;
				default:
					printf(
						"\n=== Run in defalut case, not assign register method ===\n");
					break;
				}

			} else {

				printf(GREEN
					"[UT vsync notify --- seamless ctrl] call chk exit seamless switch frame, g_counter:%u\n"
					NONE,
					g_counter);

				switch (REGISTER_METHOD) {
				case BY_SENSOR_ID:
					frameSync->fs_chk_exit_seamless_switch_frame(
						p_pf_ctrl->sensor_id);

					break;
				case BY_SENSOR_IDX:
					frameSync->fs_chk_exit_seamless_switch_frame(
						p_pf_ctrl->sensor_idx);

					break;
				default:
					printf(
						"\n=== Run in defalut case, not assign register method ===\n");
					break;
				}

			}
			break;
		}
	}
}


static void ut_ctrl_request_setup(void)
{
	int user_select_idx = 2147483647, /*input = 0,*/ ret = 0;
	unsigned int target_min_fl_us, out_fl_us;
	unsigned int i = 0;


	for (i = 0; ; ++i) {
		if (g_auto_run && g_streaming_sensors[i].sensor == NULL)
			break;

		if (g_auto_run && !g_ut_vts[i].should_trigger)
			continue;

		struct fs_perframe_st pf_ctrl = {0};


		/* 0. setup basic sensor info */
		/*    => setup some perframe_st data for below ctrl using */
		ret = ut_fs_ctrl_request_setup_basic_pf_ctrl_data(
			i, &user_select_idx, &pf_ctrl);

		if (ret == 1)
			continue;
		else if (ret == -1)
			break;


#if defined(REDUCE_UT_DEBUG_PRINTF)
		printf("\n\n\n");
		printf(GREEN
			"[UT ctrl_request_setup] i:%u, sensor_id:%#x, sensor_idx:%u\n"
			NONE,
			i, pf_ctrl.sensor_id, pf_ctrl.sensor_idx
		);
#endif // REDUCE_UT_DEBUG_PRINTF


		ut_fs_set_debug_info_sof_cnt(&pf_ctrl);


		/* 0. try trigger for gen new shutter data */
		ut_try_gen_new_shutter_data(pf_ctrl.sensor_idx);

		/* 1. setup anti-flicker */
		ut_fs_ctrl_request_setup_anti_flicker(
			user_select_idx, &pf_ctrl);

		/* 2. setup max frame rate / min framelength */
		ut_fs_ctrl_request_setup_max_frame_rate(
			i, user_select_idx, &pf_ctrl);

		/* 3. set shutter time (us) */
		ut_fs_ctrl_request_setup_do_ae_ctrl(
			i, user_select_idx, &pf_ctrl);


#if defined(USING_PRIVATE_CTRL_ORDER)
		/* N-2. call fs_update_auto_flicker_mode() */
		ut_set_fs_update_auto_flicker_mode(&pf_ctrl);

		/* N-1. call fs_update_min_framelength_lc() */
		ut_set_fs_update_min_fl_lc(&pf_ctrl);
#endif // USING_PRIVATE_CTRL_ORDER


		/* N. call fs_set_shutter() */
		if (!g_auto_run)
			printf("\n\n");

		frameSync->fs_set_shutter(&pf_ctrl);


		/* test MW query fl record info */
		switch (REGISTER_METHOD) {
		case BY_SENSOR_ID:
			frameSync->fs_get_fl_record_info(
				pf_ctrl.sensor_id,
				&target_min_fl_us, &out_fl_us);
			break;

		case BY_SENSOR_IDX:
			frameSync->fs_get_fl_record_info(
				pf_ctrl.sensor_idx,
				&target_min_fl_us, &out_fl_us);
			break;

		default:
			printf(RED
				"WARNING: Run in default case, unexpected\n"
				NONE);
			break;
		}
		printf("\n");
		printf(GREEN
			"[UT ctrl_request_setup] i:%u, sensor_id:%#x, sensor_idx:%u, target_min_fl_us:%u, out_fl_us:%u\n"
			NONE,
			i, pf_ctrl.sensor_id, pf_ctrl.sensor_idx,
			target_min_fl_us,
			out_fl_us);


		if (!g_auto_run)
			printf("\n\n");


		ut_trigger_ext_ctrl(&pf_ctrl);
	}
}


static void ut_trigger_pf_ctrl_manually(void)
{
	/* for trigger frame sync PF CTRL */
	printf(GREEN
		">>> FrameSync per-frame ctrl process, framesync sync frame(1)...\n\n\n"
		NONE);

	frameSync->fs_sync_frame(1);

	printf("\n\n\n");

	/* ut call fs_set_shutter() */
	ut_ctrl_request_setup();
}


static void ut_trigger_pf_ctrl_auto_run_normal(void)
{
	unsigned int i = 0;

	bool first_sync = false, break_sync = false, re_sync = false;
	bool pf_result = false;

	int c = 0;
	unsigned int break_sync_count = 0, re_sync_count = 0;

	bool is_ut_pass = false;


	if (g_fs_alg_stability_test_flag)
		goto RUN_PF_CTRL_AUTO_NORMAL;


	/* set auto run times */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) auto run times (suggest min:10000) : "
		NONE);
	scanf("%u", &g_run_times);

	while (g_run_times < 0) {
		printf(RED
			">>> g_run_times is a negative value, please input it again !\n"
			NONE);

		/* set auto run times */
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) auto run times (suggest min:10000) : "
			NONE);
		scanf("%u", &g_run_times);
	}
	// printf("\n\n\n");


	/* set vdiff sync success threshold */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) \"set sync helper TH\" (determin sync successfully or not, def:1000) : "
		NONE);
	scanf("%u", &g_vdiff_sync_success_th);
	g_vdiff_sync_success_th = (g_vdiff_sync_success_th > 0)
		? g_vdiff_sync_success_th
		: VDIFF_SYNC_SUCCESS_TH;


	/* set simulation for passed more than one vsync */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) simulation passed vsync(s) (Y:1 / N:0): "
		NONE);
	scanf("%u", &simulation_passed_vsyncs);
	if (simulation_passed_vsyncs) {
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) passed vsync(s) ratio (0-100) : "
			NONE);
		scanf("%u", &passed_vsyncs_ratio);
		if (passed_vsyncs_ratio > 100)
			passed_vsyncs_ratio = 100;


		printf(LIGHT_PURPLE
			">>> (Input 1 integers) max passed vsync(s) cnt : "
			NONE);
		scanf("%u", &max_pass_cnt);
		if (max_pass_cnt < 1)
			max_pass_cnt = 1;
	}


	/* if you want to lock exp / flk when auto run test */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) lock shutter value (Y:1 / N:0): "
		NONE);
	scanf("%u", &lock_exp);

	if (lock_exp) {
		printf(LIGHT_PURPLE
			">>> (Input 1 integers) lock shutter at idx (0:10002 / 1:19997 / 2:29996 / 3:40005 / 4:50004 / 5:60002) : "
			NONE);
		scanf("%u", &exp_table_idx);

		if (exp_table_idx >= EXP_TABLE_SIZE) {
			exp_table_idx = EXP_TABLE_SIZE - 1;

			printf(LIGHT_PURPLE
				">>> set non valid value, lock shutter at idx:%u automatically\n"
				NONE,
				exp_table_idx);
		}
	}

	printf(LIGHT_PURPLE
		">>> (Input 1 integers) lock anti-flicker (Y:1 / N:0): "
		NONE);
	scanf("%u", &lock_flk);


	printf("\n\n\n");


	for (i = 0; g_streaming_sensors[i].sensor != NULL; ++i) {
		printf(GREEN
			"Please select bellow sensor's MODE for auto run using :\n"
			NONE);

		g_sensor_mode[i] = print_and_select_s_mode(i);

		printf("\n\n\n");
	}


RUN_PF_CTRL_AUTO_NORMAL:

	/* auto run pf ctrl test */
	g_counter = 0;

	while (++g_counter < g_run_times) {
		/* 0. generate shutter for AE exp sync */
		// ut_gen_shutter_data();


		/* 1. start => same request ID settings will be set */
		frameSync->fs_sync_frame(1);

		/* 1.x turn on/off N:1 mode or not */
		ut_check_turn_on_off_n_1_mode(g_counter);


		/* 2. ut sensor ctrl request setup */
		ut_ctrl_request_setup();


		/* 2.x do ext ctrl if needed */
		// ut_trigger_ext_ctrl(g_counter);


		/* 3. end => end this request ID settings */
		frameSync->fs_sync_frame(0);
		frameSync->fs_sync_frame(0);


#if (WAITING_FOR_REMOVE_CODE)
		for (i = 0; i < SENSOR_MAX_NUM; ++i)
			ut_try_update_ts_diff(i);
#endif // WAITING_FOR_REMOVE_CODE


		/* 4. generate vsync data */
		ut_generate_vsync_data_pf_auto(&g_v_rec);
		ut_notify_vsync();


		/* 5. check sync result */
		// pf_result = ut_check_pf_sync_result(&g_v_rec);
		pf_result = ut_check_pf_sync_result_v2(&g_v_rec);


		if (g_counter >= 20 && !first_sync) {
			printf(RED
				"UT: Can NOT sync, pf_count:%u, first_sync:%s\n"
				NONE,
				g_counter,
				(first_sync) ? "true" : "false");

			printf(RED ">>> Press \'n\' key to continue... "NONE);
			while (((c = getchar()) != 'n') && (c != EOF))
				;
			printf("\n");
		}

		if ((g_counter >= (g_broke_at_counter+10)) && break_sync && first_sync) {
			printf(RED
				"UT: Can NOT sync from broke sync, pf_count:%u, break_sync:%s, broke at:%u\n"
				NONE,
				g_counter,
				(first_sync) ? "true" : "false",
				g_broke_at_counter);

			printf(RED ">>> Press \'n\' key to continue... "NONE);
			while (((c = getchar()) != 'n') && (c != EOF))
				;
			printf("\n");
		}

		/* first sync (from non-sync to sync) */
		if (pf_result && !first_sync && g_counter >= 3) {
			first_sync = true; // if first_sync is true, never be false
			printf(GREEN
				"UT: First Sync, pf_count:%u\n"
				NONE, g_counter);
		}

		/* sync is broken */
		if (!pf_result && first_sync && !break_sync) {
			break_sync = true;
			break_sync_count++;
			g_broke_at_counter = g_counter;
			printf(RED
				"UT: Sync Broken, pf_count:%u, times:%u\n"
				NONE,
				g_counter, break_sync_count);

			printf(RED ">>> Press \'n\' key to continue... "NONE);
			while (((c = getchar()) != 'n') && (c != EOF))
				;
			printf("\n");
		}

		/* re-sync (from broken sync to sync) */
		if (pf_result && break_sync) {
			break_sync = false;
			re_sync = true;
			re_sync_count++;
			g_broke_at_counter = 0;
			printf(GREEN
				"UT: Re-Sync, pf_count:%u, times:%u\n"
				NONE,
				g_counter, re_sync_count);
		}
	}


	/* print auto test result */
	is_ut_pass = (first_sync && !break_sync) ? true : false;
	printf(
		(is_ut_pass) ?
		GREEN
		">>> UT Result: PASS (Run %u times, Broken %u times ! first_sync:%s, break_sync:%s, re_sync:%s)\n"
		NONE :
		RED
		">>> UT Result: FAIL (Run %u times, Broken %u times ! first_sync:%s, break_sync:%s, re_sync:%s)\n"
		NONE,
		g_counter, break_sync_count,
		(first_sync) ? "true" : "false",
		(break_sync) ? "true" : "false",
		(re_sync) ? "true" : "false");
}


static void ut_trigger_pf_ctrl_auto_run(void)
{
	/* for shutter automatically generated */
	srand(time(NULL));

	// TODO: add auto test case for choice
	ut_trigger_pf_ctrl_auto_run_normal();
}


static void ut_trigger_pf_ctrl(void)
{
	unsigned int n_1_cfg_en = 0;

	printf(LIGHT_CYAN
		"\n\n\n>>> UT Trigger FrameSync PF CTRL <<<\n\n\n"
		NONE);

	/* N:1 mode cfg */
	printf(GREEN
		"Please select if enable N:1 mode :\n"
		NONE);

	/* select sensor mode */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) \"enable N:1 mode (0:disable / 1:enable)\" : "
		NONE);
	scanf("%d", &n_1_cfg_en);

	if (n_1_cfg_en)
		ut_setup_n_1_cfg(n_1_cfg_2_1_60_main_0);


	/* per-frame ctrl with auto run / manually set */
	printf(LIGHT_PURPLE
		">>> (Input 1 integers) PF CTRL with \"Auto run:1\" OR \"Manually set:0\" : "
		NONE);
	scanf("%u", &g_auto_run);

	if (g_auto_run > 0)
		ut_trigger_pf_ctrl_auto_run();
	else {
		printf("\n\n\n");
		ut_trigger_pf_ctrl_manually();
	}


	/* END */
	printf(GREEN
		"\n\n\n>>> END UT Trigger FrameSync PF CTRL <<<\n\n\n"
		NONE);
}


static void test_frame_sync_proc(void)
{
	unsigned int ret = 0;


	/* frame sync drv init, get it's address */
	ret = FrameSyncInit(&frameSync);
	printf(GREEN "\n\n\n>>> Test FrameSync Process! <<<\n" NONE);
	printf(GREEN ">>> frameSync addr = %p\n\n\n" NONE, frameSync);
	if (ret != 0) {
		printf(RED ">>> frameSync Init failed !\n" NONE);
		return;
	}


	/* 0. reset ut test variable before using */
	reset_ut_test_variables();
	ut_select_frame_sync_algorithm();


	/* 1. fs streaming... */
	ut_set_fs_streaming();


	/* 2. fs set_sync... */
	ut_set_fs_set_sync();


	/* 3. set initial vsync diff manually */
	printf(GREEN
		">>> Before running PF CTRL test, please set vsync data for first run\n"
		NONE);
	printf(GREEN
		">>> => set initial vsync diff\n\n\n"
		NONE);

	g_v_rec = ut_generate_vsync_data_manually(
		g_streaming_sensors, g_streaming_sensors_modes_list);


	/* 4. trigger FrameSync PF CTRL */
	ut_trigger_pf_ctrl();


	printf(GREEN "\n\n\n>>> END Test FrameSync Process! <<<\n" NONE);
}


static void ut_setup_streaming_data(
	const unsigned int i,
	struct ut_fs_test_sensor_cfg *p_sensor_cfg)
{
	g_streaming_sensors[i].sensor_name =
		p_sensor_cfg[i].sensor_name;

	g_streaming_sensors[i].sensor_idx =
		p_sensor_cfg[i].sensor_idx;

	g_streaming_sensors[i].tg =
		p_sensor_cfg[i].tg;

	g_streaming_sensors[i].sensor = p_sensor_cfg[i].sensor;


	g_streaming_sensors_modes_list[i].sensor_name =
		p_sensor_cfg[i].sensor_name;

	g_streaming_sensors_modes_list[i].sensor_idx =
		p_sensor_cfg[i].sensor_idx;

	g_streaming_sensors_modes_list[i].mode_list =
		p_sensor_cfg[i].mode;

	g_streaming_sensors_modes_list[i]
		.mode_list[p_sensor_cfg[i].mode_idx]
		.sensor_idx = p_sensor_cfg[i].sensor_idx;


	g_sensor_mode[i] = p_sensor_cfg[i].mode_idx;
}


static void ut_setup_ext_ctrl_cfg(
	struct ut_fs_test_ext_ctrl_cfg *ctrls)
{
	unsigned int i = 0;

	if (ctrls == NULL) {
		struct ut_fs_test_ext_ctrl_cfg clear_st = {0};

		for (i = 0; i < SENSOR_MAX_NUM; ++i)
			g_ext_ctrls[i] = clear_st;

		return;
	}

	for (i = 0;
		(ctrls[i].do_ext_fl_at_n_run != 0 ||
		ctrls[i].do_seamless_switch_at_n_run != 0); ++i) {

		g_ext_ctrls[i] = ctrls[i];
	}
}


static void ut_setup_fs_alg_stability_test_env_cfg(const unsigned int test_id)
{
	/* Env cfg */
	g_run_times = test_list[test_id].env_cfg->run_times;
	// g_run_times = 20;
	g_vdiff_sync_success_th = test_list[test_id].env_cfg->sync_th;
	simulation_passed_vsyncs = test_list[test_id].env_cfg->passed_vsync;
	passed_vsyncs_ratio = test_list[test_id].env_cfg->passed_vsync_ratio;
	max_pass_cnt = test_list[test_id].env_cfg->passed_vsync_max_cnt;

	/* test case per-frame cfg */
	lock_exp = (force_lock_exp)
		? 1
		: test_list[test_id].env_cfg->lock_exp;
	exp_table_idx = (lock_exp)
		? test_list[test_id].env_cfg->lock_exp_table_idx
		: exp_table_idx;

	lock_flk = test_list[test_id].env_cfg->lock_flk;

	/* init/reset shutter settings according to test env cfg */
	ut_gen_shutter_data();


	/* Others env cfg */
	/* EXT CTRL */
	ut_setup_ext_ctrl_cfg(test_list[test_id].env_cfg->ext_ctrls);

	/* N:1 mode cfg */
	ut_setup_n_1_cfg(test_list[test_id].n_1_cfg);

	ut_chk_and_set_async_master(
		test_list[test_id].async_master_sidx, 1);
}


static void exe_fs_alg_stability_test_item(unsigned int test_id)
{
	struct ut_fs_test_sensor_cfg *p_sensor_cfg = NULL;
	unsigned long long biggest_vts = 0;
	unsigned int i = 0;

	pthread_t thread[SENSOR_MAX_NUM];

	printf(CYAN
		"\n\n\n>>> Execute FS alg stability Test, Test_ID:%u!\n\n\n<<<\n"
		NONE,
		test_id + 1);

	printf(LIGHT_CYAN
		"\n\n\n>>> RUN Test : [%2u] %s <<<\n\n\n\n"
		NONE,
		(test_id + 1),
		test_list[test_id].test_name);


	g_alg_method = test_list[test_id].alg_method;

	g_query_tg_cnt = 0;

	/* 1. call fs streaming... with fs set_sync..., by multi-thread */
	p_sensor_cfg = test_list[test_id].sensor_cfg;

	for (i = 0; p_sensor_cfg[i].sensor != NULL; ++i) {
		/* fill in the settings */
		p_sensor_cfg[i].sync_type = test_list[test_id].sync_type[i];

		pthread_create(&thread[i], NULL,
			ut_set_fs_streaming_and_synced,
			(void *)&p_sensor_cfg[i]);


		ut_setup_streaming_data(i, p_sensor_cfg);


		/* 1.2 setup initial ut timestamp data */
		/* TODO : non-hardcode */
		g_query_tg_cnt++;

		g_v_rec.recs[i].id = frm_convert_cammux_id_to_ccu_tg_id(
			g_streaming_sensors[i].tg);

		g_v_rec.recs[i].vsyncs = 1;

		g_v_rec.recs[i].timestamps[0] = p_sensor_cfg[i].first_vts_value;
		g_ut_vts[i].timestamp[0] = p_sensor_cfg[i].first_vts_value;
		g_ut_vts[i].target_ts[0] = p_sensor_cfg[i].first_vts_value;
		g_ut_vts[i].be_triggered_at_ts =
			g_ut_vts[i].timestamp[0] + UT_SENSOR_TRIGGER_BIAS;
		// TODO: convert to time from default frame length by line time
		// g_ut_vts[i].should_be_triggered_before =
		//	g_ut_vts[i].timestamp[0] + 33350;

		// TODO: FIX ME due to forcing trigger first run.
		// g_ut_vts[i].should_trigger = 1;

		if (p_sensor_cfg[i].first_vts_value > biggest_vts)
			biggest_vts = p_sensor_cfg[i].first_vts_value;
	}

	for (i = 0; p_sensor_cfg[i].sensor != NULL; ++i)
		pthread_join(thread[i], NULL);


	/* 1.3 setup initial ut timestamp data */
	g_v_rec.ids = g_query_tg_cnt;
	g_v_rec.tick_factor = TICK_FACTOR;
	g_v_rec.cur_tick =
		// (biggest_vts + 1) * g_v_rec.tick_factor;
		ut_setup_next_pf_ctrl_trigger_timing() * g_v_rec.tick_factor;


	// printf("[0] tg:%u, [1] tg:%u, [2] tg:%u\n",
	// g_v_rec.recs[0].id,
	// g_v_rec.recs[1].id,
	// g_v_rec.recs[2].id);


	/* 2. check if in stability test mode */
	if (g_fs_alg_stability_test_flag) {
		/* 2.1 copy fs test env cfg settings */
		ut_setup_fs_alg_stability_test_env_cfg(test_id);


		/* 2.2 call frm API set vsync data */
		frm_debug_set_last_vsync_data(&g_v_rec);
		ut_notify_vsync();


		/* 2.3 pf ctrl auto run */
		ut_trigger_pf_ctrl_auto_run();
	}


	printf(GREEN
		"\n\n\n>>> END Execute FS alg stability Test, Test_ID:%u! <<<\n\n\n"
		NONE,
		(test_id + 1));

	printf(LIGHT_CYAN
		"\n\n\n>>> Test PASS : [%2u] %s <<<\n\n\n\n\n\n"
		NONE,
		(test_id + 1),
		test_list[test_id].test_name);

	for (i = 0; i < 4; ++i) {
		printf(GREEN
			"waiting for running next test case... %u(4)\n"
			NONE, i);

		sleep(5);
	}

	printf("\n\n\n");
}


static void exe_fs_alg_stability_test(void)
{
	unsigned int i = 0;
	int select = 1;

	g_fs_alg_stability_test_flag = 1;
	g_auto_run = 1;


	ut_select_frame_sync_algorithm();

	printf(GREEN
		"\n\n\n>>> Please decide whether to force lock exp! (1: Yes / 0: No) <<<\n"
		NONE);
	printf(LIGHT_PURPLE
		">>> (Input 1 integer) \"force lock exposure\" : "
		NONE);
	scanf("%u", &force_lock_exp);

	while (true) {
		printf(GREEN
				"\n\n\n>>> Please choose FrameSync algorithm stability test case bellow! <<<\n"
				NONE);
		printf(GREEN
			"[ 0] Run all case (all must run case, i.e per-frame ctrl case, except EXT CTRL)\n"
			NONE);

		for (i = 0; test_list[i].sensor_cfg != NULL; ++i) {
			printf(GREEN "[%2u] %s\n" NONE,
				i + 1,
				test_list[i].test_name);
		}
		printf(GREEN "[-1] End FrameSync algorithm stability test\n" NONE);

		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"select a case\" : "
			NONE);
		scanf("%d", &select);

		if (select <= 0)
			break;


		reset_ut_test_variables();

		exe_fs_alg_stability_test_item(select-1);
	}

	if (select < 0) {
		printf("\n\n\n");

		g_fs_alg_stability_test_flag = 0;
		g_auto_run = 0;

		return;
	}


	for (i = 0; test_list[i].sensor_cfg != NULL; ++i) {
		if (test_list[i].exe_all_skip_ext_ctrl_test == 1)
			continue;

		if (test_list[i].auto_test_must_run != 1)
			continue;

		reset_ut_test_variables();

		exe_fs_alg_stability_test_item(i);
	}

	printf("\n\n\n");


	g_fs_alg_stability_test_flag = 0;
	g_auto_run = 0;
}


static void test_hw_sensor_sync_proc(void)
{
	unsigned int ret = 0;


	/* frame sync drv init, get it's address */
	ret = FrameSyncInit(&frameSync);
	printf(GREEN
		"\n\n\n>>> Test FrameSync HW Sensor Sync Process! <<<\n"
		NONE);
	printf(GREEN ">>> frameSync addr = %p\n\n\n" NONE, frameSync);
	if (ret != 0) {
		printf(RED ">>> frameSync Init failed !\n" NONE);
		return;
	}


	/* 0. reset ut test variable before using */
	reset_ut_test_variables();


	/* 1. fs streaming... */
	ut_set_fs_streaming();


	/* 2. fs set_sync... */
	ut_set_fs_set_sync();


	/* 3. set initial vsync diff manually */
	printf(GREEN
		">>> Before running PF CTRL test, please set vsync data for first run\n"
		NONE);
	printf(GREEN
		">>> => set initial vsync diff (HW Sensor Sync case, almost zero diff.)\n\n\n"
		NONE);

	g_v_rec = ut_generate_vsync_data_manually(
		g_streaming_sensors, g_streaming_sensors_modes_list);


	/* 4. trigger FrameSync PF CTRL */
	ut_trigger_pf_ctrl();


	printf(GREEN "\n\n\n>>> END Test FrameSync Process! <<<\n" NONE);
}


int main(void)
{
	unsigned int select_ut_case = 0, terminated = 0;


	while (!terminated) {
		SHOW_TEXT();

		printf(GREEN
			"\n\n\n>>> Please choose FrameSync UT Test case bellow! <<<\n"
			NONE);
		printf(GREEN
			">>> Run : [1] FrameSync algorithm stability test\n"
			NONE);
		printf(GREEN
			">>> Run : [2] FrameSync processing test\n"
			NONE);
		printf(GREEN
			">>> Run : [3] FrameSync control flow test\n"
			NONE);
		printf(GREEN
			">>> Run : [4] FrameSync data racing test\n"
			NONE);
		printf(GREEN
			">>> Run : [5] FrameSync HW sensor sync test\n"
			NONE);
		printf(GREEN
			">>> Run : [9] TSREC function test\n"
			NONE);
		printf(GREEN
			">>> Run : [X] End UT test\n"
			NONE);

		printf(LIGHT_PURPLE
			">>> (Input 1 integer) \"select a case id in []\" : "
			NONE);
		scanf("%u", &select_ut_case);


		switch (select_ut_case) {
		case 1:
			/* run fs algorithm stability test */
			exe_fs_alg_stability_test();
			break;

		case 2:
			/* run test fs processing */
			test_frame_sync_proc();
			break;

		case 3:
			/* run fs control test */
			run_fs_control_test();
			break;

		case 4:
			/* run fs data racing test */
			run_fs_data_racing_test();
			break;

		case 5:
			/* run HW Sensor sync test */
			test_hw_sensor_sync_proc();
			break;

		case 9:
			/* run TSREC function test => ut_fs_tsrec.c */
			ut_fs_tsrec();
			break;

		default:
			terminated = 1;
			break;
		}
	}


	return 0;
}
