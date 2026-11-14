/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_data_pkt_slot.h"

#define GPS_MCUDL_INTF_SEND_MAX (1900)		/* Should be larger than GPSMDL_PKT_PAYLOAD_MAX */
#define GPS_MCUDL_PKT_WIN_RESERVED_SIZE (0) /* Now, it's set to 0 */
#define GPS_MCUDL_PKT_WIN_MARGIN (10)
#define UT_TRC(...)
#define GFNS_ASSERT(...)
#define GFNS_ASSERT_4(...)

bool gps_mcudl_pkt_send(struct gps_mcudl_data_slot_t *p_slot, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u32 payload_len)
{
	bool is_okay = false;
	gpsmdl_u8 *p_buf;
	struct gps_mcudl_slot_entry_t *entr;

	p_buf = gps_mcudl_slot_pkt_reserve(p_slot, type, payload_len, &entr);
	UT_TRC("p_buf=%p", p_buf);
	if (p_buf) {
		if (payload_ptr && payload_len > 0)
			memcpy(p_buf, payload_ptr, payload_len);

		UT_TRC("p_buf=%p, entry = %p", p_buf, entr);
		gps_mcudl_slot_pkt_ready(p_slot, entr);
		is_okay = true;
	}

	/* is_okay = true means data is put into p_slot*/
	return is_okay;
}

/* void gps_mcudl_slot_entr_disp(struct gps_mcudl_data_slot_t *slot, struct gps_mcudl_slot_entry_t *entr)*/
#define gps_mcudl_slot_entr_disp(slot, entr)                                                \
{                                                                                           \
	UT_TRC("");                                                                             \
	UT_TRC("entr: en_s = %p, entr = %p, idx = %d", slot->en_s, entr,                        \
	(int)((int)entr - (int)slot->en_s) / (int)sizeof(*entr));                               \
	UT_TRC("entr: head = %p, tail = %p, len = %d", entr->head, entr->tail, entr->pkt_len);  \
	UT_TRC("entr: flag: working = %d, ready = %d",                                          \
		entr->fg_working, entr->fg_ready);                                                  \
}
/* void gps_mcudl_slot_disp(struct gps_mcudl_data_slot_t *slot)*/
#define gps_mcudl_slot_disp(slot)                                                                 \
{                                                                                             \
	UT_TRC("");                                                                               \
	UT_TRC("slot: ptr = %p", slot);                                                           \
	UT_TRC("slot: rb_s = %p", slot->rb_s);                                                    \
	UT_TRC("slot: rb: rd = %d, ok = %d, wr = %d, bk = %d, ln = %d",                           \
			slot->rb_rd, slot->rb_ok, slot->rb_wr, slot->rb_bk, slot->rb_ln);                  \
	UT_TRC("slot: rb: lfree = %d, rfree = %d", slot->rb_lfree, slot->rb_rfree);               \
	UT_TRC("slot: rb: flag, full = %d, wrap(not used) = %d", slot->fg_rb_fu, slot->fg_rb_wp); \
	\
	UT_TRC("slot: en_s = %p", slot->en_s);                                                    \
	UT_TRC("slot: en: rd = %d, ok = %d, wr = %d, ln = %d",                                    \
			slot->en_rd, slot->en_ok, slot->en_wr, slot->en_ln);                               \
	UT_TRC("slot: en: flag, full = %d, wrap(not used) = %d", slot->fg_en_fu, slot->fg_en_wp); \
}

#if 0
void gps_mcudl_slot_protect(void)
{
}

void gps_mcudl_slot_unprotect(void)
{
}
#endif

void gps_mcudl_slot_init(struct gps_mcudl_data_slot_t *p_slot)
{
	p_slot->rbuf_cursor.is_full = 0;
	p_slot->rbuf_cursor.write_idx = 0;
	p_slot->rbuf_cursor.read_idx = 0;

	p_slot->rbuf_cursor.data_len = 0;
	p_slot->rbuf_cursor.rbuf_end = p_slot->cfg.rbuf_len;
	p_slot->rbuf_cursor.right_side_free_len = p_slot->rbuf_cursor.rbuf_end;
	p_slot->rbuf_cursor.left_side_free_len = 0;

	p_slot->entr_cursor.is_full = 0;
	p_slot->entr_cursor.read_idx = 0;
	p_slot->entr_cursor.write_idx = 0;
	p_slot->entr_cursor.write_tmp_idx = 0;
	p_slot->entr_cursor.pkt_cnt = 0;

	p_slot->rb_ok_total = 0;
	p_slot->rb_ok_peak = 0;
	p_slot->rb_fail_total = 0;
	p_slot->en_ok_total = 0;
	p_slot->en_ok_peak = 0;
	p_slot->en_fail_total = 0;
}

