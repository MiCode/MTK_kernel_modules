/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#ifdef GPS_DL_ENABLE_MET
#include "gps/bg_gps_met_top.h"
#include "gps_dl_linux_reserved_mem.h"
#include "../../hw/gps_dl_hw_priv_util.h"

#include "gps_dl_hw_atf.h"

void gps_dl_hw_dep_set_emi_write_range(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_EMI_WRITE_RANGE,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_ringbuffer_mode(unsigned int mode)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_RINGBUFFER_MODE,
			mode, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_sampling_rate(unsigned int rate)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_SAMPLING_RATE,
			rate, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_mask_signal(unsigned int mask_signal)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_MASK_SIGNAL,
			mask_signal, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_edge_detection(unsigned int edge)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_EDGE_DETECTION,
			edge, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_event_signal(unsigned int event_signal)
{
	struct arm_smccc_res res;
	int ret;


	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_EVENT_SIGNAL,
			event_signal, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_set_event_select(unsigned int event_select)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_EVENT_SELECT,
			event_select, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_enable_met(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_ENABLE_MET,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

void gps_dl_hw_dep_disable_met(void)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_DISABLE_MET,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_dl_hw_dep_get_met_read_ptr_addr(void)
{
	struct arm_smccc_res res;
	unsigned int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_GET_MET_READ_PTR_ADDR,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;

	return ret;
}

unsigned int gps_dl_hw_dep_get_met_write_ptr_addr(void)
{
	struct arm_smccc_res res;
	unsigned int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_GET_MET_WRITE_PTR_ADDR,
			0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;

	return ret;
}

void gps_dl_hw_dep_set_timer_source(unsigned int timer_source)
{
	struct arm_smccc_res res;
	unsigned int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_DEP_SET_TIMER_SOURCE,
			timer_source, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
}

#endif /* GPS_DL_ENABLE_MET */

