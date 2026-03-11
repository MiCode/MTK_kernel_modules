/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _GPS_MCUSYS_NV_COMMON_IMPL_H
#define _GPS_MCUSYS_NV_COMMON_IMPL_H

#include "gps_mcusys_nv_data_layout.h"

struct gps_mcusys_nv_data_sub_header *gps_mcusys_nv_common_get_local_sub_hdr(
	struct gps_mcusys_nv_data_header *p_hdr, bool is_on_mcu);

struct gps_mcusys_nv_data_sub_header *gps_mcusys_nv_common_get_remote_sub_hdr(
	struct gps_mcusys_nv_data_header *p_hdr, bool is_on_mcu);

bool gps_mcusys_nv_common_check_header(
	enum gps_mcusys_nv_data_id nv_id);

bool gps_mcusys_nv_common_shared_mem_take(
	enum gps_mcusys_nv_data_id nv_id, bool is_on_mcu);

void gps_mcusys_nv_common_shared_mem_give(
	enum gps_mcusys_nv_data_id nv_id, bool is_on_mcu);


typedef void (*gps_mcusys_nv_memset_pfn)(gpsmdl_u8 *p_dst, gpsmdl_u8 dat_u8, gpsmdl_u32 len);

/* return non-negtive value stands for okay */
int gps_mcusys_nv_common_shared_mem_invalidate(enum gps_mcusys_nv_data_id nv_id);
int gps_mcusys_nv_common_shared_mem_invalidate2(enum gps_mcusys_nv_data_id nv_id,
	gps_mcusys_nv_memset_pfn pfn_memset);

typedef gpsmdl_u32 (*gps_mcusys_nv_copy_pfn)(gpsmdl_u8 *p_dst, const gpsmdl_u8 *p_src, gpsmdl_u32 len);

/* return positive value stands for okay */
int gps_mcusys_nv_common_shared_mem_write(enum gps_mcusys_nv_data_id nv_id,
	const gpsmdl_u8 *p_dat, gpsmdl_u32 dat_len, gpsmdl_u32 offset);
int gps_mcusys_nv_common_shared_mem_write2(enum gps_mcusys_nv_data_id nv_id,
	const gpsmdl_u8 *p_dat, gpsmdl_u32 dat_len, gpsmdl_u32 offset,
	gps_mcusys_nv_copy_pfn pfn_copy);

/* return non-negtive value stands for okay */
int gps_mcusys_nv_common_shared_mem_read(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u8 *p_buf, gpsmdl_u32 buf_len, gpsmdl_u32 offset);
int gps_mcusys_nv_common_shared_mem_read2(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u8 *p_buf, gpsmdl_u32 buf_len, gpsmdl_u32 offset,
	gps_mcusys_nv_copy_pfn pfn_copy);

/* return non-negtive value stands for okay */
int gps_mcusys_nv_common_shared_mem_get_info(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u32 *p_dat_len, gpsmdl_u32 *p_block_size);

void gps_mcusys_nv_common_shared_mem_set_local_open(enum gps_mcusys_nv_data_id nv_id, bool is_open);
bool gps_mcusys_nv_common_shared_mem_get_remote_open(enum gps_mcusys_nv_data_id nv_id);


#endif /* _GPS_MCUSYS_NV_COMMON_IMPL_H */

