/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * brief  Declaration of library functions
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
 */

#ifndef _GPS_H_
#define _GPS_H_

#include "wmt_core.h"
#include "wmt_dev.h"
#include "osal.h"
#include "mtk_wcn_consys_hw.h"

extern phys_addr_t gConEmiPhyBase;

extern int mtk_wcn_stpgps_drv_init(void);
extern void mtk_wcn_stpgps_drv_exit(void);
#ifdef CONFIG_MTK_GPS_EMI
extern int mtk_gps_emi_init(void);
extern void mtk_gps_emi_exit(void);
#endif
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
extern int mtk_gps_fw_log_init(void);
extern void mtk_gps_fw_log_exit(void);
void GPS_fwlog_ctrl(bool on);
#endif

#endif
