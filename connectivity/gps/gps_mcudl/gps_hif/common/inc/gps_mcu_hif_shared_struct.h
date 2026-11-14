/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCU_HIF_SHARED_STRUCT_H
#define _GPS_MCU_HIF_SHARED_STRUCT_H

enum gps_mcu_hif_ch {
	GPS_MCU_HIF_CH_DMALESS_MGMT,
	GPS_MCU_HIF_CH_DMA_NORMAL,
	GPS_MCU_HIF_CH_DMA_URGENT,
	GPS_MCU_HIF_CH_NUM
};
#define GPS_MCU_HIF_CH_IS_VALID(ch) \
	((unsigned int)ch < (unsigned int)GPS_MCU_HIF_CH_NUM)

enum gps_mcu_hif_trans {
	GPS_MCU_HIF_TRANS_AP2MCU_DMALESS_MGMT,
	GPS_MCU_HIF_TRANS_MCU2AP_DMALESS_MGMT,

	GPS_MCU_HIF_TRANS_AP2MCU_DMA_NORMAL,
	GPS_MCU_HIF_TRANS_MCU2AP_DMA_NORMAL,

	GPS_MCU_HIF_TRANS_AP2MCU_DMA_URGENT,
	GPS_MCU_HIF_TRANS_MCU2AP_DMA_URGENT,

	GPS_MCU_HIF_TRANS_NUM
};
#define GPS_MCU_HIF_TRANS_IS_VALID(ch) \
	((unsigned int)ch < (unsigned int)GPS_MCU_HIF_TRANS_NUM)

struct gps_mcu_hif_trans_start_desc {
	unsigned int id;
	unsigned int addr;
	unsigned int len;
	unsigned int zero; /*reserved*/
};

struct gps_mcu_hif_trans_end_desc {
	unsigned int id;
	unsigned int len;
	unsigned int dticks;
	unsigned int zero; /*reserved*/
};

struct gps_mcu_hif_trans_rec {
	enum gps_mcu_hif_trans trans_id;
	unsigned int id;
	unsigned int len;
	unsigned int dticks;
	unsigned long total_trans_last;
	unsigned long host_us;
};

#define GPS_MCU_HIF_EMI_SHARED_STRUCT_VER 0x00000010 /* v0.10 */
#define GPS_MCU_HIF_EMI_BUF_SIZE (4*1024)
/**
 *  AP to MCU data struct definition
 */
#define GPS_MCU_HIF_AP2MCU_SHARED_DATA_SIZE (16*1024)

#define GPS_MCU_HIF_AP2MCU_INF_REGION_SIZE (64)
#define GPS_MCU_HIF_AP2MCU_BUF_REGION_SIZE (12*1024)
#define GPS_MCU_HIF_AP2MCU_RESERVED_REGION_SIZE (12*1024 - 64)

struct gps_mcu_hif_ap2mcu_inf {
	unsigned int pattern0;
	unsigned int trans_req_sent_bitmask;
	unsigned int zero1; /*reserved*/
	unsigned int zero2; /*reserved*/
	struct gps_mcu_hif_trans_start_desc ap2mcu_trans_start_desc[GPS_MCU_HIF_CH_NUM];
	struct gps_mcu_hif_trans_start_desc mcu2ap_trans_start_desc[GPS_MCU_HIF_CH_NUM];
};

union gps_mcu_hif_ap2mcu_inf_region {
	unsigned char reserved[GPS_MCU_HIF_AP2MCU_INF_REGION_SIZE];
	struct gps_mcu_hif_ap2mcu_inf inf;
};

union gps_mcu_hif_ap2mcu_buf_region {
	unsigned char reserved[GPS_MCU_HIF_AP2MCU_BUF_REGION_SIZE];
	unsigned char list[GPS_MCU_HIF_CH_NUM][GPS_MCU_HIF_EMI_BUF_SIZE];
};

struct gps_mcu_hif_ap2mcu_shared_data {
	union gps_mcu_hif_ap2mcu_inf_region inf;
	union gps_mcu_hif_ap2mcu_buf_region buf;
};

union gps_mcu_hif_ap2mcu_shared_data_union {
	unsigned char reserved[GPS_MCU_HIF_AP2MCU_SHARED_DATA_SIZE];
	struct gps_mcu_hif_ap2mcu_shared_data data;
};

/**
 *  MCU to AP data struct definition
 */