/* !! check consistency START*/
#if 1
#define GFNS_CHECK_FREE_CONSISTENCY()
#else
#define GFNS_CHECK_FREE_CONSISTENCY()                                               \
do {                                                                              \
	int rfree;                                                                  \
	int lfree;                                                                  \
	if (slot->rb_wr == slot->rb_rd) {                                             \
		GFNS_ASSERT(((slot->rb_wr == 0 && slot->rb_rfree == slot->rb_ln) ||     \
					(slot->rb_rfree + slot->rb_lfree == 0)));                   \
		break;                                                                  \
	}                                                                           \
	if (slot->rb_wr > slot->rb_rd) {                                            \
		lfree = slot->rb_rd;                                                    \
		rfree = slot->rb_bk - slot->rb_wr;                                      \
	} else {                                                                    \
		lfree = 0;                                                              \
		rfree = slot->rb_rd - slot->rb_wr;                                      \
	}                                                                           \
	GFNS_ASSERT_4((slot->rb_rfree == rfree), slot->rb_rfree, rfree, (int)slot); \
	GFNS_ASSERT_4((slot->rb_lfree == lfree), slot->rb_lfree, lfree, (int)slot); \
} while (0)
#endif
/* !! check consistency END*/

#define RESERVED_SPACE_FOR_CRITICAL_PKT (256)

struct gps_mcudl_slot_entry_t *gps_mcudl_pkt_reserve_entry_and_rbuf(struct gps_mcudl_data_slot_t *p_slot,
	enum gps_mcudl_pkt_type type, gpsmdl_u32 len)
{
	struct gps_mcudl_slot_entry_t *p_entr;
	gpsmdl_u8 slot_id;

	if (!p_slot || len <= 0)
		return NULL;
	slot_id = p_slot->cfg.slot_id;

	GFNS_CHECK_FREE_CONSISTENCY();

	/* # check has free entry*/
	if (p_slot->entr_cursor.is_full) {
		p_slot->en_fail_total += 1;
		p_slot->rb_fail_total += len;
		return NULL;
	}

	if (p_slot->rbuf_cursor.left_side_free_len < len &&
		p_slot->rbuf_cursor.right_side_free_len < len) {
		p_slot->en_fail_total += 1;
		p_slot->rb_fail_total += len;
		return NULL;
	}

	if (!gps_mcudl_pkt_is_critical_type(type)) {
		if (len > p_slot->rbuf_cursor.right_side_free_len &&
			len + RESERVED_SPACE_FOR_CRITICAL_PKT > p_slot->rbuf_cursor.left_side_free_len)
			return NULL;

		if (len <= p_slot->rbuf_cursor.right_side_free_len &&
			len + RESERVED_SPACE_FOR_CRITICAL_PKT > p_slot->rbuf_cursor.right_side_free_len &&
			RESERVED_SPACE_FOR_CRITICAL_PKT > p_slot->rbuf_cursor.left_side_free_len)
			return NULL;
	}

	gps_mcudl_slot_disp(p_slot);

	/* # take a free entry*/
	p_entr = p_slot->cfg.entry_list_ptr + p_slot->entr_cursor.write_tmp_idx;

	/* # entr_cursor.write_idx to next*/
	p_slot->entr_cursor.write_tmp_idx++;
	if (p_slot->entr_cursor.write_tmp_idx >= p_slot->cfg.entry_list_len)
		p_slot->entr_cursor.write_tmp_idx = 0;

	if (p_slot->entr_cursor.write_tmp_idx == p_slot->entr_cursor.read_idx)
		p_slot->entr_cursor.is_full = 1;

