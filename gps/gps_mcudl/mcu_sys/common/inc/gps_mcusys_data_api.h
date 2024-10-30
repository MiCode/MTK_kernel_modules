/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_MCUSYS_DATA_API_H_
#define _GPS_MCUSYS_DATA_API_H_

#include "gps_mcudl_data_intf_type.h"
#include "gps_mcusys_data_wrapper.h"

/* proc the data frame from remote side */
void gps_mcusys_data_frame_proc(const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

/* send the data frame to remote side */
void gps_mcusys_data_frame_send(struct gps_mcusys_data_sync_frame *p_frame);


#endif /* _GPS_MCUSYS_DATA_API_H_ */

