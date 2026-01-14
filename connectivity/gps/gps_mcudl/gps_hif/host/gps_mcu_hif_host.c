/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_data_pkt_slot.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_ylink.h"
#if GPS_DL_HAS_MCUDL_HAL
#include "gps_mcudl_hal_ccif.h"
#endif
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#if GPS_DL_ON_LINUX
#include <asm/io.h>
#include "gps_dl_linux_reserved_mem_v2.h"
#else
#include <string.h>
#endif

union gps_mcu_hif_ap2mcu_shared_data_union *p_gps_mcu_hif_ap2mcu_region;
union gps_mcu_hif_mcu2ap_shared_data_union *p_gps_mcu_hif_mcu2ap_region;

struct gps_mcu_hif_recv_ch_context {
	bool is_listening;
	bool fail_flag;
	gps_mcu_hif_ch_on_recv_cb custom_cb;
};

struct gps_mcu_hif_recv_ch_context g_gps_mcu_hif_recv_contexts[GPS_MCU_HIF_CH_NUM];

void gps_mcu_hif_host_init_ch(enum gps_mcu_hif_ch hif_ch)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;

	p_buf = gps_mcu_hif_get_ap2mcu_emi_buf_addr(hif_ch);
#if GPS_DL_ON_LINUX
	memset_io(p_buf, 0, gps_mcu_hif_get_ap2mcu_emi_buf_len(hif_ch));
#else
	memset(p_buf, 0, gps_mcu_hif_get_ap2mcu_emi_buf_len(hif_ch));
#endif
	start_desc.addr = 0;
	start_desc.len = 0;
	start_desc.id = 0;
	start_desc.zero = 0;
	gps_mcu_hif_set_ap2mcu_trans_start_desc(hif_ch, &start_desc);

	p_buf = gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch);
#if GPS_DL_ON_LINUX
	memset_io(p_buf, 0, gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch));
#else
	memset(p_buf, 0, gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch));
#endif
	start_desc.addr = 0;
	start_desc.len = 0;
	start_desc.id = 0;
	start_desc.zero = 0;
	gps_mcu_hif_set_mcu2ap_trans_start_desc(hif_ch, &start_desc);
}

unsigned int gps_mcu_hif_convert_ap_addr2mcu_addr(unsigned char *p_buf)
{
#if GPS_DL_ON_LINUX
	struct gps_mcudl_emi_layout *p_layout =
		gps_dl_get_conn_emi_layout_ptr();

	return (unsigned int)(p_buf - (unsigned char *)p_layout) + 0x70000000;
#else
	return (unsigned int)p_buf - 0x87000000 + 0x70000000;
#endif
}

void gps_mcu_hif_init(void)
{
#if GPS_DL_ON_LINUX
	struct gps_mcudl_emi_layout *p_layout =
		gps_dl_get_conn_emi_layout_ptr();

	p_gps_mcu_hif_ap2mcu_region = (union gps_mcu_hif_ap2mcu_shared_data_union *)&p_layout->gps_ap2mcu[0];
	p_gps_mcu_hif_mcu2ap_region = (union gps_mcu_hif_mcu2ap_shared_data_union *)&p_layout->gps_mcu2ap[0];
	MDL_LOGI("ap2mcu: p=0x%p, offset=0x%x, size=0x%lx",
		p_gps_mcu_hif_ap2mcu_region,
		gps_mcudl_get_offset_from_conn_base(p_gps_mcu_hif_ap2mcu_region),
		sizeof(*p_gps_mcu_hif_ap2mcu_region));
	MDL_LOGI("mcu2ap: p=0x%p, offset=0x%x, size=0x%lx",
		p_gps_mcu_hif_mcu2ap_region,
		gps_mcudl_get_offset_from_conn_base(p_gps_mcu_hif_mcu2ap_region),
		sizeof(*p_gps_mcu_hif_mcu2ap_region));
#else
	p_gps_mcu_hif_ap2mcu_region = (union gps_mcu_hif_ap2mcu_shared_data_union *)0x8707A000;
	p_gps_mcu_hif_mcu2ap_region = (union gps_mcu_hif_mcu2ap_shared_data_union *)0x8707E000;
#endif
	memset(&g_gps_mcu_hif_recv_contexts, 0, sizeof(g_gps_mcu_hif_recv_contexts));
	gps_mcu_hif_host_inf_init();
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMALESS_MGMT);
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMA_NORMAL);
	gps_mcu_hif_host_init_ch(GPS_MCU_HIF_CH_DMA_URGENT);
}

