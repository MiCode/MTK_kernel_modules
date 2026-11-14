/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gps_dl_linux_reserved_mem_v2.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_emi_layout.h"

unsigned long gps_dl_get_conn_emi_phys_addr(void)
{
	return (unsigned long)g_gps_dl_conn_res_emi.host_phys_addr;
}

void *gps_dl_get_conn_emi_virt_addr(void)
{
	return (void *)g_gps_dl_conn_res_emi.host_virt_addr;
}

