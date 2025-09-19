/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_data_pkt_parser.h"

#define GPSMDL_PKT_TRC(...)

gpsmdl_u8 payload_buf[GPSMDL_PKT_PAYLOAD_MAX];

void gps_mcudl_data_pkt_parser_init(struct gps_mcudl_data_pkt_parser_t *p_parser)
{
	p_parser->read_idx = 0;
	p_parser->tmp_read_idx = 0;
	p_parser->head_len = GPSMDL_PKT_HEAD_LEN;
	p_parser->remain_len = 0;

	p_parser->head_done = false;
	p_parser->false_head = 0;
	p_parser->rbuf_wrapping = false;

	p_parser->p_head = NULL;
	p_parser->pkt_cnt = 0;
	p_parser->drop_byte_cnt = 0;
	p_parser->proc_byte_cnt = 0;
	/*gfns_large_uint_set_zero(&p_parser->pkt_cnt);*/
	/*gfns_large_uint_set_zero(&p_parser->drop_byte_cnt);*/
	/*gfns_large_uint_set_zero(&p_parser->proc_byte_cnt);*/
}

static void gps_mcudl_data_pkt_proc(
	gps_mcudl_pkt_proc_fn_t p_func,
	enum gps_mcudl_pkt_type type,
	gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	if (!p_func)
		return;

	(*p_func)(type, payload_ptr, payload_len);
}

static void gps_mcudl_data_pkt_submit(struct gps_mcudl_data_pkt_parser_t *p_parser)
{
	gpsmdl_u32 i, j;
	gpsmdl_u32 payload_len;
	gpsmdl_u8 *payload_ptr;

	if (NULL == p_parser->p_head)
		return;

	payload_len = (
		(p_parser->p_head->payload_len1 << 8) |
		(p_parser->p_head->payload_len2));

	if ((p_parser->tmp_read_idx + payload_len) <= p_parser->cfg.rbuf_len) {
		/* The case payload is in the continuous buffer:*/
		payload_ptr = &p_parser->cfg.rbuf_ptr[p_parser->tmp_read_idx];
	} else {
		/* The case payload not in the continuous buffer:*/
		/*   one part of the payload at the end of the p_parser->buf*/
		/*   and the other part of payload at the begin of p_parser->buf*/
		/* copy them to a continuous buffer: payload_buf*/
		for (i = 0, j = p_parser->tmp_read_idx; j < p_parser->cfg.rbuf_len; j++, i++)
			payload_buf[i] = p_parser->cfg.rbuf_ptr[j];

		for (j = 0; i < payload_len; i++, j++)
			payload_buf[i] = p_parser->cfg.rbuf_ptr[j];
		payload_ptr = &payload_buf[0];
	}

	p_parser->pkt_cnt++;
	p_parser->proc_byte_cnt += (payload_len + GPSMDL_PKT_HEAD_LEN + GPSMDL_PKT_TAIL_LEN);
	/*gfns_large_uint_add_uint32(&p_parser->pkt_cnt, 1);*/
	/*gfns_large_uint_add_uint32(&p_parser->proc_byte_cnt, geofence_pkt_cal_pkt_len(p_parser));*/

	GPSMDL_PKT_TRC("pkt: seq=%d, type=%d, len=%d",
		p_parser->p_head->seq, p_parser->p_head->type, payload_len);

	gps_mcudl_data_pkt_proc(
		p_parser->cfg.p_pkt_proc_fn,
		p_parser->p_head->type, payload_ptr, payload_len);
}

