/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DL_LINUX_RESERVED_MEM_V2_H
#define _GPS_DL_LINUX_RESERVED_MEM_V2_H

#include "gps_dl_config.h"

#if GPS_DL_CONN_EMI_MERGED
#if GPS_DL_HAS_MCUDL
#include "gps_mcudl_emi_layout.h"
#endif

int gps_dl_get_reserved_memory_from_conninfra_drv(void);
void gps_dl_reserved_mem_init_v2(void);
void gps_dl_reserved_mem_deinit_v2(void);
void gps_dl_reserved_mem_get_conn_range(unsigned int *p_min, unsigned int *p_max);
#endif

#endif /* _GPS_DL_LINUX_RESERVED_MEM_V2_H */