	/* # get free space*/
	if (p_slot->rbuf_cursor.right_side_free_len >= len) {

		/* 2014-09-25: Should not check this, due to case: [DATA FREE DATA BK]*/
		/* GFNS_ASSERT_4((p_slot->rb_bk == p_slot->rb_ln), (int)p_slot, p_slot->rb_bk, p_slot->rb_ln);*/

		/* #pkt not wrap*/
		/* p_entr->fg_rb_wrapped = 0;*/
		p_entr->head = (struct gps_mcudl_pkt_head *)(p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.read_idx);
		p_entr->tail = p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.read_idx + len - GPSMDL_PKT_TAIL_LEN;

		if (p_slot->rbuf_cursor.right_side_free_len != len) {
			/* p_slot->rb_bk : No change*/
			p_slot->rbuf_cursor.read_idx += len;
			p_slot->rbuf_cursor.right_side_free_len -= len;
			/* p_slot->rb_lfree : No change*/
			GFNS_CHECK_FREE_CONSISTENCY();
		} else {
			if (p_slot->rbuf_cursor.left_side_free_len > 0) {
				/* [FREE DATA FREE] case*/
				GFNS_ASSERT(p_slot->rbuf_cursor.rbuf_end == p_slot->cfg.rbuf_len);
				p_slot->rbuf_cursor.read_idx = 0;
				p_slot->rbuf_cursor.right_side_free_len = p_slot->rbuf_cursor.left_side_free_len;
				p_slot->rbuf_cursor.left_side_free_len = 0;
#if 0 /* ISSUE*/
				p_slot->rbuf_cursor.rbuf_end = p_slot->cfg.rbuf_len;
				p_slot->rbuf_cursor.read_idx = 0;
				p_slot->rbuf_cursor.right_side_free_len = p_slot->cfg.rbuf_len - len;
				p_slot->rbuf_cursor.left_side_free_len = 0;
#endif
				GFNS_CHECK_FREE_CONSISTENCY();
			} else {
#if 0 /* ISSUE*/
	  /* [DATA w FREE r DATA BK] case*/
				GFNS_ASSERT_4((p_slot->rbuf_cursor.write_idx < p_slot->rbuf_cursor.rbuf_end),
				p_slot->rbuf_cursor.write_idx, p_slot->rbuf_cursor.data_len, 0);
				p_slot->rbuf_cursor.read_idx += len;
				p_slot->rbuf_cursor.right_side_free_len -= len;
#endif
				/* rfree == len, lfree == 0 :=> full!*/
				/* [DATA w FREE r DATA BK] case or [DATA w FREE l] case*/
				p_slot->rbuf_cursor.read_idx += len;
				if (p_slot->rbuf_cursor.read_idx >= p_slot->rbuf_cursor.rbuf_end)
					p_slot->rbuf_cursor.read_idx = 0; /* full flag is set in later code*/

				p_slot->rbuf_cursor.right_side_free_len -= len;

				/* BUFFER FULL CASE*/
				GFNS_ASSERT_4(p_slot->rbuf_cursor.read_idx ==
					p_slot->rbuf_cursor.write_idx,
					p_slot->rbuf_cursor.read_idx, p_slot->rbuf_cursor.write_idx, 0);
				GFNS_ASSERT_4(p_slot->rbuf_cursor.right_side_free_len == 0,
					p_slot->rbuf_cursor.right_side_free_len,
					p_slot->rbuf_cursor.left_side_free_len, 0);
				GFNS_CHECK_FREE_CONSISTENCY();
			}
		}
	} else {
		/*(p_slot->rb_rfree < len)*/
		p_entr->head = (struct gps_mcudl_pkt_head *)(p_slot->cfg.rbuf_ptr);
		p_entr->tail = p_slot->cfg.rbuf_ptr + len - GPSMDL_PKT_TAIL_LEN;

		GFNS_ASSERT_4((p_slot->rbuf_cursor.right_side_free_len > 0), 0, 0, 0);
		GFNS_ASSERT_4((p_slot->cfg.rbuf_len == p_slot->rbuf_cursor.rbuf_end),
					p_slot->cfg.rbuf_len, p_slot->rbuf_cursor.rbuf_end,
					p_slot->rbuf_cursor.right_side_free_len);

		/* Not allow p_slot->rb_rd == p_slot->rb_wr here*/
		GFNS_ASSERT_4((p_slot->rbuf_cursor.write_idx < p_slot->rbuf_cursor.read_idx), 0, 0, 0);
		GFNS_ASSERT_4((p_slot->rbuf_cursor.left_side_free_len >= len),
			p_slot->rbuf_cursor.left_side_free_len, len, 0);
		GFNS_ASSERT_4((p_slot->rbuf_cursor.left_side_free_len == p_slot->rbuf_cursor.write_idx),
			p_slot->rbuf_cursor.left_side_free_len, p_slot->rbuf_cursor.write_idx, 0);

		p_slot->rbuf_cursor.rbuf_end = p_slot->rbuf_cursor.read_idx;
		p_slot->rbuf_cursor.read_idx = len;
		p_slot->rbuf_cursor.right_side_free_len = p_slot->rbuf_cursor.left_side_free_len - len;
		p_slot->rbuf_cursor.left_side_free_len = 0;
		GFNS_CHECK_FREE_CONSISTENCY();
	}
	/* # use lfree + rfree to detect rb full*/

	if (p_slot->rbuf_cursor.left_side_free_len + p_slot->rbuf_cursor.right_side_free_len == 0)
		p_slot->rbuf_cursor.is_full = 1;
	else
		p_slot->rbuf_cursor.is_full = 0;

	/* p_entr->pkt_len_total = pkt_len_total + len;*/

	p_entr->fg_working = 1;
	p_entr->fg_ready = 0;
	p_entr->pkt_len = len;
	/* p_entr->enq_tick = cos_get_systime();*/

	/* # fill p_entr->head->*/
	p_entr->head->start_char = GPSMDL_PKT_START_CHAR;
	p_entr->head->seq = (0 & 0x3F);				  /* 0 ~ 63 */
	p_entr->head->seq |= ((slot_id << 5) & 0xE0); /* 0 ~ 8 */
	p_entr->head->type = (gpsmdl_u8)type;
	p_entr->head->payload_len1 = (len - GPSMDL_PKT_HEAD_LEN - 1) / 256;
	p_entr->head->payload_len2 = (len - GPSMDL_PKT_HEAD_LEN - 1) % 256;
	p_entr->head->chksum =
		p_entr->head->seq + p_entr->head->type + p_entr->head->payload_len1 + p_entr->head->payload_len2;