void gps_mcu_hif_lock(void)
{
	gps_mcudl_slot_protect();
}

void gps_mcu_hif_unlock(void)
{
	gps_mcudl_slot_unprotect();
}

bool gps_mcu_hif_send_v2(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len,
	enum gps_mcu_hif_send_status *if_send_ok)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;
	enum gps_mcu_hif_trans trans_id;
	enum gps_mcu_hif_send_status *send_status = if_send_ok;
	int i;

	trans_id = gps_mcu_hif_get_ap2mcu_trans(hif_ch);
	if (gps_mcu_hif_is_trans_req_sent(trans_id)) {
		if (NULL == send_status) {
			MDL_LOGW("hif_ch=%d, len=%d, send fail due to last one not finished",
				hif_ch, data_len);
		} else {
			MDL_LOGD("hif_ch=%d, len=%d, send fail due to last one not finished",
				hif_ch, data_len);
			/* TODO: Register resend for fail */
			/* TODO: Print ccif status, mcu pc, emi status here */
			*send_status = GPS_MCU_HIF_SEND_FAIL_DUE_TO_NOT_FINSIHED;
		}
		return false;
	}
#if GPS_DL_HAS_MCUDL_HAL
	if (!gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_HIF_SEND)) {
		if (NULL == send_status) {
			MDL_LOGW("hif_ch=%d, len=%d, send fail due to clr_fw_own fail",
				hif_ch, data_len);
		} else {
			MDL_LOGD("hif_ch=%d, len=%d, send fail due to clr_fw_own fail",
				hif_ch, data_len);
			*send_status = GPS_MCU_HIF_SEND_FAIL_DUE_TO_FW_OWN_FAIL;
		}
		return false;
	}
	if (gps_mcudl_hal_ccif_tx_is_busy(GPS_MCUDL_CCIF_CH4)) {
		if (NULL == send_status) {
			MDL_LOGW("hif_ch=%d, len=%d, send fail due to ccif busy",
				hif_ch, data_len);
		} else {
			MDL_LOGD("hif_ch=%d, len=%d, send fail due to ccif busy",
				hif_ch, data_len);
			*send_status = GPS_MCU_HIF_SEND_FAIL_DUE_TO_CCIF_BUSY;
		}
		(void)gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_HIF_SEND);
		return false;
	}
	gps_mcudl_hal_ccif_tx_prepare(GPS_MCUDL_CCIF_CH4);
#endif
	gps_mcu_hif_host_clr_trans_req_sent(trans_id);
	p_buf = gps_mcu_hif_get_ap2mcu_emi_buf_addr(hif_ch);
	for (i = 0; i < data_len; i++)
		p_buf[i] = p_data[i];
	gps_mcu_hif_get_ap2mcu_trans_start_desc(hif_ch, &start_desc);
	start_desc.addr = gps_mcu_hif_convert_ap_addr2mcu_addr(p_buf);
	start_desc.len = data_len;
	start_desc.id++;
	gps_mcu_hif_set_ap2mcu_trans_start_desc(hif_ch, &start_desc);
	gps_mcu_hif_host_set_trans_req_sent(trans_id);
#if GPS_DL_HAS_MCUDL_HAL
	gps_mcudl_hal_ccif_tx_trigger(GPS_MCUDL_CCIF_CH4);
	(void)gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_HIF_SEND);
#endif
	return true;
}

