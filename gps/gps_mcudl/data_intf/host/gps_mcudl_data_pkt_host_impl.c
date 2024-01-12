/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_plat_api.h"
#include "gps_dl_dma_buf.h"
#include "gps_dl_time_tick.h"
#include "gps_each_link.h"
#include "gps_mcudl_context.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_link_state.h"
#include "gps_dl_osal.h"
#include "gps_mcudl_data_pkt_payload_struct.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcusys_data_api.h"



#define RBUF_MAX (10*1024)
#define ENTR_MAX (64)
gpsmdl_u8              pars_rbuf[GPS_MDLY_CH_NUM][RBUF_MAX];
gpsmdl_u8              slot_rbuf[GPS_MDLY_CH_NUM][RBUF_MAX];
struct gps_mcudl_slot_entry_t entr_list[GPS_MDLY_CH_NUM][ENTR_MAX];
static void gps_mcudl_mcu_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len, enum gps_mcudl_yid y_id);

#define TX_BUF_MAX (GPSMDL_PKT_PAYLOAD_MAX)
struct gps_mcudl_data_trx_context {
	struct gps_mcudl_data_pkt_parser_t parser;
	struct gps_mcudl_data_rbuf_plus_t rx_rbuf;
	struct gps_mcudl_data_slot_t slot;
	gpsmdl_u8  tx_buf[TX_BUF_MAX];
	gpsmdl_u32 tx_len;
	struct gps_dl_osal_unsleepable_lock spin_lock;
	struct geofence_pkt_host_sta_s host_sta;
	gpsmdl_u64 local_flush_times;
	gpsmdl_u64 local_tx_len;
	gpsmdl_u64 remote_rx_len;
	bool wait_read_to_proc_flag;
	bool wait_write_to_flush_flag;
	bool wait_sta_to_flush_flag;
	bool write_to_mcu_may_fail_flag;
};


struct gps_mcudl_data_pkt_context {
	struct gps_mcudl_data_trx_context trx[GPS_MDLY_CH_NUM];
};


