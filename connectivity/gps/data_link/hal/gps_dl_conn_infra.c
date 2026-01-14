/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include "gps_dl_config.h"

#include "gps_dl_context.h"
#include "gps_dl_hw_ver.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_hal.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_time_tick.h"
#if GPS_DL_HAS_PLAT_DRV
#include "gps_dl_linux_reserved_mem.h"
#include "gps_dl_linux_reserved_mem_v2.h"
#endif
#if GPS_DL_HAS_MCUDL
#include "gps_mcudl_hw_mcu.h"
#endif

#define GPS_EMI_REMAP_BASE_MASK (0xFFFFF0000) /* start from 64KB boundary, get msb20 of 36bit */
#define GPS_EMI_REMAP_LENGTH    (64 * 1024 * 1024UL)
#define GPS_EMI_BUS_BASE        (0x78000000)
#define MCU_EMI_BUS_BASE        (0x70000000)

void gps_dl_emi_remap_set_gps_legacy(unsigned int min_addr, unsigned int max_addr)
{
	unsigned int aligned_addr = 0;
	unsigned int _20msb_of_36bit_phy_addr;

	/* TODO: addr may not use uint, due to addr might be 36bit and uint might be only 32bit */
	aligned_addr = (min_addr & GPS_EMI_REMAP_BASE_MASK);


	if (max_addr - aligned_addr > GPS_EMI_REMAP_LENGTH) {
		GDL_LOGE("min = 0x%09x, max = 0x%09x, base = 0x%09x, over range",
			min_addr, max_addr, aligned_addr);
	} else {
		GDL_LOGD("min = 0x%09x, max = 0x%09x, base = 0x%09x",
			min_addr, max_addr, aligned_addr);
	}

	_20msb_of_36bit_phy_addr = aligned_addr >> 16;
	GDL_LOGI("remap setting = 0x%08x", _20msb_of_36bit_phy_addr);
	gps_dl_hw_set_gps_emi_remapping(_20msb_of_36bit_phy_addr);
	gps_dl_remap_ctx_get()->gps_emi_phy_remap_base = aligned_addr;
}

#if GPS_DL_CONN_EMI_MERGED
void gps_dl_emi_remap_set_conn(unsigned int min_addr, unsigned int max_addr)
{
	unsigned int aligned_addr = 0;
	unsigned int _20msb_of_36bit_phy_addr;

	/* TODO: addr may not use uint, due to addr might be 36bit and uint might be only 32bit */
	aligned_addr = (min_addr & GPS_EMI_REMAP_BASE_MASK);


	if (max_addr - aligned_addr > GPS_EMI_REMAP_LENGTH) {
		GDL_LOGE("min = 0x%09x, max = 0x%09x, base = 0x%09x, over range",
			min_addr, max_addr, aligned_addr);
	} else {
		GDL_LOGD("min = 0x%09x, max = 0x%09x, base = 0x%09x",
			min_addr, max_addr, aligned_addr);
	}

	_20msb_of_36bit_phy_addr = aligned_addr >> 16;
	GDL_LOGI("remap setting = 0x%08x", _20msb_of_36bit_phy_addr);
#if GPS_DL_HAS_MCUDL
	gps_dl_hw_set_mcu_emi_remapping_tmp(_20msb_of_36bit_phy_addr);
#endif
	gps_dl_remap_ctx_get()->mcu_emi_phy_remap_base = aligned_addr;
}

void gps_dl_emi_remap_set_conn_mcu(unsigned int min_addr, unsigned int max_addr)
{
	unsigned int aligned_addr = 0;
	unsigned int _20msb_of_36bit_phy_addr;

	/* TODO: addr may not use uint, due to addr might be 36bit and uint might be only 32bit */
	aligned_addr = (min_addr & GPS_EMI_REMAP_BASE_MASK);


	if (max_addr - aligned_addr > GPS_EMI_REMAP_LENGTH) {
		GDL_LOGE("min = 0x%09x, max = 0x%09x, base = 0x%09x, over range",
			min_addr, max_addr, aligned_addr);
	} else {
		GDL_LOGD("min = 0x%09x, max = 0x%09x, base = 0x%09x",
			min_addr, max_addr, aligned_addr);
	}

	_20msb_of_36bit_phy_addr = aligned_addr >> 16;
	GDL_LOGI("remap setting = 0x%08x", _20msb_of_36bit_phy_addr);

	gps_dl_remap_ctx_get()->mcu_emi_phy_remap_base = aligned_addr;
}

#endif

enum GDL_RET_STATUS gps_dl_emi_remap_phy_to_bus_addr_inner(
	unsigned int phy_base, unsigned int phy_addr,
	unsigned int bus_base, unsigned int *bus_addr)
{
	if ((phy_addr >= phy_base) &&
		(phy_addr < (phy_base + GPS_EMI_REMAP_LENGTH))) {
		*bus_addr = bus_base + (phy_addr - phy_base);
		return GDL_OKAY;
	}

	GDL_LOGE("bus_addr error: phy_base=0x%x, phy_addr=0x%x, bus_base=0x%x",
		phy_base, phy_addr, bus_base);
	*bus_addr = 0;
	return GDL_FAIL;
}