#define GPS_MCU_HIF_MCU2AP_SHARED_DATA_SIZE (16*1024)

#define GPS_MCU_HIF_MCU2AP_INF_REGION_SIZE (64)
#define GPS_MCU_HIF_MCU2AP_BUF_REGION_SIZE (12*1024)
#define GPS_MCU_HIF_MCU2AP_RESERVED_REGION_SIZE (12*1024 - 64)

struct gps_mcu_hif_mcu2ap_inf {
	unsigned int pattern0;
	unsigned int trans_req_received_bitmask;
	unsigned int trans_req_finished_bitmask;
	unsigned int zero1; /*reserved*/
	struct gps_mcu_hif_trans_end_desc ap2mcu_trans_end_desc[GPS_MCU_HIF_CH_NUM];
	struct gps_mcu_hif_trans_end_desc mcu2ap_trans_end_desc[GPS_MCU_HIF_CH_NUM];
};

union gps_mcu_hif_mcu2ap_inf_region {
	unsigned char reserved[GPS_MCU_HIF_MCU2AP_INF_REGION_SIZE];
	struct gps_mcu_hif_mcu2ap_inf inf;
};

union gps_mcu_hif_mcu2ap_buf_region {
	unsigned char reserved[GPS_MCU_HIF_MCU2AP_BUF_REGION_SIZE];
	unsigned char list[GPS_MCU_HIF_CH_NUM][GPS_MCU_HIF_EMI_BUF_SIZE];
};

struct gps_mcu_hif_mcu2ap_shared_data {
	union gps_mcu_hif_mcu2ap_inf_region inf;
	union gps_mcu_hif_mcu2ap_buf_region buf;
};

union gps_mcu_hif_mcu2ap_shared_data_union {
	unsigned char reserved[GPS_MCU_HIF_MCU2AP_SHARED_DATA_SIZE];
	struct gps_mcu_hif_mcu2ap_shared_data data;
};

/* APIs */
unsigned int    gps_mcu_hif_get_ap2mcu_sram_buf_len(enum gps_mcu_hif_ch ch);
unsigned char *gps_mcu_hif_get_ap2mcu_sram_buf_addr(enum gps_mcu_hif_ch ch);
unsigned int    gps_mcu_hif_get_mcu2ap_sram_buf_len(enum gps_mcu_hif_ch ch);
unsigned char *gps_mcu_hif_get_mcu2ap_sram_buf_addr(enum gps_mcu_hif_ch ch);

unsigned int    gps_mcu_hif_get_ap2mcu_emi_buf_len(enum gps_mcu_hif_ch ch);
unsigned char *gps_mcu_hif_get_ap2mcu_emi_buf_addr(enum gps_mcu_hif_ch ch);
unsigned int    gps_mcu_hif_get_mcu2ap_emi_buf_len(enum gps_mcu_hif_ch ch);
unsigned char *gps_mcu_hif_get_mcu2ap_emi_buf_addr(enum gps_mcu_hif_ch ch);


/* AP to MCU desc */
void gps_mcu_hif_get_ap2mcu_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc);
void gps_mcu_hif_get_ap2mcu_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_start_desc);

/* Only AP can set start desc */
void gps_mcu_hif_set_ap2mcu_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc);
/* Only MCU can set end desc */
void gps_mcu_hif_set_ap2mcu_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_start_desc);


/* MCU to AP desc */
void gps_mcu_hif_get_mcu2ap_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc);
void gps_mcu_hif_get_mcu2ap_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_start_desc);

/* Only AP can set start desc */
void gps_mcu_hif_set_mcu2ap_trans_start_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_start_desc *p_start_desc);
/* Only MCU can set edn desc */
void gps_mcu_hif_set_mcu2ap_trans_end_desc(enum gps_mcu_hif_ch ch,
	struct gps_mcu_hif_trans_end_desc *p_start_desc);


void gps_mcu_hif_get_trans_start_desc(enum gps_mcu_hif_trans trans_id,
	struct gps_mcu_hif_trans_start_desc *p_start_desc);

void gps_mcu_hif_get_trans_end_desc(enum gps_mcu_hif_trans trans_id,
	struct gps_mcu_hif_trans_end_desc *p_end_desc);


void gps_mcu_hif_host_inf_init(void);
void gps_mcu_hif_target_inf_init(void);

#endif /* _GPS_MCU_HIF_SHARED_STRUCT_H */

