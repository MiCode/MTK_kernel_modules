/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __WF_TOP_CFG_REGS_H__
#define __WF_TOP_CFG_REGS_H__

#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WF_TOP_CFG_BASE \
	0x80020000

#define WF_TOP_CFG_IP_VERSION_ADDR \
	(WF_TOP_CFG_BASE + 0x10)


#define WF_TOP_CFG_IP_VERSION_IP_VERSION_ADDR \
	WF_TOP_CFG_IP_VERSION_ADDR
#define WF_TOP_CFG_IP_VERSION_IP_VERSION_MASK \
	0xFFFFFFFF
#define WF_TOP_CFG_IP_VERSION_IP_VERSION_SHFT \
	0

#ifdef __cplusplus
}
#endif

#endif