struct gps_mcudl_data_pkt_context g_data_pkt_ctx = { .trx = {
	[GPS_MDLY_NORMAL] = {
		.rx_rbuf = { .cfg = {
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.parser = { .cfg = {
			.p_pkt_proc_fn = &gps_mcudl_mcu_ch1_proc_func,
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.slot = { .cfg = {
			.slot_id = GPS_MDLY_NORMAL,
			.p_intf_send_fn = &gps_mcudl_mcu_ch1_send_func,

			.rbuf_ptr = &slot_rbuf[GPS_MDLY_NORMAL][0],
			.rbuf_len = RBUF_MAX,

			.entry_list_ptr = &entr_list[GPS_MDLY_NORMAL][0],
			.entry_list_len = ENTR_MAX,
		}, },
	},

	[GPS_MDLY_URGENT] = {
		.rx_rbuf = { .cfg = {
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_URGENT][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.parser = { .cfg = {
			.p_pkt_proc_fn = &gps_mcudl_mcu_ch2_proc_func,
			.rbuf_ptr = &pars_rbuf[GPS_MDLY_URGENT][0],
			.rbuf_len = RBUF_MAX,
		}, },
		.slot = { .cfg = {
			.slot_id = GPS_MDLY_URGENT,
			.p_intf_send_fn = &gps_mcudl_mcu_ch2_send_func,
			.rbuf_ptr = &slot_rbuf[GPS_MDLY_URGENT][0],
			.rbuf_len = RBUF_MAX,
			.entry_list_ptr = &entr_list[GPS_MDLY_URGENT][0],
			.entry_list_len = ENTR_MAX,
		}, },
	},
}, };

struct gps_mcudl_data_trx_context *get_txrx_ctx(enum gps_mcudl_yid yid)
{
	return &g_data_pkt_ctx.trx[yid];
}

bool gps_mcudl_ap2mcu_xdata_send(enum gps_mcudl_xid xid, struct gdl_dma_buf_entry *p_entry)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum GDL_RET_STATUS status;
	gpsmdl_u32 data_len = 0;
	enum gps_mcudl_pkt_type type = GPS_MDLYPL_MAXID;
	enum gps_mcudl_yid yid;
	bool xid_is_valid;
	bool send_is_okay;

	yid = GPS_MDL_X2Y(xid);
	xid_is_valid = gps_mcudl_xid2ypl_type(xid, &type);
	if (xid_is_valid)
		return false;

	p_trx_ctx = get_txrx_ctx(yid);
	status = gdl_dma_buf_entry_to_buf(p_entry,
		&p_trx_ctx->tx_buf[0], TX_BUF_MAX, &data_len);

	send_is_okay = false;
	if (status == GDL_OKAY)
		send_is_okay = gps_mcudl_ap2mcu_ydata_send(yid, type, &p_trx_ctx->tx_buf[0], data_len);

	MDL_LOGXW(xid, "x_send get=%s, len=%d, ok=%d",
		gdl_ret_to_name(status), data_len, send_is_okay);
	return send_is_okay;
}

bool gps_mcudl_ap2mcu_xdata_send_v2(enum gps_mcudl_xid x_id,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len, bool *p_to_notify)
{
	enum gps_mcudl_yid y_id;
	enum gps_mcudl_pkt_type type;
	bool xid_is_valid;
	bool send_is_okay;
	bool to_notify;

	y_id = GPS_MDL_X2Y(x_id);
	xid_is_valid = gps_mcudl_xid2ypl_type(x_id, &type);
	if (!xid_is_valid)
		return false;

	send_is_okay = gps_mcudl_ap2mcu_ydata_send(y_id, type, p_data, data_len);
	if (send_is_okay)
		to_notify = !gps_mcudl_ap2mcu_get_wait_write_flag(y_id);
	else
		to_notify = false;

	if (p_to_notify)
		*p_to_notify = to_notify;

	if (to_notify)
		gps_mcudl_ap2mcu_set_wait_write_flag(y_id, true);

	MDL_LOGXD(x_id, "x_send len=%d, ok=%d, ntf=%d", data_len, send_is_okay, to_notify);
	return send_is_okay;
}

void gps_mcudl_ap2mcu_context_init(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	gps_mcudl_slot_init(&g_data_pkt_ctx.trx[yid].slot);
	gps_mcudl_data_pkt_parser_init(&g_data_pkt_ctx.trx[yid].parser);
	gps_mcudl_data_rbuf_init(&g_data_pkt_ctx.trx[yid].rx_rbuf);
	gps_dl_osal_unsleepable_lock_init(&g_data_pkt_ctx.trx[yid].spin_lock);

	p_trx_ctx = &g_data_pkt_ctx.trx[yid];
	memset(&p_trx_ctx->host_sta, 0, sizeof(p_trx_ctx->host_sta));
	p_trx_ctx->host_sta.is_enable = true;
	gps_mcudl_flowctrl_init(yid);
	p_trx_ctx->wait_read_to_proc_flag = false;
	p_trx_ctx->wait_write_to_flush_flag = false;
	p_trx_ctx->wait_sta_to_flush_flag = false;
}

bool gps_mcudl_ap2mcu_ydata_send(enum gps_mcudl_yid yid,
	enum gps_mcudl_pkt_type type, const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = get_txrx_ctx(yid);
	return gps_mcudl_pkt_send(&p_trx_ctx->slot, type, p_data, data_len);
}

struct gps_mcudl_mcu2ap_ydata_sta {
	unsigned long curr_byte_cnt;
	unsigned long last_ntf_byte_cnt;
	unsigned long last_ntf_tick;
	unsigned int bypass_ntf_cnt;
};

struct gps_mcudl_mcu2ap_ydata_sta g_gps_mcudl_mcu2ap_ydata_sta[GPS_MDLY_CH_NUM];

void gps_mcudl_mcu2ap_ydata_sta_init(void)
{
	memset(&g_gps_mcudl_mcu2ap_ydata_sta, 0, sizeof(g_gps_mcudl_mcu2ap_ydata_sta));
}

struct gps_mcudl_mcu2ap_ydata_sta *gps_mcudl_mcu2ap_ydata_sta_get(enum gps_mcudl_yid yid)
{
	return &g_gps_mcudl_mcu2ap_ydata_sta[yid];
}

unsigned long gps_mcudl_mcu2ap_ydata_sta_get_recv_byte_cnt(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_mcu2ap_ydata_sta *p_ysta;
	unsigned long recv_byte_cnt;

	p_ysta = gps_mcudl_mcu2ap_ydata_sta_get(yid);
	gps_mcudl_slot_protect();
	recv_byte_cnt = p_ysta->curr_byte_cnt;
	gps_mcudl_slot_unprotect();
	return recv_byte_cnt;
}

void gps_mcudl_mcu2ap_ydata_sta_may_do_dump(enum gps_mcudl_yid yid, bool force)
{
	unsigned long ticks_since_last_notify;
	unsigned long bytes_since_last_notify;
	struct gps_mcudl_mcu2ap_ydata_sta *p_ysta;
	struct gps_mcudl_mcu2ap_ydata_sta ysta_bak;

	p_ysta = gps_mcudl_mcu2ap_ydata_sta_get(yid);
	gps_mcudl_slot_protect();
	ysta_bak = *p_ysta;
	gps_mcudl_slot_unprotect();

	if (ysta_bak.last_ntf_tick == 0)
		ticks_since_last_notify = 0;
	else
		ticks_since_last_notify = gps_dl_tick_get_us() - ysta_bak.last_ntf_tick;
	bytes_since_last_notify = ysta_bak.curr_byte_cnt - ysta_bak.last_ntf_byte_cnt;
	if (!force &&
		ticks_since_last_notify < 4*1000*1000 &&
		bytes_since_last_notify < (4*1024 + 512) &&
		ysta_bak.bypass_ntf_cnt < 5) {
		/* no need to show warning log if none of thresholds meet */
		return;
	}

	MDL_LOGYW(yid, "since last ntf: f=%d, dt=%lu, cnt=%u, bytes=%lu, %lu",
		force, ticks_since_last_notify, ysta_bak.bypass_ntf_cnt,
		ysta_bak.curr_byte_cnt, bytes_since_last_notify);
}

void gps_mcudl_mcu2ap_ydata_recv(enum gps_mcudl_yid yid,
	const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	struct gps_mcudl_data_rbuf_plus_t *p_rbuf;
	struct gps_mcudl_mcu2ap_ydata_sta *p_ysta;
	bool to_notify = true;

	p_trx_ctx = get_txrx_ctx(yid);
	p_rbuf = &p_trx_ctx->rx_rbuf;
	p_ysta = gps_mcudl_mcu2ap_ydata_sta_get(yid);

	/* writer */
	gps_mcudl_data_rbuf_put(p_rbuf, p_data, data_len);

	to_notify = !gps_mcudl_mcu2ap_get_wait_read_flag(yid);
	gps_mcudl_slot_protect();
	p_ysta->curr_byte_cnt += (unsigned long)data_len;
	if (to_notify) {
		p_ysta->last_ntf_tick = gps_dl_tick_get_us();
		p_ysta->last_ntf_byte_cnt = p_ysta->curr_byte_cnt;
		p_ysta->bypass_ntf_cnt = 0;
	} else
		p_ysta->bypass_ntf_cnt++;
	gps_mcudl_slot_unprotect();

	/* send msg to call gps_mcudl_ap2mcu_ydata_proc */
	if (to_notify) {
		gps_mcudl_mcu2ap_set_wait_read_flag(yid, true);
		gps_mcudl_ylink_event_send(yid, GPS_MCUDL_YLINK_EVT_ID_RX_DATA_READY);
		return;
	}

	/* force=fasle means to dump warning log only if condtion meets threshold */
	gps_mcudl_mcu2ap_ydata_sta_may_do_dump(yid, false);
}

void gps_mcudl_mcu2ap_ydata_proc(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	struct gps_mcudl_data_rbuf_plus_t *p_rbuf;
	struct gps_mcudl_data_pkt_parser_t *p_parser;
	gpsmdl_u32 reader_write_idx, reader_read_idx;

	p_trx_ctx = get_txrx_ctx(yid);
	p_rbuf = &p_trx_ctx->rx_rbuf;
	p_parser = &p_trx_ctx->parser;

	gps_mcudl_mcu2ap_set_wait_read_flag(yid, false);

	/* reader */
	gps_mcudl_data_rbuf_reader_sync_write_idx(p_rbuf);
	reader_write_idx = p_rbuf->cursor.rwi;
	gps_mcudl_data_pkt_parse(p_parser, reader_write_idx);

	reader_read_idx = p_parser->read_idx;
	p_rbuf->cursor.rri = reader_read_idx;
	gps_mcudl_data_rbuf_reader_update_read_idx(p_rbuf);

	MDL_LOGYD(yid, "reader: w=%d, r=%d", reader_write_idx, reader_read_idx);

	p_trx_ctx->host_sta.pkt_sta.total_recv =
		p_parser->proc_byte_cnt + p_parser->drop_byte_cnt;
	p_trx_ctx->host_sta.pkt_sta.total_parse_proc = (gpsmdl_u32)(p_parser->proc_byte_cnt);
	p_trx_ctx->host_sta.pkt_sta.total_parse_drop = (gpsmdl_u32)(p_parser->drop_byte_cnt);
	p_trx_ctx->host_sta.pkt_sta.total_route_drop = 0;
	p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt    = (gpsmdl_u32)(p_parser->pkt_cnt);
	p_trx_ctx->host_sta.pkt_sta.LUINT_L32_VALID_BIT = 32;
	if (yid != GPS_MDLY_URGENT)
		gps_mcudl_flowctrl_may_send_host_sta(yid);
}

void gps_mcudl_mcu_ch2_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	gps_mcudl_mcu_proc_func(type, payload_ptr, payload_len, GPS_MDLY_URGENT);
}

void gps_mcudl_mcu_ch1_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	gps_mcudl_mcu_proc_func(type, payload_ptr, payload_len, GPS_MDLY_NORMAL);
}

int gps_mcudl_mcu_ch2_send_func(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	int send_len;

	send_len = gps_mcudl_plat_mcu_ch2_write(p_data, data_len);
	MDL_LOGYD(GPS_MDLY_URGENT, "send len=%d, ret=%d", data_len, send_len);
	return send_len;
}

static void gps_mcudl_mcu_proc_func(enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len, enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];

	MDL_LOGYD(y_id, "recv type=%d, len=%d", type, payload_len);
	switch (type) {
	case GFNS_RSP_MCU_RST_UP_PKT_STA:
		MDL_LOGYI(y_id, "recv type=%d: GFNS_RSP_MCU_RST_UP_PKT_STA", type);
		memset(&p_trx_ctx->host_sta, 0, sizeof(p_trx_ctx->host_sta));
		p_trx_ctx->host_sta.is_enable = true;
		p_trx_ctx->host_sta.reset_flag = true;
		return;

	case GFNS_RSP_MCU_ACK_DN_PKT_STA: {
		struct gps_mcudl_data_pkt_mcu_sta sta;
		struct gps_mcudl_data_pkt_rec_item rec_item;
		int copy_size;
		bool to_notify = true;

		MDL_LOGYD(y_id, "recv type=%d, len=%d, expected_len=%lu",
			type, payload_len, sizeof(sta));

		/* TODO: struct size check */
		copy_size = sizeof(sta);
		if (copy_size > payload_len)
			copy_size = payload_len;
		memcpy(&sta, payload_ptr, copy_size);
		if (y_id == GPS_MDLY_NORMAL)
			gps_mcudl_flowctrl_remote_update_recv_byte(&sta, y_id);
		to_notify = !gps_mcudl_ap2mcu_get_wait_flush_flag(y_id);
		rec_item.mcu_ack.total_recv = sta.total_recv;
		rec_item.mcu_ack.total_parse_proc = sta.total_parse_proc;
		rec_item.mcu_ack.total_parse_drop = sta.total_parse_drop;
		rec_item.mcu_ack.total_route_drop = sta.total_route_drop;
		rec_item.mcu_ack.total_pkt_cnt = sta.total_pkt_cnt;
		rec_item.mcu_ack.LUINT_L32_VALID_BIT = sta.LUINT_L32_VALID_BIT;
		rec_item.mcu_ack.host_us = gps_dl_tick_get_us();
		gps_mcu_host_trans_hist_rec(&rec_item, GPS_MCUDL_HIST_REC_MCU_ACK);
		if (sta.total_recv != sta.total_parse_proc && !gps_mcu_host_trans_get_if_need_dump()) {
			gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_HOST_WR);
			gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_MCU_ACK);
			/*do dump until next session if there are some error cases*/
			gps_mcu_host_trans_set_if_need_dump(true);
		}
		if (gps_mcu_host_trans_get_if_need_dump())
			MDL_LOGYI(y_id, "recv_sta: %llu, %u, %u, %u, %u, 0x%x, ntf=%d",
			sta.total_recv,
			sta.total_parse_proc,
			sta.total_parse_drop,
			sta.total_route_drop,
			sta.total_pkt_cnt,
			sta.LUINT_L32_VALID_BIT,
			to_notify);

		if (to_notify) {
			gps_mcudl_ap2mcu_set_wait_flush_flag(y_id, true);
			gps_mcudl_ylink_event_send(y_id,
				GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
		}
		return;
	}
	case GPS_MDLYPL_MCUSYS:
		gps_mcusys_data_frame_proc(payload_ptr, payload_len);
		break;

	default:
		gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(y_id, type, payload_ptr, payload_len);
		break;
	}
}

int gps_mcudl_mcu_ch1_send_func(const gpsmdl_u8 *p_data, gpsmdl_u32 data_len)
{
	int send_len;

	send_len = gps_mcudl_plat_mcu_ch1_write(p_data, data_len);
	MDL_LOGYD(GPS_MDLY_NORMAL, "send len=%d, ret=%d", data_len, send_len);
	return send_len;
}

void gps_mcudl_ap2mcu_data_slot_flush_on_xwrite(enum gps_mcudl_xid x_id)
{
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum gps_mcudl_slot_flush_status flush_status;
	gpsmdl_u32 flush_done_len = 0;

	y_id = GPS_MDL_X2Y(x_id);
	p_trx_ctx = get_txrx_ctx(y_id);

	gps_mcudl_ap2mcu_set_wait_write_flag(y_id, false);
	flush_status = gps_mcudl_slot_flush(&p_trx_ctx->slot, &flush_done_len);
	if (flush_done_len > 0)
		gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(y_id);

	if (flush_status != FLUSH_OK)
		MDL_LOGYD(y_id, "flush: ret=%d, len=%d, x_id=%d", flush_status, flush_done_len, x_id);
	else
		MDL_LOGYD(y_id, "flush: ret=%d, len=%d, x_id=%d", flush_status, flush_done_len, x_id);
}

void gps_mcudl_ap2mcu_data_slot_flush_on_recv_sta(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	enum gps_mcudl_slot_flush_status flush_status;
	gpsmdl_u32 flush_done_len = 0;

	gps_mcudl_ap2mcu_set_wait_flush_flag(y_id, false);
	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	flush_status = gps_mcudl_slot_flush(&p_trx_ctx->slot, &flush_done_len);
	if (flush_done_len > 0)
		gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(y_id);

	MDL_LOGYD(y_id, "flush: ret=%d, len=%d", flush_status, flush_done_len);
}

void gps_mcudl_slot_protect(void)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_dl_osal_lock_unsleepable_lock(&g_data_pkt_ctx.trx[y_id].spin_lock);
}

