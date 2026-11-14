/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H
#define _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H

struct gps_mcudl_data_pkt_mcu_sta {
	gpsmdl_u64 total_recv;

	/*the bellow is just for log print, to save bandwidth, still keep them uint32*/
	gpsmdl_u32 total_parse_proc;
	gpsmdl_u32 total_parse_drop;
	gpsmdl_u32 total_route_drop; /*for host route_drop always 0*/
	gpsmdl_u32 total_pkt_cnt;

	gpsmdl_u8 LUINT_L32_VALID_BIT;
};

struct gps_mcudl_data_pkt_rec_mcu_ack {
	/*read_rec*/
	gpsmdl_u64 total_recv;

	/*the bellow is just for log print, to save bandwidth, still keep them uint32*/
	gpsmdl_u32 total_parse_proc;
	gpsmdl_u32 total_parse_drop;
	gpsmdl_u32 total_route_drop; /*for host route_drop always 0*/
	gpsmdl_u32 total_pkt_cnt;

	gpsmdl_u8 LUINT_L32_VALID_BIT;

	unsigned long host_us;
};

struct gps_mcudl_data_pkt_rec_host_wr {
	/*write_rec*/
	gpsmdl_u32 len;
	bool is_okay;

	unsigned long host_us;
};

struct gps_mcudl_data_pkt_rec_item {
	struct gps_mcudl_data_pkt_rec_mcu_ack mcu_ack;
	struct gps_mcudl_data_pkt_rec_host_wr host_wr;
};

enum gps_mcudl_rec_type {
	GPS_MCUDL_HIST_REC_MCU_ACK,
	GPS_MCUDL_HIST_REC_HOST_WR,
	GPS_MCUDL_HIST_REC_TYPE_MAX
};

#endif /* _GPS_MCUDL_DATA_PKT_PAYLOAD_STRUCT_H */
