/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_PARSER_H
#define _GPS_MCUDL_DATA_PKT_PARSER_H

#include "gps_mcudl_data_pkt.h"

typedef void (*gps_mcudl_pkt_proc_fn_t)(
	enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

struct gps_mcudl_data_pkt_parser_cfg {
	gpsmdl_u8 *rbuf_ptr;
	gpsmdl_u32 rbuf_len;
	gps_mcudl_pkt_proc_fn_t p_pkt_proc_fn;
};

struct gps_mcudl_data_pkt_parser_t {
	struct gps_mcudl_data_pkt_parser_cfg cfg;

	gpsmdl_u64 pkt_cnt;
	gpsmdl_u64 proc_byte_cnt;
	gpsmdl_u64 drop_byte_cnt;

	gpsmdl_u32 read_idx;	 /*externally know the position parsed*/
	gpsmdl_u32 tmp_read_idx; /*internally used*/

	gpsmdl_u32 head_len;
	gpsmdl_u32 remain_len;

	gpsmdl_u32 false_head; /*just for internal debug, no need large uint type*/
	bool head_done;
	bool rbuf_wrapping;

	struct gps_mcudl_pkt_head head;
	struct gps_mcudl_pkt_head *p_head;
};

void gps_mcudl_data_pkt_parser_init(struct gps_mcudl_data_pkt_parser_t *p_parser);

void gps_mcudl_data_pkt_parse(struct gps_mcudl_data_pkt_parser_t *p_parser, gpsmdl_u32 write_idx);

#endif /* _GPS_MCUDL_DATA_PKT_PARSER_H */
