/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_DL_HW_DEP_API_H
#define _GPS_DL_HW_DEP_API_H

#include "gps_dl_config.h"
#include "gps_dsp_fsm.h"

/*
 * BGF
 */
bool gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack(void);
bool gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack(void);
void gps_dl_hw_dep_may_set_bus_debug_flag(void);

/*
 * GPS
 */
bool gps_dl_hw_dep_set_dsp_on_and_poll_ack(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_set_dsp_off(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_cfg_dsp_mem(enum dsp_ctrl_enum ctrl);

/*
 * Debug
 */
void gps_dl_hw_dep_dump_host_csr_gps_info(void);
void gps_dl_hw_dep_dump_host_csr_conninfra_info(void);

/* Only need when BMASK_RW_DO_CHECK active for debug purpose */
void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr);


#endif

