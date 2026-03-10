/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_MCUSYS_NV_DATA_API_H
#define _GPS_MCUSYS_NV_DATA_API_H

#include "gps_mcusys_data.h"
#include "gps_mcusys_fsm.h"
#include "gps_mcusys_nv_common_impl.h"

void gps_mcusys_nv_data_host_init(void);
void gps_mcusys_nv_data_target_init(void);
void gps_mcusys_nv_data_on_gpsbin_state(enum gps_mcusys_gpsbin_state gpsbin_state);

struct gps_mcusys_nv_data_header *gps_mcusys_nv_data_get_hdr(enum gps_mcusys_nv_data_id nv_id);
gpsmdl_u32 gps_mcusys_nv_data_get_block_size(enum gps_mcusys_nv_data_id nv_id);

/* return true for okay */
bool gps_mcusys_nv_data_take_write_lock(enum gps_mcusys_nv_data_id nv_id);
bool gps_mcusys_nv_data_give_write_lock(enum gps_mcusys_nv_data_id nv_id);
bool gps_mcusys_nv_data_take_read_lock(enum gps_mcusys_nv_data_id nv_id);
bool gps_mcusys_nv_data_give_read_lock(enum gps_mcusys_nv_data_id nv_id);

extern void *gps_mcudl_plat_nv_emi_get_start_ptr(void);
extern void *gps_mcudl_plat_nv_emi_get_end_ptr(void);
extern void gps_mcudl_plat_nv_emi_clear(void);
extern struct gps_each_link_waitable *
	gps_nv_each_link_get_read_waitable_ptr(enum gps_mcusys_nv_data_id nv_id);

#endif /* _GPS_MCUSYS_NV_DATA_API_H */

