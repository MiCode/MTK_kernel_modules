/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCU_HIF_API_H
#define _GPS_MCU_HIF_API_H

#include "gps_mcu_hif_shared_struct.h"

#if 1 /* Impl them on target and host correspondingly */
extern union gps_mcu_hif_ap2mcu_shared_data_union *p_gps_mcu_hif_ap2mcu_region;
extern union gps_mcu_hif_mcu2ap_shared_data_union *p_gps_mcu_hif_mcu2ap_region;

void gps_mcu_hif_init(void);

/* protect critical section such as bitmask uint which may set/clear by tasks */
void gps_mcu_hif_lock(void);
void gps_mcu_hif_unlock(void);

/* return true for okay */
enum gps_mcu_hif_send_status {
	GPS_MCU_HIF_SEND_OK,
	GPS_MCU_HIF_SEND_FAIL_DUE_TO_NOT_FINSIHED,
	GPS_MCU_HIF_SEND_FAIL_DUE_TO_FW_OWN_FAIL,
	GPS_MCU_HIF_SEND_FAIL_DUE_TO_CCIF_BUSY,
	GPS_MCU_HIF_SEND_STATUS_NUM
};

bool gps_mcu_hif_send(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len);

bool gps_mcu_hif_send_v2(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len,
	enum gps_mcu_hif_send_status *if_send_ok);

typedef bool (*gps_mcu_hif_ch_on_recv_cb)(const unsigned char *p_data, unsigned int data_len);

/* data are between rd_idx and wr_idx*/
bool gps_mcu_hif_on_recv_ring(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_buf, unsigned buf_len,
	unsigned int rd_idx, unsigned int wr_idx);
#endif

#if 1 /* Impl them in common or core and reuse them for both sides */
const struct gps_mcu_hif_ap2mcu_shared_data *gps_mcu_hif_get_ap2mcu_shared_data_ptr(void);
const struct gps_mcu_hif_mcu2ap_shared_data *gps_mcu_hif_get_mcu2ap_shared_data_ptr(void);
struct gps_mcu_hif_ap2mcu_shared_data *gps_mcu_hif_get_ap2mcu_shared_data_mut_ptr(void);
struct gps_mcu_hif_mcu2ap_shared_data *gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr(void);

enum gps_mcu_hif_trans gps_mcu_hif_get_ap2mcu_trans(enum gps_mcu_hif_ch hif_ch);
enum gps_mcu_hif_trans gps_mcu_hif_get_mcu2ap_trans(enum gps_mcu_hif_ch hif_ch);
enum gps_mcu_hif_ch gps_mcu_hif_get_trans_hif_ch(enum gps_mcu_hif_trans trans_id);

bool gps_mcu_hif_is_trans_req_sent(enum gps_mcu_hif_trans trans_id);
bool gps_mcu_hif_is_trans_req_received(enum gps_mcu_hif_trans trans_id);
bool gps_mcu_hif_is_trans_req_finished(enum gps_mcu_hif_trans trans_id);

void gps_mcu_hif_host_set_trans_req_sent(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_host_clr_trans_req_sent(enum gps_mcu_hif_trans trans_id);

void gps_mcu_hif_target_set_trans_req_received(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_target_clr_trans_req_received(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_target_set_trans_req_finished(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_target_clr_trans_req_finished(enum gps_mcu_hif_trans trans_id);
#endif

#if 1 /* Target on api */
void gps_mcu_hif_target_on_trans_done(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_target_notify_host_trans_done(enum gps_mcu_hif_trans trans_id);
void gps_mcu_hif_target_on_host_trigger_tx(enum gps_mcu_hif_ch ch);
void gps_mcu_hif_target_on_host_trigger_rx(enum gps_mcu_hif_ch ch);
void gps_mcu_hif_target_ccif_irq_handler_in_task(void);
void gps_mcu_hif_target_ccif_isr(void);
#endif

#endif /* _GPS_MCU_HIF_API_H */
