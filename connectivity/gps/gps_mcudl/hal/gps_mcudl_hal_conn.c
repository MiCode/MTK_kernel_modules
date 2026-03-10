/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_hw_dep_api.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_hw_mcu.h"
#include "gps_mcudl_hal_conn.h"
#if GPS_DL_ON_LINUX
#include "dvfsrc-common.h"
#endif

void gps_mcudl_hal_get_ecid_info(void)
{
	gps_dl_hw_dep_gps_control_adie_on();
	gps_dl_hw_dep_gps_get_ecid_info();
	gps_dl_hw_dep_gps_control_adie_off();
}

#if GPS_DL_ON_LINUX
bool gps_mcudl_is_voting_for_coinninfra_on;
unsigned long gps_mcudl_vote_for_coinninfra_on_us;
unsigned long gps_mcudl_vote_phase_bitmask;

static void gps_mcudl_vote_to_deny_opp0(bool deny_opp0)
{
	mtk_dvfsrc_dynamic_opp0(VCOREOPP_GPS, deny_opp0);
}

void gps_mcudl_set_opp_vote_phase(enum gps_mcudl_hal_opp_vote_phase phase, bool is_in)
{
	smp_mb__before_atomic();
	if (is_in)
		set_bit(phase, &gps_mcudl_vote_phase_bitmask);
	else
		clear_bit(phase, &gps_mcudl_vote_phase_bitmask);
	smp_mb__after_atomic();
}

void gps_mcudl_end_all_opp_vote_phase(void)
{
	unsigned long bitmask;

	bitmask = gps_mcudl_vote_phase_bitmask;
	if (bitmask != 0)
		GDL_LOGW("bitmask=0x%08x is not zero", bitmask);
	gps_mcudl_vote_phase_bitmask = 0;
}

unsigned int gps_mcudl_get_opp_vote_phase_bitmask(void)
{
	unsigned long bitmask;

	bitmask = gps_mcudl_vote_phase_bitmask;
	return (unsigned int)bitmask;
}

void gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(bool vote)
{
	unsigned long d_us;

	if (vote) {
		if (gps_mcudl_is_voting_for_coinninfra_on) {
			d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
			GDL_LOGE("double vote true, d_us=%lu", d_us);
			return;
		}
		gps_mcudl_is_voting_for_coinninfra_on = true;
		gps_mcudl_vote_for_coinninfra_on_us = gps_dl_tick_get_us();
		gps_mcudl_vote_to_deny_opp0(true);
	} else {
		if (!gps_mcudl_is_voting_for_coinninfra_on) {
			GDL_LOGW("double vote false");
			return;
		}
		gps_mcudl_vote_to_deny_opp0(false);
		d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
		gps_mcudl_is_voting_for_coinninfra_on = false;
		GDL_LOGE("abnormal vote end-1, d_us=%lu", d_us);
	}
}

bool gps_mcudl_hw_conn_force_wake(bool enable)
{
	bool wake_okay = false;
	bool conn_on_vote = gps_mcudl_is_voting_for_coinninfra_on;
	unsigned int vote_bitmask;
	unsigned long us0, d_us;

	if (!enable) {
		wake_okay = gps_mcudl_hw_conn_force_wake_inner(false);
		return wake_okay;
	}

	vote_bitmask = gps_mcudl_get_opp_vote_phase_bitmask();
	if (vote_bitmask == 0) {
		wake_okay = gps_mcudl_hw_conn_force_wake_inner(true);
		if (conn_on_vote) {
			gps_mcudl_vote_to_deny_opp0(false);
			d_us = gps_dl_tick_get_us() - gps_mcudl_vote_for_coinninfra_on_us;
			gps_mcudl_is_voting_for_coinninfra_on = false;
			GDL_LOGE("abnormal vote end-2, d_us=%lu, w_ok=%d", d_us, wake_okay);
		}
		return wake_okay;
	}

	if (conn_on_vote) {
		us0 = gps_mcudl_vote_for_coinninfra_on_us;
		gps_mcudl_is_voting_for_coinninfra_on = false;
	} else {
		us0 = gps_dl_tick_get_us();
		gps_mcudl_vote_to_deny_opp0(true);
	}
	wake_okay = gps_mcudl_hw_conn_force_wake_inner(true);
	gps_mcudl_vote_to_deny_opp0(false);
	d_us = gps_dl_tick_get_us() - us0;
	GDL_LOGW("vote end: d_us=%lu, bitmask=0x%08x, is_cv=%d, w_ok=%d",
		d_us, vote_bitmask, conn_on_vote, wake_okay);
	return wake_okay;
}

#else
void gps_mcudl_set_opp_vote_phase(enum gps_mcudl_hal_opp_vote_phase phase, bool is_in)
{
}

void gps_mcudl_end_all_opp_vote_phase(void)
{
}

unsigned int gps_mcudl_get_opp_vote_phase_bitmask(void)
{
	return 0;
}

void gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(bool vote)
{
}

bool gps_mcudl_hw_conn_force_wake(bool enable)
{
	return gps_mcudl_hw_conn_force_wake_inner(enable);
}

#endif