bool gps_mcu_hif_send(enum gps_mcu_hif_ch hif_ch,
	const unsigned char *p_data, unsigned int data_len)
{
	return gps_mcu_hif_send_v2(hif_ch, p_data, data_len, NULL);
}

void gps_mcu_hif_recv_start(enum gps_mcu_hif_ch hif_ch)
{
	unsigned char *p_buf;
	struct gps_mcu_hif_trans_start_desc start_desc;
	enum gps_mcu_hif_trans trans_id;

	trans_id = gps_mcu_hif_get_mcu2ap_trans(hif_ch);
	if (gps_mcu_hif_is_trans_req_sent(trans_id)) {
		MDL_LOGW("hif_ch=%d, trans_id=%d, rx_ongoing", hif_ch, trans_id);
		return;
	}
	p_buf = gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch);

#if GPS_DL_HAS_MCUDL_HAL
	if (gps_mcudl_hal_ccif_tx_is_busy(GPS_MCUDL_CCIF_CH4)) {
		gps_mcu_hif_set_mcu2ap_recv_fail_flag(hif_ch, true);
		MDL_LOGW("hif_ch=%d, recv fail due to ccif busy, mcu2ap recv flag %d",
			hif_ch, gps_mcu_hif_get_mcu2ap_recv_fail_flag(hif_ch));
		return;
	}
	gps_mcudl_hal_ccif_tx_prepare(GPS_MCUDL_CCIF_CH4);
#endif
	gps_mcu_hif_get_mcu2ap_trans_start_desc(hif_ch, &start_desc);
	start_desc.addr = gps_mcu_hif_convert_ap_addr2mcu_addr(p_buf);
	start_desc.len = gps_mcu_hif_get_mcu2ap_emi_buf_len(hif_ch);
	start_desc.id++;
	gps_mcu_hif_set_mcu2ap_trans_start_desc(hif_ch, &start_desc);
	gps_mcu_hif_host_set_trans_req_sent(trans_id);
#if GPS_DL_HAS_MCUDL_HAL
	gps_mcudl_hal_ccif_tx_trigger(GPS_MCUDL_CCIF_CH4);
#endif
}

void gps_mcu_hif_recv_stop(enum gps_mcu_hif_ch hif_ch)
{
	enum gps_mcu_hif_trans trans_id;

	trans_id = gps_mcu_hif_get_mcu2ap_trans(hif_ch);
	if (!gps_mcu_hif_is_trans_req_sent(trans_id)) {
		MDL_LOGW("hif_ch=%d, trans_id=%d, no rx_ongoing", hif_ch, trans_id);
		return;
	}
	gps_mcu_hif_host_clr_trans_req_sent(trans_id);
}

void gps_mcu_hif_recv_listen_start(enum gps_mcu_hif_ch hif_ch, gps_mcu_hif_ch_on_recv_cb custom_cb)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	p_ctx->is_listening = true;
	p_ctx->custom_cb = custom_cb;
	gps_mcu_hif_recv_start(hif_ch);
}

void gps_mcu_hif_recv_listen_stop(enum gps_mcu_hif_ch hif_ch)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	p_ctx->is_listening = false;
	gps_mcu_hif_recv_stop(hif_ch);
	p_ctx->custom_cb = NULL;
}

void gps_mcu_hif_host_on_tx_finished(enum gps_mcu_hif_ch hif_ch, unsigned int data_len)
{
	unsigned char *p_data;

	p_data = gps_mcu_hif_get_ap2mcu_emi_buf_addr(hif_ch);

#if GPS_DL_ON_LINUX
	memset_io(p_data, 0x0, data_len);
#else
	memset(p_data, 0x0, data_len);
#endif
}

unsigned char gps_mcu_hif_on_recv_dispatcher_buf[GPS_MCU_HIF_EMI_BUF_SIZE];