void gps_mcudl_slot_unprotect(void)
{
	enum gps_mcudl_yid y_id;

	y_id = GPS_MDLY_NORMAL;
	gps_dl_osal_unlock_unsleepable_lock(&g_data_pkt_ctx.trx[y_id].spin_lock);
}

bool gps_mcudl_pkt_is_critical_type(gpsmdl_u8 type)
{
	enum gps_mcudl_pkt_type pkt_type;

	pkt_type = (enum gps_mcudl_pkt_type)type;
	switch (pkt_type) {
	case GPS_MDLYPL_MCUSYS:
	case GPS_MDLYPL_MCUFN:
	case GPS_MDLYPL_MNL:
	case GPS_MDLYPL_AGENT:
	case GPS_MDLYPL_NMEA:
	case GPS_MDLYPL_GDLOG:
	case GPS_MDLYPL_PMTK:
	case GPS_MDLYPL_MEAS:
	case GPS_MDLYPL_CORR:
	case GPS_MDLYPL_DSP0:
	case GPS_MDLYPL_DSP1:
		return false;

	default:
		break;
	}

	return true;
}

void gps_mcudl_flowctrl_init(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->local_flush_times = 0;
	p_trx_ctx->local_tx_len = 0;
	p_trx_ctx->remote_rx_len = 0;
}

void gps_mcudl_flowctrl_local_add_send_byte(gpsmdl_u32 delta, enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	/* TBD flow control for urgent channel */
	if (y_id == GPS_MDLY_URGENT)
		return;
	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->local_flush_times++;
	p_trx_ctx->local_tx_len += delta;
}

void gps_mcudl_flowctrl_remote_update_recv_byte(struct gps_mcudl_data_pkt_mcu_sta *p_sta, enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	/* TBD flow control for urgent channel */
	if (y_id == GPS_MDLY_URGENT)
		return;

	p_trx_ctx = get_txrx_ctx(y_id);
	p_trx_ctx->remote_rx_len = p_sta->total_recv;
}

