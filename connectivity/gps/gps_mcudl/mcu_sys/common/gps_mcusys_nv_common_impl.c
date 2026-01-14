/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "gps_mcusys_fsm.h"
#include "gps_mcusys_nv_data.h"
#include "gps_mcusys_nv_data_api.h"
#include "gps_mcusys_nv_data_layout.h"
#include "gps_mcusys_nv_common_impl.h"
#include "gps_mcusys_nv_per_side_macro.h"

struct gps_mcusys_nv_data_sub_header *gps_mcusys_nv_common_get_local_sub_hdr(
	struct gps_mcusys_nv_data_header *p_hdr, bool is_on_mcu)
{
	struct gps_mcusys_nv_data_sub_header *p_target, *p_host;

	p_target = &p_hdr->hdr_target;
	p_host   = &p_hdr->hdr_host;

	return is_on_mcu ? p_target : p_host; /* local */
}

struct gps_mcusys_nv_data_sub_header *gps_mcusys_nv_common_get_remote_sub_hdr(
	struct gps_mcusys_nv_data_header *p_hdr, bool is_on_mcu)
{
	struct gps_mcusys_nv_data_sub_header *p_target, *p_host;

	p_target = &p_hdr->hdr_target;
	p_host   = &p_hdr->hdr_host;

	return is_on_mcu ? p_host : p_target; /* remote */
}

bool gps_mcusys_nv_common_check_header(
	enum gps_mcusys_nv_data_id nv_id)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;

	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return false;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, false);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, false);

	/*check magic*/
	if (p_local->magic == GPS_MCUSYS_NV_DATA_HEADER_MAGIC &&
		p_remote->magic == GPS_MCUSYS_NV_DATA_HEADER_MAGIC) {
		return true;
	}
	GPS_OFL_TRC("magic err, local/remote: %x/%x", p_local->magic, p_remote->magic);
	return false;
}

bool gps_mcusys_nv_common_shared_mem_take(
	enum gps_mcusys_nv_data_id nv_id, bool is_on_mcu)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;

	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return false;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, is_on_mcu);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, is_on_mcu);

	/*check header*/
	if (!gps_mcusys_nv_common_check_header(nv_id))
		return false;

	/* 1. check remote occupied*/
	if (p_remote->magic == GPS_MCUSYS_NV_DATA_HEADER_MAGIC &&
		p_remote->occupied != 0) {
		return false;
	}

	/* 2. set local occupied*/
	p_local->occupied = 1;

	/* 3. check remote occupied*/
	if (p_remote->magic == GPS_MCUSYS_NV_DATA_HEADER_MAGIC &&
		p_remote->occupied != 0) {
		p_local->occupied = 0;
		return false;
	}

	return true;
}

void gps_mcusys_nv_common_shared_mem_give(
	enum gps_mcusys_nv_data_id nv_id, bool is_on_mcu)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local;

	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, is_on_mcu);
	p_local->occupied = 0;
}

