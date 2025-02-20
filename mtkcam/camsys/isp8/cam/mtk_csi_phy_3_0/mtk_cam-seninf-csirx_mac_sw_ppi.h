/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __CSIRX_MAC_SW_PPI_0_C_HEADER_H__
#define __CSIRX_MAC_SW_PPI_0_C_HEADER_H__

#define csirx_mac_csi2_pg_ctrl 0x0100
#define RG_CSI2_PG_CTRL_EN_SHIFT 0
#define RG_CSI2_PG_CTRL_EN_MASK (0x1 << 0)
#define RG_CSI2_PG_CDPHY_SEL_SHIFT 1
#define RG_CSI2_PG_CDPHY_SEL_MASK (0x1 << 1)
#define RG_CSI2_PG_SYNC_LANE0_DLY_SHIFT 8
#define RG_CSI2_PG_SYNC_LANE0_DLY_MASK (0x3 << 8)
#define RG_CSI2_PG_SYNC_LANE1_DLY_SHIFT 10
#define RG_CSI2_PG_SYNC_LANE1_DLY_MASK (0x3 << 10)
#define RG_CSI2_PG_SYNC_LANE2_DLY_SHIFT 12
#define RG_CSI2_PG_SYNC_LANE2_DLY_MASK (0x3 << 12)
#define RG_CSI2_PG_SYNC_LANE3_DLY_SHIFT 14
#define RG_CSI2_PG_SYNC_LANE3_DLY_MASK (0x3 << 14)
#define RG_CSI2_PG_SYNC_INIT_0_SHIFT 16
#define RG_CSI2_PG_SYNC_INIT_0_MASK (0x1 << 16)
#define RG_CSI2_PG_SYNC_DET_0_SHIFT 17
#define RG_CSI2_PG_SYNC_DET_0_MASK (0x1 << 17)
#define RG_CSI2_PG_SYNC_DATA_VALID_0_SHIFT 18
#define RG_CSI2_PG_SYNC_DATA_VALID_0_MASK (0x1 << 18)
#define RG_CSI2_PG_SYNC_INIT_1_SHIFT 20
#define RG_CSI2_PG_SYNC_INIT_1_MASK (0x1 << 20)
#define RG_CSI2_PG_SYNC_DET_1_SHIFT 21
#define RG_CSI2_PG_SYNC_DET_1_MASK (0x1 << 21)
#define RG_CSI2_PG_SYNC_DATA_VALID_1_SHIFT 22
#define RG_CSI2_PG_SYNC_DATA_VALID_1_MASK (0x1 << 22)
#define RG_CSI2_PG_SYNC_INIT_2_SHIFT 24
#define RG_CSI2_PG_SYNC_INIT_2_MASK (0x1 << 24)
#define RG_CSI2_PG_SYNC_DET_2_SHIFT 25
#define RG_CSI2_PG_SYNC_DET_2_MASK (0x1 << 25)
#define RG_CSI2_PG_SYNC_DATA_VALID_2_SHIFT 26
#define RG_CSI2_PG_SYNC_DATA_VALID_2_MASK (0x1 << 26)
#define RG_CSI2_PG_SYNC_INIT_3_SHIFT 28
#define RG_CSI2_PG_SYNC_INIT_3_MASK (0x1 << 28)
#define RG_CSI2_PG_SYNC_DET_3_SHIFT 29
#define RG_CSI2_PG_SYNC_DET_3_MASK (0x1 << 29)
#define RG_CSI2_PG_SYNC_DATA_VALID_3_SHIFT 30
#define RG_CSI2_PG_SYNC_DATA_VALID_3_MASK (0x1 << 30)

#define csirx_mac_csi2_pg_cphy_ctrl 0x0104
#define RG_CSI2_PG_CPHY_TYPE_0_SHIFT 0
#define RG_CSI2_PG_CPHY_TYPE_0_MASK (0x7 << 0)
#define RG_CSI2_PG_CPHY_HS_0_SHIFT 4
#define RG_CSI2_PG_CPHY_HS_0_MASK (0xf << 4)
#define RG_CSI2_PG_CPHY_TYPE_1_SHIFT 8
#define RG_CSI2_PG_CPHY_TYPE_1_MASK (0x7 << 8)
#define RG_CSI2_PG_CPHY_HS_1_SHIFT 12
#define RG_CSI2_PG_CPHY_HS_1_MASK (0xf << 12)
#define RG_CSI2_PG_CPHY_TYPE_2_SHIFT 16
#define RG_CSI2_PG_CPHY_TYPE_2_MASK (0x7 << 16)
#define RG_CSI2_PG_CPHY_HS_2_SHIFT 20
#define RG_CSI2_PG_CPHY_HS_2_MASK (0xf << 20)

#define csirx_mac_csi2_pg_data_low_0 0x0108
#define RG_CSI2_PG_DATA_0_L_SHIFT 0
#define RG_CSI2_PG_DATA_0_L_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_high_0 0x010c
#define RG_CSI2_PG_DATA_0_H_SHIFT 0
#define RG_CSI2_PG_DATA_0_H_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_low_1 0x0110
#define RG_CSI2_PG_DATA_1_L_SHIFT 0
#define RG_CSI2_PG_DATA_1_L_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_high_1 0x0114
#define RG_CSI2_PG_DATA_1_H_SHIFT 0
#define RG_CSI2_PG_DATA_1_H_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_low_2 0x0118
#define RG_CSI2_PG_DATA_2_L_SHIFT 0
#define RG_CSI2_PG_DATA_2_L_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_high_2 0x011c
#define RG_CSI2_PG_DATA_2_H_SHIFT 0
#define RG_CSI2_PG_DATA_2_H_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_low_3 0x0120
#define RG_CSI2_PG_DATA_3_L_SHIFT 0
#define RG_CSI2_PG_DATA_3_L_MASK (0xffffffff << 0)

#define csirx_mac_csi2_pg_data_high_3 0x0124
#define RG_CSI2_PG_DATA_3_H_SHIFT 0
#define RG_CSI2_PG_DATA_3_H_MASK (0xffffffff << 0)

#endif
