/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#if GPS_DL_ON_LINUX
#include <asm/io.h>
#endif
#include "gps_mcu_hif_api.h"

const struct gps_mcu_hif_ap2mcu_shared_data *gps_mcu_hif_get_ap2mcu_shared_data_ptr(void)
{
	return &p_gps_mcu_hif_ap2mcu_region->data;
}

const struct gps_mcu_hif_mcu2ap_shared_data *gps_mcu_hif_get_mcu2ap_shared_data_ptr(void)
{
	return &p_gps_mcu_hif_mcu2ap_region->data;
}

struct gps_mcu_hif_ap2mcu_shared_data *gps_mcu_hif_get_ap2mcu_shared_data_mut_ptr(void)
{
	return &p_gps_mcu_hif_ap2mcu_region->data;
}

struct gps_mcu_hif_mcu2ap_shared_data *gps_mcu_hif_get_mcu2ap_shared_data_mut_ptr(void)
{
	return &p_gps_mcu_hif_mcu2ap_region->data;
}

/* EMI */
unsigned int gps_mcu_hif_get_ap2mcu_emi_buf_len(enum gps_mcu_hif_ch ch)
{
	return GPS_MCU_HIF_EMI_BUF_SIZE;
}

unsigned char *gps_mcu_hif_get_ap2mcu_emi_buf_addr(enum gps_mcu_hif_ch ch)
{
	return &p_gps_mcu_hif_ap2mcu_region->data.buf.list[ch][0];
}

unsigned int gps_mcu_hif_get_mcu2ap_emi_buf_len(enum gps_mcu_hif_ch ch)
{
	return GPS_MCU_HIF_EMI_BUF_SIZE;
}

unsigned char *gps_mcu_hif_get_mcu2ap_emi_buf_addr(enum gps_mcu_hif_ch ch)
{
	return &p_gps_mcu_hif_mcu2ap_region->data.buf.list[ch][0];
}

/* AP to MCU desc */
void gps_mcu_hif_get_ap2mcu_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_fromio(p_start_desc,
		&p_gps_mcu_hif_ap2mcu_region->data.inf.inf.ap2mcu_trans_start_desc[ch],
		sizeof(*p_start_desc));
#else
	*p_start_desc = p_gps_mcu_hif_ap2mcu_region->data.inf.inf.ap2mcu_trans_start_desc[ch];
#endif
}

void gps_mcu_hif_get_ap2mcu_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_end_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_fromio(p_end_desc,
		&p_gps_mcu_hif_mcu2ap_region->data.inf.inf.ap2mcu_trans_end_desc[ch],
		sizeof(*p_end_desc));
#else
	*p_end_desc = p_gps_mcu_hif_mcu2ap_region->data.inf.inf.ap2mcu_trans_end_desc[ch];
#endif
}

void gps_mcu_hif_set_ap2mcu_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_toio(&p_gps_mcu_hif_ap2mcu_region->data.inf.inf.ap2mcu_trans_start_desc[ch],
		p_start_desc, sizeof(*p_start_desc));
#else
	p_gps_mcu_hif_ap2mcu_region->data.inf.inf.ap2mcu_trans_start_desc[ch] = *p_start_desc;
#endif
}

void gps_mcu_hif_set_ap2mcu_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_end_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_toio(&p_gps_mcu_hif_mcu2ap_region->data.inf.inf.ap2mcu_trans_end_desc[ch],
		p_end_desc, sizeof(*p_end_desc));
#else
	p_gps_mcu_hif_mcu2ap_region->data.inf.inf.ap2mcu_trans_end_desc[ch] = *p_end_desc;
#endif
}

/* MCU to AP desc */
void gps_mcu_hif_get_mcu2ap_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_fromio(p_start_desc,
		&p_gps_mcu_hif_ap2mcu_region->data.inf.inf.mcu2ap_trans_start_desc[ch],
		sizeof(*p_start_desc));
#else
	*p_start_desc = p_gps_mcu_hif_ap2mcu_region->data.inf.inf.mcu2ap_trans_start_desc[ch];
#endif
}

void gps_mcu_hif_get_mcu2ap_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_end_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_fromio(p_end_desc,
		&p_gps_mcu_hif_mcu2ap_region->data.inf.inf.mcu2ap_trans_end_desc[ch],
		sizeof(*p_end_desc));
#else
	*p_end_desc = p_gps_mcu_hif_mcu2ap_region->data.inf.inf.mcu2ap_trans_end_desc[ch];
#endif
}

void gps_mcu_hif_set_mcu2ap_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_toio(&p_gps_mcu_hif_ap2mcu_region->data.inf.inf.mcu2ap_trans_start_desc[ch],
		p_start_desc, sizeof(*p_start_desc));
#else
	p_gps_mcu_hif_ap2mcu_region->data.inf.inf.mcu2ap_trans_start_desc[ch] = *p_start_desc;
#endif
}

void gps_mcu_hif_set_mcu2ap_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_end_desc)
{
#if GPS_DL_ON_LINUX
	memcpy_toio(&p_gps_mcu_hif_mcu2ap_region->data.inf.inf.mcu2ap_trans_end_desc[ch],
		p_end_desc, sizeof(*p_end_desc));
#else
	p_gps_mcu_hif_mcu2ap_region->data.inf.inf.mcu2ap_trans_end_desc[ch] = *p_end_desc;
#endif
}

void gps_mcu_hif_get_trans_start_desc(enum gps_mcu_hif_trans trans_id,
	struct gps_mcu_hif_trans_start_desc *p_start_desc)
{
	enum gps_mcu_hif_ch hif_ch;

	hif_ch = gps_mcu_hif_get_trans_hif_ch(trans_id);
	switch (trans_id) {
	case GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT:
		gps_mcu_hif_get_ap2mcu_trans_start_desc(hif_ch, p_start_desc);
		break;
	case GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT:
		gps_mcu_hif_get_mcu2ap_trans_start_desc(hif_ch, p_start_desc);
		break;
	default:
		/* ASSERT(0);*/
		break;
	}
}

void gps_mcu_hif_get_trans_end_desc(enum gps_mcu_hif_trans trans_id,
	struct gps_mcu_hif_trans_end_desc *p_end_desc)
{
	enum gps_mcu_hif_ch hif_ch;

	hif_ch = gps_mcu_hif_get_trans_hif_ch(trans_id);
	switch (trans_id) {
	case GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT:
		gps_mcu_hif_get_ap2mcu_trans_end_desc(hif_ch, p_end_desc);
		break;
	case GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL:
	case GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT:
		gps_mcu_hif_get_mcu2ap_trans_end_desc(hif_ch, p_end_desc);
		break;
	default:
		/* ASSERT(0);*/
		break;
	}
}
