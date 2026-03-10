/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*! \file   "hif.h"
 *  \brief  Functions for the driver to register bus and setup the IRQ
 *
 *  Functions for the driver to register bus and setup the IRQ
 */

#ifndef _HIF_H
#define _HIF_H

#include "hif_pdma.h"

#if defined(_HIF_AXI)
#define HIF_NAME "AXI"
#else
#error "No HIF defined!"
#endif

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
#define AXI_ISR_DEBUG_LOG    0

#define AXI_WLAN_IRQ_NUMBER               16

#if (CFG_SUPPORT_CONNINFRA == 1)
#define WIFI_EMI_WFDMA_OFFSET      0x450000
#define WIFI_EMI_WFDMA_SIZE        0xF20000
#endif


/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct GL_HIF_INFO;

struct HIF_MEM {
	phys_addr_t pa;
	void *va;
};

struct HIF_MEM_OPS {
	void (*allocTxDesc)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
	void (*allocRxDesc)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
	void (*allocExtBuf)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing);
	bool (*allocTxCmdBuf)(struct RTMP_DMABUF *prDmaBuf,
			      uint32_t u4Num, uint32_t u4Idx);
	void (*allocTxDataBuf)(struct MSDU_TOKEN_ENTRY *prToken,
			       uint32_t u4Idx);
	void *(*allocRxEvtBuf)(struct GL_HIF_INFO *prHifInfo,
			       struct RTMP_DMABUF *prDmaBuf,
			       uint32_t u4Num, uint32_t u4Idx);
	void *(*allocRxDataBuf)(struct GL_HIF_INFO *prHifInfo,
				struct RTMP_DMABUF *prDmaBuf,
				uint32_t u4Num, uint32_t u4Idx);
	void *(*allocRuntimeMem)(uint32_t u4SrcLen);
	bool (*copyCmd)(struct GL_HIF_INFO *prHifInfo,
			struct RTMP_DMACB *prTxCell, void *pucBuf,
			void *pucSrc1, uint32_t u4SrcLen1,
			void *pucSrc2, uint32_t u4SrcLen2);
	bool (*copyEvent)(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RXD_STRUCT *pRxD,
			  struct RTMP_DMABUF *prDmaBuf,
			  uint8_t *pucDst, uint32_t u4Len);
	bool (*copyTxData)(struct MSDU_TOKEN_ENTRY *prToken,
			   void *pucSrc, uint32_t u4Len);
	bool (*copyRxData)(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RTMP_DMABUF *prDmaBuf,
			   struct SW_RFB *prSwRfb);
	phys_addr_t (*mapTxBuf)(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
	phys_addr_t (*mapRxBuf)(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
	void (*unmapTxBuf)(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
	void (*unmapRxBuf)(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
	void (*freeDesc)(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMABUF *prDescRing);
	void (*freeExtBuf)(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing);
	void (*freeBuf)(void *pucSrc, uint32_t u4Len);
	void (*freePacket)(struct GL_HIF_INFO *prHifInfo,
			   void *pvPacket, uint32_t u4Num);
	struct HIF_MEM *(*getRsvEmi)(struct GL_HIF_INFO *prHifInfo);
	void (*dumpTx)(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
	void (*dumpRx)(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
};

/* host interface's private data structure, which is attached to os glue
 ** layer info structure.
 */
struct GL_HIF_INFO {
	struct platform_device *pdev;
	struct device *prDmaDev;
	struct HIF_MEM_OPS rMemOps;

	uint32_t u4IrqId;
#if (CFG_SUPPORT_CONNINFRA == 1)
	uint32_t u4IrqId_1;
#endif
	int32_t u4HifCnt;

	/* AXI MMIO Base Address, all access will use */
	void *CSRBaseAddress;

	/* Shared memory of all 1st pre-allocated
	 * TxBuf associated with each TXD
	 */
	/* Shared memory for Tx descriptors */
	struct RTMP_DMABUF TxDescRing[NUM_OF_TX_RING];
	struct RTMP_TX_RING TxRing[NUM_OF_TX_RING];	/* AC0~3 + HCCA */

	/* Shared memory for RX descriptors */
	struct RTMP_DMABUF RxDescRing[NUM_OF_RX_RING];
	struct RTMP_RX_RING RxRing[NUM_OF_RX_RING];
	uint32_t u4RxDataRingSize;
	uint32_t u4RxEvtRingSize;

	union WPDMA_GLO_CFG_STRUCT GloCfg;

	u_int8_t fgIntReadClear;
	u_int8_t fgMbxReadClear;

	uint32_t u4IntStatus;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	uint32_t u4OffloadIntStatus;
#endif
	unsigned long ulIntFlag;

	struct MSDU_TOKEN_INFO rTokenInfo;

	struct ERR_RECOVERY_CTRL_T rErrRecoveryCtl;
	struct timer_list rSerTimer;
	unsigned long rSerTimerData;
	struct list_head rTxCmdQ;
	struct list_head rTxCmdFreeList;
	spinlock_t rTxCmdQLock;
	struct list_head rTxDataQ[NUM_OF_TX_RING];
	uint32_t u4TxDataQLen[NUM_OF_TX_RING];
	spinlock_t rTxDataQLock[NUM_OF_TX_RING];
#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	struct timer_list rTxDelayTimer;
	unsigned long rTxDelayTimerData;
	unsigned long ulTxDataTimeout;
#endif /* CFG_SUPPORT_TX_DATA_DELAY == 1 */

	bool fgIsPowerOn;
	bool fgForceReadWriteReg;

	uint32_t u4WakeupIntSta;
	bool fgIsBackupIntSta;

	unsigned long ulHifIntEnBits;
};

struct BUS_INFO {
	const uint32_t top_cfg_base;	/* TOP_CFG_BASE address */
	const struct PCIE_CHIP_CR_MAPPING *bus2chip;
	const struct PCIE_CHIP_CR_REMAPPING *bus2chip_remap;
	const uint32_t tx_ring_cmd_idx;
	const uint32_t tx_ring_wa_cmd_idx;
	const uint32_t tx_ring_fwdl_idx;
	const uint32_t tx_ring0_data_idx;
	const uint32_t tx_ring1_data_idx;
	const uint32_t tx_ring2_data_idx;
	const uint32_t tx_ring3_data_idx;
	const uint32_t rx_data_ring_num;
	const uint32_t rx_evt_ring_num;
	uint32_t rx_data_ring_size;
	uint32_t rx_evt_ring_size;
	uint32_t rx_data_ring_prealloc_size;
	const uint32_t max_static_map_addr;
	const uint32_t fw_own_clear_addr;
	const uint32_t fw_own_clear_bit;
	const bool fgCheckDriverOwnInt;
	const uint32_t u4DmaMask;
	/* host pdma/wfdma0 base address */
	const uint32_t host_dma0_base;
	/* host wfdma1 base address */
	const uint32_t host_dma1_base;
	/* host ext conn hif wrap */
	const uint32_t host_ext_conn_hif_wrap_base;
	const uint32_t host_int_status_addr;
	const uint32_t host_int_txdone_bits;
	const uint32_t host_int_rxdone_bits;

	/* tx pdma/wfdma ring base address */
	const uint32_t host_tx_ring_base;
	/* tx pdma/wfdma ring ext control base address */
	const uint32_t host_tx_ring_ext_ctrl_base;
	/* tx pdma/wfdma ring cpu index address */
	const uint32_t host_tx_ring_cidx_addr;
	/* tx pdma/wfdma ring dma index address */
	const uint32_t host_tx_ring_didx_addr;
	/* tx pdma/wfdma ring count address */
	const uint32_t host_tx_ring_cnt_addr;

	/* rx pdma/wfdma ring base address */
	const uint32_t host_rx_ring_base;
	/* rx pdma/wfdma ring ext control base address */
	const uint32_t host_rx_ring_ext_ctrl_base;
	/* rx pdma/wfdma ring cpu index address */
	const uint32_t host_rx_ring_cidx_addr;
	/* rx pdma/wfdma ring dma index address */
	const uint32_t host_rx_ring_didx_addr;
	/* rx pdma/wfdma ring count address */
	const uint32_t host_rx_ring_cnt_addr;

#if (CFG_SUPPORT_CONNAC2X == 1)
	/* rx wfdma_1 ring base address */
	const uint32_t host_wfdma1_rx_ring_base;
	/* rx wfdma_1 ring cpu index address */
	const uint32_t host_wfdma1_rx_ring_cidx_addr;
	/* rx wfdma_1 ring dma index address */
	const uint32_t host_wfdma1_rx_ring_didx_addr;
	/* rx wfdma_1 ring count address */
	const uint32_t host_wfdma1_rx_ring_cnt_addr;
	/* rx wfdma_1 ring ext control base address */
	const uint32_t host_wfdma1_rx_ring_ext_ctrl_base;
#endif /* CFG_SUPPORT_CONNAC2X == 1 */

	struct wfdma_group_info *wfmda_host_tx_group;
	const uint32_t wfmda_host_tx_group_len;
	struct wfdma_group_info *wfmda_host_rx_group;
	const uint32_t wfmda_host_rx_group_len;
	struct wfdma_group_info *wfmda_wm_tx_group;
	const uint32_t wfmda_wm_tx_group_len;
	struct wfdma_group_info *wfmda_wm_rx_group;
	const uint32_t wfmda_wm_rx_group_len;

	struct DMASHDL_CFG *prDmashdlCfg;
	struct PLE_TOP_CR *prPleTopCr;
	struct PSE_TOP_CR *prPseTopCr;
	struct PP_TOP_CR *prPpTopCr;
	struct pse_group_info *prPseGroup;
	const uint32_t u4PseGroupLen;

	void (*pdmaSetup)(struct GLUE_INFO *prGlueInfo, u_int8_t enable,
		bool fgResetHif);
	uint32_t (*updateTxRingMaxQuota)(struct ADAPTER *prAdapter,
		uint8_t ucWmmIndex, uint32_t u4MaxQuota);
	void (*enableInterrupt)(struct ADAPTER *prAdapter);
	void (*disableInterrupt)(struct ADAPTER *prAdapter);
	void (*configWfdmaIntMask)(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
	void (*configWfdmaRxRingTh)(struct ADAPTER *prAdapter, uint32_t u4Num,
				    u_int8_t fgIsData);
	void (*disableSwInterrupt)(struct ADAPTER *prAdapter);
	void (*processTxInterrupt)(struct ADAPTER *prAdapter);
	void (*processRxInterrupt)(struct ADAPTER *prAdapter);
	void (*processAbnormalInterrupt)(struct ADAPTER *prAdapter);
	void (*lowPowerOwnRead)(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
	void (*lowPowerOwnSet)(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
	void (*lowPowerOwnClear)(struct ADAPTER *prAdapter,
		u_int8_t *pfgResult);
	void (*wakeUpWiFi)(struct ADAPTER *prAdapter);
	bool (*isValidRegAccess)(struct ADAPTER *prAdapter,
				 uint32_t u4Register);
	void (*getMailboxStatus)(struct ADAPTER *prAdapter, uint32_t *pu4Val);
	void (*setDummyReg)(struct GLUE_INFO *prGlueInfo);
	void (*checkDummyReg)(struct GLUE_INFO *prGlueInfo);
	void (*tx_ring_ext_ctrl)(struct GLUE_INFO *prGlueInfo,
		struct RTMP_TX_RING *tx_ring, uint32_t index);
	void (*rx_ring_ext_ctrl)(struct GLUE_INFO *prGlueInfo,
		struct RTMP_RX_RING *rx_ring, uint32_t index);
	void (*wfdmaManualPrefetch)(struct GLUE_INFO *prGlueInfo);
	void (*processSoftwareInterrupt)(struct ADAPTER *prAdapter);
	void (*softwareInterruptMcu)(struct ADAPTER *prAdapter,
		u_int32_t intrBitMask);
	void (*hifRst)(struct GLUE_INFO *prGlueInfo);
	void (*devReadIntStatus)(struct ADAPTER *prAdapter,
		uint32_t *pu4IntStatus);
	/* Do DMASDHL init when WIFISYS is initialized at probe, L0.5 reset,
	 * etc.
	 */
	void (*DmaShdlInit)(struct ADAPTER *prAdapter);
	/* Although DMASHDL was init, we need to reinit it again due to falcon
	 * L1 reset, etc. Take MT7961 as example. The difference between
	 * mt7961DmashdlInit and mt7961DmashdlReInit is that we don't init CRs
	 * such as refill, min_quota, max_quota in mt7961DmashdlReInit, which
	 * are backup and restored in fw. The reason why some DMASHDL CRs are
	 * reinit by driver and some by fw is
	 *     1. Some DMASHDL CRs shall be inited before fw releases UMAC reset
	 *        in L1 procedure. Then, these CRs are backup and restored by fw
	 *     2. However, the backup and restore of each DMASHDL CR in fw needs
	 *        wm DLM space. So, we save DLM space by reinit the remaining
	 *        DMASHDL CRs in driver.
	 */
	void (*DmaShdlReInit)(struct ADAPTER *prAdapter);
	uint8_t (*setRxRingHwAddr)(struct RTMP_RX_RING *prRxRing,
		struct BUS_INFO *prBusInfo,
		uint32_t u4SwRingIdx);
	bool (*wfdmaAllocRxRing)(
		struct GLUE_INFO *prGlueInfo,
		bool fgAllocMem);
	void (*setDmaIntMask)(struct GLUE_INFO *prGlueInfo,
		uint8_t ucType, u_int8_t fgEnable);
	void (*enableFwDlMode)(struct ADAPTER *prAdapter);
	void (*setupMcuEmiAddr)(struct ADAPTER *prAdapter);
	void (*showDebugInfo)(struct GLUE_INFO *prGlueInfo);
	void (*disableDevice)(struct GLUE_INFO *prGlueInfo);

#if (CFG_COALESCING_INTERRUPT == 1)
	uint32_t (*setWfdmaCoalescingInt)(struct ADAPTER *prAdapter,
		u_int8_t fgEnable);
#endif

	struct SW_WFDMA_INFO rSwWfdmaInfo;
	struct SW_EMI_RING_INFO rSwEmiRingInfo;

#if CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE
	u_int8_t (*checkPortForRxEventFromPse)(struct ADAPTER *prAdapter,
		uint8_t u2Port);
#endif
	struct timespec64 rHifIntTs;
	uint32_t u4EnHifIntTs;
	uint32_t u4HifIntTsCnt;

	u_int8_t fgUpdateWfdmaTh;
	uint32_t u4WfdmaTh;
};

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

#define axi_resource_start(d, v)  (0x18000000)
#define axi_resource_len(d, v)    (0x100000)
#define axi_name(d)               ("AXI-BUS")

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint32_t glRegisterBus(probe_card pfProbe, remove_card pfRemove);

void glUnregisterBus(remove_card pfRemove);

void glSetHifInfo(struct GLUE_INFO *prGlueInfo, unsigned long ulCookie);

void glClearHifInfo(struct GLUE_INFO *prGlueInfo);

void glResetHifInfo(struct GLUE_INFO *prGlueInfo);

u_int8_t glBusInit(void *pvData);

void glBusRelease(void *pData);

int32_t glBusSetIrq(void *pvData, void *pfnIsr, void *pvCookie);

void glBusFreeIrq(void *pvData, void *pvCookie);

void glSetPowerState(struct GLUE_INFO *prGlueInfo, uint32_t ePowerMode);

void glGetDev(void *ctx, void **dev);

void glGetHifDev(struct GL_HIF_INFO *prHif, struct device **dev);

struct mt66xx_hif_driver_data *get_platform_driver_data(void);

void glGetChipInfo(void **prChipInfo);

int32_t glBusFuncOn(void);
void glBusFuncOff(void);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif /* _HIF_H */
