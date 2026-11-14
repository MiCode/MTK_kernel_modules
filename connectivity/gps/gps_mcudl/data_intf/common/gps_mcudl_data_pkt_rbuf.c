/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_data_pkt_rbuf.h"
#include "gps_mcudl_data_pkt_slot.h"

#define GFNS_RBUF_DBG(...)
#define GFNS_RBUF_TRC(fmt, ...)
#define GFNS_RBUF_WRN(...)
#define GFNS_RBUF_ERR(...)
#define GFNS_RBUF_ASSERT(x)
#define GFNS_RBUF_ASSERT_1(x, y1)
#define GFNS_RBUF_ASSERT_2(x, y1, y2)
#define GFNS_RBUF_ASSERT_3(x, y1, y2, y3)

gpsmdl_u32 gps_mcudl_data_rbuf_get_free_size(struct gps_mcudl_data_rbuf_plus_t *p_rbuf)
{
	gpsmdl_u32 free_len;

	if (p_rbuf->cursor.wff)
		free_len = 0;
	else if (p_rbuf->cursor.wwi == p_rbuf->cursor.wri)
		free_len = p_rbuf->cfg.rbuf_len;
	else if (p_rbuf->cursor.wwi < p_rbuf->cursor.wri)
		free_len = p_rbuf->cursor.wri - p_rbuf->cursor.wwi;
	else
		free_len = p_rbuf->cfg.rbuf_len + p_rbuf->cursor.wri - p_rbuf->cursor.wwi;
	return free_len;
}

/*#define GFNS_ATOM_OP(x)   (*(volatile gfns_atomic *)(&(x)))*/
#define GFNS_ATOM_OP(x) (x)

#if 0
/*#include "nds32_intrinsic.h"*/
/*#define DSB()   __nds32_dsb()*/
#define DSB() (__asm__ __volatile__("dsb\n"))
#else
#define DSB()
#endif

/* return true if set true ok, false for reader updated the read pointer */
bool gfns_rbuf_try_to_set_full_flag(struct gps_mcudl_data_rbuf_plus_t *p)
{
	gpsmdl_u32 rri_tmp;

	gps_mcudl_slot_protect();
	GFNS_ATOM_OP(p->cursor.wff_bak) = true;
	DSB();

	rri_tmp = GFNS_ATOM_OP(p->cursor.rri_bak);
	DSB();
	gps_mcudl_slot_unprotect();

	if (p->cursor.wri != rri_tmp) {
		/* reader has updated read pointer, full flag should not be set*/
		gps_mcudl_slot_protect();
		GFNS_ATOM_OP(p->cursor.wff_bak) = false;
		DSB();
		gps_mcudl_slot_unprotect();

		return false;
	}
	/* reader not update read pointer, so full flag set ok*/
	return true;
}

bool gps_mcudl_data_rbuf_is_full(struct gps_mcudl_data_rbuf_plus_t *p_rbuf)
{
	gpsmdl_u32 wff_tmp;

	gps_mcudl_slot_protect();
	wff_tmp = GFNS_ATOM_OP(p_rbuf->cursor.wff_bak);
	DSB();
	gps_mcudl_slot_unprotect();

	if (wff_tmp)
		p_rbuf->cursor.wff = true;

	return wff_tmp;
}

void gps_mcudl_data_rbuf_clear_full_flag(struct gps_mcudl_data_rbuf_plus_t *p)
{
	/* if need twice, call this api twice*/
	gps_mcudl_slot_protect();
	GFNS_ATOM_OP(p->cursor.wff_bak) = false;
	DSB();
	gps_mcudl_slot_unprotect();
}

void gps_mcudl_data_rbuf_writer_update_write_idx(struct gps_mcudl_data_rbuf_plus_t *p)
{
	gps_mcudl_slot_protect();
	GFNS_ATOM_OP(p->cursor.wwi_bak) = p->cursor.wwi;
	DSB();
	gps_mcudl_slot_unprotect();

	/* no need to proc wff due to it already handled*/
	GFNS_RBUF_TRC("r=%d, w=%d, f=%d, l=%d", p->cursor.rri, p->cursor.wwi, p->cursor.wff, p->cfg.rbuf_len);
}

