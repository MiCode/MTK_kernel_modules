/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __GPS_MCUDL_DATA_PKT_RBUF_H__
#define __GPS_MCUDL_DATA_PKT_RBUF_H__

#include "gps_mcudl_data_intf_type.h"

#define GFNS_RBUF_STA (1)

typedef gpsmdl_u32 (*gps_mcudl_rbuf_proc_fn_t)(
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

struct gps_mcudl_data_rbuf_cfg {
	gpsmdl_u8 *rbuf_ptr;
	gpsmdl_u32 rbuf_len;
};

struct gps_mcudl_data_rbuf_sta {
	gpsmdl_u32 total_put_byte;
	gpsmdl_u32 total_get_byte;
	gpsmdl_u32 total_drop_byte;
	gpsmdl_u32 total_last_drop_byte;
};

struct gps_mcudl_data_rbuf_cursor {
	gpsmdl_u32 wwi_bak; /* can be accessed any time, atomic operation */
	gpsmdl_u32 rri_bak; /* can be accessed any time, atomic operation */
	bool wff_bak; /* read to empty flag */

	/* wwi --> wwi_bak --> rwi */
	gpsmdl_u32 wwi; /* writer's write index */
	gpsmdl_u32 wri; /* writer's read  index */

	/* rri --> rri_bak --> wri */
	gpsmdl_u32 rri; /* reader's read index */
	gpsmdl_u32 rwi; /* reader's write index */

	/* for full/empty detect */
	bool wff; /* write to full flag */
	bool rff;
};

/*  \brief
 *    data struct for high prority writer and low prority reader
 *    note: 1. wwi, wri is always accessible by writer
 *          2. rri, rwi is always accessible by reader
 *          3. method "sync*" to sync writer and reader index
 **/
struct gps_mcudl_data_rbuf_plus_t {
	struct gps_mcudl_data_rbuf_cfg cfg;
	struct gps_mcudl_data_rbuf_cursor cursor;
	/* for statistic */
#ifdef GFNS_RBUF_STA
	struct gps_mcudl_data_rbuf_sta sta;
#endif
};

void gps_mcudl_data_rbuf_init(struct gps_mcudl_data_rbuf_plus_t *p_rbuf);

/* return the length of be but ok **/
gpsmdl_u32 gps_mcudl_data_rbuf_put(struct gps_mcudl_data_rbuf_plus_t *p_rbuf,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

void gps_mcudl_data_rbuf_get_to_proc(struct gps_mcudl_data_rbuf_plus_t *p_rbuf,
	gps_mcudl_rbuf_proc_fn_t proc_func);

void gps_mcudl_data_rbuf_writer_update_write_idx(struct gps_mcudl_data_rbuf_plus_t *p_rbuf);
/* return true if get new value */
bool gps_mcudl_data_rbuf_writer_sync_read_idx(struct gps_mcudl_data_rbuf_plus_t *p_rbuf);

void gps_mcudl_data_rbuf_reader_update_read_idx(struct gps_mcudl_data_rbuf_plus_t *p_rbuf);
/* return true if get new value */
bool gps_mcudl_data_rbuf_reader_sync_write_idx(struct gps_mcudl_data_rbuf_plus_t *p_rbuf);

#endif /* __GPS_MCUDL_DATA_PKT_RBUF_H__ */