static bool gps_mcudl_data_pkt_check_head(struct gps_mcudl_data_pkt_parser_t *p_parser, gpsmdl_u32 i)
{
	gpsmdl_u32 i1, i2, i3, i4, i5;
	bool found = false;
	gpsmdl_u8 cal_chksum;

	i1 = (i + 1 >= p_parser->cfg.rbuf_len) ? i + 1 - p_parser->cfg.rbuf_len : i + 1;
	i2 = (i + 2 >= p_parser->cfg.rbuf_len) ? i + 2 - p_parser->cfg.rbuf_len : i + 2;
	i3 = (i + 3 >= p_parser->cfg.rbuf_len) ? i + 3 - p_parser->cfg.rbuf_len : i + 3;
	i4 = (i + 4 >= p_parser->cfg.rbuf_len) ? i + 4 - p_parser->cfg.rbuf_len : i + 4;
	i5 = (i + 5 >= p_parser->cfg.rbuf_len) ? i + 5 - p_parser->cfg.rbuf_len : i + 5;

	cal_chksum = (gpsmdl_u8)(p_parser->cfg.rbuf_ptr[i1] + p_parser->cfg.rbuf_ptr[i2] +
		p_parser->cfg.rbuf_ptr[i3] + p_parser->cfg.rbuf_ptr[i4]);
	found = (GPSMDL_PKT_START_CHAR == p_parser->cfg.rbuf_ptr[i] && p_parser->cfg.rbuf_ptr[i5] == cal_chksum);

	if (found) {
		p_parser->head.start_char = GPSMDL_PKT_START_CHAR;
		p_parser->head.seq = p_parser->cfg.rbuf_ptr[i1];
		p_parser->head.type = p_parser->cfg.rbuf_ptr[i2];
		p_parser->head.payload_len1 = p_parser->cfg.rbuf_ptr[i3];
		p_parser->head.payload_len2 = p_parser->cfg.rbuf_ptr[i4];
		p_parser->head.chksum = p_parser->cfg.rbuf_ptr[i5];
		p_parser->p_head = &p_parser->head;
		p_parser->remain_len = ((p_parser->p_head->payload_len1 << 8) | (p_parser->p_head->payload_len2)) + 1;
	}

	GPSMDL_PKT_TRC("check_head  : ok=%d, idx=%d, %x, %x, %x, %x, %x, %x", i, found,
		p_parser->cfg.rbuf_ptr[i], p_parser->cfg.rbuf_ptr[i1], p_parser->cfg.rbuf_ptr[i2],
		p_parser->cfg.rbuf_ptr[i3], p_parser->cfg.rbuf_ptr[i4], p_parser->cfg.rbuf_ptr[i5]);

	return found;
}

static int gps_mcudl_data_pkt_check_remain(struct gps_mcudl_data_pkt_parser_t *p_parser, gpsmdl_u32 i)
{
	gpsmdl_u32 ii;
	bool found = false;

	ii = (i + p_parser->remain_len - 1);
	if (ii >= p_parser->cfg.rbuf_len)
		ii -= p_parser->cfg.rbuf_len;

	found = (GPSMDL_PKT_END_CHAR == p_parser->cfg.rbuf_ptr[ii]);
	GPSMDL_PKT_TRC("check_remain: ok=%d, idx=%d, %x", ii, found, p_parser->cfg.rbuf_ptr[ii]);
	return found;
}

static void gps_mcudl_data_pkt_parse_inner(struct gps_mcudl_data_pkt_parser_t *p_parser, gpsmdl_u32 write_idx)
{
	gpsmdl_u32 parse_idx;

	if (p_parser->tmp_read_idx >= write_idx)
		return;

	for (;;) {
		if (!p_parser->head_done) {
			for (
				parse_idx = p_parser->tmp_read_idx;
				parse_idx + GPSMDL_PKT_HEAD_LEN <= write_idx; parse_idx++) {
				/*GPSMDL_PKT_TRC("%d", p_parser->buf[parse_idx]);*/

				if (gps_mcudl_data_pkt_check_head(p_parser, parse_idx)) {
					p_parser->tmp_read_idx = GPSMDL_PKT_HEAD_LEN + parse_idx;
					p_parser->head_done = true;
					break;
				}
				p_parser->read_idx++;
				p_parser->tmp_read_idx = p_parser->read_idx;
				p_parser->drop_byte_cnt++;
				/*gfns_large_uint_add_uint32(&p_parser->drop_byte_cnt, 1);*/
			}
		}

		if (p_parser->head_done) {
			if (p_parser->tmp_read_idx + p_parser->remain_len <= write_idx) {
				if (gps_mcudl_data_pkt_check_remain(p_parser, p_parser->tmp_read_idx)) {
					gps_mcudl_data_pkt_submit(p_parser);
					p_parser->tmp_read_idx += p_parser->remain_len;
					p_parser->read_idx = p_parser->tmp_read_idx;
					p_parser->head_done = false;
				} else {
					/*roll_back;*/
					p_parser->read_idx++;
					p_parser->tmp_read_idx = p_parser->read_idx;
					p_parser->drop_byte_cnt++;
					/*gfns_large_uint_add_uint32(&p_parser->drop_byte_cnt, 1);*/
					p_parser->false_head++;
				}
				p_parser->head_done = false;
			} else
				break;
		} else
			break;
	}
}

