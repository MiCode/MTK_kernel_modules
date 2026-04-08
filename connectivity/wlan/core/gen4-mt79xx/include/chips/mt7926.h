/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file  mt7926.h
 *    \brief This file contains the info of MT7926
 */

#ifdef MT7926

#ifndef _MT7926_H
#define _MT7926_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/*TODO: To use correct ID after FPGA uses correct ID @20170927*/

#define CONNAC2X_TOP_HCR 0x70010200
#define CONNAC2X_TOP_HVR 0x70010204
#define CONNAC2X_TOP_FVR 0x88000004
#define CONNAC2x_CONN_CFG_ON_BASE	0x7C060000
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT         0

#define MT7926_UDMA_CONN_INFRA_STATUS_SEL   0x74000A4C
#define MT7926_UDMA_CONN_INFRA_STATUS_VAL   0xD8000000
#define MT7926_UDMA_CONN_INFRA_STATUS       0x74000A48

#define MT7926_CHIP_ID                 (0x7926)
#define MT7926_SW_SYNC0                CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR
#define MT7926_SW_SYNC0_RDY_OFFSET \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT
#define MT7926_PATCH_START_ADDR        (0x00900000)
#define MT7926_TOP_CFG_BASE			CONN_CFG_BASE
#define MT7926_TX_DESC_APPEND_LENGTH   32
#define MT7926_RX_DESC_LENGTH   24
#define MT7926_ARB_AC_MODE_ADDR (0x820E315C)

#define MT7926_A_DIE_VER_ADDR 0x70010020
#define MT7926_A_DIE_VER_BIT  BIT(7)

#define MT7926_BT_FW_VER_ADDR 0x7C812004
#define MT7926_BT_FW_VER_MASK 0xFF

/*------------------------------------------------------------------------------
 * MACRO for WTBL INFO GET
 *------------------------------------------------------------------------------
 */

#define MT7926_WIFI_LWTBL_BASE 0x820d4200
#define MT7926_WIFI_UWTBL_BASE 0x820c4094


/*------------------------------------------------------------------------------
 * MACRO for WFDMA INFO GET
 *------------------------------------------------------------------------------
 */
#define MT7926_WFDMA_COUNT 1

/*------------------------------------------------------------------------------
 * MACRO for debug MCU
 *------------------------------------------------------------------------------
 */
#define CURRENT_PC 0x3F
#define PC_LOG_IDX 0x20
#define PC_LOG_NUM 32


/*******************************************************************************
 *                         D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _MT7926_H */

#endif  /* MT7926 */
