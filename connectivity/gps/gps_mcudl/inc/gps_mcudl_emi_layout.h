/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_EMI_LAYOUT_H
#define _GPS_MCUDL_EMI_LAYOUT_H

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

void gps_mcudl_show_emi_layout(void);
unsigned int gps_mcudl_get_offset_from_conn_base(void *p);

#endif /* _GPS_MCUDL_EMI_LAYOUT_H */