	if (
		((p_entr->tail + 1)) ==
		((p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.rbuf_end))) {
		GFNS_ASSERT_4((((gpsmdl_u32)(p_slot->cfg.rbuf_ptr)) == (gpsmdl_u32)(p_slot->cfg.rbuf_ptr +
			p_slot->rbuf_cursor.read_idx)), (int)p_entr->tail,
			(int)p_slot->cfg.rbuf_ptr, p_slot->rbuf_cursor.read_idx);
	} else {
		GFNS_ASSERT_4((((gpsmdl_u32)(p_entr->tail + 1)) == (gpsmdl_u32)(p_slot->cfg.rbuf_ptr +
			p_slot->rbuf_cursor.read_idx)), (int)p_entr->tail,
			(int)p_slot->cfg.rbuf_ptr, p_slot->rbuf_cursor.read_idx);
	}

	p_slot->en_ok_total += 1;
	p_slot->rb_ok_total += len;

	gps_mcudl_slot_disp(p_slot);
	gps_mcudl_slot_entr_disp(p_slot, p_entr);

	return p_entr;
}

gpsmdl_u8 *gps_mcudl_slot_pkt_reserve(struct gps_mcudl_data_slot_t *p_slot,
	enum gps_mcudl_pkt_type type, gpsmdl_u32 payload_len,
	struct gps_mcudl_slot_entry_t **p_entr_ret)
{
	int task_id = 0;
	int len;
	gpsmdl_u8 *ret;
	struct gps_mcudl_slot_entry_t *p_entr;

	gps_mcudl_slot_disp(p_slot);

	/* # check parameters*/
	if (payload_len > GPSMDL_PKT_PAYLOAD_MAX)
		return NULL;

	/* # calculate pkt total length*/
	len = GPSMDL_PKT_HEAD_LEN + payload_len + 1;

	/* may pend low priority pkt type if slot is near full */
	if (gps_mcudl_slot_may_pend_pkt_type_if_near_full(p_slot, type, len))
		return NULL;

	gps_mcudl_slot_protect();
	p_entr = gps_mcudl_pkt_reserve_entry_and_rbuf(p_slot, type, len);
	gps_mcudl_slot_unprotect();

	*p_entr_ret = NULL;
	ret = NULL;
	if (p_entr) {
		ret = (gpsmdl_u8 *)&p_entr->head[1];
		p_entr->task_id = task_id;
		*p_entr_ret = p_entr;
	}

	return ret;
}

void gps_mcudl_slot_pkt_ready(struct gps_mcudl_data_slot_t *p_slot, struct gps_mcudl_slot_entry_t *p_entr)
{
	bool to_flush = false;

	UT_TRC("p_entr = %p", p_entr);

	if (!p_entr)
		return;

	*(p_entr->tail) = GPSMDL_PKT_END_CHAR;

	gps_mcudl_slot_protect();
	p_entr->fg_working = 0;
	p_entr->fg_ready = 1;
	gps_mcudl_slot_update_pkt_cnt_and_data_len(p_slot);

	/* may set the threshold to trigger flush*/
	to_flush = (p_slot->entr_cursor.pkt_cnt >= 1 || p_slot->rbuf_cursor.data_len >= 1);
	gps_mcudl_slot_unprotect();

	/*
	 * DO NOT flush here, due to it may be in xlink threads.
	 * TODO:
	 *   1. Refactor for sharing same code with UT.
	 *   2. Move flush notify here to avoid so many GPS_DL_EVT_LINK_WRITE.
	 */
#if 0
	enum gps_mcudl_slot_flush_status flush_result = FLUSH_OK;

	if (!to_flush)
		return;

	gps_mcudl_slot_flush_inner(p_slot, &flush_result, GPS_MCUDL_PKT_WIN_RESERVED_SIZE);
#endif
}

