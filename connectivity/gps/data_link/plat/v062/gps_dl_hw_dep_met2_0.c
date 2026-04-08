/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#ifdef GPS_DL_ENABLE_MET
#include "gps/bg_gps_met_top.h"
#include "gps_dl_linux_reserved_mem.h"
#include "../../hw/gps_dl_hw_priv_util.h"


void gps_dl_hw_dep_set_emi_write_range(unsigned int bus_emi_met_phy_addr)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_START_ADDR_MET_START_ADDR, bus_emi_met_phy_addr);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_MET_END_ADDR_MET_END_ADDR,
		(bus_emi_met_phy_addr+GPS_MET_MEM_SIZE));
}

void gps_dl_hw_dep_set_ringbuffer_mode(unsigned int mode)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL1_MET_RING_BUF_MODE, mode);
}

void gps_dl_hw_dep_set_sampling_rate(unsigned int rate)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_OSC_CNT_TARGET, rate);
}

void gps_dl_hw_dep_set_mask_signal(unsigned int mask_signal)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_MASK_0_MET_EVENT_MASK_0, mask_signal);
}

void gps_dl_hw_dep_set_mask_signal2(unsigned int mask_signal)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_MASK_1_MET_EVENT_MASK_1, mask_signal);
}

void gps_dl_hw_dep_set_edge_detection(unsigned int edge)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EDGE_DET_0_MET_POSEDGE_DET_EN_0, edge);
}

void gps_dl_hw_dep_set_edge_detection2(unsigned int edge)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EDGE_DET_1_MET_POSEDGE_DET_EN_1, edge);
}

void gps_dl_hw_dep_set_edge_detection3(unsigned int edge)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EDGE_DET_2_MET_NEGEDGE_DET_EN_0, edge);
}

void gps_dl_hw_dep_set_edge_detection4(unsigned int edge)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EDGE_DET_3_MET_NEGEDGE_DET_EN_1, edge);
}

void gps_dl_hw_dep_set_event_signal(unsigned int event_signal)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_3_4b_sel,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_2_4b_sel,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_1_4b_sel,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_0_4b_sel,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_3,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_2,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_1,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_2_0,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_1,
		event_signal);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_EVENT_SEL_MET_EVENT_SEL_0,
		event_signal);
}

void gps_dl_hw_dep_set_event_select(unsigned int event_select)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_EVENT_SEL0_GPS_MET_GPS_EVENT_SEL, event_select);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_EVENT_SEL0_GPS_MET_AFE_SEL, 0X1);
}

void gps_dl_hw_dep_enable_met(void)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_CLK_EN, 1);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_RECORD_EN, 1);
}

void gps_dl_hw_dep_disable_met(void)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_CLK_EN, 0);
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_RECORD_EN, 0);
}

unsigned int gps_dl_hw_dep_get_met_read_ptr_addr(void)
{
	return gps_bus_to_host(BG_GPS_MET_TOP_V2_GPS_MET_TOP_MET_READ_PTR_ADDR);
}

unsigned int gps_dl_hw_dep_get_met_write_ptr_addr(void)
{
	return gps_bus_to_host(BG_GPS_MET_TOP_V2_GPS_MET_TOP_MET_WRITE_PTR_ADDR);
}

void gps_dl_hw_dep_set_timer_source(unsigned int timer_source)
{
	GDL_HW_SET_GPS_ENTRY(BG_GPS_MET_TOP_V2_GPS_MET_TOP_CTL0_MET_SMP_CLK_SRC, timer_source);
}

#endif /* GPS_DL_ENABLE_MET */

