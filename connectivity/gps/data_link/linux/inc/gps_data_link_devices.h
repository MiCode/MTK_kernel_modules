/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DATA_LINK_DEVICES_H
#define _GPS_DATA_LINK_DEVICES_H

#include "gps_dl_config.h"

int mtk_gps_data_link_devices_init(void);
void mtk_gps_data_link_devices_exit(void);
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
extern int mtk_gps_fw_log_init(void);
extern void mtk_gps_fw_log_exit(void);
/*void GPS_fwlog_ctrl(bool on);*/
#endif

#endif /* _GPS_DATA_LINK_DEVICES_H */

