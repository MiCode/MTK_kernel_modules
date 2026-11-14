/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_REGS_H__
#define __BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_REGS_H__

#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_BASE \
	(0x1803B000 + CONN_INFRA_REMAPPING_OFFSET)

#define BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_ADDR \
	(BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_BASE + 0x010)


#define BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_conn_infra_off2on_apb_bus__u_p_d_n9__way_en_ADDR \
	BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_ADDR
#define BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_conn_infra_off2on_apb_bus__u_p_d_n9__way_en_MASK \
	0x0000FFFF
#define BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_conn_infra_off2on_apb_bus__u_p_d_n9__way_en_SHFT \
	0

#ifdef __cplusplus
}
#endif

#endif
