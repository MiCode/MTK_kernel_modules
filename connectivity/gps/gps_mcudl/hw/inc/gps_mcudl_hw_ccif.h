/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HW_CCIF_H
#define _GPS_MCUDL_HW_CCIF_H

#include "gps_mcudl_hw_type.h"
#include "gps_mcudl_hal_ccif.h"

bool gps_mcudl_hw_ccif_is_tch_busy(enum gps_mcudl_ccif_ch ch);
void gps_mcudl_hw_ccif_set_tch_busy(enum gps_mcudl_ccif_ch ch);
unsigned int gps_mcudl_hw_ccif_get_tch_busy_bitmask(void);
void gps_mcudl_hw_ccif_clr_tch_busy_bitmask(void);
void gps_mcudl_hw_ccif_clr_rch_busy_bitmask(void);

void gps_mcudl_hw_ccif_set_tch_start(enum gps_mcudl_ccif_ch ch);
unsigned int gps_mcudl_hw_ccif_get_tch_start_bitmask(void);

unsigned int gps_mcudl_hw_ccif_get_rch_bitmask(void);
void gps_mcudl_hw_ccif_set_rch_ack(enum gps_mcudl_ccif_ch ch);


enum gpsmdl_ccif_misc_cr_group {
	GMDL_CCIF_MISC0,
	GMDL_CCIF_MISC1,
	GMDL_CCIF_MISC_NUM
};
void gps_mcudl_hw_ccif_set_irq_mask(enum gpsmdl_ccif_misc_cr_group id, unsigned int mask);
unsigned int gps_mcudl_hw_ccif_get_irq_mask(enum gpsmdl_ccif_misc_cr_group id);

void gps_mcudl_hw_ccif_set_dummy(enum gpsmdl_ccif_misc_cr_group id, unsigned int val);
unsigned int gps_mcudl_hw_ccif_get_dummy(enum gpsmdl_ccif_misc_cr_group id);

unsigned int gps_mcudl_hw_ccif_get_shadow(enum gpsmdl_ccif_misc_cr_group id);

#endif /* _GPS_MCUDL_HW_CCIF_H */