void gps_mcudl_data_pkt_parse(struct gps_mcudl_data_pkt_parser_t *p_parser, gpsmdl_u32 write_idx)
{
	gpsmdl_u32 parse_idx;

	if (write_idx == p_parser->tmp_read_idx) {
		/* no data need to be parsed */
		return;
	}

	if (write_idx > p_parser->tmp_read_idx) {
		/* data is in staright buffer */
		gps_mcudl_data_pkt_parse_inner(p_parser, write_idx);
		return;
	}

	/*
	 * here: write_idx < p_parser->tmp_r
	 * data is wrapped into 2 parts
	 */
	do {
		/*
		 * if( (!p_parser->head_done && p_parser->tmp_r + p_parser->head_len <= p_parser->buf_len) ||
		 * ( p_parser->head_done && p_parser->tmp_r + p_parser->remain_len <= p_parser->buf_len)){
		*/
		if (!p_parser->rbuf_wrapping) {

			gps_mcudl_data_pkt_parse_inner(p_parser, p_parser->cfg.rbuf_len);

			/* CASE: head or pkt end is just aligned with buf end*/
			if (p_parser->tmp_read_idx == p_parser->cfg.rbuf_len) {
				p_parser->tmp_read_idx = 0;
				if (p_parser->read_idx == p_parser->cfg.rbuf_len)
					p_parser->read_idx = 0;
				/*p_parser->fg_buf_wrap = 0;*/
				break; /* wrap done*/
			}
		}

		/* CASE: head or pkt remain is wrapped by buf end*/
		if (!p_parser->head_done) {
			for (parse_idx = p_parser->tmp_read_idx;
				parse_idx + GPSMDL_PKT_HEAD_LEN <= write_idx + p_parser->cfg.rbuf_len &&
				parse_idx <= p_parser->cfg.rbuf_len;
				parse_idx++) {
				if (gps_mcudl_data_pkt_check_head(p_parser, parse_idx)) {
					p_parser->tmp_read_idx = GPSMDL_PKT_HEAD_LEN + parse_idx;
					p_parser->head_done = true;
					break;
					/* break to if*/
				} else {
					p_parser->read_idx++;
					p_parser->tmp_read_idx = p_parser->read_idx;
					p_parser->drop_byte_cnt++;
					/*gfns_large_uint_add_uint32(&p_parser->drop_byte_cnt, 1);*/
				}
			}
			/* Should not*/
			if (p_parser->tmp_read_idx >= p_parser->cfg.rbuf_len) {
				p_parser->tmp_read_idx -= p_parser->cfg.rbuf_len;
				if (p_parser->read_idx >= p_parser->cfg.rbuf_len)
					p_parser->read_idx -= p_parser->cfg.rbuf_len;
				p_parser->rbuf_wrapping = false;
				break; /*wrap done*/
			}
		} else {	/*else is enough*/
			/*if(p_parser->head_done) {*/
			if (p_parser->tmp_read_idx + p_parser->remain_len <= write_idx + p_parser->cfg.rbuf_len) {
				if (gps_mcudl_data_pkt_check_remain(p_parser, p_parser->tmp_read_idx)) {
					gps_mcudl_data_pkt_submit(p_parser);
					p_parser->tmp_read_idx = p_parser->tmp_read_idx + p_parser->remain_len -
						p_parser->cfg.rbuf_len;
					p_parser->read_idx = p_parser->tmp_read_idx;
					p_parser->head_done = false;
					p_parser->rbuf_wrapping = false;
					break; /*wrap done*/
				}
				/*roll_back2;*/
				p_parser->drop_byte_cnt++;
				/*gfns_large_uint_add_uint32(&p_parser->drop_byte_cnt, 1);*/
				p_parser->false_head++;
				p_parser->read_idx++;
				if (p_parser->read_idx >= p_parser->cfg.rbuf_len) {
					p_parser->read_idx -= p_parser->cfg.rbuf_len;
					p_parser->tmp_read_idx = p_parser->read_idx;
					p_parser->rbuf_wrapping = false;
					break; /* wrap done*/
				}
				p_parser->tmp_read_idx = p_parser->read_idx;
				p_parser->head_done = false;
				p_parser->rbuf_wrapping = false; /*wrap not start*/
				continue;		/* roll back;*/
			}
		}
		p_parser->rbuf_wrapping = true;
		return; /* not enough data;*/
	} while (1);
	gps_mcudl_data_pkt_parse_inner(p_parser, write_idx);
}
