/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_TIMER_H
#define _GPS_MCUDL_HAL_TIMER_H

#include "gps_dl_osal.h"
#include "gps_dl_config.h"

#if GPS_DL_STATE_NOTIFY

void gps_mcudl_hal_timer_setup(void);
void gps_mcudl_hal_timer_destroy(void);
void gps_mcudl_hal_timer_init(void);
void gps_mcudl_hal_timer_deinit(void);
void gps_mcudl_hal_kctrld_timer_refersh(void);
void gps_mcudl_hal_kctrld_timer_start(void);
void gps_mcudl_hal_kctrld_timer_stop(void);
void gps_mcudl_hal_ccif_isr_timer_refresh(void);
void gps_mcudl_hal_ccif_isr_timer_start(void);
void gps_mcudl_hal_ccif_isr_timer_stop(void);
void gps_mcudl_hal_kctrld_timer_notify(void);
void gps_mcudl_hal_ccif_isr_timer_notify(void);
void gps_mcudl_hal_kctrld_timer_timeout_and_ntf_inner(void);
void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf_inner(void);

#if GPS_DL_ON_LINUX
void gps_mcudl_hal_kctrld_timer_timeout_and_ntf(struct timer_list *p_timer);
void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf(struct timer_list *p_timer);
#else
void gps_mcudl_hal_kctrld_timer_timeout_and_ntf(unsigned long data);
void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf(unsigned long data);
#endif

#endif

#endif /* _GPS_MCUDL_HAL_TIMER_H */

