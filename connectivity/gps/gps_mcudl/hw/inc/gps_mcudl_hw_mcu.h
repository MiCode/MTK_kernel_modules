/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HW_MCU_H
#define _GPS_MCUDL_HW_MCU_H

#include "gps_mcudl_hw_type.h"
#include "gps_mcudl_xlink.h"

bool gps_mcudl_hw_conn_force_wake(bool enable);
bool gps_mcudl_hw_conn_force_wake_inner(bool enable);

bool gps_mcudl_hw_mcu_do_on_with_rst_held(void);
void gps_mcudl_hw_mcu_speed_up_clock(void);
void gps_mcudl_hw_mcu_release_rst(void);
bool gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us(unsigned int timeout_us);
void gps_mcudl_hw_mcu_show_status(void);
void gps_mcudl_hw_mcu_show_pc_log(void);
bool gps_mcudl_hw_bg_is_readable(void);

bool gps_mcudl_hw_mcu_set_or_clr_fw_own(bool to_set);
bool gps_mcudl_hw_mcu_set_or_clr_fw_own_is_okay(bool check_set);
void gps_mcudl_hw_mcu_do_off(void);

/* tmp*/
#if 1
void gps_dl_hw_set_mcu_emi_remapping_tmp(unsigned int _20msb_of_36bit_phy_addr);
unsigned int gps_dl_hw_get_mcu_emi_remapping_tmp(void);
void gps_dl_hw_set_gps_dyn_remapping_tmp(unsigned int val);
extern void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len);
#endif
/*tmp*/

void gps_mcudl_hw_may_set_link_power_flag(enum gps_mcudl_xid xid, bool power_ctrl);

#endif /* _GPS_MCUDL_HW_MCU_H */

