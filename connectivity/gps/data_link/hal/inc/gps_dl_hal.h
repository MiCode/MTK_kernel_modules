/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DL_HAL_H
#define _GPS_DL_HAL_H

#include "gps_dl_config.h"
#include "gps_dl_dma_buf.h"
#include "gps_dl_hal_type.h"
#include "gps_dl_isr.h"
#include "gps_dl_hw_dep_macro.h"

/* for gps_each_device.c */


/* for gps_each_link.c */
enum gps_dl_hal_power_ctrl_op_enum {
	GPS_DL_HAL_POWER_OFF = 0,
	GPS_DL_HAL_POWER_ON = 1,
	GPS_DL_HAL_ENTER_DPSLEEP = 2,
	GPS_DL_HAL_LEAVE_DPSLEEP = 3,
	GPS_DL_HAL_ENTER_DPSTOP = 4,
	GPS_DL_HAL_LEAVE_DPSTOP = 5,
	GPS_DL_HAL_POWER_OP_MAX
};

struct gps_each_dsp_reg_read_value {
	unsigned int record_d2a_index;
	/* g_gps_rec_dsp_value include d2a0 history value and last d2a1 value */
	unsigned int g_gps_rec_dsp_value[GPS_DSP_REG_DBG_POLL_MAX + 1];
};

bool gps_dl_hal_conn_infra_driver_on(void);
void gps_dl_hal_conn_infra_driver_off(void);
void gps_dl_hal_conn_infra_driver_debug_dump(void);
void gps_dl_hal_link_confirm_dma_stop(enum gps_dl_link_id_enum link_id);
int gps_dl_hal_conn_power_ctrl(enum gps_dl_link_id_enum link_id, int op);
int gps_dl_hal_link_power_ctrl(enum gps_dl_link_id_enum link_id,
	enum gps_dl_hal_power_ctrl_op_enum op);
int gps_dl_hal_link_power_ctrl_inner(enum gps_dl_link_id_enum link_id,
	enum gps_dl_hal_power_ctrl_op_enum op);
void gps_dl_hal_link_clear_hw_pwr_stat(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_link_may_disable_bpll(void);
#if GPS_DL_ON_LINUX
bool gps_dl_hal_md_blanking_init_pta(void);
void gps_dl_hal_md_blanking_deinit_pta(void);
#endif
void gps_dl_hal_md_blanking_ctrl(bool on);
void gps_dl_hal_a2d_tx_dma_start(enum gps_dl_link_id_enum link_id,
	struct gdl_dma_buf_entry *p_entry);
void gps_dl_hal_d2a_rx_dma_start(enum gps_dl_link_id_enum link_id,
	struct gdl_dma_buf_entry *p_entry);

void gps_dl_hal_a2d_tx_dma_stop(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_d2a_rx_dma_stop(enum gps_dl_link_id_enum link_id);

unsigned int gps_dl_hal_d2a_rx_dma_get_rx_len(enum gps_dl_link_id_enum link_id);
enum GDL_RET_STATUS gps_dl_hal_d2a_rx_dma_get_write_index(
	enum gps_dl_link_id_enum link_id, unsigned int *p_write_index);

enum GDL_RET_STATUS gps_dl_hal_a2d_tx_dma_wait_until_done_and_stop_it(
	enum gps_dl_link_id_enum link_id, int timeout_usec, bool return_if_not_start);
enum GDL_RET_STATUS gps_dl_hal_d2a_rx_dma_wait_until_done(
	enum gps_dl_link_id_enum link_id, int timeout_usec);
enum GDL_RET_STATUS gps_dl_hal_wait_and_handle_until_usrt_has_data(
	enum gps_dl_link_id_enum link_id, int timeout_usec);
enum GDL_RET_STATUS gps_dl_hal_wait_and_handle_until_usrt_has_nodata_or_rx_dma_done(
	enum gps_dl_link_id_enum link_id, int timeout_usec, bool to_handle);

enum GDL_RET_STATUS gps_dl_hal_poll_event(
	unsigned int L1_evt_in, unsigned int L5_evt_in,
	unsigned int *pL1_evt_out, unsigned int *pL5_evt_out, unsigned int timeout_usec);

int gps_dl_hal_usrt_direct_write(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len);
int gps_dl_hal_usrt_direct_read(enum gps_dl_link_id_enum link_id,
	unsigned char *buf, unsigned int len);

void gps_each_dsp_reg_read_ack(
	enum gps_dl_link_id_enum link_id, const struct gps_dl_hal_mcub_info *p_d2a);
void gps_each_dsp_reg_dump_if_any_rec(enum gps_dl_link_id_enum link_id);
void gps_each_dsp_reg_gourp_read_init(enum gps_dl_link_id_enum link_id);
void gps_each_dsp_reg_gourp_read_start(enum gps_dl_link_id_enum link_id,
	bool dbg, unsigned int round_max);
void gps_dl_hal_show_dsp_reg(unsigned char *tag,  enum gps_dl_link_id_enum link_id,
	unsigned int *buf, unsigned int len);

enum GDL_RET_STATUS gps_each_dsp_reg_read_request(
	enum gps_dl_link_id_enum link_id, unsigned int reg_addr);
void gps_each_dsp_reg_gourp_read_next(enum gps_dl_link_id_enum link_id, bool restart);

bool gps_dl_hal_get_dma_irq_en_flag(void);
void gps_dl_hal_set_dma_irq_en_flag(bool enable);
bool gps_dl_hal_get_mcub_irq_dis_flag(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_set_mcub_irq_dis_flag(enum gps_dl_link_id_enum link_id, bool disable);
bool gps_dl_hal_get_irq_dis_flag(enum gps_dl_link_id_enum link_id,
	enum gps_dl_each_link_irq_type type);
void gps_dl_hal_set_irq_dis_flag(enum gps_dl_link_id_enum link_id,
	enum gps_dl_each_link_irq_type type, bool disable);
bool gps_dl_hal_get_need_clk_ext_flag(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_set_need_clk_ext_flag(enum gps_dl_link_id_enum link_id, bool need);

#define GPSDL_CLOCK_FLAG_TCXO      (0x00)
#define GPSDL_CLOCK_FLAG_COTMS     (0x21)
#define GPSDL_CLOCK_FLAG_52M_COTMS (0x51)
int gps_dl_hal_get_clock_flag(void);
void gps_dl_hal_load_clock_flag(void);
bool gps_dl_hal_get_deep_stop_mode_revert_for_mvcd(enum gps_dl_link_id_enum link_id);
void gps_dl_hal_set_deep_stop_mode_revert_for_mvcd(enum gps_dl_link_id_enum link_id, bool revert_for_mvcd);

void gps_dl_hal_set_conn_infra_ver(unsigned int ver);
unsigned int gps_dl_hal_get_conn_infra_ver(void);
void gps_dl_hal_set_adie_ver(unsigned int ver);
unsigned int gps_dl_hal_get_adie_ver(void);
bool gps_dl_hal_conn_infra_ver_is_mt6885(void);
bool gps_dl_hal_conn_infra_ver_is_mt6893(void);

#endif /* _GPS_DL_HAL_H */