#define CONN_RECV_BUF_MAX (3000)
gpsmdl_u32 gps_mcudl_flowctrl_cal_window_size(void)
{
	gpsmdl_u32 win_size;
	enum gps_mcudl_yid y_id;
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	y_id = GPS_MDLY_NORMAL;
	p_trx_ctx = get_txrx_ctx(y_id);

	if (p_trx_ctx->local_tx_len >= p_trx_ctx->remote_rx_len + CONN_RECV_BUF_MAX)
		win_size = 0;
	else
		win_size = CONN_RECV_BUF_MAX -
			(gpsmdl_u32)(p_trx_ctx->local_tx_len - p_trx_ctx->remote_rx_len);

	MDL_LOGYD(y_id, "cal_win=%u, local_tx=%llu, remote_rx=%llu", win_size,
		p_trx_ctx->local_tx_len, p_trx_ctx->remote_rx_len);
	return win_size;
}

#define GPS_MCUDL_PKT_HOST_ACK_LEN (2*1024)
void gps_mcudl_flowctrl_may_send_host_sta(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	struct geofence_pkt_host_sta_s rec_items;
	gpsmdl_u64 not_ack_len;
	bool to_notify = false;
	bool old_reset_flag = false;
	unsigned long curr_tick;
	static unsigned long last_tick;
	int delta_recv_len;

	p_trx_ctx = get_txrx_ctx(yid);
	not_ack_len = p_trx_ctx->host_sta.pkt_sta.total_recv - p_trx_ctx->host_sta.last_ack_recv_len;
	memset(&rec_items, 0, sizeof(rec_items));

	/* ack the length of host already received for two case:*/
	/* 1. ack the total length host received every n (KB)*/
	/* 2. ack if connsys need to reset host statistic, all of the sta value should be set to zero.*/
	/*	  connsys will block sending until host ack of this reset cmd back.*/
	if ((not_ack_len >= GPS_MCUDL_PKT_HOST_ACK_LEN) ||
		(p_trx_ctx->host_sta.reset_flag)) {
		old_reset_flag = p_trx_ctx->host_sta.reset_flag;
		if (p_trx_ctx->host_sta.is_enable) {
			gps_mcudl_ap2mcu_ydata_send(yid, GFNS_REQ_MCU_ACK_UP_PKT_STA,
				(gpsmdl_u8 *)&(p_trx_ctx->host_sta.pkt_sta),
				sizeof(p_trx_ctx->host_sta.pkt_sta));
			p_trx_ctx->host_sta.reset_flag = 0;
		}

		to_notify = !gps_mcudl_ap2mcu_get_wait_flush_flag(yid);
		curr_tick = gps_dl_tick_get_us();

		rec_items.pkt_sta.total_recv = p_trx_ctx->host_sta.pkt_sta.total_recv;
		rec_items.last_ack_recv_len = p_trx_ctx->host_sta.last_ack_recv_len;
		rec_items.pkt_sta.total_parse_proc = p_trx_ctx->host_sta.pkt_sta.total_parse_proc;
		rec_items.pkt_sta.total_pkt_cnt = p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt;
		rec_items.pkt_sta.total_parse_drop = p_trx_ctx->host_sta.pkt_sta.total_parse_drop;
		rec_items.pkt_sta.total_route_drop = p_trx_ctx->host_sta.pkt_sta.total_route_drop;
		rec_items.is_enable = p_trx_ctx->host_sta.is_enable;
		rec_items.reset_flag = p_trx_ctx->host_sta.reset_flag;

		gps_mcudl_host_sta_hist_rec(yid, &rec_items);

		if ((curr_tick - last_tick) >= 1000000) {
			delta_recv_len = (int)(p_trx_ctx->host_sta.pkt_sta.total_recv -
				p_trx_ctx->host_sta.last_print_recv_len);
			MDL_LOGYI(yid,
				"send_host_ack:recv32=%u,recv_isr=%lu,recv_tsk=%llu,last=%llu,delta=%d,proc=%u,pkt=%u,pdrop=%u,rdrop=%u,en=%u,rst=%u,nack=%llu,ntf=%d",
				/* trim to 32bits to match with FW log */
				(unsigned int)p_trx_ctx->host_sta.pkt_sta.total_recv,
				gps_mcudl_mcu2ap_ydata_sta_get_recv_byte_cnt(yid),
				p_trx_ctx->host_sta.pkt_sta.total_recv,
				p_trx_ctx->host_sta.last_ack_recv_len, delta_recv_len,
				p_trx_ctx->host_sta.pkt_sta.total_parse_proc,
				p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt,
				p_trx_ctx->host_sta.pkt_sta.total_parse_drop,
				p_trx_ctx->host_sta.pkt_sta.total_route_drop,
				p_trx_ctx->host_sta.is_enable,
				old_reset_flag,
				not_ack_len,
				to_notify);
			last_tick = curr_tick;
			p_trx_ctx->host_sta.last_print_recv_len = p_trx_ctx->host_sta.last_ack_recv_len;
		}

		if (p_trx_ctx->host_sta.is_enable && to_notify) {
			gps_mcudl_ap2mcu_set_wait_flush_flag(yid, true);
			gps_mcudl_ylink_event_send(yid,
				GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
		}

		p_trx_ctx->host_sta.last_ack_recv_len = p_trx_ctx->host_sta.pkt_sta.total_recv;
	}
}

void gps_mcudl_flowctrl_dump_host_sta(enum gps_mcudl_yid yid)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	gpsmdl_u64 not_ack_len;
	bool old_reset_flag = false;

	p_trx_ctx = get_txrx_ctx(yid);
	not_ack_len = p_trx_ctx->host_sta.pkt_sta.total_recv - p_trx_ctx->host_sta.last_ack_recv_len;
	old_reset_flag = p_trx_ctx->host_sta.reset_flag;
	MDL_LOGYW(yid,
		"dump_host_ack:recv=%u,last=%u,proc=%u,pkt=%u,pdrop=%u,rdrop=%u,en=%u,rst=%u,nack=%llu",
		(gpsmdl_u32)(p_trx_ctx->host_sta.pkt_sta.total_recv),
		(gpsmdl_u32)(p_trx_ctx->host_sta.last_ack_recv_len),
		p_trx_ctx->host_sta.pkt_sta.total_parse_proc,
		p_trx_ctx->host_sta.pkt_sta.total_pkt_cnt,
		p_trx_ctx->host_sta.pkt_sta.total_parse_drop,
		p_trx_ctx->host_sta.pkt_sta.total_route_drop,
		p_trx_ctx->host_sta.is_enable,
		old_reset_flag,
		not_ack_len);
}

#define GPS_MCU_HOST_STA_REC_MAX (10)
struct geofence_pkt_host_sta_s g_gps_mcudl_host_sta_rec_array[GPS_MCU_HOST_STA_REC_MAX][GPS_MDLY_CH_NUM];
unsigned long g_gps_mcudl_host_sta_rec_cnt[GPS_MDLY_CH_NUM];
bool g_gps_mcudl_host_sta_rec_in_dump[GPS_MDLY_CH_NUM];

