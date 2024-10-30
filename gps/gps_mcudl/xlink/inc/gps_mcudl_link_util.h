/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_LINK_UTIL_H
#define _GPS_MCUDL_LINK_UTIL_H

#include "gps_mcudl_each_link.h"
#include "gps_each_link.h"

void gps_mcudl_each_link_mutexes_init(struct gps_mcudl_each_link *p);
void gps_mcudl_each_link_mutexes_deinit(struct gps_mcudl_each_link *p);
void gps_mcudl_each_link_spin_locks_init(struct gps_mcudl_each_link *p);
void gps_mcudl_each_link_spin_locks_deinit(struct gps_mcudl_each_link *p);

void gps_mcudl_each_link_mutex_take(enum gps_mcudl_xid x_id,
	enum gps_each_link_mutex mtx_id);
bool gps_mcudl_each_link_mutex_take2(enum gps_mcudl_xid x_id,
	enum gps_each_link_mutex mtx_id);
void gps_mcudl_each_link_mutex_give(enum gps_mcudl_xid x_id,
	enum gps_each_link_mutex mtx_id);
void gps_mcudl_each_link_spin_lock_take(enum gps_mcudl_xid x_id,
	enum gps_each_link_spinlock spin_lock_id);
void gps_mcudl_each_link_spin_lock_give(enum gps_mcudl_xid x_id,
	enum gps_each_link_spinlock spin_lock_id);

void gps_mcudl_each_link_waitable_reset(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type);
int gps_mcudl_each_link_take_big_lock(enum gps_mcudl_xid x_id,
	enum gps_each_link_lock_reason reason);
int gps_mcudl_each_link_give_big_lock(enum gps_mcudl_xid x_id);

#endif /* _GPS_MCUDL_LINK_UTIL_H */

