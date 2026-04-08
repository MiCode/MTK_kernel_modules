/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"

/*#include "gps_dl_hw_api.h"*/
#include "gps/bgf_gps_dma.h"
#include "gps_dl_context.h"
#include "../gps_dl_hw_priv_util.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hw_dep_api.h"

void gps_dl_hw_dep_set_dma_start(enum gps_dl_hal_dma_ch_index channel,
	  struct gdl_hw_dma_transfer *p_transfer)
{
	unsigned int bus_addr_of_data_start;
	unsigned int bus_addr_of_buf_start;
	unsigned int gdl_ret;

	gdl_ret = gps_dl_emi_remap_phy_to_bus_addr(p_transfer->transfer_start_addr, &bus_addr_of_data_start);
	gdl_ret = gps_dl_emi_remap_phy_to_bus_addr(p_transfer->buf_start_addr, &bus_addr_of_buf_start);

	switch (channel) {
	case GPS_DL_DMA_LINK0_A2D:
		if (gps_dl_is_1byte_mode())
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA1_CON_ADDR, 0x00128014);
		else
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA1_CON_ADDR, 0x00128016);

		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA1_PGMADDR_PGMADDR_ADDR,
			bus_addr_of_data_start);
		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA1_WPTO_WPTO_ADDR,
			bus_addr_of_buf_start);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA1_WPPT_WPPT, p_transfer->len_to_wrap);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA1_COUNT_LEN, p_transfer->transfer_max_len);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA1_START_STR, 1);
		break;
	case GPS_DL_DMA_LINK0_D2A:
		if (gps_dl_is_1byte_mode())
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA2_CON_ADDR, 0x00078018);
		else
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA2_CON_ADDR, 0x0007801A);

		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA2_PGMADDR_PGMADDR_ADDR,
			bus_addr_of_data_start);
		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA2_WPTO_WPTO_ADDR,
			bus_addr_of_buf_start);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA2_WPPT_WPPT, p_transfer->len_to_wrap);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA2_COUNT_LEN, p_transfer->transfer_max_len);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA2_START_STR, 1);
		break;
	case GPS_DL_DMA_LINK1_A2D:
		if (gps_dl_is_1byte_mode())
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA3_CON_ADDR, 0x00328014);
		else
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA3_CON_ADDR, 0x00328016);

		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA3_PGMADDR_PGMADDR_ADDR,
			bus_addr_of_data_start);
		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA3_WPTO_WPTO_ADDR,
			bus_addr_of_buf_start);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA3_WPPT_WPPT, p_transfer->len_to_wrap);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA3_COUNT_LEN, p_transfer->transfer_max_len);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA3_START_STR, 1);
		break;
	case GPS_DL_DMA_LINK1_D2A:
		if (gps_dl_is_1byte_mode())
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA4_CON_ADDR, 0x00278018);
		else
			GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA4_CON_ADDR, 0x0027801A);

		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA4_PGMADDR_PGMADDR_ADDR,
			bus_addr_of_data_start);
		GDL_HW_WR_GPS_REG(BGF_GPS_DMA_DMA4_WPTO_WPTO_ADDR,
			bus_addr_of_buf_start);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA4_WPPT_WPPT, p_transfer->len_to_wrap);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA4_COUNT_LEN, p_transfer->transfer_max_len);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA4_START_STR, 1);
		break;
	default:
		return;
	}


}

void gps_dl_hw_dep_set_dma_stop(enum gps_dl_hal_dma_ch_index channel)
{
	/* Poll until DMA IDLE */
	switch (channel) {
	case GPS_DL_DMA_LINK0_A2D:
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA1_START_STR, 0);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA1_ACKINT_ACK, 1);
		break;
	case GPS_DL_DMA_LINK0_D2A:
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA2_START_STR, 0);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA2_ACKINT_ACK, 1);
		break;
	case GPS_DL_DMA_LINK1_A2D:
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA3_START_STR, 0);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA3_ACKINT_ACK, 1);
		break;
	case GPS_DL_DMA_LINK1_D2A:
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA4_START_STR, 0);
		GDL_HW_SET_GPS_ENTRY(BGF_GPS_DMA_DMA4_ACKINT_ACK, 1);
		break;
	default:
		return;
	}

}

bool gps_dl_hw_dep_get_dma_int_status(enum gps_dl_hal_dma_ch_index channel)
{
	 /* ASSERT(channel >= 0 && channel <= GPS_DL_DMA_CH_NUM); */
	switch (channel) {
	case GPS_DL_DMA_LINK0_A2D:
		return (bool)GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA1_INTSTA_INT);
	case GPS_DL_DMA_LINK0_D2A:
		return (bool)GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA2_INTSTA_INT);
	case GPS_DL_DMA_LINK1_A2D:
		return (bool)GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA3_INTSTA_INT);
	case GPS_DL_DMA_LINK1_D2A:
		return (bool)GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA4_INTSTA_INT);
	default:
		return false;
	}

}

void gps_dl_hw_dep_save_dma_status_struct(
	 enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	unsigned int offset =
		(BGF_GPS_DMA_DMA2_WPPT_ADDR - BGF_GPS_DMA_DMA1_WPPT_ADDR) * ch;

	p->wrap_count       = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_WPPT_ADDR + offset);
	p->wrap_to_addr     = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_WPTO_ADDR + offset);
	p->total_count      = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_COUNT_ADDR + offset);
	p->config           = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_CON_ADDR + offset);
	p->start_flag       = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_START_ADDR + offset);
	p->intr_flag        = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_INTSTA_ADDR + offset);
	p->left_count       = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_RLCT_ADDR + offset);
	p->curr_addr        = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_PGMADDR_ADDR + offset);
	p->state            = GDL_HW_RD_GPS_REG(BGF_GPS_DMA_DMA1_STATE_ADDR + offset);

}

void gps_dl_hw_dep_print_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	GDL_LOGW("dma ch %d, addr curr = 0x%08x, wrap = 0x%08x; str/int/sta = %d/%d/%d",
		ch, p->curr_addr, p->wrap_to_addr,
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_START_STR, p->start_flag),
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_INTSTA_INT, p->intr_flag),
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_STATE_STATE, p->state));

	GDL_LOGW("dma ch %d, count left/wrap/total = %d/%d/%d",
		ch, p->left_count, p->wrap_count, p->total_count);

	GDL_LOGW("dma ch %d, conf = 0x%08x, master = %d, b2w = %d, w2b = %d, size = %d",
		ch, p->config,
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_CON_MAS, p->config),
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_CON_B2W, p->config),
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_CON_W2B, p->config),
		GDL_HW_EXTRACT_ENTRY(BGF_GPS_DMA_DMA1_CON_SIZE, p->config));

}

unsigned int gps_dl_hw_dep_get_dma_left_len(enum gps_dl_hal_dma_ch_index channel)
{
	/* ASSERT(channel >= 0 && channel <= GPS_DL_DMA_CH_NUM); */
	switch (channel) {
	case GPS_DL_DMA_LINK0_A2D:
		return GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA1_RLCT_RLCT);
	case GPS_DL_DMA_LINK0_D2A:
		return GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA2_RLCT_RLCT);
	case GPS_DL_DMA_LINK1_A2D:
		return GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA3_RLCT_RLCT);
	case GPS_DL_DMA_LINK1_D2A:
		return GDL_HW_GET_GPS_ENTRY(BGF_GPS_DMA_DMA4_RLCT_RLCT);
	default:
		return 0;
	}

}
