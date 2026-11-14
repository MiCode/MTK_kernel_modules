/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_DL_HAL_MET_2_0_H
#define _GPS_DL_HAL_MET_2_0_H

#include "gps_dl_config.h"

#define GPS_DEBUG_MET_SETTINGS_BUFFER_MODE_VALID	((1)<<0)
#define GPS_DEBUG_MET_SETTINGS_SAMPLE_RATE_VALID	((1)<<1)
#define GPS_DEBUG_MET_SETTINGS_MASK_SIGNAL_VALID	((1)<<2)
#define GPS_DEBUG_MET_SETTINGS_EVENT_SIGNAL_VALID	((1)<<3)
#define GPS_DEBUG_MET_SETTINGS_EADE_DETECTION_VALID	((1)<<4)
#define GPS_DEBUG_MET_SETTINGS_EVENT_SELECT_VALID	((1)<<5)
#define GPS_DEBUG_MET_SETTINGS_TIMER_SOURCE_VALID	((1)<<6)
#if GPS_DL_MET_V2
#define GPS_DEBUG_MET_SETTINGS_MASK_SIGNAL2_VALID	((1)<<7)
#define GPS_DEBUG_MET_SETTINGS_EADE_DETECTION2_VALID	((1)<<8)
#define GPS_DEBUG_MET_SETTINGS_EADE_DETECTION3_VALID	((1)<<9)
#define GPS_DEBUG_MET_SETTINGS_EADE_DETECTION4_VALID	((1)<<10)
#endif

#if GPS_DL_MET_V2
#define GPS_DEBUG_MET_OUTPUT_LEN			(64)
#else
#define GPS_DEBUG_MET_OUTPUT_LEN			(32)
#endif

enum gps_debug_met_status {
	GPS_DEBUG_MET_CLOSED,
	GPS_DEBUG_MET_OPENED
};

enum gps_debug_met_operator_index {
	GPS_DEBUG_OP_START,
	GPS_DEBUG_OP_STOP,
	GPS_DEBUG_OP_CLEAR,
	GPS_DEBUG_OP_SET_BUFFER_MODE,
	GPS_DEBUG_OP_SET_SAMPLE_RATE,
	GPS_DEBUG_OP_SET_MASK_SIGNAL,
	GPS_DEBUG_OP_SET_EVENT_SIGNAL,
	GPS_DEBUG_OP_SET_EDGE_DETECTION,
	GPS_DEBUG_OP_SET_EVENT_SELECT,
	GPS_DEBUG_OP_SET_TIMER_SOURCE,
#if GPS_DL_MET_V2
	GPS_DEBUG_OP_SET_MASK_SIGNAL2,
	GPS_DEBUG_OP_SET_EDGE_DETECTION2,
	GPS_DEBUG_OP_SET_EDGE_DETECTION3,
	GPS_DEBUG_OP_SET_EDGE_DETECTION4,
#endif
	GPS_DEBUG_OP_END_INDEX
};

struct gps_debug_met_settings {
	unsigned char is_ringbuffer_mode;
	unsigned char timer_source;
	unsigned char sample_rate;
	unsigned char event_select;
	unsigned int mask_signal;
#if GPS_DL_MET_V2
	unsigned int mask_signal2;
#endif
	unsigned int event_signal;
	unsigned int edge_detection;
#if GPS_DL_MET_V2
	unsigned int edge_detection2;
	unsigned int edge_detection3;
	unsigned int edge_detection4;
#endif
};

struct gps_debug_met_contex {
	enum gps_debug_met_status status;
	unsigned int setting_bitmap;
	struct gps_debug_met_settings settings;
	/*struct mutex lock;*/
};

int gps_debug_met_start(struct gps_debug_met_contex *contex);
int gps_debug_met_stop(struct gps_debug_met_contex *contex);
void gps_debug_met_clear(struct gps_debug_met_contex *contex);
int gps_debug_met_set_parameter(struct gps_debug_met_contex *contex,
	enum gps_debug_met_operator_index op, unsigned int value);

extern struct gps_dl_iomem_addr_map_entry g_gps_dl_res_emi;
extern struct gps_debug_met_contex g_gps_debug_met_contex;

#endif /* _GPS_MET_2_0_H */

