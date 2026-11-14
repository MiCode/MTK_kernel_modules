/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#if GPS_DL_ON_LINUX
#include <asm/io.h>
#else
#include <string.h>
#endif
#include "gps_mcu_hif_shared_struct.h"
#include "gps_mcu_hif_api.h"

enum gps_mcu_hif_trans gps_mcu_hif_get_ap2mcu_trans(enum gps_mcu_hif_ch hif_ch)
{
	if (hif_ch == GPS_MCU_HIF_CH_DMALESS_MGMT)
		return GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT;
	else if (hif_ch == GPS_MCU_HIF_CH_DMA_NORMAL)
		return GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL;
	else if (hif_ch == GPS_MCU_HIF_CH_DMA_URGENT)
		return GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT;

	return GPS_MCU_HIF_TRANS_NUM;
}

enum gps_mcu_hif_trans gps_mcu_hif_get_mcu2ap_trans(enum gps_mcu_hif_ch hif_ch)
{
	if (hif_ch == GPS_MCU_HIF_CH_DMALESS_MGMT)
		return GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT;
	else if (hif_ch == GPS_MCU_HIF_CH_DMA_NORMAL)
		return GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL;
	else if (hif_ch == GPS_MCU_HIF_CH_DMA_URGENT)
		return GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT;

	return GPS_MCU_HIF_TRANS_NUM;
}

enum gps_mcu_hif_ch gps_mcu_hif_get_trans_hif_ch(enum gps_mcu_hif_trans trans_id)
{
	switch (trans_id) {
	case GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT:
		return GPS_MCU_HIF_CH_DMALESS_MGMT;
	case GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT:
		return GPS_MCU_HIF_CH_DMALESS_MGMT;
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL:
		return GPS_MCU_HIF_CH_DMA_NORMAL;
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL:
		return GPS_MCU_HIF_CH_DMA_NORMAL;
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT:
		return GPS_MCU_HIF_CH_DMA_URGENT;
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT:
		return GPS_MCU_HIF_CH_DMA_URGENT;
	default:
		break;
	}

	return GPS_MCU_HIF_CH_NUM;
}

bool gps_mcu_hif_is_trans_req_sent(enum gps_mcu_hif_trans trans_id)
{
	const struct gps_mcu_hif_ap2mcu_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return false;

	p_shared = gps_mcu_hif_get_ap2mcu_shared_data_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_sent_bitmask);
#else
	bitmask = p_shared->inf.inf.trans_req_sent_bitmask;
#endif
	gps_mcu_hif_unlock();
	return !!(bitmask & (1UL << trans_id));
}

bool gps_mcu_hif_is_trans_req_received(enum gps_mcu_hif_trans trans_id)
{
	const struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return false;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_received_bitmask);
#else
	bitmask = p_shared->inf.inf.trans_req_received_bitmask;
#endif
	gps_mcu_hif_unlock();
	return !!(bitmask & (1UL << trans_id));
}

bool gps_mcu_hif_is_trans_req_finished(enum gps_mcu_hif_trans trans_id)
{
	const struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return false;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_finished_bitmask);
#else
	bitmask = p_shared->inf.inf.trans_req_finished_bitmask;
#endif
	gps_mcu_hif_unlock();
	return !!(bitmask & (1UL << trans_id));
}

void gps_mcu_hif_host_set_trans_req_sent(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_ap2mcu_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_ap2mcu_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_sent_bitmask);
	bitmask |= (1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_sent_bitmask);
#else
	p_shared->inf.inf.trans_req_sent_bitmask |= (1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_host_clr_trans_req_sent(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_ap2mcu_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_ap2mcu_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_sent_bitmask);
	bitmask &= ~(1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_sent_bitmask);
#else
	p_shared->inf.inf.trans_req_sent_bitmask &= ~(1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_target_set_trans_req_received(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_received_bitmask);
	bitmask |= (1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_received_bitmask);
#else
	p_shared->inf.inf.trans_req_received_bitmask |= (1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_target_clr_trans_req_received(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_received_bitmask);
	bitmask &= ~(1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_received_bitmask);
#else
	p_shared->inf.inf.trans_req_received_bitmask &= ~(1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_target_set_trans_req_finished(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_finished_bitmask);
	bitmask |= (1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_finished_bitmask);
#else
	p_shared->inf.inf.trans_req_finished_bitmask |= (1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_target_clr_trans_req_finished(enum gps_mcu_hif_trans trans_id)
{
	struct gps_mcu_hif_mcu2ap_shared_data *p_shared;
	unsigned int bitmask;

	if (!GPS_MCU_HIF_TRANS_IS_VALID(trans_id))
		return;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr();
	gps_mcu_hif_lock();
#if GPS_DL_ON_LINUX
	bitmask = readl(&p_shared->inf.inf.trans_req_finished_bitmask);
	bitmask &= ~(1UL << trans_id);
	writel(bitmask, &p_shared->inf.inf.trans_req_finished_bitmask);
#else
	p_shared->inf.inf.trans_req_finished_bitmask &= ~(1UL << trans_id);
#endif
	gps_mcu_hif_unlock();
}

void gps_mcu_hif_host_inf_init(void)
{
	struct gps_mcu_hif_ap2mcu_shared_data *p_shared;

	p_shared = gps_mcu_hif_get_ap2mcu_shared_data_mut_ptr();
#if GPS_DL_ON_LINUX
	memset_io(&p_shared->inf, 0, sizeof(p_shared->inf));
	writel(0x11111111, &p_shared->inf.inf.pattern0);
#else
	memset(&p_shared->inf, 0, sizeof(p_shared->inf));
	p_shared->inf.inf.pattern0 = 0x11111111;
#endif
}

void gps_mcu_hif_target_inf_init(void)
{
	struct gps_mcu_hif_mcu2ap_shared_data *p_shared;

	p_shared = gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr();

#if GPS_DL_ON_LINUX
	memset_io(&p_shared->inf, 0, sizeof(p_shared->inf));
	writel(0x22222222, &p_shared->inf.inf.pattern0);
#else
	memset(&p_shared->inf, 0, sizeof(p_shared->inf));
	p_shared->inf.inf.pattern0 = 0x22222222;
#endif
}