void gps_mcu_hif_host_on_rx_finished(enum gps_mcu_hif_ch hif_ch, unsigned int data_len)
{
/* #define GPS_DL_DBG_MEM_CPY_MIPS */
	struct gps_mcu_hif_recv_ch_context *p_ctx;
	unsigned char *p_data;

#ifdef GPS_DL_DBG_MEM_CPY_MIPS
	unsigned long t1 = 0;
	unsigned long t2 = 0;
#endif

	p_data = gps_mcu_hif_get_mcu2ap_emi_buf_addr(hif_ch);
	p_ctx = &g_gps_mcu_hif_recv_contexts[hif_ch];
	if (!p_ctx->is_listening) {
#if GPS_DL_ON_LINUX
		memset_io(p_data, 0x0, data_len);
#else
		memset(p_data, 0x0, data_len);
#endif
		return;
	}

#ifdef GPS_DL_DBG_MEM_CPY_MIPS
	t1 = gps_dl_tick_get_us();
#endif

#if GPS_DL_ON_LINUX
	memcpy_fromio(&gps_mcu_hif_on_recv_dispatcher_buf[hif_ch], p_data, data_len);
	memset_io(p_data, 0x0, data_len);
#else
	memcpy(&gps_mcu_hif_on_recv_dispatcher_buf[hif_ch], p_data, data_len);
	memset(p_data, 0x0, data_len);
#endif

#ifdef GPS_DL_DBG_MEM_CPY_MIPS
	t2 = gps_dl_tick_get_us();
	MDL_LOGW("clr mem used %lu us, data_len=%u\n", t2-t1, data_len);
#endif

	gps_mcu_hif_recv_start(hif_ch);
	if (!p_ctx->custom_cb)
		return;

	(*p_ctx->custom_cb)(&gps_mcu_hif_on_recv_dispatcher_buf[hif_ch], data_len);
}

void gps_mcu_hif_host_trans_finished(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_trans_start_desc start_desc;
	struct gps_mcu_hif_trans_end_desc end_desc;
	struct gps_mcu_hif_trans_rec trans_rec;
	enum gps_mcu_hif_ch hif_ch;

	memset(&start_desc, 0, sizeof(start_desc));
	memset(&end_desc, 0, sizeof(end_desc));
	memset(&trans_rec, 0, sizeof(trans_rec));
	hif_ch = gps_mcu_hif_get_trans_hif_ch(trans_id);
	gps_mcu_hif_get_trans_start_desc(trans_id, &start_desc);
	gps_mcu_hif_get_trans_end_desc(trans_id, &end_desc);
	if (start_desc.id != end_desc.id) {
		/* bad one, change to LOGD due to it's normal and frequent */
		MDL_LOGD("ch=%d, trans_id=%d, desc_id=(%d, %d), mismatch",
			hif_ch, trans_id, start_desc.id, end_desc.id);
		return;
	}

	trans_rec.trans_id = trans_id;
	trans_rec.id = end_desc.id;
	trans_rec.len = end_desc.len;
	trans_rec.dticks = end_desc.dticks;
	trans_rec.host_us = gps_dl_tick_get_us();
	switch (trans_id) {
	case GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT:
		MDL_LOGD("tx_done, ch=%d, id=%d, len=%d, dt_32k=%d",
			hif_ch, end_desc.id, end_desc.len, end_desc.dticks);
		gps_mcu_hif_host_trans_hist_rec(&trans_rec);
		gps_mcu_hif_host_on_tx_finished(hif_ch, end_desc.len);
		gps_mcu_hif_host_clr_trans_req_sent(trans_id);
		break;

	case GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT:
		MDL_LOGD("rx_done, ch=%d, id=%d, len=%d, dt_32k=%d",
			hif_ch, end_desc.id, end_desc.len, end_desc.dticks);
		if (trans_id == GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL) {
			trans_rec.total_trans_last =
				gps_mcudl_mcu2ap_ydata_sta_get_recv_byte_cnt(GPS_MDLY_NORMAL);
		/* flow control for urgent channel not ready */
		#if 0
		} else if (trans_id == GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT) {
			trans_rec.total_trans_last =
				gps_mcudl_mcu2ap_ydata_sta_get_recv_byte_cnt(GPS_MDLY_URGENT);
		#endif
		}
		gps_mcu_hif_host_trans_hist_rec(&trans_rec);
		gps_mcu_hif_host_clr_trans_req_sent(trans_id);
		gps_mcu_hif_host_on_rx_finished(hif_ch, end_desc.len);
		break;

	default:
		break;
	}
}

