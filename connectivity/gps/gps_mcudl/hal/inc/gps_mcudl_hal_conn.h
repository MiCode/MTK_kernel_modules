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
bool gps_mcudl_hal_dump_power_state(void);

#endif /* _GPS_MCUDL_HAL_CONN_H */

