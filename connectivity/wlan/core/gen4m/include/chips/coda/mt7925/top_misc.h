/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __TOP_MISC_REGS_H__
#define __TOP_MISC_REGS_H__
#include "hal_common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define TOP_MISC_BASE \
	0x70006000
#define TOP_MISC_DEBUG_TOP_SEL_ADDR \
	(TOP_MISC_BASE + 0x150)
#define TOP_MISC_DEBUG_TOP_RG_ADDR \
	(TOP_MISC_BASE + 0x154)
#define TOP_MISC_DEBUG_TOP_SEL_DEBUG_TOP_SEL_ADDR \
	TOP_MISC_DEBUG_TOP_SEL_ADDR
#define TOP_MISC_DEBUG_TOP_SEL_DEBUG_TOP_SEL_MASK \
	0xFFFFFFFF
#define TOP_MISC_DEBUG_TOP_SEL_DEBUG_TOP_SEL_SHFT \
	0
#define TOP_MISC_DEBUG_TOP_RG_DEBUG_TOP_RG_ADDR \
	TOP_MISC_DEBUG_TOP_RG_ADDR
#define TOP_MISC_DEBUG_TOP_RG_DEBUG_TOP_RG_MASK \
	0xFFFFFFFF
#define TOP_MISC_DEBUG_TOP_RG_DEBUG_TOP_RG_SHFT \
	0
#ifdef __cplusplus
}
#endif
#endif