void gps_mcu_hif_host_ccif_irq_handler_in_isr(void)
{
	enum gps_mcu_hif_trans trans_id;
	enum gps_mcu_hif_ch hif_ch;

	for (trans_id = 0; trans_id < GPS_MCU_HIF_TRANS_NUM; trans_id++) {
		if (!gps_mcu_hif_is_trans_req_sent(trans_id))
			continue;
		if (gps_mcu_hif_is_trans_req_received(trans_id))
			continue;
		if (!gps_mcu_hif_is_trans_req_finished(trans_id))
			continue;
		gps_mcu_hif_host_trans_finished(trans_id);
	}
	if (gps_mcudl_ap2mcu_get_write_fail_flag(GPS_MDLY_URGENT) == true) {
		gps_mcudl_ylink_event_send(GPS_MDLY_URGENT,
				GPS_MCUDL_YLINK_EVT_ID_SLOT_FLUSH_ON_RECV_STA);
	}

	for (hif_ch = GPS_MCU_HIF_CH_DMALESS_MGMT; hif_ch < GPS_MCU_HIF_CH_NUM; hif_ch++) {
		if (gps_mcu_hif_get_mcu2ap_recv_fail_flag(hif_ch)) {
			MDL_LOGW("ch %d hif_recv fail, retry", hif_ch);
			gps_mcu_hif_set_mcu2ap_recv_fail_flag(hif_ch, false);
			gps_mcu_hif_recv_start(hif_ch);
		}
	}
}


void gps_mcu_hif_host_dump_ch(enum gps_mcu_hif_ch hif_ch)
{
	enum gps_mcu_hif_trans ap2mcu, mcu2ap;

	ap2mcu = gps_mcu_hif_get_ap2mcu_trans(hif_ch);
	mcu2ap = gps_mcu_hif_get_mcu2ap_trans(hif_ch);
	gps_mcu_hif_host_dump_trans(ap2mcu);
	gps_mcu_hif_host_dump_trans(mcu2ap);
}

void gps_mcu_hif_host_dump_trans(enum gps_mcu_hif_trans trans_id)
{
	bool sent, recv, fin;
	struct gps_mcu_hif_trans_start_desc start_desc;
	struct gps_mcu_hif_trans_end_desc end_desc;

	if ((unsigned int)trans_id >= (unsigned int)GPS_MCU_HIF_TRANS_NUM) {
		MDL_LOGW("trans_id=%d, invalid", trans_id);
		return;
	}

	sent = gps_mcu_hif_is_trans_req_sent(trans_id);
	recv = gps_mcu_hif_is_trans_req_received(trans_id);
	fin = gps_mcu_hif_is_trans_req_finished(trans_id);

	memset(&start_desc, 0, sizeof(start_desc));
	memset(&end_desc, 0, sizeof(end_desc));
	gps_mcu_hif_get_trans_start_desc(trans_id, &start_desc);
	gps_mcu_hif_get_trans_end_desc(trans_id, &end_desc);

	MDL_LOGW("trans_id=%d, state=[%d,%d,%d], str=[%d,%d], end=[%d,%d,%d]",
		trans_id, sent, recv, fin,
		start_desc.id, start_desc.len,
		end_desc.id, end_desc.len, end_desc.dticks);
}


#define GPS_MCU_HIF_TRANS_REC_MAX (48)
struct gps_mcu_hif_trans_rec g_gps_mcu_hif_trans_rec_array[GPS_MCU_HIF_TRANS_REC_MAX];
unsigned long g_gps_mcu_hif_trans_rec_cnt;
bool g_gps_mcu_hif_trans_rec_in_dump;

