/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_RESET_H
#define _GPS_MCUDL_RESET_H

#include "gps_dl_subsys_reset.h"

enum GDL_RET_STATUS gps_mcudl_reset_level_set_and_trigger(
	enum gps_each_link_reset_level level, bool wait_reset_done);
int gps_mcudl_trigger_gps_subsys_reset(bool wait_reset_done, const char *p_reason);

void gps_mcudl_handle_connsys_reset_done(void);

void gps_mcudl_connsys_coredump_init(void);
void gps_mcudl_connsys_coredump_deinit(void);
void gps_mcudl_connsys_coredump_start(void);
void gps_mcudl_connsys_coredump_start_wrapper(void);

#endif /* _GPS_MCUDL_RESET_H */

