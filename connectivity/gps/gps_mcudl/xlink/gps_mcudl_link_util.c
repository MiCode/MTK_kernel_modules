/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_time_tick.h"

#include "gps_each_link.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hal_util.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_lib_misc.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_osal.h"
#include "gps_dl_name_list.h"
#include "gps_dl_context.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_util.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_log.h"
#if GPS_DL_ON_LINUX
#include <asm/bitops.h>
#endif

void gps_mcudl_each_link_mutexes_init(struct gps_mcudl_each_link *p)
{
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_MTX_NUM; i++)
		gps_dl_osal_sleepable_lock_init(&p->mutexes[i]);
}

void gps_mcudl_each_link_mutexes_deinit(struct gps_mcudl_each_link *p)
{
	enum gps_each_link_mutex i;

	for (i = 0; i < GPS_DL_MTX_NUM; i++)
		gps_dl_osal_sleepable_lock_deinit(&p->mutexes[i]);
}

void gps_mcudl_each_link_spin_locks_init(struct gps_mcudl_each_link *p)
{
	enum gps_each_link_spinlock i;

	for (i = 0; i < GPS_DL_SPINLOCK_NUM; i++)
		gps_dl_osal_unsleepable_lock_init(&p->spin_locks[i]);
}

void gps_mcudl_each_link_spin_locks_deinit(struct gps_mcudl_each_link *p)
{
#if 0
	enum gps_each_link_spinlock i;

	for (i = 0; i < GPS_DL_SPINLOCK_NUM; i++)
		osal_unsleepable_lock_deinit(&p->spin_locks[i]);
#endif
}

bool gps_mcudl_each_link_mutex_take2(enum gps_mcudl_xid x_id, enum gps_each_link_mutex mtx_id)
{
	/* TODO: check range */
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);
	int mutex_take_retval;

	/* handle killed */
	mutex_take_retval = gps_dl_osal_lock_sleepable_lock(&p->mutexes[mtx_id]);
	if (mutex_take_retval) {
		MDL_LOGXW_DRW(x_id, "mtx_id=%d, mutex_take_retval=%d", mtx_id, mutex_take_retval);
		return false;
	}
	return true;
}

void gps_mcudl_each_link_mutex_take(enum gps_mcudl_xid x_id,
	enum gps_each_link_mutex mtx_id)
{
	(void)gps_mcudl_each_link_mutex_take2(x_id, mtx_id);
}

void gps_mcudl_each_link_mutex_give(enum gps_mcudl_xid x_id,
	enum gps_each_link_mutex mtx_id)
{
	/* TODO: check range */
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_dl_osal_unlock_sleepable_lock(&p->mutexes[mtx_id]);
}

void gps_mcudl_each_link_spin_lock_take(enum gps_mcudl_xid x_id,
	enum gps_each_link_spinlock spin_lock_id)
{
	/* TODO: check range */
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_dl_osal_lock_unsleepable_lock(&p->spin_locks[spin_lock_id]);
}

void gps_mcudl_each_link_spin_lock_give(enum gps_mcudl_xid x_id,
	enum gps_each_link_spinlock spin_lock_id)
{
	/* TODO: check range */
	struct gps_mcudl_each_link *p = gps_mcudl_link_get(x_id);

	gps_dl_osal_unlock_unsleepable_lock(&p->spin_locks[spin_lock_id]);
}


void gps_mcudl_each_link_waitable_reset(enum gps_mcudl_xid x_id,
	enum gps_each_link_waitable_type type)
{
	struct gps_mcudl_each_link *p_link = gps_mcudl_link_get(x_id);

	/* TOOD: check NULL and boundary */
#if GPS_DL_ON_LINUX
	clear_bit(0, &p_link->waitables[type].fired);
#else
	p_link->waitables[type].fired = false;
#endif
}

int gps_mcudl_each_link_take_big_lock(enum gps_mcudl_xid x_id,
	enum gps_each_link_lock_reason reason)
{
	gps_mcudl_each_link_mutex_take(x_id, GPS_DL_MTX_BIG_LOCK);
	return 0;
}

int gps_mcudl_each_link_give_big_lock(enum gps_mcudl_xid x_id)
{
	gps_mcudl_each_link_mutex_give(x_id, GPS_DL_MTX_BIG_LOCK);
	return 0;
}

