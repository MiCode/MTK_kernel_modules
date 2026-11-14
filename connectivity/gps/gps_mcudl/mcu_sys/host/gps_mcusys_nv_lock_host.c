/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_dl_log.h"
#include "gps_mcusys_fsm.h"

void gps_mcusys_nvlock_fsm(enum gps_mcusys_nv_data_id nv_id, enum gps_mcusys_nvlock_event_id lock_evt)
{
	bool no_log = false;

	switch (lock_evt) {
	case GPS_MCUSYS_NVLOCK_HOST_INIT:
		no_log = true;
		break;

	case GPS_MCUSYS_NVLOCK_MCU_PRE_ON:
		no_log = true;
		break;

	case GPS_MCUSYS_NVLOCK_MCU_POST_ON:
		no_log = true;
		break;

	case GPS_MCUSYS_NVLOCK_MCU_PRE_OFF:
		no_log = true;
		break;

	case GPS_MCUSYS_NVLOCK_MCU_POST_OFF:
		no_log = true;
		break;

	case GPS_MCUSYS_NVLOCK_HOST_TAKE_REQ:
		break;

	case GPS_MCUSYS_NVLOCK_MCU_TAKE_ACK:
		break;

	case GPS_MCUSYS_NVLOCK_HOST_GIVE_REQ:
		break;

	case GPS_MCUSYS_NVLOCK_MCU_GIVE_ACK:
		break;

	default:
		break;
	}

	if (!no_log)
		GDL_LOGI("nv_id=%d, lock_evt=%d", nv_id, lock_evt);
}

