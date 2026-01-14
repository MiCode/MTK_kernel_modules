/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
#include "gps_dl_config.h"
#include "gps_dl_log.h"
#include "gps_dl_iomem_dump.h"
#include "gps_dl_linux_plat_drv.h"

void gps_dl_iomem_dump(unsigned int dummy_addr, unsigned int dump_len)
{
	const struct gps_dl_iomem_addr_map_entry *p_iomem;
	unsigned int real_dump_len = 0, left_dump_len = 0;
	unsigned int offset = 0;
	unsigned int *p_u32;

	p_iomem = gps_dl_match_major_iomem(dummy_addr, &offset);
	if (p_iomem == NULL) {
		GDL_LOGE("dummy_addr=0x%x, dump_len=0x%x, invalid dump range",
			dummy_addr, dump_len);
		return;
	}

	/* force 4 bytes alignment */
	dummy_addr = (dummy_addr / 4) * 4;

	if (dump_len == 0)
		real_dump_len = 4;
	else
		real_dump_len = ((dump_len + 3) / 4) * 4;

	if (offset + real_dump_len > p_iomem->length)
		real_dump_len = p_iomem->length - offset;

	GDL_LOGW("dummy_addr=0x%x, dump_len=0x%x(0x%x), offset=0x%x, iomem:phys=0x%08x, len=0x%x, virt=0x%p",
		dummy_addr, dump_len, real_dump_len, offset,
		p_iomem->host_phys_addr, p_iomem->length, p_iomem->host_virt_addr);

	p_u32 = (unsigned int *)(offset + (unsigned char *)p_iomem->host_virt_addr);
	for (left_dump_len = real_dump_len; left_dump_len >= 16; left_dump_len -= 16) {
		GDL_LOGW("[0x%08x]: %08x %08x %08x %08x",
			dummy_addr, *p_u32, *(p_u32 + 1), *(p_u32 + 2), *(p_u32 + 3));
		p_u32 += 4;
		dummy_addr += 16;
	}
	if (left_dump_len >= 12) {
		GDL_LOGW("[0x%08x]: %08x %08x %08x",
			dummy_addr, *p_u32, *(p_u32 + 1), *(p_u32 + 2));
	} else if (left_dump_len >= 8) {
		GDL_LOGW("[0x%08x]: %08x %08x",
			dummy_addr, *p_u32, *(p_u32 + 1));
	} else if (left_dump_len >= 4) {
		GDL_LOGW("[0x%08x]: %08x",
			dummy_addr, *p_u32);
	}
}