void gps_mcudl_host_sta_hist_init(void)
{
	gps_mcudl_slot_protect();
	memset(&g_gps_mcudl_host_sta_rec_array, 0, sizeof(g_gps_mcudl_host_sta_rec_array));
	g_gps_mcudl_host_sta_rec_cnt[GPS_MDLY_NORMAL] = 0;
	g_gps_mcudl_host_sta_rec_cnt[GPS_MDLY_URGENT] = 0;
	g_gps_mcudl_host_sta_rec_in_dump[GPS_MDLY_NORMAL] = false;
	g_gps_mcudl_host_sta_rec_in_dump[GPS_MDLY_URGENT] = false;
	gps_mcudl_slot_unprotect();
}

void gps_mcudl_host_sta_hist_rec(enum gps_mcudl_yid yid, struct geofence_pkt_host_sta_s *host_sta)
{
	bool in_dump;
	unsigned long index;
	gpsmdl_u64 not_ack_len;
	bool old_reset_flag = false;

	gps_mcudl_slot_protect();
	index = g_gps_mcudl_host_sta_rec_cnt[yid];
	in_dump = g_gps_mcudl_host_sta_rec_in_dump[yid];
	not_ack_len = host_sta->pkt_sta.total_recv - host_sta->last_ack_recv_len;
	old_reset_flag = host_sta->reset_flag;

	/* Skip record and dump immediately if im_dump */
	if (in_dump) {
		gps_mcudl_slot_unprotect();
		MDL_LOGW(
			"dump_host_ack:recv=%u,last=%u,proc=%u,pkt=%u,pdrop=%u,rdrop=%u,en=%u,rst=%u,nack=%llu",
			(gpsmdl_u32)(host_sta->pkt_sta.total_recv),
			(gpsmdl_u32)(host_sta->last_ack_recv_len),
			host_sta->pkt_sta.total_parse_proc,
			host_sta->pkt_sta.total_pkt_cnt,
			host_sta->pkt_sta.total_parse_drop,
			host_sta->pkt_sta.total_route_drop,
			host_sta->is_enable,
			old_reset_flag,
			not_ack_len);
		return;
	}

	/* Add a record */
	index = (g_gps_mcudl_host_sta_rec_cnt[yid] % GPS_MCU_HOST_STA_REC_MAX);
	g_gps_mcudl_host_sta_rec_array[index][yid] = *host_sta;
	g_gps_mcudl_host_sta_rec_cnt[yid]++;
	gps_mcudl_slot_unprotect();
}

void gps_mcudl_host_sta_hist_dump_rec(unsigned long index, struct geofence_pkt_host_sta_s *host_sta)
{
	gpsmdl_u64 not_ack_len;
	bool old_reset_flag = false;

	not_ack_len = host_sta->pkt_sta.total_recv - host_sta->last_ack_recv_len;
	old_reset_flag = host_sta->reset_flag;

	MDL_LOGW(
		"dump_host_ack:recv=%u,last=%u,proc=%u,pkt=%u,pdrop=%u,rdrop=%u,en=%u,rst=%u,nack=%llu",
		(gpsmdl_u32)(host_sta->pkt_sta.total_recv),
		(gpsmdl_u32)(host_sta->last_ack_recv_len),
		host_sta->pkt_sta.total_parse_proc,
		host_sta->pkt_sta.total_pkt_cnt,
		host_sta->pkt_sta.total_parse_drop,
		host_sta->pkt_sta.total_route_drop,
		host_sta->is_enable,
		old_reset_flag,
		not_ack_len);
}

void gps_mcudl_host_sta_hist_dump(enum gps_mcudl_yid yid)
{
	unsigned long index;

	gps_mcudl_slot_protect();
	index = g_gps_mcudl_host_sta_rec_cnt[yid];
	if (g_gps_mcudl_host_sta_rec_in_dump[yid] || index == 0) {
		gps_mcudl_slot_unprotect();
		MDL_LOGW("in_dump=1, cnt=%lu, skip", index);
		return;
	}
	g_gps_mcudl_host_sta_rec_in_dump[yid] = true;
	gps_mcudl_slot_unprotect();

	if (g_gps_mcudl_host_sta_rec_cnt[yid] <= GPS_MCU_HOST_STA_REC_MAX) {
		for (index = 0; index < g_gps_mcudl_host_sta_rec_cnt[yid]; index++) {
			gps_mcudl_host_sta_hist_dump_rec(index,
				&g_gps_mcudl_host_sta_rec_array[index][yid]);
		}
	} else {
		index = g_gps_mcudl_host_sta_rec_cnt[yid] - GPS_MCU_HOST_STA_REC_MAX + 1;
		for (; index < g_gps_mcudl_host_sta_rec_cnt[yid]; index++) {
			gps_mcudl_host_sta_hist_dump_rec(index,
				&g_gps_mcudl_host_sta_rec_array[index % GPS_MCU_HOST_STA_REC_MAX][yid]);
		}
	}

	gps_mcudl_slot_protect();
	g_gps_mcudl_host_sta_rec_in_dump[yid] = false;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_mcu2ap_get_wait_read_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_read_to_proc_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_mcu2ap_set_wait_read_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_read_to_proc_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_ap2mcu_get_wait_write_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_write_to_flush_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_wait_write_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_write_to_flush_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_ap2mcu_get_wait_flush_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->wait_sta_to_flush_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_wait_flush_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->wait_sta_to_flush_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_ap2mcu_get_write_fail_flag(enum gps_mcudl_yid y_id)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;
	bool flag;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	flag = p_trx_ctx->write_to_mcu_may_fail_flag;
	gps_mcudl_slot_unprotect();
	return flag;
}

void gps_mcudl_ap2mcu_set_write_fail_flag(enum gps_mcudl_yid y_id, bool flag)
{
	struct gps_mcudl_data_trx_context *p_trx_ctx;

	p_trx_ctx = &g_data_pkt_ctx.trx[y_id];
	gps_mcudl_slot_protect();
	p_trx_ctx->write_to_mcu_may_fail_flag = flag;
	gps_mcudl_slot_unprotect();
}

bool gps_mcudl_slot_may_pend_pkt_type_if_near_full(struct gps_mcudl_data_slot_t *p_slot,
	enum gps_mcudl_pkt_type type, int len)
{
	bool near_full;

	/* near_full is always false if the pkt not in list */
	if (!((unsigned int)type >= (unsigned int)GPS_MDLYPL_MCUSYS &&
		(unsigned int)type < (unsigned int)GPS_MDLYPL_MAXID))
		return false;

	/* If there is no space for ap2mcu ack pkt for flowctrl, mcu2ap will be stuck.
	 * When slot is near full, pending xlink pkt(which can be blocking and sent later),
	 * in order to keep some space for flowctrl pkt.
	 */
	gps_mcudl_slot_protect();
	/* Now, we think it's near full if only 10 pkt_entry or 2KB buf left */
	near_full = (p_slot->entr_cursor.pkt_cnt + 10 >= ENTR_MAX ||
		p_slot->rbuf_cursor.data_len + 2048 >= RBUF_MAX);
	gps_mcudl_slot_unprotect();