enum GDL_RET_STATUS gps_dl_emi_remap_phy_to_bus_addr(unsigned int phy_addr, unsigned int *bus_addr)
{
	unsigned int phy_base;

#if GPS_DL_CONN_EMI_MERGED
	phy_base = gps_dl_remap_ctx_get()->mcu_emi_phy_remap_base;
	return gps_dl_emi_remap_phy_to_bus_addr_inner(phy_base, phy_addr, MCU_EMI_BUS_BASE, bus_addr);
#else
	phy_base = gps_dl_remap_ctx_get()->gps_emi_phy_remap_base;
	return gps_dl_emi_remap_phy_to_bus_addr_inner(phy_base, phy_addr, GPS_EMI_BUS_BASE, bus_addr);
#endif
}

void gps_dl_emi_remap_calc_and_set(void)
{
	enum gps_dl_link_id_enum  i;
	struct gps_each_link *p_link = NULL;

	unsigned int min_addr = 0xFFFFFFFF;
	unsigned int max_addr = 0;
	unsigned int tx_end;
	unsigned int rx_end;

#if GPS_DL_HAS_PLAT_DRV
	if (gps_dl_reserved_mem_is_ready()) {
		gps_dl_reserved_mem_show_info();
		gps_dl_reserved_mem_get_gps_legacy_range(&min_addr, &max_addr);
		gps_dl_emi_remap_set_gps_legacy(min_addr, max_addr);
#if GPS_DL_CONN_EMI_MERGED
		gps_dl_reserved_mem_get_conn_range(&min_addr, &max_addr);
		gps_dl_emi_remap_set_conn(min_addr, max_addr);
#endif
		return;
	}
#endif

	for (i = 0; i < GPS_DATA_LINK_NUM; i++) {
		p_link = gps_dl_link_get(i);

		min_addr = (p_link->rx_dma_buf.phy_addr < min_addr) ? p_link->rx_dma_buf.phy_addr : min_addr;
		min_addr = (p_link->tx_dma_buf.phy_addr < min_addr) ? p_link->tx_dma_buf.phy_addr : min_addr;

		rx_end = p_link->rx_dma_buf.phy_addr + p_link->rx_dma_buf.len;
		tx_end = p_link->tx_dma_buf.phy_addr + p_link->tx_dma_buf.len;

		max_addr = (rx_end > min_addr) ? rx_end : max_addr;
		max_addr = (tx_end > min_addr) ? tx_end : max_addr;
	}
	GDL_LOGD("cal from dma buffers: min = 0x%x, max = 0x%x", min_addr, max_addr);
	gps_dl_emi_remap_set_gps_legacy(min_addr, max_addr);
}


unsigned int g_gps_dl_hal_conn_infra_poll_ok_ver;

void gps_dl_hal_set_conn_infra_ver(unsigned int ver)
{
	g_gps_dl_hal_conn_infra_poll_ok_ver = ver;
}

unsigned int gps_dl_hal_get_conn_infra_ver(void)
{
	return g_gps_dl_hal_conn_infra_poll_ok_ver;
}

unsigned int g_gps_dl_hal_adie_ver;

void gps_dl_hal_set_adie_ver(unsigned int ver)
{
	g_gps_dl_hal_adie_ver = ver;
}

unsigned int gps_dl_hal_get_adie_ver(void)
{
	return g_gps_dl_hal_adie_ver;
}

bool gps_dl_hal_conn_infra_ver_is_mt6885(void)
{
	/* is_mt6885 valid after gps_dl_hw_gps_common_on */
	return (gps_dl_hal_get_conn_infra_ver() == GDL_HW_CONN_INFRA_VER_MT6885);
}

bool gps_dl_hal_conn_infra_ver_is_mt6893(void)
{
	/* is_mt6893 valid after gps_dl_hw_gps_common_on */
	return (gps_dl_hal_get_conn_infra_ver() == GDL_HW_CONN_INFRA_VER_MT6893);
}

struct gps_dl_gps_awake_status g_gps_dl_awake_status;
void gps_dl_hal_set_gps_awake_status(bool is_gps_awake)
{
	unsigned long curr_ms = gps_dl_tick_get_ms();

	g_gps_dl_awake_status.is_awake = is_gps_awake;
	g_gps_dl_awake_status.updated_ms = curr_ms;

}
void gps_dl_hal_get_gps_awake_status(struct gps_dl_gps_awake_status *p_awake)
{
	if (p_awake == NULL)
		return;

	*p_awake = g_gps_dl_awake_status;
}
