/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gps_mcudl_log.h"

#include "gps_mcudl_emi_layout.h"
#include "gps_mcudl_emi_layout_soc711x.h"
#include "gps_mcudl_emi_layout_soc721a.h"

#include "gps_dl_linux_reserved_mem_v2.h"


#if GPS_DL_HAS_MCUDL

const struct gps_mcudl_emi_region_item *g_gps_mcu_emi_region_list_ptr;
unsigned int g_gps_mcu_emi_layout_size;

void gps_mcudl_set_emi_layout(enum gps_mcudl_emi_layout_name layout)
{
	switch (layout) {
	case GDL_EMI_LAYOUT_SOC711X:
		g_gps_mcu_emi_region_list_ptr = &c_gps_mcudl_emi_region_list_soc711x[0];
		g_gps_mcu_emi_layout_size = c_gps_mcudl_emi_layout_size_soc711x;
		MDL_LOGI_INI("gps_emi_layout: soc711x, sz=0x%x",
			g_gps_mcu_emi_layout_size);
		return;

	case GDL_EMI_LAYOUT_SOC721A:
		g_gps_mcu_emi_region_list_ptr = &c_gps_mcudl_emi_region_list_soc721a[0];
		g_gps_mcu_emi_layout_size = c_gps_mcudl_emi_layout_size_soc721a;
		MDL_LOGI_INI("gps_emi_layout: soc721a, sz=0x%x",
			g_gps_mcu_emi_layout_size);
		return;

	default:
		MDL_LOGE_INI("gps_emi_layout: not set");
		return;
	}
}

unsigned int gps_mcudl_get_emi_layout_size(void)
{
	return g_gps_mcu_emi_layout_size;
}

void *gps_mcudl_get_emi_region_info(enum gps_mcudl_emi_region_id idx,
	struct gps_mcudl_emi_region_item *p_region)
{
	const struct gps_mcudl_emi_region_item *p_region_tmp;
	unsigned char *p_conn_emi;

	if ((unsigned int)idx >= (unsigned int)GDL_EMI_REGION_CNT)
		goto err;

	if (g_gps_mcu_emi_region_list_ptr == NULL)
		goto err;

	p_region_tmp = &g_gps_mcu_emi_region_list_ptr[idx];
	if (!p_region_tmp->valid)
		goto err;

	p_conn_emi = (unsigned char *)gps_dl_get_conn_emi_virt_addr();
	if (p_region) {
		p_region->offset = p_region_tmp->offset;
		p_region->length = p_region_tmp->length;
		p_region->valid = true;
	}
	return (void *)(p_conn_emi + p_region_tmp->offset);

err:
	if (p_region) {
		p_region->offset = 0;
		p_region->length = 0;
		p_region->valid = false;
	}
	return NULL;
}

void gps_mcudl_show_emi_layout(void)
{
	struct gps_mcudl_emi_region_item region;
	void *region_virt_addr;

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_MCU_BIN, &region);
	GDL_LOGI("mcu_bin   : 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_GPS_BIN, &region);
	GDL_LOGI("gps_bin   : 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_MNL_BIN, &region);
	GDL_LOGI("mnl_bin   : 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_LEGACY, &region);
	GDL_LOGI("gps_legacy: 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_NVEMI, &region);
	GDL_LOGI("gps_nv_emi: 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_NVEMI, &region);
	GDL_LOGI("gps_nv_emi: 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_AP2MCU, &region);
	GDL_LOGI("gps_ap2mcu: 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);

	region_virt_addr = gps_mcudl_get_emi_region_info(GDL_EMI_REGION_MCU2AP, &region);
	GDL_LOGI("gps_mcu2ap: 0x%p, offset=0x%06x, sz=0x%x",
		region_virt_addr, region.offset, region.length);
}

unsigned int gps_mcudl_get_offset_from_conn_base(void *p)
{
	void *p_base;

	p_base = gps_dl_get_conn_emi_virt_addr();
	return GDL_OFFSET(p, p_base);
}

#endif /* GPS_DL_HAS_MCUDL */

