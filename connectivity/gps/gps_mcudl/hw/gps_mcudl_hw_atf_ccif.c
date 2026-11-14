/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_mcudl_hw_ccif.h"
#include "gps_mcudl_hw_dep_macro.h"
#include "gps_mcudl_hw_priv_util.h"
#include "gps_dl_time_tick.h"
#include "gps_mcudl_log.h"
#include "gps_dl_hw_atf.h"

unsigned int gps_mcudl_hw_ccif_is_tch_busy_atf(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_IS_TCH_BUSY_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

bool gps_mcudl_hw_ccif_is_tch_busy(enum gps_mcudl_ccif_ch ch)
{
	unsigned int bitmask;
	int i = 0;

	do {
		bitmask = gps_mcudl_hw_ccif_is_tch_busy_atf();
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
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_SET_TCH_BUSY_OPID,
			ch, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_mcudl_hw_ccif_set_tch_start(enum gps_mcudl_ccif_ch ch)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_SET_TCH_START_OPID,
			ch, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_mcudl_hw_ccif_get_tch_busy_bitmask(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_TCH_BUSY_BITMASK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

void gps_mcudl_hw_ccif_clr_tch_busy_bitmask(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_CLR_TCH_BUSY_BITMASK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_mcudl_hw_ccif_clr_rch_busy_bitmask(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_CLR_RCH_BUSY_BITMASK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_mcudl_hw_ccif_get_tch_start_bitmask(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_TCH_START_BITMASK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

unsigned int gps_mcudl_hw_ccif_get_rch_bitmask(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_RCH_BITMASK_OPID,
			0, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

void gps_mcudl_hw_ccif_set_rch_ack(enum gps_mcudl_ccif_ch ch)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_SET_RCH_ACK_OPID,
			ch, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_mcudl_hw_ccif_set_irq_mask(enum gpsmdl_ccif_misc_cr_group id, unsigned int mask)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_SET_IRQ_MASK_OPID,
			id, mask, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_mcudl_hw_ccif_get_irq_mask(enum gpsmdl_ccif_misc_cr_group id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_IRQ_MASK_OPID,
			id, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

void gps_mcudl_hw_ccif_set_dummy(enum gpsmdl_ccif_misc_cr_group id, unsigned int val)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_SET_DUMMY_OPID,
			id, val, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_mcudl_hw_ccif_get_dummy(enum gpsmdl_ccif_misc_cr_group id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_DUMMY_OPID,
			id, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

unsigned int gps_mcudl_hw_ccif_get_shadow(enum gpsmdl_ccif_misc_cr_group id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MCUDL_HW_CCIF_GET_SHADOW_OPID,
			id, 0, 0, 0, 0, 0, &res);
	ret = (unsigned int)res.a0;
	return ret;
}

