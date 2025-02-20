/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/printk.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>

#include "mtk_cam-seninf-pkvm.h"

#define PFX "mtk_cam-seninf-pkvm"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)

int seninf_pkvm_open_session(void)
{
	return SENINF_PKVM_RETURN_SUCCESS;
}

int seninf_pkvm_close_session(void)
{
	return SENINF_PKVM_RETURN_SUCCESS;
}

int seninf_pkvm_checkpipe(u64 SecInfo_addr)
{
	struct arm_smccc_res res;

	if (!SecInfo_addr)
		return SENINF_PKVM_RETURN_ERROR;

	LOG_INF("[%s] secInfo_addr 0x%llx", __func__, SecInfo_addr);
	arm_smccc_smc(MTK_SIP_KERNEL_ISP_CONTROL, SENINF_TEE_CMD_CHECKPIPE,
		(u32)SecInfo_addr, (u32)(SecInfo_addr >> 32), 0, 0, 0, 0, &res);

	return SENINF_PKVM_RETURN_SUCCESS;
}

int seninf_pkvm_free(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_KERNEL_ISP_CONTROL, SENINF_TEE_CMD_FREE,
		0, 0, 0, 0, 0, 0, &res);

	return SENINF_PKVM_RETURN_SUCCESS;
}
