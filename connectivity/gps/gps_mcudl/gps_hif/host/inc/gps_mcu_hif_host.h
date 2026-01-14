/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCU_HIF_HOST_H
#define _GPS_MCU_HIF_HOST_H

#include "gps_mcu_hif_api.h"

void gps_mcu_hif_host_init_ch(enum gps_mcu_hif_ch hif_ch);
void gps_mcu_hif_recv_start(enum gps_mcu_hif_ch hif_ch);
void gps_mcu_hif_recv_stop(enum gps_mcu_hif_ch hif_ch);
void gps_mcu_hif_recv_listen_start(enum gps_mcu_hif_ch hif_ch, gps_mcu_hif_ch_on_recv_cb custom_cb);
void gps_mcu_hif_recv_listen_stop(enum gps_mcu_hif_ch hif_ch);
void gps_mcu_hif_host_on_tx_finished(enum gps_mcu_hif_ch hif_ch, unsigned int data_len);
void gps_mcu_hif_host_on_rx_finished(enum gps_mcu_hif_ch hif_ch, unsigned int data_len);
void gps_mcu_hif_host_trans_finished(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_host_ccif_irq_handler_in_isr(void);

void gps_mcu_hif_host_dump_ch(enum gps_mcu_hif_ch hif_ch);
void gps_mcu_hif_host_dump_trans(enum gps_mcu_hif_trans trans_id);

void gps_mcu_hif_host_trans_hist_init(void);
void gps_mcu_hif_host_trans_hist_rec(struct gps_mcu_hif_trans_rec *p_rec);
void gps_mcu_hif_host_trans_hist_dump(void);
bool gps_mcu_hif_get_mcu2ap_recv_fail_flag(enum gps_mcu_hif_ch ch);
void gps_mcu_hif_set_mcu2ap_recv_fail_flag(enum gps_mcu_hif_ch ch, bool flag);

#endif /* _GPS_MCU_HIF_HOST_H */
