/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef _CONNSYS_STEP_H_
#define _CONNSYS_STEP_H_

#define STEP_TP_NO_DEFINE 0

enum consys_wf_step_trigger_point {
	STEP_WF_TP_NO_DEFINE = STEP_TP_NO_DEFINE,
	STEP_WF_TP_COMMAND_TIMEOUT,
	STEP_WF_TP_BEFORE_CHIP_RESET,
	STEP_WF_TP_AFTER_CHIP_RESET,

	STEP_WF_TP_MAX,
};

enum consys_bt_step_trigger_point {
	STEP_BT_TP_NO_DEFINE = STEP_TP_NO_DEFINE,
	STEP_BT_TP_COMMAND_TIMEOUT,
	STEP_BT_TP_BEFORE_CHIP_RESET,
	STEP_BT_TP_AFTER_CHIP_RESET,

	STEP_BT_TP_MAX,
};

enum consys_gps_step_trigger_point {
	STEP_GPS_TP_NO_DEFINE = STEP_TP_NO_DEFINE,
	STEP_GPS_TP_COMMAND_TIMEOUT,
	STEP_GPS_TP_BEFORE_CHIP_RESET,
	STEP_GPS_TP_AFTER_CHIP_RESET,

	STEP_GPS_TP_MAX,
};

enum consys_fm_step_trigger_point {
	STEP_FM_TP_NO_DEFINE = STEP_TP_NO_DEFINE,
	STEP_FM_TP_COMMAND_TIMEOUT,
	STEP_FM_TP_BEFORE_CHIP_RESET,
	STEP_FM_TP_AFTER_CHIP_RESET,

	STEP_FM_TP_MAX,
};

/******************************************
 * readable 
 * 1: can read, 0: CANNOT read
 ******************************************/
struct consys_step_register_cb {
	int (*readable_cb)(unsigned long);
};

/******************************************
 * drv down should call consys_step_xx_register(NULL);
 ******************************************/
void consys_step_bt_register(struct consys_step_register_cb *cb);
void consys_step_bt_do_action(enum consys_bt_step_trigger_point);

void consys_step_wf_register(struct consys_step_register_cb *cb);
void consys_step_wf_do_action(enum consys_wf_step_trigger_point);

void consys_step_gps_register(struct consys_step_register_cb *cb);
void consys_step_gps_do_action(enum consys_gps_step_trigger_point);

void consys_step_fm_register(struct consys_step_register_cb *cb);
void consys_step_fm_do_action(enum consys_fm_step_trigger_point);


#endif /* end of _CONNSYS_STEP_H_ */