bool gps_mcudl_data_rbuf_writer_sync_read_idx(struct gps_mcudl_data_rbuf_plus_t *p)
{
	gpsmdl_u32 wff_tmp;
	gpsmdl_u32 rri_tmp;

	gps_mcudl_slot_protect();
	wff_tmp = GFNS_ATOM_OP(p->cursor.wff_bak);
	DSB();

	rri_tmp = GFNS_ATOM_OP(p->cursor.rri_bak);
	DSB();
	gps_mcudl_slot_unprotect();

	GFNS_RBUF_TRC("r=%d, w=%d, f=%d, l=%d", p->cursor.rri, p->cursor.wwi, p->cursor.wff, p->cfg.rbuf_len);
	if (p->cursor.wff) {
		/* wff set only when p->wwi == p->wri*/
		GFNS_RBUF_ASSERT(p->cursor.wwi == p->cursor.wri);
		if (wff_tmp) {
			/* rri should not change if wff_tmp is true;*/
			GFNS_RBUF_ASSERT(rri_tmp == p->cursor.wri);
			return false;
		}
		/* reader proc full buffer to empty*/
		p->cursor.wff = false;
		return true;
	}
	/* wff_tmp should not be true if p->wff is false*/
	GFNS_RBUF_ASSERT(!wff_tmp);
	if (p->cursor.wri != rri_tmp) {
		p->cursor.wri = rri_tmp;
		return true;
	} else
		return false;
}

void gps_mcudl_data_rbuf_reader_update_read_idx(struct gps_mcudl_data_rbuf_plus_t *p)
{
	gps_mcudl_slot_protect();
	if (p->cursor.rri == p->cursor.rri_bak) {
		/* rri not changed, need to check rff*/
		GFNS_ATOM_OP(p->cursor.wff_bak) = p->cursor.rff;
		DSB();
	} else {
		/* read pointer should be changed in advance of full flag*/
		GFNS_ATOM_OP(p->cursor.rri_bak) = p->cursor.rri;
		DSB();

		GFNS_ATOM_OP(p->cursor.wff_bak) = false; /* no twice write*/
		DSB();
	}
	gps_mcudl_slot_unprotect();
	GFNS_RBUF_TRC("r=%d, w=%d, f=%d, l=%d", p->cursor.rri, p->cursor.wwi, p->cursor.wff, p->cfg.rbuf_len);
}

bool gps_mcudl_data_rbuf_reader_sync_write_idx(struct gps_mcudl_data_rbuf_plus_t *p)
{
	/* gfns_atomic wwi_tmp;*/
	gpsmdl_u32 wwi_tmp;

	gps_mcudl_slot_protect();
	wwi_tmp = GFNS_ATOM_OP(p->cursor.wwi_bak);
	DSB();
	gps_mcudl_slot_unprotect();

	GFNS_RBUF_TRC("r=%d, w=%d, f=%d, l=%d", p->cursor.rri, p->cursor.wwi, p->cursor.wff, p->cfg.rbuf_len);
	if (p->cursor.rwi != wwi_tmp) {
		GFNS_RBUF_ASSERT_3(wwi_tmp < p->cfg.rbuf_len, p->cursor.rwi, wwi_tmp, p->cfg.rbuf_len);
		p->cursor.rwi = wwi_tmp;
		if (p->cursor.rwi == p->cursor.rri)
			p->cursor.rff = true;
		return true;
	}
	/* write pointer equal to read pointer, need to check full or empty*/
	if (p->cursor.rwi == p->cursor.rri) {
		/* At the time write and read pointer are equal, wff_bak has alreay*/
		/*  been set, it's safe to read or write it in reader task.*/
		gps_mcudl_slot_protect();
		p->cursor.rff = GFNS_ATOM_OP(p->cursor.wff_bak);
		DSB();
		gps_mcudl_slot_unprotect();
	}

	if (p->cursor.rff)
		return true;
	else
		return false; /* return FALSE to indicate value not changed.*/
}

