/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  connac.h
 *    \brief This file contains the info of CONNAC
 */

#ifndef _CMM_ASIC_CONNAC_H
#define _CMM_ASIC_CONNAC_H

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CONNAC_CHIP_IP_VERSION			(0x10020300)
#define CONNAC_CHIP_IP_CONFIG			(0x1)
#define USB_HIF_TXD_LEN    4
#define NIC_TX_PSE_HEADER_LENGTH                4

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void asicCapInit(struct ADAPTER *prAdapter);
void asicEnableFWDownload(struct ADAPTER *prAdapter,
	u_int8_t fgEnable);
uint32_t asicGetChipID(struct ADAPTER *prAdapter);
void fillNicTxDescAppendWithCR4(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t *prTxDescBuffer);
void fillTxDescAppendByHost(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	phys_addr_t rDmaAddr, uint32_t u4Idx, u_int8_t fgIsLast,
	uint8_t *pucBuffer);
void fillTxDescAppendByCR4(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo, uint16_t u4MsduId,
	phys_addr_t rDmaAddr, uint32_t u4Idx, u_int8_t fgIsLast,
	uint8_t *pucBuffer);
void fillTxDescTxByteCount(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc);
void fillTxDescTxByteCountWithCR4(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc);

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
/* DMS Scheduler Init */
void asicPcieDmaShdlInit(struct ADAPTER *prAdapter);
void asicPdmaLoopBackConfig(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable);
void asicPdmaIntMaskConfig(struct GLUE_INFO *prGlueInfo,
	uint8_t ucType,
	u_int8_t fgEnable);
void asicPdmaConfig(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable,
		bool fgResetHif);
uint32_t asicUpdatTxRingMaxQuota(struct ADAPTER *prAdapter,
	uint8_t ucWmmIndex, uint32_t u4MaxQuota);
void asicEnableInterrupt(struct ADAPTER *prAdapter);
void asicDisableInterrupt(struct ADAPTER *prAdapter);
void asicLowPowerOwnRead(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
void asicLowPowerOwnSet(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
void asicLowPowerOwnClear(struct ADAPTER *prAdapter,
	u_int8_t *pfgResult);
void asicLowPowerOwnClearPCIe(struct ADAPTER *prAdapter,
	u_int8_t *pfgResult);
bool asicIsValidRegAccess(struct ADAPTER *prAdapter, uint32_t u4Register);
void asicGetMailboxStatus(struct ADAPTER *prAdapter, uint32_t *pu4Val);
void asicSetDummyReg(struct GLUE_INFO *prGlueInfo);
void asicCheckDummyReg(struct GLUE_INFO *prGlueInfo);
void asicPdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	uint32_t index);
void asicPdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index);
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
/* DMS Scheduler Init */
void asicUsbDmaShdlInit(struct ADAPTER *prAdapter);
void asicUdmaTxTimeoutEnable(struct ADAPTER *prAdapter);
u_int8_t asicUsbSuspend(struct ADAPTER *prAdapter,
	struct GLUE_INFO *prGlueInfo);
uint8_t asicUsbEventEpDetected(struct ADAPTER *prAdapter);
void asicUdmaRxFlush(struct ADAPTER *prAdapter, u_int8_t bEnable);
void asicPdmaHifReset(struct ADAPTER *prAdapter, u_int8_t bRelease);
void fillUsbHifTxDesc(uint8_t **pDest, uint16_t *pInfoBufLen,
	uint8_t ucPacketType);
#endif /* _HIF_USB */

#if defined(_HIF_SDIO)
void fillSdioHifTxDesc(uint8_t **pDest, uint16_t *pInfoBufLen,
	uint8_t ucPacketType);
#endif /* _HIF_SDIO */

void asicFillInitCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	uint16_t *pu2BufInfoLen,
	u_int8_t *pucSeqNum,
	void **pCmdBuf);
void asicFillCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	u_int8_t *pucSeqNum,
	void **pCmdBuf);
void asicInitTxdHook(
	struct TX_DESC_OPS_T *prTxDescOps);
void asicInitRxdHook(
	struct RX_DESC_OPS_T *prRxDescOps);
#if (CFG_SUPPORT_MSP == 1)
void asicRxProcessRxvforMSP(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prRetSwRfb);
#endif /* CFG_SUPPORT_MSP == 1 */
uint8_t asicRxGetRcpiValueFromRxv(
	uint8_t ucRcpiMode,
	struct SW_RFB *prSwRfb);
#if (CFG_SUPPORT_PERF_IND == 1)
void asicRxPerfIndProcessRXV(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb,
	uint8_t ucBssIndex);
#endif

#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
u_int8_t conn1_rst_L0_notify_step2(void);
#endif
#endif /* _CMM_ASIC_CONNAC_H */