void gps_mcudl_slot_update_pkt_cnt_and_data_len(struct gps_mcudl_data_slot_t *p_slot)
{
	struct gps_mcudl_slot_entry_t *p_entr = NULL;
	gpsmdl_u32 entr_idx = 0;
	gpsmdl_u32 loop_start_idx = 0;
	gpsmdl_u32 loop_end_idx = 0;

	if (p_slot->entr_cursor.write_tmp_idx ==
		p_slot->entr_cursor.write_idx)
		return;

	if (p_slot->entr_cursor.write_idx <
		p_slot->entr_cursor.write_tmp_idx) {
		loop_start_idx = p_slot->entr_cursor.write_idx;
		loop_end_idx = p_slot->entr_cursor.write_tmp_idx;
	} else {
		loop_start_idx = p_slot->entr_cursor.write_idx;
		loop_end_idx = p_slot->cfg.entry_list_len;
	}

	do {
		for (entr_idx = loop_start_idx; entr_idx < loop_end_idx; entr_idx++) {
			p_entr = p_slot->cfg.entry_list_ptr + entr_idx;
			/* gps_mcudl_slot_entr_disp(p_slot, p_entr);*/
			if (!p_entr->fg_ready || p_entr->fg_working) {
				p_slot->entr_cursor.write_idx = entr_idx;
				break;
			} else if (entr_idx + 1 == loop_end_idx) {
				/* here means it should wrap back*/
				p_slot->entr_cursor.write_idx = loop_end_idx;
			}

			p_slot->rbuf_cursor.data_len += p_entr->pkt_len;
			p_slot->entr_cursor.pkt_cnt++;
			p_entr->fg_ready = 0; /* it's more safe to set fg_ready to 0*/
		}
		if (entr_idx < p_slot->cfg.entry_list_len)
			break;

		loop_start_idx = 0;
		loop_end_idx = p_slot->entr_cursor.write_tmp_idx;
		p_slot->entr_cursor.write_idx = 0;
	} while (entr_idx == p_slot->cfg.entry_list_len);

	GFNS_ASSERT_4(p_slot->entr_cursor.write_idx < p_slot->cfg.entry_list_len,
		p_slot->entr_cursor.write_idx, entr_idx, (int)p_slot);

	if (p_slot->entr_cursor.write_tmp_idx > p_slot->entr_cursor.read_idx)
		GFNS_ASSERT((p_slot->entr_cursor.write_tmp_idx >= entr_idx &&
			p_slot->entr_cursor.read_idx <= p_slot->entr_cursor.write_idx));
	else
		GFNS_ASSERT((p_slot->entr_cursor.write_tmp_idx <= entr_idx ||
			p_slot->entr_cursor.read_idx >= p_slot->entr_cursor.write_idx));

	if (p_slot->en_ok_peak < p_slot->entr_cursor.pkt_cnt)
		p_slot->en_ok_peak = p_slot->entr_cursor.pkt_cnt;

	if (p_slot->rb_ok_peak < p_slot->rbuf_cursor.data_len)
		p_slot->rb_ok_peak = p_slot->rbuf_cursor.data_len;
}

/* ret is_okay*/
bool gps_mcudl_slot_trigger_send(struct gps_mcudl_data_slot_t *p_slot, const gpsmdl_u8 *p_data, gpsmdl_u32 len)
{
	gps_mcudl_intf_send_fn_t p_send_fn;
	int send_len;
	bool is_okay;

	if (!p_slot)
		return true;

	p_send_fn = p_slot->cfg.p_intf_send_fn;

	if (!p_send_fn)
		return true;

	send_len = (*p_send_fn)(p_data, len);
	if (send_len < 0)
		return false;

	is_okay = ((gpsmdl_u32)send_len == len);
	return is_okay;
}

