/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_DATA_PKT_SLOT_H
#define _GPS_MCUDL_DATA_PKT_SLOT_H

#include "gps_mcudl_data_pkt.h"
#include "gps_mcudl_data_pkt_payload_struct.h"

enum gps_mcudl_slot_flush_status {
	FLUSH_OK,
	FLUSH_ERR_WIN_NOT_ENOUGH,
	FLUSH_ERR_SEND_FAIL,
	FLUSH_ERR_INVALID_PARAM,
};

struct gps_mcudl_slot_entry_t {
	struct gps_mcudl_pkt_head *head;
	gpsmdl_u8 *tail;
	gpsmdl_u8 task_id;
	gpsmdl_u8 fg_working;
	gpsmdl_u8 fg_ready;
	gpsmdl_u32 pkt_len;
	gpsmdl_u32 enq_tick;
};

/* return value:
 * non negative : is the length sent
 * negative     : stands for error code
 */
typedef int (*gps_mcudl_intf_send_fn_t)(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len);

struct gps_mcudl_slot_cfg {
	gpsmdl_u8 slot_id;
	gps_mcudl_intf_send_fn_t p_intf_send_fn;

	gpsmdl_u8 *rbuf_ptr;
	gpsmdl_u32 rbuf_len;

	struct gps_mcudl_slot_entry_t *entry_list_ptr;
	gpsmdl_u32 entry_list_len;
};

struct gps_mcudl_slot_rbuf_cursor {
	bool is_full;
	gpsmdl_u32 read_idx;
	gpsmdl_u32 write_idx;
	gpsmdl_u32 rbuf_end; /* It may be set to a value less than cfg.rbuf_len */

	gpsmdl_u32 data_len;
	gpsmdl_u32 left_side_free_len; /* If is rbuf is empty, left size len = 0 */
	gpsmdl_u32 right_side_free_len;
};

struct gps_mcudl_slot_entr_cursor {
	bool is_full;
	gpsmdl_u32 read_idx;
	gpsmdl_u32 write_idx;
	gpsmdl_u32 write_tmp_idx;

	gpsmdl_u32 pkt_cnt;
};

struct gps_mcudl_data_slot_t {
	/* read only */
	struct gps_mcudl_slot_cfg cfg;

	/* runtime */
	struct gps_mcudl_slot_rbuf_cursor rbuf_cursor;
	struct gps_mcudl_slot_entr_cursor entr_cursor;

	gpsmdl_u32 rb_ok_total;
	gpsmdl_u32 rb_ok_peak;
	gpsmdl_u32 rb_fail_total;

	gpsmdl_u32 en_ok_total;
	gpsmdl_u32 en_ok_peak;
	gpsmdl_u32 en_fail_total;
};

void gps_mcudl_slot_init(struct gps_mcudl_data_slot_t *p_slot);

enum gps_mcudl_slot_flush_status gps_mcudl_slot_flush(
	struct gps_mcudl_data_slot_t *p_slot, gpsmdl_u32 *p_flush_done_size);

/* return send_is_okay.
 * okay means pkt is put into slot, but may not be flush to intf yet.
 */
bool gps_mcudl_pkt_send(struct gps_mcudl_data_slot_t *slot,
	enum gps_mcudl_pkt_type type, const gpsmdl_u8 *payload_ptr, gpsmdl_u32 payload_len);

/* Need to impl */
void gps_mcudl_slot_protect(void);
void gps_mcudl_slot_unprotect(void);
bool gps_mcudl_pkt_is_critical_type(gpsmdl_u8 type);
bool gps_mcudl_slot_may_pend_pkt_type_if_near_full(struct gps_mcudl_data_slot_t *p_slot,
	enum gps_mcudl_pkt_type type, int len);

void gps_mcudl_flowctrl_init(enum gps_mcudl_yid yid);
gpsmdl_u32 gps_mcudl_flowctrl_cal_window_size(void);
void gps_mcudl_flowctrl_remote_update_recv_byte(struct gps_mcudl_data_pkt_mcu_sta *p_sta, enum gps_mcudl_yid y_id);
void gps_mcudl_flowctrl_local_add_send_byte(gpsmdl_u32 delta, enum gps_mcudl_yid y_id);
void gps_mcudl_flowctrl_may_send_host_sta(enum gps_mcudl_yid yid);
void gps_mcudl_flowctrl_dump_host_sta(enum gps_mcudl_yid yid);

gpsmdl_u8 *gps_mcudl_slot_pkt_reserve(struct gps_mcudl_data_slot_t *p_slot,
	enum gps_mcudl_pkt_type type, gpsmdl_u32 payload_len,
	struct gps_mcudl_slot_entry_t **p_entr_ret);

void gps_mcudl_slot_pkt_ready(struct gps_mcudl_data_slot_t *p_slot, struct gps_mcudl_slot_entry_t *p_entr);

enum gps_mcudl_slot_flush_status gps_mcudl_slot_flush_inner(
	struct gps_mcudl_data_slot_t *p_slot, gpsmdl_u32 *p_flush_done_size, gpsmdl_u32 reserved_win_sz);
void gps_mcudl_slot_update_pkt_cnt_and_data_len(struct gps_mcudl_data_slot_t *p_slot);

#endif /* _GPS_MCUDL_DATA_PKT_SLOT_H */