void gps_mcudl_data_rbuf_init(struct gps_mcudl_data_rbuf_plus_t *p_rbuf)
{
	memset(&p_rbuf->cursor, 0, sizeof(p_rbuf->cursor));
	memset(&p_rbuf->sta, 0, sizeof(p_rbuf->sta));
}

void gfns_rbuf_put_work(struct gps_mcudl_data_rbuf_plus_t *p_rbuf,
	const gpsmdl_u8 *buf, gpsmdl_u32 send_len)
{
	if (p_rbuf->cursor.wwi < p_rbuf->cursor.wri ||
		p_rbuf->cursor.wwi + send_len < p_rbuf->cfg.rbuf_len) { /* 1-segment*/
		memcpy(p_rbuf->cfg.rbuf_ptr + p_rbuf->cursor.wwi, buf, send_len);
		p_rbuf->cursor.wwi += send_len; /* will not wrap*/
	} else { /* 2-segment, the 2rd one might be zero*/
		gpsmdl_u32 len1, len2;

		len1 = p_rbuf->cfg.rbuf_len - p_rbuf->cursor.wwi;
		len2 = send_len - len1;
		GFNS_RBUF_ASSERT_3(len2 < send_len, len2, send_len, len1);
		memcpy(p_rbuf->cfg.rbuf_ptr + p_rbuf->cursor.wwi, buf, len1);
		if (len2 > 0)
			memcpy(p_rbuf->cfg.rbuf_ptr, buf + len1, len2);
		p_rbuf->cursor.wwi = len2; /* may wrap case --> wwi == 0 (len2 == 0)*/
	}
}

gpsmdl_u32 gps_mcudl_data_rbuf_put(struct gps_mcudl_data_rbuf_plus_t *p_rbuf,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	gpsmdl_u32 free_len, to_put_len, put_done_len, left_len;

	free_len = gps_mcudl_data_rbuf_get_free_size(p_rbuf);
	to_put_len = data_len;
	put_done_len = 0;

	if (free_len <= data_len) {
		left_len = data_len;
		do {
			/* if not enough, try to sync*/
			gps_mcudl_data_rbuf_writer_sync_read_idx(p_rbuf);
			free_len = gps_mcudl_data_rbuf_get_free_size(p_rbuf);
			if (free_len <= left_len && free_len > 0)
				to_put_len = free_len;
			else {
				if (free_len > left_len) {
					gfns_rbuf_put_work(p_rbuf, p_data + put_done_len, left_len);
					put_done_len += left_len;
					left_len = 0;
					break; /* finish*/
				}
				GFNS_RBUF_ASSERT_1(free_len == 0, free_len);
				break; /* rbuf full, drop data!*/
			}

			GFNS_RBUF_ASSERT_2(to_put_len <= p_rbuf->cfg.rbuf_len, free_len, p_rbuf->cfg.rbuf_len);

			if (!gfns_rbuf_try_to_set_full_flag(p_rbuf))
				continue; /* reader update, re-sync wri*/

			gfns_rbuf_put_work(p_rbuf, p_data + put_done_len, to_put_len);
			put_done_len += to_put_len;
			left_len -= to_put_len;

			if (gps_mcudl_data_rbuf_is_full(p_rbuf))
				break; /* still full -> break, drop data!*/
			if (put_done_len >= left_len)
				break; /* send finish*/
			/* reader change full to non-full, re-sync wri*/
		} while (1);
	} else {
		gfns_rbuf_put_work(p_rbuf, p_data, data_len);
		put_done_len = data_len;
		left_len = 0;
	}

	gps_mcudl_data_rbuf_writer_update_write_idx(p_rbuf);

#ifdef GFNS_RBUF_STA
	p_rbuf->sta.total_put_byte += put_done_len;
	p_rbuf->sta.total_drop_byte += (data_len - put_done_len);
#endif
	return put_done_len;
}

