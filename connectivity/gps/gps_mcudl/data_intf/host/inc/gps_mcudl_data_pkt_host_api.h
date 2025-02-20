/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_HOST_API_H
#define _GPS_MCUDL_DATA_PKT_HOST_API_H

#include "gps_mcudl_xlink.h"
#include "gps_mcudl_data_intf_type.h"
#include "gps_mcudl_data_pkt.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_data_pkt_rbuf.h"
#include "gps_mcudl_data_pkt_parser.h"
#include "gps_dl_dma_buf.h"

struct geofence_pkt_host_sta_s {
	struct gps_mcudl_data_pkt_mcu_sta pkt_sta;
	gpsmdl_u64 last_ack_recv_len;
	gpsmdl_u64 last_print_recv_len;
	bool reset_flag;
	bool is_enable;
};

bool gps_mcudl_xid2ypl_type(enum gps_mcudl_xid x_id, enum gps_mcudl_pkt_type *p_type);
bool gps_mcudl_ypl_type2xid(enum gps_mcudl_pkt_type type, enum gps_mcudl_xid *p_xid);

void gps_mcudl_ap2mcu_context_init(enum gps_mcudl_yid yid);
bool gps_mcudl_ap2mcu_xdata_send(enum gps_mcudl_xid xid, struct gdl_dma_buf_entry *p_entry);
bool gps_mcudl_ap2mcu_xdata_send_v2(enum gps_mcudl_xid x_id,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len, bool *p_to_notify);

bool gps_mcudl_ap2mcu_ydata_send(enum gps_mcudl_yid yid, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

void gps_mcudl_mcu2ap_ydata_sta_init(void);
void gps_mcudl_mcu2ap_ydata_sta_may_do_dump(enum gps_mcudl_yid yid, bool force);
unsigned long gps_mcudl_mcu2ap_ydata_sta_get_recv_byte_cnt(enum gps_mcudl_yid yid);

void gps_mcudl_mcu2ap_ydata_recv(enum gps_mcudl_yid yid,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);
void gps_mcudl_mcu2ap_ydata_proc(enum gps_mcudl_yid yid);

void gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(enum gps_mcudl_xid x_id);
void gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(enum gps_mcudl_yid y_id);

void gps_mcudl_mcu_ch1_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);
void gps_mcudl_mcu_ch2_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

int gps_mcudl_mcu_ch1_send_func(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);
int gps_mcudl_mcu_ch2_send_func(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

void gps_mcudl_host_sta_hist_init(void);
void gps_mcudl_host_sta_hist_rec(enum gps_mcudl_yid yid, struct geofence_pkt_host_sta_s *host_sta);
void gps_mcudl_host_sta_hist_dump_rec(unsigned long index, struct geofence_pkt_host_sta_s *host_sta);
void gps_mcudl_host_sta_hist_dump(enum gps_mcudl_yid yid);

bool gps_mcudl_mcu2ap_get_wait_read_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_mcu2ap_set_wait_read_flag(enum gps_mcudl_yid y_id, bool flag);
bool gps_mcudl_ap2mcu_get_wait_write_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_wait_write_flag(enum gps_mcudl_yid y_id, bool flag);
bool gps_mcudl_ap2mcu_get_wait_flush_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_wait_flush_flag(enum gps_mcudl_yid y_id, bool flag);
bool gps_mcudl_ap2mcu_get_write_fail_flag(enum gps_mcudl_yid y_id);
void gps_mcudl_ap2mcu_set_write_fail_flag(enum gps_mcudl_yid y_id, bool flag);

void gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(enum gps_mcudl_yid y_id);
void gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(enum gps_mcudl_yid y_id, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len);

void gps_mcudl_mcu2ap_test_bypass_set(bool bypass);
bool gps_mcudl_mcu2ap_test_bypass_get(void);

void gps_mcudl_mcu2ap_rec_init(void);
void gps_mcudl_mcu2ap_rec_dump(void);

void gps_mcu_host_trans_hist_init(void);
void gps_mcu_host_trans_hist_rec(struct gps_mcudl_data_pkt_rec_item *p_rec, enum gps_mcudl_rec_type rec_point);
void gps_mcu_host_trans_hist_dump(enum gps_mcudl_rec_type rec_point);
bool gps_mcu_host_trans_set_if_need_dump(bool if_print);
bool gps_mcu_host_trans_get_if_need_dump(void);

void gps_mcudl_mcu2ap_put_to_xlink_fail_rec_init(void);
void gps_mcudl_mcu2ap_put_to_xlink_fail_rec_dump(void);

void gps_mcudl_mcu2ap_arrange_pkt_dump_after_ap_resume(void);
void gps_mcudl_mcu2ap_clear_ap_resume_pkt_dump_flag(void);

#endif /* _GPS_MCUDL_DATA_PKT_HOST_API_H */
