/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_CONN_H
#define _GPS_MCUDL_HAL_CONN_H

void gps_mcudl_hal_conn_do_on(void);
void gps_mcudl_hal_conn_do_off(void);
bool gps_mcudl_hal_conn_is_okay(void);
void gps_mcudl_hal_get_ecid_info(void);


enum gps_mcudl_hal_opp_vote_phase {
	GPS_MCU_OPENING,
	GPS_MCU_CLOSING,
	GPS_MNLD_FSM_STARTING,
	GPS_MNLD_FSM_STOPPING,
	GPS_MNLD_LPPM_CLOSING,
	GPS_DSP_NOT_WORKING,
};

void gps_mcudl_set_opp_vote_phase(enum gps_mcudl_hal_opp_vote_phase phase, bool is_in);
void gps_mcudl_end_all_opp_vote_phase(void);
unsigned int gps_mcudl_get_opp_vote_phase_bitmask(void);

void gps_mcudl_vote_to_deny_opp0_for_coinninfra_on(bool vote);


#endif /* _GPS_MCUDL_HAL_CONN_H */