enum gps_mcudl_slot_flush_status gps_mcudl_slot_flush_best_fit_in_window(
	struct gps_mcudl_data_slot_t *p_slot,
	gpsmdl_u32 window_size, gpsmdl_u32 *p_actual_send_size)
{
	struct gps_mcudl_slot_entry_t *p_entr = NULL;
	gpsmdl_u32 best_fit_size = 0;
	bool send_is_okay = false;

	gpsmdl_u32 entr_idx;
	gpsmdl_u32 loop_start_idx = 0;
	gpsmdl_u32 loop_end_idx = 0;

	int en_ok_len = 0;
	const gpsmdl_u8 *p_data;

	/* check parameters*/
	UT_TRC("");

	if (!p_slot)
		return FLUSH_ERR_INVALID_PARAM;

	if (window_size < GPS_MCUDL_PKT_WIN_MARGIN)
		return FLUSH_ERR_WIN_NOT_ENOUGH;

	/* keep some margin*/
	window_size -= GPS_MCUDL_PKT_WIN_MARGIN;

	/* check data emty*/
	UT_TRC("");
	gps_mcudl_slot_disp(p_slot);

	gps_mcudl_slot_protect();
	gps_mcudl_slot_update_pkt_cnt_and_data_len(p_slot);

	if (p_slot->entr_cursor.read_idx ==
			p_slot->entr_cursor.write_idx &&
		!p_slot->entr_cursor.is_full) {
		/* no pkt in slot */
		gps_mcudl_slot_unprotect();
		*p_actual_send_size = 0;
		return FLUSH_OK;
	}

	GFNS_ASSERT_4(p_slot->entr_cursor.pkt_cnt > 0,
		p_slot->entr_cursor.pkt_cnt, p_slot->entr_cursor.write_idx, p_slot->entr_cursor.read_idx);
	GFNS_ASSERT_4(p_slot->rbuf_cursor.data_len > 0,
		p_slot->rbuf_cursor.data_len, p_slot->rbuf_cursor.write_idx, p_slot->entr_cursor.write_tmp_idx);

	UT_TRC("");
	if (p_slot->entr_cursor.read_idx <
		p_slot->entr_cursor.write_idx) {
		loop_start_idx = p_slot->entr_cursor.read_idx;
		loop_end_idx = p_slot->entr_cursor.write_idx;
	} else {
		loop_start_idx = p_slot->entr_cursor.read_idx;
		loop_end_idx = p_slot->cfg.entry_list_len;
	}

	GFNS_ASSERT_4(loop_start_idx >= 0 && loop_start_idx < p_slot->cfg.entry_list_len,
				loop_start_idx, loop_end_idx, p_slot->cfg.entry_list_len);

	GFNS_ASSERT_4((((void *)(p_slot->cfg.entry_list_ptr[loop_start_idx].head)) ==
				((void *)(p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.write_idx))),
				(unsigned int)p_slot->cfg.entry_list_ptr[loop_start_idx].head,
				(unsigned int)p_slot->cfg.rbuf_ptr, p_slot->rbuf_cursor.write_idx);

	best_fit_size = 0;
	do {
		for (entr_idx = loop_start_idx; entr_idx < loop_end_idx; entr_idx++) {

			p_entr = p_slot->cfg.entry_list_ptr + entr_idx;
			gps_mcudl_slot_entr_disp(p_slot, p_entr);

			if (/* p_entr->fg_rb_wrapped*/
				(void *)p_entr->head ==
					(void *)p_slot->cfg.rbuf_ptr && entr_idx != p_slot->entr_cursor.read_idx
				/*&& entr_idx != loop_start_idx*/
			)
				break;

			if (best_fit_size + p_entr->pkt_len > window_size)
				break;

			best_fit_size += p_entr->pkt_len;
			en_ok_len++;
		}

		if (entr_idx < p_slot->cfg.entry_list_len)
			break;

		loop_start_idx = 0;
		loop_end_idx = p_slot->entr_cursor.write_idx;
	} while (entr_idx == p_slot->cfg.entry_list_len);
	/* while(1); -- more robust way to avoid dead loop*/

	GFNS_ASSERT_4((best_fit_size <= window_size), best_fit_size, window_size, entr_idx);

	if (best_fit_size == 0) {
		gps_mcudl_slot_unprotect();
		return FLUSH_ERR_WIN_NOT_ENOUGH;
	}

	p_entr = p_slot->cfg.entry_list_ptr + entr_idx;
	gps_mcudl_slot_disp(p_slot);

	GFNS_ASSERT_4(p_slot->rbuf_cursor.write_idx + best_fit_size <= p_slot->rbuf_cursor.rbuf_end,
				p_slot->rbuf_cursor.write_idx, best_fit_size, p_slot->rbuf_cursor.rbuf_end);

	GFNS_ASSERT_4(p_slot->rbuf_cursor.rbuf_end <= p_slot->cfg.rbuf_len,
				p_slot->cfg.rbuf_len, p_slot->rbuf_cursor.right_side_free_len,
				p_slot->rbuf_cursor.left_side_free_len);

	p_data = (const gpsmdl_u8 *)(p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.write_idx);
	gps_mcudl_slot_unprotect();

	send_is_okay = gps_mcudl_slot_trigger_send(p_slot, p_data, best_fit_size);
	if (!send_is_okay)
		return FLUSH_ERR_SEND_FAIL;

	gps_mcudl_slot_protect();
	p_slot->entr_cursor.pkt_cnt -= en_ok_len;
	p_slot->rbuf_cursor.data_len -= best_fit_size;

	GFNS_ASSERT_4((p_slot->entr_cursor.pkt_cnt >= 0 && p_slot->rbuf_cursor.data_len >= 0),
		p_slot->entr_cursor.pkt_cnt, en_ok_len, best_fit_size);

	p_slot->entr_cursor.is_full = 0;

/* should be*/
#if 0
	if (p_slot->rbuf_cursor.write_idx + best_fit_size == p_slot->rbuf_cursor.read_idx)
		/* EMPTY*/
	else if (p_slot->rbuf_cursor.write_idx + best_fit_size == p_slot->rbuf_cursor.rbuf_end)
		/* ROLL BACK*/
	else if (p_slot->rbuf_cursor.write_idx + best_fit_size < p_slot->rbuf_cursor.rbuf_end)
		/* Normal*/

	else
		GFNS_ASSERT(0);
#endif

	if (entr_idx == p_slot->entr_cursor.write_tmp_idx) {
		GFNS_ASSERT((p_slot->entr_cursor.write_tmp_idx == p_slot->entr_cursor.write_idx));

		/*
		 *GFNS_ASSERT((entr_last != NULL));
		 *if( ((gpsmdl_u32)(entr_last->tail + 1)) == p_slot->rbuf_cursor.rbuf_end) {
		 *	GFNS_ASSERT_4((((gpsmdl_u32)(p_slot->rb_s)) ==
		 *	(gpsmdl_u32)(p_slot->rb_s + p_slot->rb_wr)),
		 *		entr_last->tail, p_slot->rb_s, p_slot->rb_wr);
		 *} else {
		 *	GFNS_ASSERT_4((((gpsmdl_u32)(entr_last->tail + 1)) ==
		 *		(gpsmdl_u32)(p_slot->rb_s + p_slot->rb_wr)),
		 *		entr_last->tail, p_slot->rb_s, p_slot->rb_wr);
		 *}
		*/

		/* Buf empty*/
		if (p_slot->rbuf_cursor.read_idx > p_slot->rbuf_cursor.write_idx) {
			/* [FREE r DATA w FREE]: r -> w*/
			GFNS_ASSERT((p_slot->rbuf_cursor.write_idx + best_fit_size == p_slot->rbuf_cursor.read_idx));
		} else {
			/* GFNS_ASSERT((p_slot->rb_rd + best_fit_size == p_slot->rb_wr + p_slot->rb_ln));*/
			/* GFNS_ASSERT((p_slot->rb_rd + best_fit_size ==*/
			/* p_slot->rb_wr + p_slot->rbuf_cursor.rbuf_end));*/

			/* ~~[DATA w FREE r DATA bk [*] ln]: r -> w~~ => impossible*/

			/* [w FREE r DATA bk [*] ln ]: r -> w => Must bk == ln*/
			GFNS_ASSERT_4((p_slot->cfg.rbuf_len == p_slot->rbuf_cursor.rbuf_end), p_slot->cfg.rbuf_len,
				p_slot->rbuf_cursor.rbuf_end, (int)p_slot);
			GFNS_ASSERT_4((p_slot->rbuf_cursor.read_idx == 0), p_slot->rbuf_cursor.read_idx,
				p_slot->rbuf_cursor.write_idx, best_fit_size);

			/*
			 * GFNS_ASSERT_4((p_slot->rb_rd + best_fit_size ==
			 * p_slot->rb_wr + p_slot->rb_bk), p_slot->rb_bk,
			 * p_slot->rb_rd, best_fit_size);
			*/
		}

		p_slot->rbuf_cursor.rbuf_end = p_slot->cfg.rbuf_len;
		p_slot->rbuf_cursor.write_idx = 0;
		p_slot->rbuf_cursor.right_side_free_len = p_slot->cfg.rbuf_len;
		p_slot->rbuf_cursor.left_side_free_len = 0;

		p_slot->rbuf_cursor.read_idx = 0;

		GFNS_CHECK_FREE_CONSISTENCY();

		p_slot->entr_cursor.read_idx = entr_idx;

		/* GFNS_ASSERT_4(p_slot->en_rd == p_slot->en_wr, p_slot->en_rd, p_slot->en_wr, (int)p_slot);*/
		/* GFNS_ASSERT_4(p_slot->en_rd == p_slot->en_ok, p_slot->en_rd, p_slot->en_ok, (int)p_slot);*/
		GFNS_ASSERT_4(p_slot->entr_cursor.is_full == 0, p_slot->entr_cursor.read_idx,
			p_slot->entr_cursor.is_full, (int)p_slot);

#if 0
		/* loop done*/
		if (p_slot->rbuf_cursor.write_idx == p_slot->rbuf_cursor.rbuf_end) {
			/* ring buf empty*/
			GFNS_ASSERT((p_slot->rbuf_cursor.read_idx == 0));
			p_slot->rbuf_cursor.rbuf_end = p_slot->cfg.rbuf_len;
			p_slot->rbuf_cursor.write_idx = 0;
			p_slot->rbuf_cursor.right_side_free_len = p_slot->cfg.rbuf_len;
			p_slot->rbuf_cursor.left_side_free_len = 0;
			GFNS_CHECK_FREE_CONSISTENCY();

		} else {
			/* ring buf not empty*/
			/* p_slot->rbuf_cursor.rbuf_end : No change*/
			p_slot->rbuf_cursor.write_idx += best_fit_size;
			/*p_slot->rbuf_cursor.right_side_free_len : No change*/
			p_slot->rbuf_cursor.left_side_free_len += best_fit_size;
			GFNS_CHECK_FREE_CONSISTENCY();
		}
#endif
	} else {
		/* entr_idx is in not empty range*/
		if (p_slot->entr_cursor.write_idx > p_slot->entr_cursor.read_idx) {
			GFNS_ASSERT((
				p_slot->entr_cursor.write_idx >= entr_idx &&
				p_slot->entr_cursor.read_idx < entr_idx));
		} else {
			GFNS_ASSERT((
				p_slot->entr_cursor.write_idx <= entr_idx ||
				p_slot->entr_cursor.read_idx > entr_idx));
		}

		p_slot->entr_cursor.read_idx = entr_idx;

		if ((void *)p_entr->head == (void *)p_slot->cfg.rbuf_ptr) {
			/* break due to rb wrap*/
			p_slot->rbuf_cursor.rbuf_end = p_slot->cfg.rbuf_len;
			p_slot->rbuf_cursor.write_idx = 0;
			p_slot->rbuf_cursor.right_side_free_len = p_slot->cfg.rbuf_len - p_slot->rbuf_cursor.read_idx;
			p_slot->rbuf_cursor.left_side_free_len = 0;
			GFNS_CHECK_FREE_CONSISTENCY();

			/* Check entr_idx consistency*/
			GFNS_ASSERT(((void *)(p_slot->cfg.entry_list_ptr[p_slot->entr_cursor.read_idx].head) ==
				(void *)(p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.write_idx)));

			/* fg_send_remain = 1;*/
			/* tmpGFNS_pkt_slot_send_NoProtect(p_slot, send_func, window_size - best_fit_size);*/
		} else {
			/* best_fit_size reaches window_size limitation*/
			/* ring buf not empty and no wrap*/

			if (p_slot->rbuf_cursor.left_side_free_len > 0 ||
				(p_slot->rbuf_cursor.write_idx == 0 && p_slot->rbuf_cursor.read_idx != 0)) {
				/* if both rd & wr == 0, full -> not full case*/
				/* [FREE DATA FREE]*/
				/* [(r) DATA FREE]*/
				/* p_slot->rbuf_cursor.rbuf_end : No change*/
				p_slot->rbuf_cursor.write_idx += best_fit_size;
				/* p_slot->rb_rfree : No change*/
				p_slot->rbuf_cursor.left_side_free_len += best_fit_size;

				GFNS_CHECK_FREE_CONSISTENCY();
			} else {
				GFNS_CHECK_FREE_CONSISTENCY();

				GFNS_ASSERT_4(best_fit_size <= p_slot->rbuf_cursor.rbuf_end, best_fit_size,
					p_slot->rbuf_cursor.rbuf_end, (int)p_slot);

				/* [DATA FREE (r_old) DATA (r_new) DATA]*/
				/* p_slot->rb_bk : No change*/
				p_slot->rbuf_cursor.write_idx += best_fit_size;
				p_slot->rbuf_cursor.right_side_free_len += best_fit_size;
				/* p_slot->rb_lfree;*/

				GFNS_ASSERT_4(p_slot->rbuf_cursor.right_side_free_len <= p_slot->rbuf_cursor.rbuf_end,
					best_fit_size, p_slot->rbuf_cursor.right_side_free_len,
					p_slot->rbuf_cursor.write_idx);
				GFNS_CHECK_FREE_CONSISTENCY();
			}

			/* Check entr_idx consistency*/
			GFNS_ASSERT(((void *)(p_slot->cfg.entry_list_ptr[p_slot->entr_cursor.read_idx].head) ==
				(void *)(p_slot->cfg.rbuf_ptr + p_slot->rbuf_cursor.write_idx)));
		}
	}
/* 1ms ~= 10byte, 1byte ~ 0.1ms ~ 3.2768tick*/
/* cos_delay_time(best_fit_size*3);*/

/* out caller will loop in again, then remain can be sent*/
#if 0
	/* continue sending until p_slot has less data*/
	/*if (p_slot->rbuf_cursor.data_len * 4 > window_size || fg_send_remain)*/
	if (fg_send_remain) {
		gps_mcudl_slot_update_pkt_cnt_and_data_len(p_slot);
		tmpGFNS_pkt_slot_send_NoProtect(p_slot, send_func, window_size);
	}
#endif
	gps_mcudl_slot_unprotect();

	*p_actual_send_size = best_fit_size;
	return FLUSH_OK;
}

