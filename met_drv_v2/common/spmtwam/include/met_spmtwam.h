/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MET_SPMTWAM_H__
#define __MET_SPMTWAM_H__

#define MET_SPMTWAM_TAG "[met_spmtwam]"
#define MET_SPMTWAM_ERR(format, ...) \
	do { \
		MET_TRACE(MET_SPMTWAM_TAG format, ##__VA_ARGS__); \
		PR_BOOTMSG(MET_SPMTWAM_TAG format, ##__VA_ARGS__); \
	} while (0)

#define TWAM_ENABLE true
#define TWAM_DISABLE false
#define	TWAM_SPEED_MODE true
#define	TWAM_NORMAL_MODE false
#define TWAM_DEBUG_SIG_ENABLE 1
#define TWAM_DEBUG_SIG_DISABLE 0
#define TWAM_SINGLE_IDLE_SIGNAL "single"
#define TWAM_MULTIPLE_IDLE_SIGNAL "multiple"

struct met_spmtwam_para {
	char idle_sig;
	int event;
};


/* event counters by HW spec */
#define MAX_EVENT_COUNT 4
#define MAX_TWAM_EVENT_COUNT 32

#endif
