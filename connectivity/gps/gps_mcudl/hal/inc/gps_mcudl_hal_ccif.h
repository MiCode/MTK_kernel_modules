/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_CCIF_H
#define _GPS_MCUDL_HAL_CCIF_H

#include "gps_mcudl_hw_type.h"

enum gps_mcudl_ccif_ch {
	GPS_MCUDL_CCIF_CH1 = 0,
	GPS_MCUDL_CCIF_CH2 = 1,
	GPS_MCUDL_CCIF_CH3 = 2,
	GPS_MCUDL_CCIF_CH4 = 3,
	GPS_MCUDL_CCIF_CH5 = 4,
	GPS_MCUDL_CCIF_CH6 = 5,
	GPS_MCUDL_CCIF_CH7 = 6,
	GPS_MCUDL_CCIF_CH8 = 7,
	GPS_MCUDL_CCIF_CH_NUM
};

bool gps_mcudl_hal_ccif_tx_is_busy(enum gps_mcudl_ccif_ch ch);
void gps_mcudl_hal_ccif_tx_prepare(enum gps_mcudl_ccif_ch ch);
void gps_mcudl_hal_ccif_tx_trigger(enum gps_mcudl_ccif_ch ch);
bool gps_mcudl_hal_ccif_tx_is_ack_done(enum gps_mcudl_ccif_ch ch);

void gps_mcudl_hal_ccif_show_status(void);

extern bool g_gps_fw_log_is_on;
extern unsigned int g_gps_fw_log_irq_cnt;
extern unsigned long g_gps_ccif_irq_cnt;
void gps_mcudl_hal_ccif_rx_isr(void);
void gps_mcudl_hal_wdt_isr(void);
void gps_mcudl_hal_wdt_dump(void);

bool gps_mcudl_hal_get_ccif_irq_en_flag(void);
void gps_mcudl_hal_set_ccif_irq_en_flag(bool enable);
void gps_mcudl_hal_wdt_init(void);

#endif /* _GPS_MCUDL_HAL_CCIF_H */