enum gps_mcudl_slot_flush_status gps_mcudl_slot_flush_inner(
	struct gps_mcudl_data_slot_t *p_slot, gpsmdl_u32 *p_flush_done_size, gpsmdl_u32 reserved_win_sz)
{
	gpsmdl_u32 calculated_win_sz;
	gpsmdl_u32 send_max = GPS_MCUDL_INTF_SEND_MAX;
	gpsmdl_u32 send_win_sz;

	gpsmdl_u32 send_sz;
	gpsmdl_u32 total_send_sz;
	enum gps_mcudl_slot_flush_status flush_status;

	/*
	 * Send pkt until any of the conditions:
	 *   1. there is no pkt in slot   -> status = FLUSH_OK
	 *   2. send window is not enough -> status = FLUSH_ERR_WIN_NOT_ENOUGH
	 *   3. suffer send fail          -> status = FLUSH_ERR_SEND_FAIL
	 *   4. suffer invalid param      -> status = FLUSH_ERR_INVALID_PARAM
	 */
	total_send_sz = 0;
	do {
		calculated_win_sz = gps_mcudl_flowctrl_cal_window_size();
		calculated_win_sz =
			(calculated_win_sz > reserved_win_sz) ? (calculated_win_sz - reserved_win_sz) : 0;

		/* send_win_sz = MIN(calculated_win_sz, send_max)*/
		send_win_sz = (send_max < calculated_win_sz) ? send_max : calculated_win_sz;
		if (!send_win_sz) {
			flush_status = FLUSH_ERR_WIN_NOT_ENOUGH;
			break;
		}

		flush_status = gps_mcudl_slot_flush_best_fit_in_window(p_slot, send_win_sz, &send_sz);
		if (flush_status == FLUSH_OK) {
			gps_mcudl_flowctrl_local_add_send_byte(send_sz, p_slot->cfg.slot_id);
			total_send_sz += send_sz;
		}
	} while (flush_status == FLUSH_OK && send_sz > 0);

	if (p_flush_done_size)
		*p_flush_done_size = total_send_sz;

	return flush_status;
}

enum gps_mcudl_slot_flush_status gps_mcudl_slot_flush(
	struct gps_mcudl_data_slot_t *p_slot, gpsmdl_u32 *p_flush_done_size)
{
	enum gps_mcudl_slot_flush_status flush_status;

	flush_status = gps_mcudl_slot_flush_inner(
		p_slot, p_flush_done_size, GPS_MCUDL_PKT_WIN_RESERVED_SIZE);
	return flush_status;
}
