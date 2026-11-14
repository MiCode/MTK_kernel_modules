/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef CONNSYS_SMC_H
#define CONNSYS_SMC_H

#include <linux/soc/mediatek/mtk_sip_svc.h>

#define CONNSYS_SMC_CALL_VOID(OPID, x2, x3, x4, x5, x6, x7) \
struct arm_smccc_res res; \
arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, OPID, \
	      x2, x3, x4, x5, x6, x7, &res)

#define CONNSYS_SMC_CALL_RET(OPID, x2, x3, x4, x5, x6, x7, ret) \
struct arm_smccc_res res; \
arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, OPID, \
	      x2, x3, x4, x5, x6, x7, &res);  \
ret = res.a0;

enum conn_smc_opid {
	/* connac 1 smc ops */
	SMC_CONNSYS_EMI_SET_REMAPPING_REG_CONNV1_OPID = 100,
	SMC_CONNSYS_ADIE_CHIPID_DETECT_CONNV1_OPID = 101,
};

#endif	/* CONNSYS_SMC_H */
