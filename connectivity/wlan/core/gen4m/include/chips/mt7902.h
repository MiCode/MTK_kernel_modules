/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  mt7902.h
 *    \brief This file contains the info of MT7902
 */

#if defined(MT7902)

#ifndef _MT7902_H
#define _MT7902_H

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

#define MT7902_UDMA_CONN_INFRA_STATUS_SEL   0x74000A4C
#define MT7902_UDMA_CONN_INFRA_STATUS_VAL   0xD8000000
#define MT7902_UDMA_CONN_INFRA_STATUS       0x74000A48

#define MT7902_CHIP_ID                 (0x7902)
#define MT7902_SW_SYNC0                CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR
#define MT7902_SW_SYNC0_RDY_OFFSET \
	CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT
#define MT7902_PATCH_START_ADDR        (0x00900000)
#define MT7902_TOP_CFG_BASE			CONN_CFG_BASE
#define MT7902_TX_DESC_APPEND_LENGTH   32
#define MT7902_RX_DESC_LENGTH   24
#define MT7902_ARB_AC_MODE_ADDR (0x820E315C)

#define MT7902_A_DIE_VER_ADDR 0x70010020
#define MT7902_A_DIE_VER_BIT  BIT(7)

#define MT7902_BT_FW_VER_ADDR 0x7C812004
#define MT7902_BT_FW_VER_MASK 0xFF

/*------------------------------------------------------------------------------
 * MACRO for WTBL INFO GET
 *------------------------------------------------------------------------------
 */

#define MT7902_WIFI_LWTBL_BASE 0x820d4200
#define MT7902_WIFI_UWTBL_BASE 0x820c4094


/*------------------------------------------------------------------------------
 * MACRO for WFDMA INFO GET
 *------------------------------------------------------------------------------
 */
#define MT7902_WFDMA_COUNT 1

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
void mt7902_show_ple_info(
	struct ADAPTER *prAdapter,
	u_int8_t fgDumpTxd);

void mt7902_show_wfdma_info(
	struct ADAPTER *prAdapter);

void mt7902_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

#if defined(_HIF_SDIO)
u_int8_t mt7902_sdio_show_mcu_debug_info(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuf, uint32_t u4Max,
	uint8_t ucFlag, uint32_t *pu4Length);
#endif

#if (CFG_SUPPORT_DEBUG_SOP == 1)

#if defined(_HIF_PCIE)
void pcie_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void pcie_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void pcie_mt7902_dump_bgfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void pcie_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_PCIE) */

#if defined(_HIF_USB)
void usb_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void usb_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void usb_mt7902_dump_bgfsys_debug_cr(struct ADAPTER *prAdapter);
void usb_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_USB) */

#if defined(_HIF_SDIO)
void sdio_mt7902_dump_power_debug_cr(struct ADAPTER *prAdapter);
void sdio_mt7902_dump_wfsys_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
void sdio_mt7902_dump_conninfra_debug_cr(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif /* #if defined(_HIF_SDIO) */

u_int8_t mt7902_show_debug_sop_info(struct ADAPTER *prAdapter,
	uint8_t ucCase);
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)

void mt7902EnableInterrupt(
	struct ADAPTER *prAdapter);
void mt7902DisableInterrupt(
	struct ADAPTER *prAdapter);
uint8_t mt7902SetRxRingHwAddr(
	struct RTMP_RX_RING *prRxRing,
	struct BUS_INFO *prBusInfo,
	uint32_t u4SwRingIdx);
bool mt7902LiteWfdmaAllocRxRing(
	struct GLUE_INFO *prGlueInfo,
	bool fgAllocMem);
void mt7902Connac2xProcessTxInterrupt(
	struct ADAPTER *prAdapter);
void mt7902Connac2xProcessRxInterrupt(
	struct ADAPTER *prAdapter);
void mt7902WfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	uint32_t index);
void mt7902WfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index);
void mt7902Connac2xWfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);

#if CONFIG_PHYS_ADDR_T_64BIT
void mt7902WfdmaRingCntBaseExtCtrl(
	struct GLUE_INFO *prGlueInfo, uint32_t hw_cnt_addr,
	uint32_t phy_addr_ext, uint32_t cnt);
#endif /* CONFIG_PHYS_ADDR_T_64BIT */
void mt7902ReadIntStatus(
	struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus);


#endif

#if defined(_HIF_USB)
uint8_t mt7902Connac2xUsbEventEpDetected(struct ADAPTER *prAdapter);
uint16_t mt7902Connac2xUsbRxByteCount(
	struct ADAPTER *prAdapter,
	struct BUS_INFO *prBusInfo,
	uint8_t *pRXD);
#endif

void mt7902ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx);
void mt7902ConstructPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);

#if CFG_SUPPORT_WIFI_DL_BT_PATCH
void mt7902ConstructBtPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
uint32_t mt7902DownloadBtPatch(struct ADAPTER *prAdapter);
#endif

#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
void mt7902ConstructZbPatchName(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucName, uint8_t *pucNameIdx);
uint32_t mt7902DownloadZbPatch(struct ADAPTER *prAdapter);
#endif

void mt7902_icapRiseVcoreClockRate(void);
void mt7902_icapDownVcoreClockRate(void);
uint32_t mt7902ConstructBufferBinFileName(struct ADAPTER *prAdapter,
	uint8_t *aucEeprom);
u_int8_t mt7902GetRxDbgInfoSrc(struct ADAPTER *prAdapter);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _MT7902_H */

#endif  /* defined(MT7902) */
