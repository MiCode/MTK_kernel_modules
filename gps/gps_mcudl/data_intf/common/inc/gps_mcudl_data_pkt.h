/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_H
#define _GPS_MCUDL_DATA_PKT_H

#include "gps_mcudl_data_intf_type.h"

#define GPSMDL_PKT_HEAD_LEN (6)
#define GPSMDL_PKT_TAIL_LEN (1)
#define GPSMDL_PKT_PAYLOAD_MAX (1800)

#define GPSMDL_PKT_START_CHAR (0xA5)
#define GPSMDL_PKT_END_CHAR (0xA3)

struct gps_mcudl_pkt_head {
	gpsmdl_u8 start_char;
	gpsmdl_u8 seq;
	gpsmdl_u8 type;
	gpsmdl_u8 payload_len1;
	gpsmdl_u8 payload_len2;
	gpsmdl_u8 chksum;
};

#endif /* _GPS_MCUDL_DATA_PKT_H */
