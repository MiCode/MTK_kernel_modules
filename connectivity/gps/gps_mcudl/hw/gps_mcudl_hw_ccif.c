/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_hw_ccif.h"
#include "gps_mcudl_hw_dep_macro.h"
#include "gps_mcudl_hw_priv_util.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_log.h"

bool gps_mcudl_hw_ccif_is_tch_busy(enum gps_mcudl_ccif_ch ch)
{
	unsigned int bitmask;
	int i = 0;

	do {
		bitmask = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_BUSY_ADDR);
		if (!(bitmask & (1UL << ch))) {
			if (i >= 10)
				MDL_LOGW("wait loop = %d, okay", i);
			return false;
		}

		gps_dl_sleep_us(20, 40);
		i++;
	} while (i < 5000);
	MDL_LOGE("wait loop = %d, still busy", i);
	return true;
}

void gps_mcudl_hw_ccif_set_tch_busy(enum gps_mcudl_ccif_ch ch)
{
	/* Note:
	 * 1. Do not use read-modify-write
	 * 2. Writing 0 is no effect
	 * So, we should write the corresponding bit directly.
	 */
	GDL_HW_WR_CONN_INFRA_REG(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_BUSY_ADDR, 1UL << ch);
}

void gps_mcudl_hw_ccif_set_tch_start(enum gps_mcudl_ccif_ch ch)
{
	GDL_HW_SET_CONN_INFRA_ENTRY(AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_TCHNUM_TCHNUM, ch);
}

unsigned int gps_mcudl_hw_ccif_get_tch_busy_bitmask(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_BUSY_BUSY);
}

void gps_mcudl_hw_ccif_clr_tch_busy_bitmask(void)
{
	unsigned int bitmask;

	bitmask = GDL_HW_GET_CONN_INFRA_ENTRY(
		BGF2AP_CONN_INFRA_ON_CCIF4_BGF2AP_PCCIF_RCHNUM_RCHNUM);
	if (!bitmask)
		return;
	GDL_HW_WR_CONN_INFRA_REG(
		BGF2AP_CONN_INFRA_ON_CCIF4_BGF2AP_PCCIF_ACK_ADDR, bitmask);
}

void gps_mcudl_hw_ccif_clr_rch_busy_bitmask(void)
{
	unsigned int bitmask;

	bitmask = GDL_HW_GET_CONN_INFRA_ENTRY(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_RCHNUM_RCHNUM);
	if (!bitmask)
		return;
	GDL_HW_WR_CONN_INFRA_REG(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_ACK_ADDR, bitmask);
}

unsigned int gps_mcudl_hw_ccif_get_tch_start_bitmask(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_START_START);
}

unsigned int gps_mcudl_hw_ccif_get_rch_bitmask(void)
{
	return GDL_HW_GET_CONN_INFRA_ENTRY(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_RCHNUM_RCHNUM);
}

void gps_mcudl_hw_ccif_set_rch_ack(enum gps_mcudl_ccif_ch ch)
{
	GDL_HW_WR_CONN_INFRA_REG(
		AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_ACK_ADDR, 1UL << ch);
}

void gps_mcudl_hw_ccif_set_irq_mask(enum gpsmdl_ccif_misc_cr_group id, unsigned int mask)
{
	if (id == GMDL_CCIF_MISC0)
		GDL_HW_WR_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_IRQ0_MASK_ADDR, mask);
	else if (id == GMDL_CCIF_MISC1)
		GDL_HW_WR_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_IRQ1_MASK_ADDR, mask);
}

unsigned int gps_mcudl_hw_ccif_get_irq_mask(enum gpsmdl_ccif_misc_cr_group id)
{
	unsigned int val = 0;

	if (id == GMDL_CCIF_MISC0)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_IRQ0_MASK_ADDR);
	else if (id == GMDL_CCIF_MISC1)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_IRQ1_MASK_ADDR);
	return val;
}

void gps_mcudl_hw_ccif_set_dummy(enum gpsmdl_ccif_misc_cr_group id, unsigned int val)
{
	if (id == GMDL_CCIF_MISC0)
		GDL_HW_WR_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_DUMMY1_ADDR, val);
	else if (id == GMDL_CCIF_MISC1)
		GDL_HW_WR_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_DUMMY2_ADDR, val);
}

unsigned int gps_mcudl_hw_ccif_get_dummy(enum gpsmdl_ccif_misc_cr_group id)
{
	unsigned int val = 0;

	if (id == GMDL_CCIF_MISC0)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_DUMMY1_ADDR);
	else if (id == GMDL_CCIF_MISC1)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_AP2BGF_PCCIF_DUMMY2_ADDR);
	return val;
}

unsigned int gps_mcudl_hw_ccif_get_shadow(enum gpsmdl_ccif_misc_cr_group id)
{
	unsigned int val = 0;

	if (id == GMDL_CCIF_MISC0)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_BGF2AP_SHADOW1_ADDR);
	else if (id == GMDL_CCIF_MISC1)
		val = GDL_HW_RD_CONN_INFRA_REG(
			AP2BGF_CONN_INFRA_ON_CCIF4_BGF2AP_SHADOW2_ADDR);
	return val;
}
