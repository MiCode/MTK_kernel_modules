/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"

/*#include "gps_dl_hw_api.h"*/
#include "gps/conn_mcu_dma.h"
#include "gps/conn_mcu_config.h"
#include "gps_dl_context.h"
#include "../gps_dl_hw_priv_util.h"
#include "gps_dl_hal_api.h"
#include "gps_dl_hw_dep_api.h"

#include "gps_dl_hw_atf.h"

void gps_dl_hw_dep_set_dma_start(enum gps_dl_hal_dma_ch_index channel,
	  struct gdl_hw_dma_transfer *p_transfer)
{
	unsigned int bus_addr_of_data_start;
	unsigned int bus_addr_of_buf_start;
	unsigned int bus_len_to_wrap;
	unsigned int bus_addr_of_data_sub_buf_offset;
	enum gps_dl_hal_dma_ch_bus_id bus_id = GPS_DL_DMA_BUS_ID_NUM;
	unsigned int bus_transfer_max_len;
	bool is_1byte;
	unsigned int gdl_ret;
	struct arm_smccc_res res;
	int ret;

	gdl_ret = gps_dl_emi_remap_phy_to_bus_addr(p_transfer->transfer_start_addr, &bus_addr_of_data_start);
	gdl_ret = gps_dl_emi_remap_phy_to_bus_addr(p_transfer->buf_start_addr, &bus_addr_of_buf_start);

	bus_addr_of_data_sub_buf_offset = bus_addr_of_data_start - bus_addr_of_buf_start;
	bus_len_to_wrap = p_transfer->len_to_wrap;
	bus_transfer_max_len = p_transfer->transfer_max_len;
	is_1byte = gps_dl_is_1byte_mode();

	switch (channel) {
	case GPS_DL_DMA_LINK0_A2D:
		bus_id = GPS_DL_DMA_BUS_ID0_A2D;
		break;
	case GPS_DL_DMA_LINK0_D2A:
		bus_id = GPS_DL_DMA_BUS_ID0_D2A;
		break;
	case GPS_DL_DMA_LINK1_A2D:
		bus_id = GPS_DL_DMA_BUS_ID1_A2D;
		break;
	case GPS_DL_DMA_LINK1_D2A:
		bus_id = GPS_DL_DMA_BUS_ID1_D2A;
		break;
	default:
		break;
	}

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_SET_DMA_START_OPID,
			channel, bus_addr_of_data_sub_buf_offset,
			bus_id, bus_len_to_wrap, bus_transfer_max_len, is_1byte, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_dma_stop(enum gps_dl_hal_dma_ch_index channel)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_SET_DMA_STOP_OPID,
			channel, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

bool gps_dl_hw_dep_get_dma_int_status(enum gps_dl_hal_dma_ch_index channel)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GET_DMA_INT_STATUS_OPID,
			channel, 0, 0, 0, 0, 0, &res);
	ret = (bool)res.a0;
	return ret;
}

void gps_dl_hw_dep_save_dma_status_struct(
	 enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	unsigned int offset =
		(CONN_MCU_DMA_DMA8_WPPT_ADDR - CONN_MCU_DMA_DMA7_WPPT_ADDR) * ch;

	p->wrap_count		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_WPPT_ADDR + offset);
	p->wrap_to_addr		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_WPTO_ADDR + offset);
	p->total_count		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_COUNT_ADDR + offset);
	p->config		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_CON_ADDR + offset);
	p->start_flag		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_START_ADDR + offset);
	p->intr_flag		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_INTSTA_ADDR + offset);
	p->left_count		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_RLCT_ADDR + offset);
	p->curr_addr		 = GDL_HW_RD_GPS_REG(CONN_MCU_DMA_DMA7_PGMADDR_ADDR + offset);
}

void gps_dl_hw_dep_print_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	GDL_LOGW("dma ch %d, addr curr = 0x%08x, wrap = 0x%08x; str/int = %d/%d",
		ch, p->curr_addr, p->wrap_to_addr,
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_START_STR, p->start_flag),
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_INTSTA_INT, p->intr_flag));

	GDL_LOGW("dma ch %d, count left/wrap/total = %d/%d/%d",
		ch, p->left_count, p->wrap_count, p->total_count);

	GDL_LOGW("dma ch %d, conf = 0x%08x, master = %d, b2w = %d, w2b = %d, size = %d",
		ch, p->config,
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_CON_MAS, p->config),
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_CON_B2W, p->config),
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_CON_W2B, p->config),
		GDL_HW_EXTRACT_ENTRY(CONN_MCU_DMA_DMA7_CON_SIZE, p->config));
}

unsigned int gps_dl_hw_dep_get_dma_left_len(enum gps_dl_hal_dma_ch_index channel)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GET_DMA_LEFT_LEN_OPID,
			channel, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