	if (near_full)
		MDL_LOGW("hit:type=0x%x, len=%d", type, len);
	return near_full;
}

void gps_mcudl_ap2mcu_try_to_wakeup_xlink_writer(enum gps_mcudl_yid y_id)
{
	enum gps_mcudl_xid x_id;
	struct gps_mcudl_each_link *p_xlink;

	MDL_LOGYD(y_id, "");
	for (x_id = GPS_MDLX_MCUSYS; x_id < GPS_MDLX_CH_NUM; x_id++) {
		p_xlink = gps_mcudl_link_get(x_id);
		gps_dl_link_wake_up(&p_xlink->waitables[GPS_DL_WAIT_WRITE]);
	}
}


bool gps_mcudl_xid2ypl_type(enum gps_mcudl_xid x_id, enum gps_mcudl_pkt_type *p_type)
{
	enum gps_mcudl_pkt_type type;
	bool is_okay = true;

	switch (x_id) {
	case GPS_MDLX_MCUSYS:
		type = GPS_MDLYPL_MCUSYS;
		break;
	case GPS_MDLX_MCUFN:
		type = GPS_MDLYPL_MCUFN;
		break;
	case GPS_MDLX_MNL:
		type = GPS_MDLYPL_MNL;
		break;
	case GPS_MDLX_AGENT:
		type = GPS_MDLYPL_AGENT;
		break;
	case GPS_MDLX_NMEA:
		type = GPS_MDLYPL_NMEA;
		break;
	case GPS_MDLX_GDLOG:
		type = GPS_MDLYPL_GDLOG;
		break;
	case GPS_MDLX_PMTK:
		type = GPS_MDLYPL_PMTK;
		break;
	case GPS_MDLX_MEAS:
		type = GPS_MDLYPL_MEAS;
		break;
	case GPS_MDLX_CORR:
		type = GPS_MDLYPL_CORR;
		break;
	case GPS_MDLX_DSP0:
		type = GPS_MDLYPL_DSP0;
		break;
	case GPS_MDLX_DSP1:
		type = GPS_MDLYPL_DSP1;
		break;

	case GPS_MDLX_AOL_TEST:
		type = GPS_MDLYPL_AOLTS;
		break;
	case GPS_MDLX_MPE_TEST:
		type = GPS_MDLYPL_MPETS;
		break;
	case GPS_MDLX_SCP_TEST:
		type = GPS_MDLYPL_SCPTS;
		break;

	case GPS_MDLX_LPPM:
		type = GPS_MDLYPL_LPPM;
		break;
	case GPS_MDLX_MPELOG:
		type = GPS_MDLYPL_MPELOG;
		break;

	default:
		is_okay = false;
		return is_okay;
	}

	is_okay = true;
	if (p_type)
		*p_type = type;
	return is_okay;
}

bool gps_mcudl_ypl_type2xid(enum gps_mcudl_pkt_type type, enum gps_mcudl_xid *p_xid)
{
	enum gps_mcudl_xid x_id;
	bool is_okay = true;

	switch (type) {
	case GPS_MDLYPL_MCUSYS:
		x_id = GPS_MDLX_MCUSYS;
		break;
	case GPS_MDLYPL_MCUFN:
		x_id = GPS_MDLX_MCUFN;
		break;
	case GPS_MDLYPL_MNL:
		x_id = GPS_MDLX_MNL;
		break;
	case GPS_MDLYPL_AGENT:
		x_id = GPS_MDLX_AGENT;
		break;
	case GPS_MDLYPL_NMEA:
		x_id = GPS_MDLX_NMEA;
		break;
	case GPS_MDLYPL_GDLOG:
		x_id = GPS_MDLX_GDLOG;
		break;
	case GPS_MDLYPL_PMTK:
		x_id = GPS_MDLX_PMTK;
		break;
	case GPS_MDLYPL_MEAS:
		x_id = GPS_MDLX_MEAS;
		break;
	case GPS_MDLYPL_CORR:
		x_id = GPS_MDLX_CORR;
		break;
	case GPS_MDLYPL_DSP0:
		x_id = GPS_MDLX_DSP0;
		break;
	case GPS_MDLYPL_DSP1:
		x_id = GPS_MDLX_DSP1;
		break;

	case GPS_MDLYPL_AOLTS:
		x_id = GPS_MDLX_AOL_TEST;
		break;
	case GPS_MDLYPL_MPETS:
		x_id = GPS_MDLX_MPE_TEST;
		break;
	case GPS_MDLYPL_SCPTS:
		x_id = GPS_MDLX_SCP_TEST;
		break;

	case GPS_MDLYPL_LPPM:
		x_id = GPS_MDLX_LPPM;
		break;
	case GPS_MDLYPL_MPELOG:
		x_id = GPS_MDLX_MPELOG;
		break;

	default:
		is_okay = false;
		return is_okay;
	}

	is_okay = true;
	if (p_xid)
		*p_xid = x_id;
	return is_okay;
}

#define MCU2AP_PAYLOAD_REC_MAX (8)
#define GPS_PKT_PL_DUMP_STR "pl=[%02x %02x %02x %02x, %02x %02x %02x %02x]"

#define MCU2AP_PKT_REC_MAX (32)
struct gps_mcudl_mcu2ap_pkt_rec_item {
	unsigned long tick_us;
	gpsmdl_u8 payload[MCU2AP_PAYLOAD_REC_MAX];
	enum gps_mcudl_yid y_id;
	enum gps_mcudl_xid x_id;
	enum gps_mcudl_pkt_type type;
	bool do_wake;
	gpsmdl_u16 len;
};

struct gps_mcudl_mcu2ap_pkt_rec_item g_gps_mcu2ap_pkt_rec_list[MCU2AP_PKT_REC_MAX];
unsigned long g_gps_mcu2ap_pkt_rec_cnt;

unsigned long gps_mcudl_mcu2ap_rec_add_item(struct gps_mcudl_mcu2ap_pkt_rec_item *p_item)
{
	unsigned long index;

	index = (g_gps_mcu2ap_pkt_rec_cnt % MCU2AP_PKT_REC_MAX);
	g_gps_mcu2ap_pkt_rec_list[index] = *p_item;
	g_gps_mcu2ap_pkt_rec_cnt++;
	return index;
}

void gps_mcudl_mcu2ap_rec_dump_item(struct gps_mcudl_mcu2ap_pkt_rec_item *p_item, unsigned long index)
{
	MDL_LOGW("i=%lu,us=%lu,y=%d,x=%2d,type=0x%02x,w=%d,len=%4d,"GPS_PKT_PL_DUMP_STR,
		index, p_item->tick_us, p_item->y_id, p_item->x_id,
		p_item->type, p_item->do_wake, p_item->len,
		p_item->payload[0], p_item->payload[1], p_item->payload[2], p_item->payload[3],
		p_item->payload[4], p_item->payload[5], p_item->payload[6], p_item->payload[7]);
}

