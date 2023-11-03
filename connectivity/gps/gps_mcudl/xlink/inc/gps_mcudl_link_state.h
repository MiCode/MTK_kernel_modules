/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_LINK_STATE_H
#define _GPS_MCUDL_LINK_STATE_H

#include "gps_mcudl_each_link.h"
#include "gps_each_link.h"

void gps_mcudl_each_link_set_bool_flag(enum gps_mcudl_xid link_id,
	enum gps_each_link_bool_state name, bool value);
bool gps_mcudl_each_link_get_bool_flag(enum gps_mcudl_xid link_id,
	enum gps_each_link_bool_state name);

void gps_mcudl_each_link_set_ready_to_write(enum gps_mcudl_xid link_id, bool is_ready);
bool gps_mcudl_each_link_is_ready_to_write(enum gps_mcudl_xid link_id);

void gps_mcudl_each_link_set_active(enum gps_mcudl_xid link_id, bool is_active);
bool gps_mcudl_each_link_is_active(enum gps_mcudl_xid link_id);

void gps_mcudl_each_link_inc_session_id(enum gps_mcudl_xid link_id);
int gps_mcudl_each_link_get_session_id(enum gps_mcudl_xid link_id);

enum gps_each_link_state_enum gps_mcudl_each_link_get_state(enum gps_mcudl_xid link_id);
void gps_mcudl_each_link_set_state(enum gps_mcudl_xid link_id, enum gps_each_link_state_enum state);
bool gps_mcudl_each_link_change_state_from(enum gps_mcudl_xid link_id,
	enum gps_each_link_state_enum from, enum gps_each_link_state_enum to);

#if GPS_DL_ON_LINUX
wait_queue_head_t *gps_mcudl_each_link_poll_get_wq_ptr(enum gps_mcudl_xid link_id);
#endif
bool gps_mcudl_each_link_poll_is_in_data_ready(enum gps_mcudl_xid link_id);

#endif /* _GPS_MCUDL_LINK_STATE_H */

