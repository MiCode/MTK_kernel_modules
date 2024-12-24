/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_linux_reserved_mem.h"
#include "gps_dl_linux_reserved_mem_v2.h"
#if GPS_DL_HAS_CONNINFRA_DRV
#include "conninfra.h"
#endif

#if GPS_DL_CONN_EMI_MERGED
phys_addr_t gConnRsvMemPhyBase;
unsigned long long gConnRsvMemSize;
struct gps_dl_iomem_addr_map_entry g_gps_dl_conn_res_emi;

int gps_dl_get_reserved_memory_from_conninfra_drv(void)
{
	phys_addr_t emi_base = 0;
	unsigned int emi_size = 0;
#if GPS_DL_HAS_MCUDL
	unsigned int gps_legacy_emi_offset;
	phys_addr_t gps_legacy_emi_base;
	unsigned int gps_legacy_emi_size;
#endif

#if GPS_DL_HAS_CONNINFRA_DRV
	conninfra_get_emi_phy_addr(CONNSYS_EMI_FW, &emi_base, &emi_size);
#endif
	GDL_LOGI("conn_emi: base=0x%llx, size=0x%x", emi_base, emi_size);
	gConnRsvMemPhyBase = emi_base;
	gConnRsvMemSize = emi_size;

	/* gps legacy emi is part of conn emi when GPS_DL_CONN_EMI_MERGED */
#if GPS_DL_HAS_MCUDL
	gps_legacy_emi_offset = (unsigned int)(unsigned long)&(((struct gps_mcudl_emi_layout *)0)->gps_legacy[0]);
	gps_legacy_emi_size = sizeof(((struct gps_mcudl_emi_layout *)0)->gps_legacy);
	gps_legacy_emi_base = emi_base + gps_legacy_emi_offset;
	gGpsRsvMemPhyBase = gps_legacy_emi_base;
	gGpsRsvMemSize = gps_legacy_emi_size;
	GDL_LOGI("gps_legacy_emi: offset=0x%x, base=0x%llx, size=0x%x",
		gps_legacy_emi_offset, gps_legacy_emi_base, gps_legacy_emi_size);
#endif
	return 0;
}

void gps_dl_reserved_mem_init_v2(void)
{
	void __iomem *host_virt_addr = NULL;
	unsigned int min_size = sizeof(struct gps_mcudl_emi_layout);

	if (gConnRsvMemPhyBase == (phys_addr_t)NULL || gConnRsvMemSize < min_size) {
		GDL_LOGW_INI("res_mem: base = 0x%llx, size = 0x%llx, min_size = 0x%x, not enough",
			(unsigned long long)gConnRsvMemPhyBase,
			(unsigned long long)gConnRsvMemSize, min_size);
		/* Just show warning*/
		/* return;*/
	}

	host_virt_addr = ioremap(gConnRsvMemPhyBase, gConnRsvMemSize);

	if (host_virt_addr == NULL) {
		GDL_LOGE_INI("res_mem: base = 0x%llx, size = 0x%llx, ioremap fail",
			(unsigned long long)gConnRsvMemPhyBase, (unsigned long long)gConnRsvMemSize);
		return;
	}

	g_gps_dl_conn_res_emi.host_phys_addr = gConnRsvMemPhyBase;
	g_gps_dl_conn_res_emi.host_virt_addr = host_virt_addr;
	g_gps_dl_conn_res_emi.length = gConnRsvMemSize;

	GDL_LOGI_INI("phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x, min_size = 0x%x",
		g_gps_dl_conn_res_emi.host_phys_addr,
		g_gps_dl_conn_res_emi.host_virt_addr,
		g_gps_dl_conn_res_emi.length, min_size);
	gps_mcudl_show_emi_layout();
}

