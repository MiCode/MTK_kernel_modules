/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _GPS_DL_EMI_LAYOUT_H
#define _GPS_DL_EMI_LAYOUT_H

#define GDL_OFFSET(high, low) ((unsigned int)((unsigned char *)high - (unsigned char *)low))

extern struct gps_dl_iomem_addr_map_entry g_gps_dl_conn_res_emi;

unsigned long gps_dl_get_conn_emi_phys_addr(void);
void *gps_dl_get_conn_emi_virt_addr(void);

#endif /* _GPS_DL_EMI_LAYOUT_H */