void gps_mcu_hif_host_trans_hist_init(void)
{
	gps_mcudl_slot_protect();
	memset(&g_gps_mcu_hif_trans_rec_array[0], 0, sizeof(g_gps_mcu_hif_trans_rec_array));
	g_gps_mcu_hif_trans_rec_cnt = 0;
	g_gps_mcu_hif_trans_rec_in_dump = false;
	gps_mcudl_slot_unprotect();
}

void gps_mcu_hif_host_trans_hist_rec(struct gps_mcu_hif_trans_rec *p_rec)
{
	bool in_dump;
	unsigned long index;

	gps_mcudl_slot_protect();
	index = g_gps_mcu_hif_trans_rec_cnt;
	in_dump = g_gps_mcu_hif_trans_rec_in_dump;

	/* Skip record and dump immediately if im_dump */
	if (in_dump) {
		MDL_LOGW("in_dump=1, cnt=%lu, trans_id=%d, id=%u, len=%d, dt_32k=%d, ttrans_last=%lu",
			index, p_rec->trans_id, p_rec->id, p_rec->len, p_rec->dticks,
			p_rec->total_trans_last);
		gps_mcudl_slot_unprotect();
		return;
	}

	/* Add a record */
	index = (g_gps_mcu_hif_trans_rec_cnt % GPS_MCU_HIF_TRANS_REC_MAX);
	g_gps_mcu_hif_trans_rec_array[index] = *p_rec;
	g_gps_mcu_hif_trans_rec_cnt++;
	gps_mcudl_slot_unprotect();
}

void gps_mcu_hif_host_trans_hist_dump_rec(unsigned long index, struct gps_mcu_hif_trans_rec *p_rec)
{
	MDL_LOGW("i=%lu, trans_id=%d, id=%u, len=%d, dt_32k=%d, ttr_last=%lu, ts=%lu",
		index, p_rec->trans_id, p_rec->id, p_rec->len, p_rec->dticks,
		p_rec->total_trans_last, p_rec->host_us);
}

void gps_mcu_hif_host_trans_hist_dump(void)
{
	unsigned long index;

	gps_mcudl_slot_protect();
	index = g_gps_mcu_hif_trans_rec_cnt;
	if (g_gps_mcu_hif_trans_rec_in_dump || index == 0) {
		gps_mcudl_slot_unprotect();
		MDL_LOGW("in_dump=1, cnt=%lu, skip", index);
		return;
	}
	g_gps_mcu_hif_trans_rec_in_dump = true;
	gps_mcudl_slot_unprotect();

	if (g_gps_mcu_hif_trans_rec_cnt <= GPS_MCU_HIF_TRANS_REC_MAX) {
		for (index = 0; index < g_gps_mcu_hif_trans_rec_cnt; index++) {
			gps_mcu_hif_host_trans_hist_dump_rec(index,
				&g_gps_mcu_hif_trans_rec_array[index]);
		}
	} else {
		index = g_gps_mcu_hif_trans_rec_cnt - GPS_MCU_HIF_TRANS_REC_MAX + 1;
		for (; index < g_gps_mcu_hif_trans_rec_cnt; index++) {
			gps_mcu_hif_host_trans_hist_dump_rec(index,
				&g_gps_mcu_hif_trans_rec_array[index % GPS_MCU_HIF_TRANS_REC_MAX]);
		}
	}

	gps_mcudl_slot_protect();
	g_gps_mcu_hif_trans_rec_in_dump = false;
	gps_mcudl_slot_unprotect();
}

/*no spin_lock protection, only 1 thread call this api in GPS working*/
bool gps_mcu_hif_get_mcu2ap_recv_fail_flag(enum gps_mcu_hif_ch ch)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[ch];
	return p_ctx->fail_flag;
}

void gps_mcu_hif_set_mcu2ap_recv_fail_flag(enum gps_mcu_hif_ch ch, bool flag)
{
	struct gps_mcu_hif_recv_ch_context *p_ctx;

	p_ctx = &g_gps_mcu_hif_recv_contexts[ch];
	p_ctx->fail_flag = flag;
}

