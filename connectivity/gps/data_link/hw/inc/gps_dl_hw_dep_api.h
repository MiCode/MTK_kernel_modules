/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_DL_HW_DEP_API_H
#define _GPS_DL_HW_DEP_API_H

#include "gps_dl_config.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_hal_api.h"

/*
 * BGF
 */
void gps_dl_hw_dep_gps_sw_request_peri_usage(bool request);
bool gps_dl_hw_dep_en_gps_func_and_poll_bgf_ack(void);
bool gps_dl_hw_dep_poll_bgf_bus_and_gps_top_ack(void);
void gps_dl_hw_dep_may_set_bus_debug_flag(void);
bool gps_dl_hw_dep_may_enable_bpll(void);
void gps_dl_hw_dep_may_disable_bpll(void);
void gps_dl_hw_dep_may_remap_conn2ap_gps_peri(void);
bool gps_dl_hw_dep_may_check_conn_infra_restore_done(void);
void gps_dl_hw_dep_may_set_conn_infra_l1_request(bool request);
bool gps_dl_hw_dep_may_set_gps_axi_sleep_prot_ctrl(unsigned int val);
void gps_dl_hw_dep_gps_sw_request_emi_usage(bool request);
bool gps_dl_hw_gps_common_on_inner(void);
int gps_dl_hw_gps_sleep_prot_ctrl(int op);
void gps_dl_hw_dep_gps_control_adie_on_inner_1(bool if_on);
bool gps_dl_hw_dep_gps_control_adie_on(void);
void gps_dl_hw_dep_gps_control_adie_off(void);
void gps_dl_hw_dep_adie_mt6686_dump_status(void);
bool gps_dl_hw_dep_gps_get_ecid_info(void);

struct gps_dl_power_raw_state {
	unsigned int mcu_pc;
	unsigned int is_hw_clk_ext;
	unsigned int sw_gps_ctrl;
};
void gps_dl_hw_dep_gps_dump_power_state(struct gps_dl_power_raw_state *p_raw);
unsigned int gps_dl_hw_gps_get_adie_id_from_conninfra(void);

/*
 * GPS
 */
bool gps_dl_hw_dep_set_dsp_on_and_poll_ack(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_set_dsp_off(enum gps_dl_link_id_enum link_id);
bool gps_dl_hw_dep_poll_gps_sleep_state(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_set_dsp_dpstop(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_cfg_dsp_mem(enum dsp_ctrl_enum ctrl);
void gps_dl_hw_dep_common_enter_dpstop_dsleep(void);
void gps_dl_hw_dep_common_leave_dpstop_dsleep(void);
void gps_dl_hw_dep_common_clear_wakeup_source(void);

/*
 * Debug
 */
void gps_dl_hw_dep_dump_gps_pos_info(enum gps_dl_link_id_enum link_id);
void gps_dl_hw_dep_dump_host_csr_range(unsigned int flag_start, unsigned int len);
void gps_dl_hw_dep_dump_host_csr_gps_info(void);
void gps_dl_hw_dep_dump_host_csr_conninfra_info(void);

/*
 * DMA
 */
struct gps_dl_hw_dma_status_struct {
	unsigned int wrap_count;
	unsigned int wrap_to_addr;
	unsigned int total_count;
	unsigned int config;
	unsigned int start_flag;
	unsigned int intr_flag;
	unsigned int left_count;
	unsigned int curr_addr;
	unsigned int state;
};

void gps_dl_hw_dep_set_dma_start(enum gps_dl_hal_dma_ch_index channel,
	struct gdl_hw_dma_transfer *p_transfer);
void gps_dl_hw_dep_set_dma_stop(enum gps_dl_hal_dma_ch_index channel);
bool gps_dl_hw_dep_get_dma_int_status(enum gps_dl_hal_dma_ch_index channel);
void gps_dl_hw_dep_save_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p);
void gps_dl_hw_dep_print_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p);
unsigned int gps_dl_hw_dep_get_dma_left_len(enum gps_dl_hal_dma_ch_index channel);

/* Only need when BMASK_RW_DO_CHECK active for debug purpose */
void gps_dl_hw_dep_may_do_bus_check_and_print(unsigned int host_addr);
void gps_dl_hw_dep_dump_gps_rf_temp_cr(void);

/*
 * MET2.0
 */
#if GPS_DL_MET_V2
void gps_dl_hw_dep_set_emi_write_range(unsigned int bus_emi_met_phy_addr);
#else
void gps_dl_hw_dep_set_emi_write_range(void);
#endif
void gps_dl_hw_dep_set_ringbuffer_mode(unsigned int mode);
void gps_dl_hw_dep_set_sampling_rate(unsigned int rate);
void gps_dl_hw_dep_set_mask_signal(unsigned int mask_signal);
void gps_dl_hw_dep_set_edge_detection(unsigned int edge);
void gps_dl_hw_dep_set_event_signal(unsigned int event_signal);
void gps_dl_hw_dep_set_event_select(unsigned int event_select);
void gps_dl_hw_dep_enable_met(void);
void gps_dl_hw_dep_disable_met(void);
unsigned int gps_dl_hw_dep_get_met_read_ptr_addr(void);
unsigned int gps_dl_hw_dep_get_met_write_ptr_addr(void);
void gps_dl_hw_dep_set_timer_source(unsigned int timer_source);
#if GPS_DL_MET_V2
void gps_dl_hw_dep_set_mask_signal2(unsigned int mask_signal);
void gps_dl_hw_dep_set_edge_detection2(unsigned int edge);
void gps_dl_hw_dep_set_edge_detection3(unsigned int edge);
void gps_dl_hw_dep_set_edge_detection4(unsigned int edge);
#endif

struct gps_dl_hw_host_csr_dump_range {
	unsigned int flag_start;
	unsigned int len;
};
#endif

/*
 * A-Die ctrl
 */
bool gps_dl_hw_dep_gps_control_adie_on_6985(void);
bool gps_dl_hw_dep_gps_control_adie_on_6989(void);
bool gps_dl_hw_dep_gps_control_adie_on_6991(void);

void gps_dl_hw_dep_gps_control_adie_off_6985(void);
void gps_dl_hw_dep_gps_control_adie_off_6989(void);
bool gps_dl_hw_dep_gps_control_adie_on_6878(void);
void gps_dl_hw_dep_gps_control_adie_off_6878(void);
void gps_dl_hw_dep_gps_control_adie_off_6991(void);

