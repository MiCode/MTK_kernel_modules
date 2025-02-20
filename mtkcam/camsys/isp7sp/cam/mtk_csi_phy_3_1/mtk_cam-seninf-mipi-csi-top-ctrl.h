/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MIPI_CSI_TOP_CTRL_0_C_HEADER_H__
#define __MIPI_CSI_TOP_CTRL_0_C_HEADER_H__

#define CSI_CSR_TOP_CTRL 0x0000
#define CSR_P2P_FLUSH_M_SHIFT 0
#define CSR_P2P_FLUSH_M_MASK (0xf << 0)
#define CSR_P2P_ULTRA_M_SHIFT 4
#define CSR_P2P_ULTRA_M_MASK (0xf << 4)
#define CSR_P2P_THRES_QOS_SHIFT 8
#define CSR_P2P_THRES_QOS_MASK (0xf << 8)

#define CSI_CSR_TOP_CK_EN 0x0004
#define CSR_TOP_CK_EN_SHIFT 0
#define CSR_TOP_CK_EN_MASK (0xffffffff << 0)

#define CSI_CSR_TOP_SW_RESET_B 0x0008
#define CSR_TOP_SW_RESET_SHIFT 0
#define CSR_TOP_SW_RESET_MASK (0xffffffff << 0)

#define CSI_CSR_TOP_MUX_SEL 0x000c
#define CSR_CSI_TOP_MUX_SEL_SHIFT 0
#define CSR_CSI_TOP_MUX_SEL_MASK (0xffffffff << 0)

#define CSI_CSR_DBG_MUX_SEL 0x0010
#define CSI_CSI_DBG_MUX_SEL_SHIFT 0
#define CSI_CSI_DBG_MUX_SEL_MASK (0x1f << 0)
#define CSR_CSI_FM_MUX_SEL_SHIFT 8
#define CSR_CSI_FM_MUX_SEL_MASK (0x7 << 8)

#define RO_TOP_DBG_0 0x0020
#define RO_TOP_DBG_0_SHIFT 0
#define RO_TOP_DBG_0_MASK (0xffffffff << 0)

#define RO_TOP_DBG_1 0x0024
#define RO_TOP_DBG_1_SHIFT 0
#define RO_TOP_DBG_1_MASK (0xffffffff << 0)

#endif
