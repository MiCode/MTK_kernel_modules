/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_COMMON_TYPE_H
#define _GPS_MCUDL_COMMON_TYPE_H

#if 0 /* stdlib */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#else /* linux kernel */
#include <linux/types.h>
#include <linux/string.h>
#endif

/* Note: linux kernel check_patch.pl does not allow new typedef from basic types
 * such as int.
 */
typedef u8   gpsmdl_u8;
typedef u16  gpsmdl_u16;
typedef u32  gpsmdl_u32;
typedef u64  gpsmdl_u64;

enum gps_mcudl_yid {
	GPS_MDLY_NORMAL,
	GPS_MDLY_URGENT,
	GPS_MDLY_CH_NUM
};

#define GPS_MDL_X2Y(xid) \
	((xid == GPS_MDLX_AGENT) ? GPS_MDLY_URGENT : GPS_MDLY_NORMAL)

enum gps_mcudl_pkt_type {
#if 0
	GPS_MDLYPL_MCUCTRL = 0x20,
	GPS_MDLYPL_FLOWCTRL,
#endif
	GFNS_RSP_MCU_ACK_DN_PKT_STA = (0x30 + 5), /* 53 */
	GFNS_RSP_MCU_RST_UP_PKT_STA = (0x30 + 6), /* 54 */

	GFNS_REQ_MCU_RST_DN_PKT_STA = (0x76), /* 118 */
	GFNS_REQ_MCU_ACK_UP_PKT_STA = (0x7E), /* 126 */

	GPS_MDLYPL_MCUSYS = 0xA0,
	GPS_MDLYPL_MCUFN,
	GPS_MDLYPL_MNL,
	GPS_MDLYPL_AGENT,
	GPS_MDLYPL_NMEA,
	GPS_MDLYPL_GDLOG,
	GPS_MDLYPL_PMTK,
	GPS_MDLYPL_MEAS,
	GPS_MDLYPL_CORR,
	GPS_MDLYPL_DSP0,
	GPS_MDLYPL_DSP1,

	GPS_MDLYPL_AOLTS,
	GPS_MDLYPL_MPETS,
	GPS_MDLYPL_SCPTS,

	GPS_MDLYPL_MPELOG,
	GPS_MDLYPL_LPPM,

	GPS_MDLYPL_MAXID
};

struct gps_mdly_dn_flowctrl {
	gpsmdl_u32 size;
	gpsmdl_u32 host_recv_win_size;
	gpsmdl_u64 host_recv_total_bytes;
	gpsmdl_u64 host_recv_total_pkts;
};

struct gps_mdly_up_flowctrl {
	gpsmdl_u32 size;
	gpsmdl_u32 target_recv_win_size;
	gpsmdl_u64 target_recv_total_bytes;
	gpsmdl_u64 target_recv_total_pkts;
};

#endif /* _GPS_MCUDL_COMMON_TYPE_H */
