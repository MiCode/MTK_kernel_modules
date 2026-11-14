/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_PLAT_API_H
#define _GPS_MCUDL_PLAT_API_H

/* impl@gps_emi.c */
void gps_mcudl_may_do_fw_loading(void);
void gps_mcudl_clear_fw_loading_done_flag(void);
void gps_mcudl_set_need_to_load_fw_in_drv(bool need);


/* impl@stpgps or dedicated_mcu.c */
int gps_mcudl_plat_mcu_open(void);
int gps_mcudl_plat_mcu_close(void);
int gps_mcudl_plat_mcu_ch1_write(const unsigned char *kbuf, unsigned int count);
int gps_mcudl_plat_mcu_ch2_write(const unsigned char *kbuf, unsigned int count);
int gps_mcudl_plat_mcu_ch1_read_nonblock(unsigned char *kbuf, unsigned int count);


/* impl@gps_mcudl */
void gps_mcudl_plat_mcu_ch1_event_cb(void);
void gps_mcudl_plat_mcu_ch1_read_proc(void);
void gps_mcudl_plat_mcu_ch1_read_proc2(const unsigned char *p_data, unsigned int data_len);

void gps_mcudl_plat_mcu_ch1_reset_start_cb(void);
void gps_mcudl_plat_mcu_ch1_reset_end_cb(void);

extern bool g_gps_conninfa_on;
extern bool g_gps_tia_on;


#endif /* _GPS_MCUDL_PLAT_API_H */

