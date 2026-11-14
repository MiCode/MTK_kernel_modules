/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  mt7961.h
*    \brief This file contains the info of MT7961
*/

#if defined(MT7961) || defined(MT7922) || defined(MT7902)

#ifndef _MT7961_H
#define _MT7961_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/*TODO: To use correct ID after FPGA uses correct ID @20170927*/

#define CONNAC2X_TOP_HCR 0x70010200
#define CONNAC2X_TOP_HVR 0x70010204
#define CONNAC2X_TOP_FVR 0x88000004
#define CONNAC2x_CONN_CFG_ON_BASE		0x7C060000
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0

#define MT7961_CHIP_ID				(0x7961)
#define MT7961_SW_SYNC0 \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR
#define MT7961_SW_SYNC0_RDY_OFFSET \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT
#define MT7961_PATCH_START_ADDR			(0x00900000)
#define MT7961_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define MT7961_TX_DESC_APPEND_LENGTH		32
#define MT7961_RX_DESC_LENGTH			24
#define MT7961_ARB_AC_MODE_ADDR			(0x820E315C)

#define MT7961_A_DIE_VER_ADDR			0x70010020
#define MT7961_A_DIE_VER_BIT			BIT(7)
#define MT7961_A_DIE_7921			0
#define MT7961_A_DIE_7920			BIT(7)
#define MT7961_A_DIE_7921_FLAVOR		0x1
#define MT7961_A_DIE_7920_FLAVOR		0x1a

#define MT7961_BT_FW_VER_ADDR			0x7C812004
#define MT7961_BT_FW_VER_MASK			0xFF

/*------------------------------------------------------------------------------
 * MACRO for debug MCU
 *------------------------------------------------------------------------------
 */
#define CURRENT_PC 0x3F
#define PC_LOG_IDX 0x20
#define PC_LOG_NUM 32

/*------------------------------------------------------------------------------
 * MACRO for WTBL INFO GET
 *------------------------------------------------------------------------------
 */

#define MT7961_WIFI_LWTBL_BASE 0x820d4200
#define MT7961_WIFI_UWTBL_BASE 0x820c4094

#if CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE
#define MT7961_HOST_RX_WM_EVENT_FROM_PSE_RX_RING4_SIZE	32
#endif

/*------------------------------------------------------------------------------
 * MACRO for WFDMA INFO GET
 *------------------------------------------------------------------------------
 */
#define MT7961_WFDMA_COUNT 1

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt7961_show_ple_info(
	struct ADAPTER *prAdapter,
	u_int8_t fgDumpTxd);

void mt7961_show_pse_info(
	struct ADAPTER *prAdapter);

void mt7961_show_wfdma_info(
	struct ADAPTER *prAdapter);

#if defined(_HIF_SDIO)
u_int8_t sdio_show_mcu_debug_info(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max,
	uint8_t ucFlag, uint32_t *pu4Length);
#elif defined(_HIF_USB)
u_int8_t usb_show_mcu_debug_info(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max,
	uint8_t ucFlag, uint32_t *pu4Length);
#elif defined(_HIF_PCIE)
u_int8_t pcie_show_mcu_debug_info(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max,
	uint8_t ucFlag, uint32_t *pu4Length);
#endif

#if (CFG_SUPPORT_DEBUG_SOP == 1)
#if defined(_HIF_USB)
void usb_mt7961_dump_subsys_debug_cr(struct ADAPTER *prAdapter);
void usb_mt7961_dump_conninfra_debug_cr(struct ADAPTER *prAdapter);
#elif defined(_HIF_PCIE)
void pcie_mt7961_dump_subsys_debug_cr(struct ADAPTER *prAdapter);
void pcie_mt7961_dump_conninfra_debug_cr(struct ADAPTER *prAdapter);
#endif
u_int8_t mt7961_show_debug_sop_info(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif


#if defined(_HIF_PCIE) || defined(_HIF_AXI)

void mt7961WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	uint32_t index);

void mt7961WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index);

#if (CFG_COALESCING_INTERRUPT == 1)
uint32_t mt7961setWfdmaCoalescingInt(
	struct ADAPTER *prAdapter,
	u_int8_t fgEnable);
#endif

#endif /* _HIF_PCIE || _HIF_AXI */

void mt7961SerInit(struct ADAPTER *prAdapter,
		   const u_int8_t fgAtResetFlow);

void mt7961DumpSerDummyCR(
	struct ADAPTER *prAdapter);

uint32_t mt7961GetFwVer(struct ADAPTER *prAdapter);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _MT7961_H */

#endif  /* MT7961 */