void gps_mcudl_mcu2ap_rec_dump(void)
{
	unsigned long index;
	struct gps_mcudl_mcu2ap_pkt_rec_item *p_item;

	if (g_gps_mcu2ap_pkt_rec_cnt == 0) {
		MDL_LOGW("no_rec");
		return;
	}

	if (g_gps_mcu2ap_pkt_rec_cnt <= MCU2AP_PKT_REC_MAX)
		index = 0;
	else
		index = g_gps_mcu2ap_pkt_rec_cnt - MCU2AP_PKT_REC_MAX;

	for ( ; index < g_gps_mcu2ap_pkt_rec_cnt; index++) {
		p_item = &g_gps_mcu2ap_pkt_rec_list[index % MCU2AP_PKT_REC_MAX];
		gps_mcudl_mcu2ap_rec_dump_item(p_item, index);
	}
}

void gps_mcudl_mcu2ap_rec_init(void)
{
	g_gps_mcu2ap_pkt_rec_cnt = 0;
}

struct gps_mcudl_mcu2ap_put_to_xlink_fail_rec_item {
	unsigned long print_us;
	unsigned int drop_cnt;
	unsigned int drop_len;
};

struct gps_mcudl_mcu2ap_put_to_xlink_fail_rec_item g_gps_mcu2ap_put_to_xlink_fail_rec_list[GPS_MDLX_CH_NUM];

void gps_mcudl_mcu2ap_put_to_xlink_fail_rec_dump(void)
{
	enum gps_mcudl_xid xid = 0;
	struct gps_mcudl_mcu2ap_put_to_xlink_fail_rec_item *p_item;

	for ( ; xid < GPS_MDLX_CH_NUM; xid++) {
		p_item = &g_gps_mcu2ap_put_to_xlink_fail_rec_list[xid];
		if (p_item->drop_cnt != 0)
			MDL_LOGXW(xid, "drop_cnt = %d, drop_len = %d",
				p_item->drop_cnt, p_item->drop_len);
	}
}

void gps_mcudl_mcu2ap_put_to_xlink_fail_rec_init(void)
{
	memset(g_gps_mcu2ap_put_to_xlink_fail_rec_list, 0,
		sizeof(g_gps_mcu2ap_put_to_xlink_fail_rec_list));
}


bool g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag;
unsigned int g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt;
#define GPS_MCUD_MCU2AP_AFTER_AP_RESUME_DUMP_MAX (2)

void gps_mcudl_mcu2ap_arrange_pkt_dump_after_ap_resume(void)
{
	g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag = true;
	g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt = 0;
}

void gps_mcudl_mcu2ap_clear_ap_resume_pkt_dump_flag(void)
{
	g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag = false;
	g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt = 0;
}


void gps_mcudl_mcu2ap_try_to_wakeup_xlink_reader(enum gps_mcudl_yid y_id, enum gps_mcudl_pkt_type type,
	const gpsmdl_u8 *payload_ptr, gpsmdl_u16 payload_len)
{
	enum gps_mcudl_xid x_id;
	enum gps_mcudl_xid x_id_next;
	struct gps_mcudl_each_link *p_xlink;
	enum GDL_RET_STATUS gdl_ret;
	bool do_wake;
	struct gps_mcudl_mcu2ap_pkt_rec_item rec_item;
	int rec_pl_len;
	unsigned long curr_tick;
	unsigned long record_pkt_idx;

	if (!gps_mcudl_ypl_type2xid(type, &x_id)) {
		MDL_LOGYW(y_id, "recv type=%d, len=%d, no x_id", type, payload_len);
		return;
	}

	if (gps_mcudl_mcu2ap_test_bypass_get()) {
		MDL_LOGYW(y_id, "recv type=%d, len=%d, bypass", type, payload_len);
		return;
	}

_loop_start:
	if (x_id == GPS_MDLX_GDLOG) {
		/* duplicate the payload to GDLOG2 device node */
		x_id_next = GPS_MDLX_GDLOG2;
	} else if (x_id == GPS_MDLX_MPELOG) {
		/* duplicate the payload to MPELOG2 device node */
		x_id_next = GPS_MDLX_MPELOG2;
	} else {
		/* other payload type only be dispatched to signle device node */
		x_id_next = GPS_MDLX_CH_NUM;
	}

	MDL_LOGYD(y_id, "recv type=%d, len=%d, to x_id=%d", type, payload_len, x_id);
	p_xlink = gps_mcudl_link_get(x_id);

	if (gps_mcudl_each_link_get_state(x_id) != LINK_OPENED)
		goto _loop_end;

	gdl_ret = gdl_dma_buf_put(&p_xlink->rx_dma_buf, payload_ptr, payload_len);
	/* Openwrt, coverity is going to check enum variable more than 0 */
	/* Andriod, coverity do not */
	/* no need sync this code between Andriod and Openwrt*/
	if (gdl_ret != GDL_OKAY && x_id < GPS_MDLX_CH_NUM) {
		g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].drop_cnt++;
		g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].drop_len += payload_len;
		curr_tick = gps_dl_tick_get_us();
		if (curr_tick - g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].print_us > 1000000UL) {
			MDL_LOGXW(x_id,
				"gdl_dma_buf_put: ret=%s, entry: %u, byte: %u, drop_cnt: %u, drop_len: %u",
				gdl_ret_to_name(gdl_ret), gps_dma_buf_count_data_entry(&p_xlink->rx_dma_buf),
				gps_dma_buf_count_data_byte(&p_xlink->rx_dma_buf),
				g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].drop_cnt,
				g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].drop_len);
			g_gps_mcu2ap_put_to_xlink_fail_rec_list[x_id].print_us = curr_tick;
		}
		/* not return, still wakeup to avoid race condition */
		/* goto _loop_end; */
	}

	do_wake = gps_dl_link_wake_up2(&p_xlink->waitables[GPS_DL_WAIT_READ]);
	if (do_wake && (x_id != GPS_MDLX_GDLOG) && (x_id != GPS_MDLX_GDLOG2) &&
		(x_id != GPS_MDLX_MPELOG) && (x_id != GPS_MDLX_MPELOG2)) {
		MDL_LOGXD(x_id, "do_wake=%d, type=%d, len=%d, y_id=%d",
			do_wake, type, payload_len, y_id);
	} else {
		MDL_LOGXD(x_id, "do_wake=%d, type=%d, len=%d, y_id=%d",
			do_wake, type, payload_len, y_id);
	}

	/* Recording pkt info */
	rec_item.tick_us = gps_dl_tick_get_us();
	rec_item.x_id = x_id;
	rec_item.y_id = y_id;
	rec_item.type = type;
	rec_item.len = payload_len;
	rec_item.do_wake = do_wake;
	if (payload_len >= MCU2AP_PAYLOAD_REC_MAX)
		rec_pl_len = MCU2AP_PAYLOAD_REC_MAX;
	else
		rec_pl_len = payload_len;
	memset(&rec_item.payload[0], 0, sizeof(rec_item.payload));
	if (rec_pl_len > 0)
		memcpy(&rec_item.payload[0], payload_ptr, rec_pl_len);

	record_pkt_idx = gps_mcudl_mcu2ap_rec_add_item(&rec_item);
	if (gdl_ret != GDL_OKAY || g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag) {
		MDL_LOGXW(x_id, "gdl_dma_buf_put: ret=%s, pkt_idx=%lu, ap_resume=%d,%d",
			gdl_ret_to_name(gdl_ret), record_pkt_idx,
			g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag,
			g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt);
		gps_mcudl_mcu2ap_rec_dump_item(&rec_item, record_pkt_idx);
	}
	if (g_gps_mcudl_mcu2ap_after_ap_resume_dump_flag) {
		g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt++;
		if (g_gps_mcudl_mcu2ap_after_ap_resume_dump_cnt >=
			GPS_MCUD_MCU2AP_AFTER_AP_RESUME_DUMP_MAX)
			gps_mcudl_mcu2ap_clear_ap_resume_pkt_dump_flag();
	}