void gps_mcudl_data_rbuf_get_to_proc(struct gps_mcudl_data_rbuf_plus_t *p, gps_mcudl_rbuf_proc_fn_t proc_func)
{
	gpsmdl_u32 proc_len1 = 0;
	gpsmdl_u32 proc_len2 = 0;

	gps_mcudl_data_rbuf_reader_sync_write_idx(p);

	GFNS_RBUF_ASSERT(proc_func);

	/* Empty case */
	if (p->cursor.rri == p->cursor.rwi && !p->cursor.rff) {
		/* GFNS_RBUF_TRC("buf(0x%x, l=%d) empty: rri = %d, rwi = %d, rff = %d",*/
		/*     p->buf, p->max, p->rri, p->rwi, p->rff);*/
		return;
	}

	/* Non empty case */
	do {
		if (p->cursor.rri >= p->cursor.rwi) {
			/* GFNS_RBUF_TRC("to proc (0x%x + %d), len = %d",*/
			/*     p->buf, p->rri, p->max - p->rri);*/
			proc_len1 = proc_func(p->cfg.rbuf_ptr + p->cursor.rri, p->cfg.rbuf_len - p->cursor.rri);

			if (proc_len1 > 0) {
				p->cursor.rff = false;

				if (proc_len1 == p->cfg.rbuf_len - p->cursor.rri)
					p->cursor.rri = 0; /* 1st of 2 cases: buffer full -> buffer empty*/
				else {
					GFNS_RBUF_ASSERT_3(proc_len1 < p->cfg.rbuf_len - p->cursor.rri,
						proc_len1, p->cfg.rbuf_len, p->cursor.rri);
					p->cursor.rri += proc_len1;
				}
			}

			/* proc_len != input_len case will be proc by next step*/
		}

		/* Might be 2 cases: 1. above step is skipped; 2. not skip */
		if (p->cursor.rri < p->cursor.rwi) {
			/* GFNS_RBUF_TRC("to proc (0x%x + %d), len = %d",*/
			/*     p->buf, p->rri, p->rwi - p->rri);*/
			proc_len2 = proc_func(p->cfg.rbuf_ptr + p->cursor.rri, p->cursor.rwi - p->cursor.rri);

			GFNS_RBUF_ASSERT_3(proc_len2 <= p->cursor.rwi - p->cursor.rri,
				proc_len2, p->cursor.rwi, p->cursor.rri);

			if (proc_len2 > 0) {
				p->cursor.rff = false;
				/* should be +=, if above step not skip, previous rri should be 0*/
				p->cursor.rri += proc_len2;
			}
		}
	} while (0);

#ifdef GFNS_RBUF_STA
	p->sta.total_get_byte += proc_len1;
	p->sta.total_get_byte += proc_len2;
	if (p->sta.total_drop_byte != p->sta.total_last_drop_byte) {
		GFNS_RBUF_TRC("rbuf %p updt rri %d -> %d, proc = %d (%d + %d), total: put=%d, get = %d, drop = %d",
			p, p->cursor.rri_bak, p->cursor.rri, proc_len1 + proc_len2, proc_len1, proc_len2,
			p->sta.total_put_byte, p->sta.total_get_byte, p->sta.total_drop_byte);
		p->sta.total_last_drop_byte = p->sta.total_drop_byte;
	}
#else
	/* GFNS_RBUF_TRC("rbuf %p updt rri %d -> %d, proc = %d (%d + %d)",*/
	/*     p, p->rri_bak, p->rri, proc_len1 + proc_len2, proc_len1, proc_len2);*/
#endif

	gps_mcudl_data_rbuf_reader_update_read_idx(p);
}
