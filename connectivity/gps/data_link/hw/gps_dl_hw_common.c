/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hw_priv_util.h"
#include "gps_dl_hw_common.h"
#include "gps_dl_name_list.h"
#include "gps_dl_hal.h"

const unsigned char gps_dl_irq_status_bit_shift_mt6989[GPS_DL_IRQ_NUM + 1] = {
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L1_IRQ_BUS_B_SHFT, /*has_data*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L1_IRQ_BUS_B_SHFT + 1, /*has_nodata*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L1_IRQ_BUS_B_SHFT + 2, /*mcub*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L5_IRQ_BUS_B_SHFT, /*has_data*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L5_IRQ_BUS_B_SHFT + 1, /*has_nodata*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_L5_IRQ_BUS_B_SHFT + 2, /*mcub*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_GPS_DMA_IRQ_B_SHFT, /*dma*/
#if GPS_DL_HAS_MCUDL
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_CCIF_BGF2AP_SW_IRQ_B_SHFT, /*ccif*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_CONN2AP_SW_IRQ_B_SHFT, /*conn2ap*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_BGFSYS_WDT_IRQ_B_SHFT, /*wdt*/
	CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_CONN_BGF_HIF_ON_HOST_INT_B_SHFT, /*hif_on*/
#endif
	0xFF, /*invalid*/
};

bool gps_dl_hw_is_gps_irq_triggerd(enum gps_dl_irq_index_enum irq_id)
{
	unsigned int irq_status_bit_shfit = 0, irq_status = 0;
	bool is_irq_real_triggerd = false;

	if (irq_id >= GPS_DL_IRQ_NUM)
		return false;

	if (GDL_HW_CONN_INFRA_VER_MT6989 == gps_dl_hal_get_conn_infra_ver())
		irq_status_bit_shfit = gps_dl_irq_status_bit_shift_mt6989[irq_id];
	else
		irq_status_bit_shfit = 0xff;
	if (irq_status_bit_shfit >= 32)
		return false;

	irq_status = GDL_HW_RD_CONN_INFRA_REG(CONN_DBG_CTL_IRQ_FOR_APMCU_STAT_ADDR);
	is_irq_real_triggerd = ((1UL << irq_status_bit_shfit) != (irq_status | 1UL << irq_status_bit_shfit));
	if (!is_irq_real_triggerd)
		GDL_LOGD("irq_status = %x, irq_id/irq_name: %d/%s, irq_status_bit_shfit = %d",
			irq_status, irq_id, gps_dl_irq_name(irq_id), irq_status_bit_shfit);

	return is_irq_real_triggerd;
}

