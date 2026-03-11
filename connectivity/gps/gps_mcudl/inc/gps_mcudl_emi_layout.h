/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 - 2023 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EMI_LAYOUT_H
#define _GPS_MCUDL_EMI_LAYOUT_H

#include "gps_dl_config.h"

#if GPS_DL_HAS_MCUDL

#include "gps_dl_emi_layout.h"

/* move to gps_mcudl_emi_layout_soc721a.h */
#if 0
struct gps_mcudl_emi_layout {
	unsigned char mcu_bin[0x020000];    /*0xF000_0000 ~ 0xF002_0000, 128KB*/
	unsigned char gps_bin[0x032000];    /*0xF002_0000 ~ 0xF005_2000, 200KB*/
	unsigned char dsp_bin[0x080000];    /*0xF005_2000 ~ 0xF00D_2000, 512KB*/
	unsigned char mnl_bin[0x2EE000];    /*0xF00D_2000 ~ 0xF03C_0000, 3000KB*/
	unsigned char mpe_bin[0x0];         /*0xF03C_0000 ~ 0xF03C_0000, 0KB*/
	unsigned char mcu_ro_rsv[0x0];      /*0xF03C_0000 ~ 0xF03C_0000, 0KB*/

	unsigned char mcu_gps_rw[0x085000]; /*0xF03C_0000 ~ 0xF044_5000, 532KB*/
	unsigned char scp_batch[0x04B000];  /*0xF044_5000 ~ 0xF049_0000, 300KB*/
	unsigned char mcu_rw_rsv[0x0];      /*0xF049_0000 ~ 0xF049_0000, 0KB*/
	unsigned char mcu_share[0x040000];  /*0xF049_0000 ~ 0xF04D_0000, 256KB*/
	unsigned char gps_legacy[0x078000]; /*0xF04D_0000 ~ 0xF054_8000, 480KB*/
	unsigned char gps_nv_emi[0x0C0000]; /*0xF054_8000 ~ 0xF060_8000, 768KB*/
	unsigned char gps_ap2mcu[0x004000]; /*0xF060_8000 ~ 0xF060_C000, 16KB*/
	unsigned char gps_mcu2ap[0x004000]; /*0xF060_C000 ~ 0xF061_0000, 16KB*/
};
#endif

struct gps_mcudl_emi_region_item {
	unsigned int offset;
	unsigned int length;
	bool valid;
};

enum gps_mcudl_emi_region_id {
	GDL_EMI_REGION_MCU_BIN,
	GDL_EMI_REGION_GPS_BIN,
	GDL_EMI_REGION_DSP_BIN,
	GDL_EMI_REGION_MNL_BIN,

	GDL_EMI_REGION_LEGACY,
	GDL_EMI_REGION_NVEMI,
	GDL_EMI_REGION_AP2MCU,
	GDL_EMI_REGION_MCU2AP,

	GDL_EMI_REGION_CNT
};

enum gps_mcudl_emi_layout_name {
	GDL_EMI_LAYOUT_SOC711X,
	GDL_EMI_LAYOUT_SOC721A,

	GDL_EMI_LAYOUT_NUM
};

void gps_mcudl_set_emi_layout(enum gps_mcudl_emi_layout_name layout);
unsigned int gps_mcudl_get_emi_layout_size(void);

void *gps_mcudl_get_emi_region_info(enum gps_mcudl_emi_region_id idx,
	struct gps_mcudl_emi_region_item *p_region);

void gps_mcudl_show_emi_layout(void);
unsigned int gps_mcudl_get_offset_from_conn_base(void *p);

#endif /* GPS_DL_HAS_MCUDL */

#endif /* _GPS_MCUDL_EMI_LAYOUT_H */