void gps_dl_reserved_mem_deinit_v2(void)
{
	GDL_LOGI_INI("phy_addr = 0x%08x, vir_addr = 0x%p, size = 0x%x",
		g_gps_dl_conn_res_emi.host_phys_addr,
		g_gps_dl_conn_res_emi.host_virt_addr,
		g_gps_dl_conn_res_emi.length);

	if (g_gps_dl_conn_res_emi.host_virt_addr != NULL)
		iounmap(g_gps_dl_conn_res_emi.host_virt_addr);
	else
		GDL_LOGW_INI("host_virt_addr already null, not do iounmap");

	g_gps_dl_conn_res_emi.host_phys_addr = (dma_addr_t)NULL;
	g_gps_dl_conn_res_emi.host_virt_addr = (void *)NULL;
	g_gps_dl_conn_res_emi.length = 0;
}

void gps_dl_reserved_mem_get_conn_range(unsigned int *p_min, unsigned int *p_max)
{
	*p_min = g_gps_dl_conn_res_emi.host_phys_addr;
	*p_max = g_gps_dl_conn_res_emi.host_phys_addr + g_gps_dl_conn_res_emi.length;
}

#if GPS_DL_HAS_MCUDL
struct gps_mcudl_emi_layout *gps_dl_get_conn_emi_layout_ptr(void)
{
	struct gps_mcudl_emi_layout *p_mem_vir;

	p_mem_vir = (struct gps_mcudl_emi_layout *)g_gps_dl_conn_res_emi.host_virt_addr;
	return p_mem_vir;
}

unsigned long gps_dl_get_conn_emi_phys_addr(void)
{
	return (unsigned long)g_gps_dl_conn_res_emi.host_phys_addr;
}

#define GDL_OFFSET(high, low) ((unsigned int)((unsigned char *)high - (unsigned char *)low))
void gps_mcudl_show_emi_layout(void)
{
	struct gps_mcudl_emi_layout *p;

	p = gps_dl_get_conn_emi_layout_ptr();
	GDL_LOGI("mcu_bin   : 0x%p, offset=0x%06x, sz=0x%lx",
		&p->mcu_bin[0], GDL_OFFSET(&p->mcu_bin[0], p), sizeof(p->mcu_bin));
	GDL_LOGI("gps_bin   : 0x%p, offset=0x%06x, sz=0x%lx",
		&p->gps_bin[0], GDL_OFFSET(&p->gps_bin[0], p), sizeof(p->gps_bin));
	GDL_LOGI("mnl_bin   : 0x%p, offset=0x%06x, sz=0x%lx",
		&p->mnl_bin[0], GDL_OFFSET(&p->mnl_bin[0], p), sizeof(p->mnl_bin));
	GDL_LOGI("gps_legacy: 0x%p, offset=0x%06x, sz=0x%lx",
		&p->gps_legacy[0], GDL_OFFSET(&p->gps_legacy[0], p), sizeof(p->gps_legacy));
	GDL_LOGI("gps_nv_emi: 0x%p, offset=0x%06x, sz=0x%lx",
		&p->gps_nv_emi[0], GDL_OFFSET(&p->gps_nv_emi[0], p), sizeof(p->gps_nv_emi));
	GDL_LOGI("gps_ap2mcu: 0x%p, offset=0x%06x, sz=0x%lx",
		&p->gps_ap2mcu[0], GDL_OFFSET(&p->gps_ap2mcu[0], p), sizeof(p->gps_ap2mcu));
	GDL_LOGI("gps_mcu2ap: 0x%p, offset=0x%06x, sz=0x%lx",
		&p->gps_mcu2ap[0], GDL_OFFSET(&p->gps_mcu2ap[0], p), sizeof(p->gps_mcu2ap));
}

unsigned int gps_mcudl_get_offset_from_conn_base(void *p)
{
	struct gps_mcudl_emi_layout *p_base;

	p_base = gps_dl_get_conn_emi_layout_ptr();
	return GDL_OFFSET(p, p_base);
}
#endif /* GPS_DL_HAS_MCUDL */

#endif /* GPS_DL_CONN_EMI_MERGED */

