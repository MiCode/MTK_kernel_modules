/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_mcudl_hal_timer.h"
#include "gps_dl_config.h"
#include "gps_dl_ctrld.h"
#include "gps_mcudl_hal_ccif.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcudl_ylink.h"

#if GPS_DL_STATE_NOTIFY

struct gps_mcudl_timer_context {
	bool init_done;
	struct gps_dl_osal_timer timer_to_monitor;
	bool timer_setup_done;
};

struct gps_mcudl_timer_context g_gps_monitor_ktimer;
struct gps_mcudl_timer_context g_gps_monitor_ctimer;

void gps_mcudl_hal_timer_setup(void)
{
#if GPS_DL_ON_LINUX
	g_gps_monitor_ktimer.timer_to_monitor.timeoutHandler =
		&gps_mcudl_hal_kctrld_timer_timeout_and_ntf;
	g_gps_monitor_ktimer.timer_to_monitor.timeroutHandlerData = 0;
	(void)gps_dl_osal_timer_create(&g_gps_monitor_ktimer.timer_to_monitor);
	g_gps_monitor_ktimer.timer_setup_done = true;

	g_gps_monitor_ctimer.timer_to_monitor.timeoutHandler =
		&gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf;
	g_gps_monitor_ctimer.timer_to_monitor.timeroutHandlerData = 0;
	(void)gps_dl_osal_timer_create(&g_gps_monitor_ctimer.timer_to_monitor);
	g_gps_monitor_ctimer.timer_setup_done = true;
#else
	g_gps_monitor_ktimer.timer_setup_done = false;
	g_gps_monitor_ctimer.timer_setup_done = false;
#endif
}

void gps_mcudl_hal_timer_destroy(void)
{
	g_gps_monitor_ktimer.timer_setup_done = false;
	g_gps_monitor_ctimer.timer_setup_done = false;
}

void gps_mcudl_hal_timer_init(void)
{
	gps_mcul_hal_user_fw_own_lock();
	g_gps_monitor_ktimer.init_done = true;
	g_gps_monitor_ctimer.init_done = true;
	gps_mcul_hal_user_fw_own_unlock();
}

void gps_mcudl_hal_timer_deinit(void)
{
	if (g_gps_monitor_ktimer.timer_setup_done)
		gps_dl_osal_timer_stop_sync(&g_gps_monitor_ktimer.timer_to_monitor);

	if (g_gps_monitor_ctimer.timer_setup_done)
		gps_dl_osal_timer_stop_sync(&g_gps_monitor_ctimer.timer_to_monitor);

	gps_mcul_hal_user_fw_own_lock();
	g_gps_monitor_ktimer.init_done = false;
	g_gps_monitor_ctimer.init_done = false;
	gps_mcul_hal_user_fw_own_unlock();
}

#define KCTRLD_TIMER_1S 1000
#define CCIF_ISR_TIMER_1S 1000
void gps_mcudl_hal_kctrld_timer_refersh(void)
{
	gps_dl_osal_timer_stop(&g_gps_monitor_ktimer.timer_to_monitor);
	gps_dl_osal_timer_start(&g_gps_monitor_ktimer.timer_to_monitor,
		KCTRLD_TIMER_1S);
}

void gps_mcudl_hal_kctrld_timer_start(void)
{
	gps_dl_osal_timer_start(&g_gps_monitor_ktimer.timer_to_monitor, KCTRLD_TIMER_1S);
}

void gps_mcudl_hal_kctrld_timer_stop(void)
{
	gps_dl_osal_timer_stop(&g_gps_monitor_ktimer.timer_to_monitor);
}

void gps_mcudl_hal_ccif_isr_timer_refresh(void)
{
	gps_dl_osal_timer_stop(&g_gps_monitor_ctimer.timer_to_monitor);
	gps_dl_osal_timer_start(&g_gps_monitor_ctimer.timer_to_monitor,
		CCIF_ISR_TIMER_1S);
}

void gps_mcudl_hal_ccif_isr_timer_start(void)
{
	gps_dl_osal_timer_start(&g_gps_monitor_ctimer.timer_to_monitor, CCIF_ISR_TIMER_1S);
}

void gps_mcudl_hal_ccif_isr_timer_stop(void)
{
	gps_dl_osal_timer_stop(&g_gps_monitor_ctimer.timer_to_monitor);
}

void gps_mcudl_hal_kctrld_timer_notify(void)
{
	gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_KCTRLD_TIMER);
}

void gps_mcudl_hal_ccif_isr_timer_notify(void)
{
	gps_mcudl_ylink_event_send(GPS_MDLY_NORMAL, GPS_MCUDL_YLINK_EVT_ID_CCIF_ISR_TIMER);
}

void gps_mcudl_hal_kctrld_timer_timeout_and_ntf_inner(void)
{
	gps_dl_ctrl_thread_check_is_blocking();
	gps_mcudl_hal_kctrld_timer_notify();
}

void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf_inner(void)
{
	gps_mcudl_hal_ccif_isr_is_blocking();
	gps_mcudl_hal_ccif_isr_timer_notify();
}

#if GPS_DL_ON_LINUX
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
void gps_mcudl_hal_kctrld_timer_timeout_and_ntf(struct timer_list *p_timer)
{
	gps_mcudl_hal_kctrld_timer_timeout_and_ntf_inner();
}

void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf(struct timer_list *p_timer)
{
	gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf_inner();
}

#else
void gps_mcudl_hal_kctrld_timer_timeout_and_ntf(unsigned long data)
{
	gps_mcudl_hal_kctrld_timer_timeout_and_ntf_inner();
}

void gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf(unsigned long data)
{
	gps_mcudl_hal_ccif_isr_timer_timeout_and_ntf_inner();
}
#endif
#endif

#endif