int gps_mcusys_nv_common_shared_mem_invalidate2(enum gps_mcusys_nv_data_id nv_id,
	gps_mcusys_nv_memset_pfn pfn_memset)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 block_size;
	gpsmdl_u32 old_local_size;
	gpsmdl_u32 write_times;
	gpsmdl_u8 *p_dst;
	bool take_okay;
	gpsmdl_u32 local_ver, remote_ver, new_ver;
	gpsmdl_u32 old_size;
	GPSMDL_PLAT_TICK_TYPE tick0, tick1;

	tick0 = GPSMDL_PLAT_TICK_GET();
	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return -1;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_dst = (gpsmdl_u8 *)&p_hdr->data_start[0];
	block_size = p_local->block_size;
	old_local_size = p_local->data_size;

	take_okay = gps_mcusys_nv_common_shared_mem_take(nv_id, NV_IS_ON_MCU);
	if (!take_okay) {
		GPS_OFL_TRC("nv_id=%d, block_size=%d, local_size=%d, take failed",
				   nv_id, block_size, old_local_size);
		return -1;
	}

	remote_ver = p_remote->version;
	local_ver = p_local->version;
	/* TODO: version may overflow 32bit */
	if (remote_ver > local_ver) {
		old_size = p_remote->data_size;
		new_ver = remote_ver + 1;
	} else {
		old_size = p_local->data_size;
		new_ver = local_ver + 1;
	}

	/*ALPS08204728, emi data may modified unexpectedly, check data if use*/
	if (block_size != (gps_mcusys_nv_data_get_block_size(nv_id) - sizeof(struct gps_mcusys_nv_data_header))) {
		GPS_OFL_TRC("block_size err, nv_id=%d, block_size=%d", nv_id, block_size);
		return -1;
	}
	if ((p_dst < (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_start_ptr()) ||
		(p_dst > (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_end_ptr())) {
		GPS_OFL_TRC("dst err, nv_id=%d, p_dst = %p/%p/%p",
			nv_id, p_dst,
			gps_mcudl_plat_nv_emi_get_start_ptr(),
			gps_mcudl_plat_nv_emi_get_end_ptr());
		return -1;
	}

	if (old_local_size > 0 && old_local_size <= block_size) {
		if (pfn_memset)
			(*pfn_memset)(p_dst, 0, old_local_size);
		else
			(void)memset(p_dst, 0, old_local_size);
	}

	p_local->data_size = 0;
	p_local->version = new_ver;
	write_times = ++p_local->write_times;
	gps_mcusys_nv_common_shared_mem_give(nv_id, NV_IS_ON_MCU);
	tick1 = GPSMDL_PLAT_TICK_GET();

	GPS_OFL_TRC("nv_id=%d, block_size=%d, wr_times=%d, ver=%d,%d->%d, old_size=%d,%d, dtick=%lu%s",
		nv_id, block_size, write_times,
		local_ver, remote_ver, new_ver, old_size, old_local_size,
		tick1 - tick0, GPSMDL_PLAT_TICK_UNIT_STR);
	return 0;
}

int gps_mcusys_nv_common_shared_mem_invalidate(enum gps_mcusys_nv_data_id nv_id)
{
	return gps_mcusys_nv_common_shared_mem_invalidate2(nv_id, NULL);
}

int gps_mcusys_nv_common_shared_mem_write2(enum gps_mcusys_nv_data_id nv_id,
	const gpsmdl_u8 *p_dat, gpsmdl_u32 dat_len, gpsmdl_u32 offset,
	gps_mcusys_nv_copy_pfn pfn_copy)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 _min, _max;
	gpsmdl_u32 block_size;
	gpsmdl_u32 write_times;
	gpsmdl_u8 *p_dst;
	bool take_okay;
	gpsmdl_u32 local_ver, remote_ver, new_ver;
	gpsmdl_u32 old_size, new_size;
	GPSMDL_PLAT_TICK_TYPE tick0, tick1;

	new_size = 0;
	tick0 = GPSMDL_PLAT_TICK_GET();
	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return 0;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);

	_min = offset;
	_max = offset + dat_len;
	block_size = p_local->block_size;

	if (_max >= block_size) {
		/* error: not write part of data */
		GPS_OFL_TRC("nv_id=%d, offset=%d, dat_len=%d, block_size=%d, out of boundary",
			nv_id, offset, dat_len, block_size);
		return 0;
	}

	if (_max <= _min) {
		/* error: _max overflow 32bit */
		GPS_OFL_TRC("nv_id=%d, offset=%d, dat_len=%d, block_size=%d, out of max u32",
			nv_id, offset, dat_len, block_size);
		return 0;
	}

	p_dst = (gpsmdl_u8 *)&p_hdr->data_start[0];
	take_okay = gps_mcusys_nv_common_shared_mem_take(nv_id, NV_IS_ON_MCU);
	if (!take_okay) {
		GPS_OFL_TRC("nv_id=%d, block_size=%d, dat_len=%d, take failed",
				   nv_id, block_size, dat_len);
		return -1;
	}

	remote_ver = p_remote->version;
	local_ver = p_local->version;
	/* TODO: version may overflow 32bit */
	if (remote_ver > local_ver) {
		old_size = p_remote->data_size;
		new_ver = remote_ver + 1;
	} else {
		old_size = p_local->data_size;
		new_ver = local_ver + 1;
	}

	/*ALPS08204728, emi data may modified unexpectedly, check data if use*/
	if (block_size != (gps_mcusys_nv_data_get_block_size(nv_id) - sizeof(struct gps_mcusys_nv_data_header))) {
		GPS_OFL_TRC("block_size err, nv_id=%d, block_size=%d", nv_id, block_size);
		return -1;
	}
	if ((p_dst < (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_start_ptr()) ||
		(p_dst > (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_end_ptr())) {
		GPS_OFL_TRC("dst err, nv_id=%d, p_dst = %p/%p/%p",
			nv_id, p_dst,
			gps_mcudl_plat_nv_emi_get_start_ptr(),
			gps_mcudl_plat_nv_emi_get_end_ptr());
		return -1;
	}

	if (pfn_copy) {
		/* TODO: may handle the return length of pfn_copy */
		(void)(*pfn_copy)(p_dst + _min, p_dat, dat_len);
	} else
		memcpy(p_dst + _min, p_dat, dat_len);
	if (old_size < _max) {
		p_local->data_size = _max;
		new_size = p_local->data_size;
	}
	p_local->version = new_ver;
	new_ver = p_local->version;
	write_times = ++p_local->write_times;
	gps_mcusys_nv_common_shared_mem_give(nv_id, NV_IS_ON_MCU);
	tick1 = GPSMDL_PLAT_TICK_GET();

	GPS_OFL_TRC(
		"nv_id=%d, offset=%d, block_size=%d, wr_times=%d, ver=%d,%d->%d, size=%d,%d->%d, dtick=%lu%s",
		nv_id, offset, block_size, write_times,
		local_ver, remote_ver, new_ver, old_size, _max, new_size,
		tick1 - tick0, GPSMDL_PLAT_TICK_UNIT_STR);
	return dat_len;
}

int gps_mcusys_nv_common_shared_mem_write(enum gps_mcusys_nv_data_id nv_id,
	const gpsmdl_u8 *p_dat, gpsmdl_u32 dat_len, gpsmdl_u32 offset)
{
	return gps_mcusys_nv_common_shared_mem_write2(nv_id, p_dat, dat_len, offset, NULL);
}


int gps_mcusys_nv_common_shared_mem_read2(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u8 *p_buf, gpsmdl_u32 buf_len, gpsmdl_u32 offset,
	gps_mcusys_nv_copy_pfn pfn_copy)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 _min, _max;
	gpsmdl_u32 data_size;
	gpsmdl_u32 read_times;
	gpsmdl_u32 read_len;
	gpsmdl_u8 *p_src;
	bool take_okay;
	gpsmdl_u32 local_ver, remote_ver, old_ver;
	gpsmdl_u32 remote_size, old_size;
	GPSMDL_PLAT_TICK_TYPE tick0, tick1;

	tick0 = GPSMDL_PLAT_TICK_GET();
	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return -1;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);

	_min = offset;
	_max = offset + buf_len;
	read_len = buf_len;

	take_okay = gps_mcusys_nv_common_shared_mem_take(nv_id, NV_IS_ON_MCU);
	if (!take_okay) {
		GPS_OFL_TRC("nv_id=%d, buf_len=%d, take failed",
			nv_id, buf_len);
		return -1;
	}

	remote_ver = p_remote->version;
	local_ver = p_local->version;
	data_size = p_local->data_size;
	if (remote_ver > local_ver) {
		remote_size = p_remote->data_size;
		p_local->version = remote_ver;
		p_local->data_size = remote_size;
		old_ver = local_ver;
		old_size = data_size;
		local_ver = p_local->version;
		data_size = p_local->data_size;
		GPS_OFL_TRC("nv_id=%d, ver: %d->%d, size: %d->%d",
			nv_id, old_ver, local_ver, old_size, data_size);
	}

	if (_min >= data_size)
		read_len = 0;
	else if (_max > data_size)
		read_len = buf_len - (_max - data_size);
	else
		read_len = buf_len;

	if (read_len > 0) {
		p_src = (gpsmdl_u8 *)&p_hdr->data_start[0];
		/*ALPS08204728, emi data may modified unexpectedly, check data if use*/
		if ((p_src < (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_start_ptr()) ||
			(p_src > (gpsmdl_u8 *)gps_mcudl_plat_nv_emi_get_end_ptr())) {
			GPS_OFL_TRC("p_src err, nv_id=%d, p_src = %p/%p/%p",
				nv_id, p_src,
				gps_mcudl_plat_nv_emi_get_start_ptr(),
				gps_mcudl_plat_nv_emi_get_end_ptr());
			return -1;
		}
		if (pfn_copy) {
			/* TODO: may handle the return length of pfn_copy */
			(void)(*pfn_copy)(p_buf, p_src + _min, read_len);
		} else
			memcpy(p_buf, p_src + _min, read_len);
		++p_local->read_times;
	}
	read_times = p_local->read_times;
	gps_mcusys_nv_common_shared_mem_give(nv_id, NV_IS_ON_MCU);
	tick1 = GPSMDL_PLAT_TICK_GET();

	GPS_OFL_TRC(
		"nv_id=%d, offset=%d, buf_len=%d, rd_times=%d, ver=%d,%d, size=%d,%d, dtick=%lu%s",
		nv_id, offset, buf_len, read_times,
		local_ver, remote_ver, data_size, read_len,
		tick1 - tick0, GPSMDL_PLAT_TICK_UNIT_STR);
	return read_len;
}

