/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#ifndef _GPS_MCUDL_LINK_SYNC_H
#define _GPS_MCUDL_LINK_SYNC_H

#include "gps_mcudl_xlink.h"
#include "gps_dl_base.h"
#include "gps_each_link.h"

void gps_mcudl_link_waitable_reset(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type);

enum GDL_RET_STATUS gps_mcudl_link_try_wait_on(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type);


void gps_mcudl_link_open_wait(enum gps_mcudl_xid link_id, long *p_sigval);
void gps_mcudl_link_open_ack(enum gps_mcudl_xid x_id, bool okay);

void gps_mcudl_link_close_wait(enum gps_mcudl_xid link_id, long *p_sigval);
void gps_mcudl_link_close_ack(enum gps_mcudl_xid link_id);

void gps_mcudl_link_reset_ack(enum gps_mcudl_xid link_id);
void gps_mcudl_link_on_post_conn_reset(enum gps_mcudl_xid link_id);

int gps_mcudl_link_wait_state_ntf(enum gps_mcudl_xid x_id, long *p_sigval);
void gps_mcudl_link_trigger_state_ntf(enum gps_mcudl_xid x_id);
void gps_mcudl_link_trigger_state_ntf_all(void);


#endif /* _GPS_MCUDL_LINK_SYNC_H */