_loop_end:
	if ((unsigned int)x_id_next < (unsigned int)GPS_MDLX_CH_NUM) {
		x_id = x_id_next;
		goto _loop_start;
	}
}

bool g_mcu2ap_test_xdata_bypass;

void gps_mcudl_mcu2ap_test_bypass_set(bool bypass)
{
	g_mcu2ap_test_xdata_bypass = bypass;
}

bool gps_mcudl_mcu2ap_test_bypass_get(void)
{
	return g_mcu2ap_test_xdata_bypass;
}

#define GPS_MCU_TRANS_REC_MAX (20)
struct gps_mcudl_data_pkt_rec_item g_gps_mcu_trans_rec_array[GPS_MCU_TRANS_REC_MAX];
unsigned long g_gps_mcu_trans_rec_cnt[GPS_MCUDL_HIST_REC_TYPE_MAX];
bool g_gps_mcu_trans_rec_in_dump[GPS_MCUDL_HIST_REC_TYPE_MAX];

void gps_mcu_host_trans_hist_init(void)
{
	gps_mcudl_slot_protect();
	memset(&g_gps_mcu_trans_rec_array[0], 0, sizeof(g_gps_mcu_trans_rec_array));
	g_gps_mcu_trans_rec_cnt[GPS_MCUDL_HIST_REC_MCU_ACK] = 0;
	g_gps_mcu_trans_rec_cnt[GPS_MCUDL_HIST_REC_HOST_WR] = 0;
	g_gps_mcu_trans_rec_in_dump[GPS_MCUDL_HIST_REC_MCU_ACK] = false;
	g_gps_mcu_trans_rec_in_dump[GPS_MCUDL_HIST_REC_HOST_WR] = false;
	/*no need dump when reopen gps_mcudl*/
	gps_mcu_host_trans_set_if_need_dump(false);
	gps_mcudl_slot_unprotect();
}

void gps_mcu_host_trans_hist_rec(struct gps_mcudl_data_pkt_rec_item *p_rec, enum gps_mcudl_rec_type rec_point)
{
	bool in_dump;
	unsigned long index;

	gps_mcudl_slot_protect();
	index = g_gps_mcu_trans_rec_cnt[rec_point];
	in_dump = g_gps_mcu_trans_rec_in_dump[rec_point];

	/* Skip record and dump immediately if im_dump */
	if (in_dump) {
		gps_mcudl_slot_unprotect();
		if (rec_point == GPS_MCUDL_HIST_REC_MCU_ACK)
			MDL_LOGW("RD: in_dump=1, cnt=%lu, recv_sta: %llu, %u, %u, %u, %u, 0x%x, ts=%lu",
				index, p_rec->mcu_ack.total_recv, p_rec->mcu_ack.total_parse_proc,
				p_rec->mcu_ack.total_parse_drop, p_rec->mcu_ack.total_route_drop,
				p_rec->mcu_ack.total_pkt_cnt, p_rec->mcu_ack.LUINT_L32_VALID_BIT,
				p_rec->mcu_ack.host_us);
		else
			MDL_LOGW("WR: in_dump=1, cnt=%lu, len=%d, is_ok=%d, ts=%lu",
			index, p_rec->host_wr.len, p_rec->host_wr.is_okay, p_rec->host_wr.host_us);
		return;
	}

	/* Add a record */
	index = (g_gps_mcu_trans_rec_cnt[rec_point] % GPS_MCU_TRANS_REC_MAX);
	if (rec_point == GPS_MCUDL_HIST_REC_MCU_ACK)
		g_gps_mcu_trans_rec_array[index].mcu_ack = (*p_rec).mcu_ack;
	else
		g_gps_mcu_trans_rec_array[index].host_wr = (*p_rec).host_wr;
	g_gps_mcu_trans_rec_cnt[rec_point]++;
	gps_mcudl_slot_unprotect();
}

void gps_mcu_host_trans_hist_dump_rec(unsigned long index, struct gps_mcudl_data_pkt_rec_item *p_rec,
	enum gps_mcudl_rec_type rec_point)
{
	if (rec_point == GPS_MCUDL_HIST_REC_MCU_ACK)
		MDL_LOGW("RD: i=%lu, recv_sta: %llu, %u, %u, %u, %u, 0x%x, ts=%lu",
			index, p_rec->mcu_ack.total_recv, p_rec->mcu_ack.total_parse_proc,
			p_rec->mcu_ack.total_parse_drop, p_rec->mcu_ack.total_route_drop,
			p_rec->mcu_ack.total_pkt_cnt, p_rec->mcu_ack.LUINT_L32_VALID_BIT,
			p_rec->mcu_ack.host_us);
	else
		MDL_LOGW("WR: i=%lu, len=%d, is_ok=%d, ts=%lu",
		index, p_rec->host_wr.len, p_rec->host_wr.is_okay, p_rec->host_wr.host_us);
}

void gps_mcu_host_trans_hist_dump(enum gps_mcudl_rec_type rec_point)
{
	unsigned long index;

	gps_mcudl_slot_protect();
	index = g_gps_mcu_trans_rec_cnt[rec_point];
	if (g_gps_mcu_trans_rec_in_dump[rec_point] || index == 0) {
		gps_mcudl_slot_unprotect();
		MDL_LOGW("in_dump=1, cnt=%lu, rec_point=%d, skip", index, rec_point);
		return;
	}
	g_gps_mcu_trans_rec_in_dump[rec_point] = true;
	gps_mcudl_slot_unprotect();

	if (g_gps_mcu_trans_rec_cnt[rec_point] <= GPS_MCU_TRANS_REC_MAX) {
		for (index = 0; index < g_gps_mcu_trans_rec_cnt[rec_point]; index++) {
			gps_mcu_host_trans_hist_dump_rec(index,
				&g_gps_mcu_trans_rec_array[index], rec_point);
		}
	} else {
		index = g_gps_mcu_trans_rec_cnt[rec_point] - GPS_MCU_TRANS_REC_MAX + 1;
		for (; index < g_gps_mcu_trans_rec_cnt[rec_point]; index++) {
			gps_mcu_host_trans_hist_dump_rec(index,
				&g_gps_mcu_trans_rec_array[index % GPS_MCU_TRANS_REC_MAX], rec_point);
		}
	}

	gps_mcudl_slot_protect();
	g_gps_mcu_trans_rec_in_dump[rec_point] = false;
	gps_mcudl_slot_unprotect();
}

bool if_need_print;
bool gps_mcu_host_trans_set_if_need_dump(bool if_print)
{
	bool last_need_print = if_need_print;

	if_need_print = if_print;
	return last_need_print;
}

bool gps_mcu_host_trans_get_if_need_dump(void)
{
	return if_need_print;
}

