/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_MBRAINK_H
#define _GPS_MCUDL_MBRAINK_H

#include "mbraink_bridge_gps.h"

enum gnss2mbr_status gps_mcudl_mbraink2gps_get_lp_data(enum mbr2gnss_reason, struct gnss2mbr_lp_data *);
enum gnss2mbr_status gps_mcudl_mbraink2gps_get_mcu_data(enum mbr2gnss_reason, struct gnss2mbr_mcu_data *);

void gps_mcudl_linux_register_cbs_to_mbraink(void);
void gps_mcudl_linux_unregister_cbs_to_mbraink(void);

#endif /* _GPS_MCUDL_MBRAINK_H */

