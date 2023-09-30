/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_CONTEXT_H
#define _GPS_MCUDL_CONTEXT_H

#include "gps_dl_config.h"
#include "gps_mcudl_data_intf_type.h"

#if GPS_DL_ON_LINUX
#include "gps_nv_each_device.h"
#include "gps_mcudl_each_device.h"
#endif
#include "gps_each_link.h"
#include "gps_mcudl_each_link.h"

struct gps_mcudl_ctx {
	int major;
	int minor;
#if GPS_DL_ON_LINUX
	struct gps_mcudl_each_device devices[GPS_MDLX_CH_NUM];
	struct gps_nv_each_device nv_devices[GPS_MCUSYS_NV_DATA_NUM];
#endif
	struct gps_mcudl_each_link links[GPS_MDLX_CH_NUM];
/*#if !GPS_DL_NO_USE_IRQ*/
	/*struct gps_each_irq irqs[GPS_DL_IRQ_NUM];*/
/*#endif*/
	/*struct gps_dl_remap_ctx remap_ctx;*/
};

struct gps_mcudl_each_device *gps_mcudl_device_get(enum gps_mcudl_xid x_id);
struct gps_mcudl_each_link *gps_mcudl_link_get(enum gps_mcudl_xid x_id);

#endif /* _GPS_MCUDL_CONTEXT_H */