int gps_mcusys_nv_common_shared_mem_read(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u8 *p_buf, gpsmdl_u32 buf_len, gpsmdl_u32 offset)
{
	return gps_mcusys_nv_common_shared_mem_read2(nv_id, p_buf, buf_len, offset, NULL);
}

int gps_mcusys_nv_common_shared_mem_get_info(enum gps_mcusys_nv_data_id nv_id,
	gpsmdl_u32 *p_dat_len, gpsmdl_u32 *p_block_size)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 block_size;
	gpsmdl_u32 data_size;
	gpsmdl_u32 read_times;
	bool take_okay;
	gpsmdl_u32 local_ver, remote_ver, old_ver;
	gpsmdl_u32 remote_size, old_size;
	GPSMDL_PLAT_TICK_TYPE tick0, tick1;

	tick0 = GPSMDL_PLAT_TICK_GET();
	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return -1;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);

	take_okay = gps_mcusys_nv_common_shared_mem_take(nv_id, NV_IS_ON_MCU);
	if (!take_okay) {
		GPS_OFL_TRC("nv_id=%d, take failed", nv_id);
		return -1;
	}

	remote_ver = p_remote->version;
	local_ver = p_local->version;
	data_size = p_local->data_size;
	block_size = p_local->block_size;
	read_times = p_local->read_times;
	if (remote_ver > local_ver) {
		remote_size = p_remote->data_size;
		p_local->version = remote_ver;
		p_local->data_size = remote_size;
		old_ver = local_ver;
		old_size = data_size;
		local_ver = p_local->version;
		data_size = p_local->data_size;
		GPS_OFL_TRC("nv_id=%d, ver: %d->%d, size: %d->%d",
			nv_id, old_ver, local_ver, old_size, data_size);
	}
	gps_mcusys_nv_common_shared_mem_give(nv_id, NV_IS_ON_MCU);
	tick1 = GPSMDL_PLAT_TICK_GET();

	GPS_OFL_DBG(
		"nv_id=%d, block_size=%d, data_size=%d, rd_times=%d, ver=%d,%d, dtick=%lu%s",
		nv_id, block_size, data_size, read_times, local_ver, remote_ver,
		tick1 - tick0, GPSMDL_PLAT_TICK_UNIT_STR);

	if (p_dat_len)
		*p_dat_len = data_size;

	if (p_block_size)
		*p_block_size = block_size;

	return 0;
}

void gps_mcusys_nv_common_shared_mem_set_local_open(enum gps_mcusys_nv_data_id nv_id, bool is_open)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 old_local_flags, new_local_flags, remote_flags;

	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);

	old_local_flags = p_local->flags;
	if (is_open)
		p_local->flags = old_local_flags | (1UL<<0);
	else
		p_local->flags = old_local_flags & ~(1UL<<0);
	new_local_flags = p_local->flags;
	remote_flags = p_remote->flags;
	if ((remote_flags != 0) ||
		(is_open && (old_local_flags != 0)) ||
		(!is_open && (old_local_flags == 0)))
		GPS_OFL_TRC("nv_id=%d, flags: local=0x%x->0x%x, remote=0x%x",
			nv_id, old_local_flags, new_local_flags, remote_flags);
}

bool gps_mcusys_nv_common_shared_mem_get_remote_open(enum gps_mcusys_nv_data_id nv_id)
{
	struct gps_mcusys_nv_data_header *p_hdr;
	struct gps_mcusys_nv_data_sub_header *p_local, *p_remote;
	gpsmdl_u32 local_flags, remote_flags;

	p_hdr = gps_mcusys_nv_data_get_hdr(nv_id);
	if (!p_hdr) {
		GPS_OFL_TRC("nv_id=%d, p_hdr get failed", nv_id);
		return false;
	}

	p_local = gps_mcusys_nv_common_get_local_sub_hdr(p_hdr, NV_IS_ON_MCU);
	p_remote = gps_mcusys_nv_common_get_remote_sub_hdr(p_hdr, NV_IS_ON_MCU);
	local_flags = p_local->flags;
	remote_flags = p_remote->flags;
	GPS_OFL_TRC("nv_id=%d, flags: local=0x%x, remote=0x%x",
		nv_id, local_flags, remote_flags);
	return !!(remote_flags & (1UL<<0));
}

