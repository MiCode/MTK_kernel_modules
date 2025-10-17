/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "hif.h"
 *    \brief  Functions for the driver to register bus and setup the IRQ
 *
 *    Functions for the driver to register bus and setup the IRQ
 */

#ifndef _HIF_H
#define _HIF_H

#include "hif_pdma.h"

#if defined(_HIF_PCIE)
#define HIF_NAME "PCIE"
#else
#error "No HIF defined!"
#endif


#if defined(CFG_MTK_WIFI_PCIE_SUPPORT) && CFG_MTK_ANDROID_WMT
#include "pcie-mediatek-gen3.h"
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
#define PCIE_ISR_DEBUG_LOG                0

#if CFG_SUPPORT_PCIE_ASPM
#define ENABLE_ASPM_L1 2
#define DISABLE_ASPM_L1 0

#define  PCI_EXT_CAP_ID_L1PMSS				0x1E
#define  PCI_L1PMSS_CAP						4
#define  PCI_L1PM_CAP_PCIPM_L12				0x00000001
#define  PCI_L1PM_CAP_PCIPM_L11				0x00000002
#define  PCI_L1PM_CAP_ASPM_L12				0x00000004
#define  PCI_L1PM_CAP_ASPM_L11				0x00000008
#define  PCI_L1PM_CAP_L1PM_SS				0x00000010
#define  PCI_L1PM_CAP_PORT_CMR_TIME_MASK	0x0000FF00
#define  PCI_L1PM_CAP_PWR_ON_SCALE_MASK		0x00030000
#define  PCI_L1PM_CAP_PWR_ON_VALUE_MASK		0x00F80000
#define  PCI_L1PMSS_CTR1					8
#define  PCI_L1PM_CTR1_PCIPM_L12_EN			0x00000001
#define  PCI_L1PM_CTR1_PCIPM_L11_EN			0x00000002
#define  PCI_L1PM_CTR1_ASPM_L12_EN			0x00000004
#define  PCI_L1PM_CTR1_ASPM_L11_EN			0x00000008
#define  PCI_L1PMSS_ENABLE_MASK	            (PCI_L1PM_CTR1_PCIPM_L12_EN |   \
					PCI_L1PM_CTR1_PCIPM_L11_EN |   \
					PCI_L1PM_CTR1_ASPM_L12_EN |    \
					PCI_L1PM_CTR1_ASPM_L11_EN)
#define  PCI_L1PM_ENABLE_MASK			0x3

#define PCIE_LOW_POWER_CTRL_DIS_L1		BIT(9)
#define PCIE_LOW_POWER_CTRL_DIS_L1_1	BIT(10)
#define PCIE_LOW_POWER_CTRL_DIS_L1_2	BIT(11)

#define PCIE_ASPM_CHECK_L1(reg)	((((reg) & PCI_EXP_LNKCAP_ASPMS) >> 10) & 0x2)

#endif

#if CFG_SUPPORT_PCIE_GEN_SWITCH
#define PCI_SPEED_MASK  0xf
#define LINK_RETRAIN_TIMEOUT HZ
#endif

#if (CFG_MTK_WIFI_ON_READ_BY_CFG_SPACE == 1)
/* Offset:0x490, value: 0x1D1E*/
#define PCIE_CFGSPACE_MCU_IDLE_OFFSET		0x490

/* Offset:0x48C,
 * BIT[14]: own status - 0:driver own, 1:fw own
 * BIT[30:28]: fw sync - 3:fw ready
 */
#define PCIE_CFGSPACE_BASE_OFFSET		0x48C
#define PCIE_CFGSPACE_OWN_STATUS_SHIFT		14
#define PCIE_CFGSPACE_OWN_STATUS_MASK		0x1
#define PCIE_CFGSPACE_FW_STATUS_SYNC_SHIFT	28
#define PCIE_CFGSPACE_FW_STATUS_SYNC_MASK	0x3
#endif /* CFG_MTK_WIFI_ON_READ_BY_CFG_SPACE */

#define PCIE_EP_CONFIG_SPACE_SIZE	16

#if (CFG_PCIE_GEN_SWITCH == 1)
#define PCIE_STOP_TRANSITION_NOT_START  0
#define PCIE_STOP_TRANSITION_ON_GOING   1
#define PCIE_STOP_TRANSITION_END        2
#define PCIE_RECOVER_TRANSITION_DONE    3
#define PCIE_MD_REJECT_GEN_SWITCH       4
#define PCIE_MD_BYPASS_GEN_SWITCH_START       5
#define PCIE_MD_BYPASS_GEN_SWITCH_END         6
#endif
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct GL_HIF_INFO;

struct HIF_MEM_OPS {
	void (*allocTxDesc)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
	void (*allocRxDesc)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
	void (*allocExtBuf)(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Align);
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
	phys_addr_t (*mapTxDataBuf)(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
	phys_addr_t (*mapTxCmdBuf)(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
	phys_addr_t (*mapRxBuf)(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
	void (*unmapTxDataBuf)(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
	void (*unmapTxCmdBuf)(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
	void (*unmapRxBuf)(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
	void (*freeDesc)(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMABUF *prDescRing);
	void (*freeExtBuf)(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing);
	void (*freeDataBuf)(void *pucSrc, uint32_t u4Len,
				phys_addr_t rDmaAddr, uint32_t u4Idx);
	void (*freeCmdBuf)(void *pucSrc, uint32_t u4Len);
	void (*freePacket)(struct GL_HIF_INFO *prHifInfo,
			   void *pvPacket, uint32_t u4Num);
	struct HIF_MEM *(*getWifiMiscRsvEmi)(
				struct mt66xx_chip_info *prChipInfo,
				enum WIFI_MISC_MEM_BLOCK_NAME u4idx);
	void (*dumpTx)(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
	void (*dumpRx)(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
};

enum pcie_suspend_state {
	PCIE_STATE_PRE_SUSPEND_WAITING, /* Waiting for FW suspend flow status */
	PCIE_STATE_PRE_SUSPEND_DONE,
	PCIE_STATE_PRE_SUSPEND_FAIL,
	PCIE_STATE_SUSPEND_ENTERING,
	PCIE_STATE_PRE_RESUME_DONE,
	PCIE_STATE_SUSPEND
};

enum pcie_vote_user {
	PCIE_VOTE_USER_DRVOWN = 0,
	PCIE_VOTE_USER_LOG_RESET,
	PCIE_VOTE_USER_MDDP,
	PCIE_VOTE_USER_NUM
};

#if CFG_SUPPORT_PCIE_ASPM
enum pcie_aspm_state {
	PCIE_STATE_L0,
	PCIE_STATE_L1,
	PCIE_STATE_L1_2,
	PCIE_STATE_NUM
};
#endif

#if CFG_PCIE_GEN_SWITCH
struct RX_IDLE_STATE {
	uint32_t u4WFIdle;
	uint32_t u4FWIdle;
};
#endif /*CFG_SUPPORT_PCIE_GEN_SWITCH*/


#if CFG_SUPPORT_HIF_RX_NAPI
struct HIF_NAPI_DEVICE {
	struct net_device dev;
	struct napi_struct napi;
	struct GLUE_INFO *prGlueInfo;
	struct task_struct *napi_thread;
	uint32_t u4ThreadPid;
	u_int8_t fgIsRun;
	unsigned long ulFlag;
	uint32_t u4DrvOwnCnt;
};
#endif /* CFG_SUPPORT_HIF_RX_NAPI */

/* host interface's private data structure, which is attached to os glue
 ** layer info structure.
 */

struct GL_HIF_INFO {
	struct pci_dev *pdev;
	struct pci_dev *prDmaDev;
	struct GLUE_INFO *prGlueInfo;
	struct HIF_MEM_OPS rMemOps;

	uint32_t u4IrqId;
	uint32_t u4IrqId_1;
	int32_t u4HifCnt;

	/* Shared memory of all 1st pre-allocated
	 * TxBuf associated with each TXD
	 */
	/* Shared memory for Tx descriptors */
	struct RTMP_DMABUF TxDescRing[NUM_OF_TX_RING];
	/* AC0~3 + HCCA */
	struct RTMP_TX_RING TxRing[NUM_OF_TX_RING];

	/* Shared memory for RX descriptors */
	struct RTMP_DMABUF RxDescRing[NUM_OF_RX_RING];
	struct RTMP_RX_RING RxRing[NUM_OF_RX_RING];
#if CFG_MTK_WIFI_WFDMA_WB
	struct RTMP_DMABUF rRingDmyRd;
	struct RTMP_DMABUF rRingDmyWr;
	struct RTMP_DMABUF rRingDmyDbg;
	struct RTMP_DMABUF rRingIntSta;
	struct RTMP_DMABUF rRingDidx;
	struct RTMP_DMABUF rRingCidx;
	struct RTMP_DMABUF rHwDoneFlag;
	struct RTMP_DMABUF rSwDoneFlag;

	struct RTMP_DMABUF rRingMdIntSta;
	struct RTMP_DMABUF rRingMdDidx;

	struct WFDMA_EMI_DONE_FLAG rIntFlag;
	u_int8_t fgIsUrgentCidxFetch;
	u_int8_t fgIsNeedCidxFetchFlag;
	u_int8_t fgIsCidxFetchNewTx;
	unsigned long ulCidxFetchTimeout;
	uint32_t u4WbIntSta;
	uint32_t u4WbMdIntSta;
#endif /* CFG_MTK_WIFI_WFDMA_WB */
	uint32_t u4RxDataRingSize;
	uint32_t u4RxEvtRingSize;

	union WPDMA_GLO_CFG_STRUCT GloCfg;

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* MAWD */
	struct RTMP_DMABUF HifTxDescRing[NUM_OF_TX_RING];
	struct RTMP_TX_RING MawdTxRing[NUM_OF_TX_RING];
	struct RTMP_DMABUF ErrRptRing;
	uint32_t u4MawdL2TblCnt;
	u_int8_t fgIsMawdSuspend;

	/* RRO */
	struct RTMP_DMABUF RxBlkDescRing[MAWD_NUM_OF_RX_BLK_RING];
	struct RTMP_RX_RING RxBlkRing[MAWD_NUM_OF_RX_BLK_RING];
	struct RTMP_DMABUF BaBitmapCache;
	struct RTMP_DMABUF AddrArray;
	struct RTMP_DMABUF IndCmdRing;
	struct RTMP_DMABUF AckSnCmdRing;
	struct list_head rRcbUsedList[NUM_OF_RX_RING];
	uint32_t u4RcbUsedListCnt[NUM_OF_RX_RING];
	struct list_head rRcbFreeList;
	uint32_t u4RcbFreeListCnt;
	struct hlist_head arRcbHTbl[RRO_HASH_TABLE_SIZE];
	struct hlist_head rRcbHTblFreeList;
	uint32_t u4RroMagicCnt;
	uint32_t u4IndCmdDmaIdx;
	uint32_t u4RxBlkDidx;
	uint32_t u4RxBlkMagicCnt;
	uint32_t u4OffloadIntStatus;
	uint32_t u4RcbErrorCnt;
	uint32_t u4RcbSkipCnt;
	uint32_t u4RcbFixCnt;
	uint32_t u4RcbHeadCnt;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	u_int8_t fgIntReadClear;
	u_int8_t fgMbxReadClear;

	uint32_t u4IntStatus;
	unsigned long ulIntFlag;

	struct MSDU_TOKEN_INFO rTokenInfo;

	struct ERR_RECOVERY_CTRL_T rErrRecoveryCtl;
	struct timer_list rSerTimer;
	unsigned long rSerTimerData;
#if CFG_MTK_MDDP_SUPPORT
	uint8_t fgMdResetInd;
#if (CFG_PCIE_GEN_SWITCH == 1)
	struct timer_list rGenSwitch4MddpTimer;
	unsigned long rGenSwitch4MddpTimerData;
	uint32_t u4GenSwitchState;
#endif /* CFG_PCIE_GEN_SWITCH */
#endif /* CFG_MTK_MDDP_SUPPORT */
	struct list_head rTxCmdQ;
	struct list_head rTxCmdFreeList;
	spinlock_t rTxCmdQLock;
	struct list_head rTxDataQ[NUM_OF_TX_RING];
	uint32_t u4TxDataQLen[NUM_OF_TX_RING];
	spinlock_t rTxDataQLock[NUM_OF_TX_RING];
#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
#if CFG_SUPPORT_HRTIMER
	struct hrtimer rTxDelayTimer;
#else
	struct timer_list rTxDelayTimer;
#endif
	unsigned long rTxDelayTimerData;
	unsigned long ulTxDataTimeout;
#endif /* CFG_SUPPORT_TX_DATA_DELAY == 1 */

	bool fgIsPowerOff;
	bool fgIsDumpLog;
	bool fgIsEnTxRingSizeCtrl;

	uint32_t u4WakeupIntSta;
	bool fgIsBackupIntSta;

	enum pcie_suspend_state eSuspendtate;
	uint32_t u4VoteState;
#if CFG_SUPPORT_PCIE_ASPM
	uint32_t u4PcieLTR;
	uint32_t u4PcieASPM;
	enum pcie_aspm_state eCurPcieState;
	enum pcie_aspm_state eNextPcieState;
#endif

	unsigned long ulHifIntEnBits;
	uint32_t u4IntBitSetCnt;

#if CFG_NEW_HIF_DEV_REG_IF
	struct HIF_DEV_REG_RECORD arMmioReadHistory[HIF_DEV_REG_HISTORY_SIZE];
	uint32_t u4MmioReadHistoryIdx;
	uint32_t u4MmioReadReasonCnt[HIF_DEV_REG_MAX];
#endif /* CFG_NEW_HIF_DEV_REG_IF */
#if CFG_SUPPORT_WED_PROXY
	irq_handler_t irq_handler;
	irq_handler_t irq_handler_thread;
#endif
#if CFG_SUPPORT_HIF_RX_NAPI
	struct HIF_NAPI_DEVICE rNapiDev;
#endif /* CFG_SUPPORT_HIF_RX_NAPI */

#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
	u_int8_t fgEnablePcieCfgDump;
#endif /* CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG */
	u_int8_t fgIsDebugSopOnGoing;

	u_int8_t fgIsTriggerRxTimeout;
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
	const uint32_t tx_prio_data_idx;
	const uint32_t tx_altx_data_idx;
	const uint32_t rx_data_ring_num;
	const uint32_t rx_evt_ring_num;
	const uint32_t rx_data_ring_size;
	const uint32_t rx_evt_ring_size;
	const uint32_t rx_data_ring_prealloc_size;
	const unsigned int max_static_map_addr;
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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	/* MAWD */
	const uint32_t mawd_array_base_l;
	const uint32_t mawd_array_base_m;
	const uint32_t mawd_rx_blk_ctrl0;
	const uint32_t mawd_rx_blk_ctrl1;
	const uint32_t mawd_rx_blk_ctrl2;
	const uint32_t mawd_md_rx_blk_ctrl0;
	const uint32_t mawd_md_rx_blk_ctrl1;
	const uint32_t mawd_md_rx_blk_ctrl2;
	const uint32_t mawd_ring_ctrl0;
	const uint32_t mawd_ring_ctrl1;
	const uint32_t mawd_ring_ctrl2;
	const uint32_t mawd_ring_ctrl3;
	const uint32_t mawd_ring_ctrl4;
	const uint32_t mawd_hif_txd_ctrl0;
	const uint32_t mawd_hif_txd_ctrl1;
	const uint32_t mawd_hif_txd_ctrl2;
	const uint32_t mawd_err_rpt_ctrl0;
	const uint32_t mawd_err_rpt_ctrl1;
	const uint32_t mawd_err_rpt_ctrl2;
	const uint32_t mawd_settings0;
	const uint32_t mawd_settings1;
	const uint32_t mawd_settings2;
	const uint32_t mawd_settings3;
	const uint32_t mawd_settings4;
	const uint32_t mawd_settings5;
	const uint32_t mawd_settings6;
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

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
	struct pcie_msi_info pcie_msi_info;
	const u_int8_t is_en_drv_ctrl_pci_msi_irq;
	uint32_t u4ConfigSpace[PCIE_EP_CONFIG_SPACE_SIZE];
	uint8_t ucConfigSpaceBkDone;

	void (*pcieMsiMaskIrq)(uint32_t u4Irq, uint32_t u4Bit);
	void (*pcieMsiUnmaskIrq)(uint32_t u4Irq, uint32_t u4Bit);
	void (*pdmaSetup)(struct GLUE_INFO *prGlueInfo, u_int8_t enable,
		bool fgResetHif);
	uint32_t (*updateTxRingMaxQuota)(struct ADAPTER *prAdapter,
		uint8_t ucWmmIndex, uint32_t u4MaxQuota);
	void (*pdmaStop)(struct GLUE_INFO *prGlueInfo, u_int8_t enable);
	u_int8_t (*pdmaPollingIdle)(struct GLUE_INFO *prGlueInfo);
	void (*enableInterrupt)(struct ADAPTER *prAdapter);
	void (*disableInterrupt)(struct ADAPTER *prAdapter);
	void (*configWfdmaIntMask)(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
	void (*configWfdmaRxRingTh)(struct ADAPTER *prAdapter, uint32_t u4Num,
				    u_int8_t fgIsData);
	void (*disableSwInterrupt)(struct ADAPTER *prAdapter);
	void (*processTxInterrupt)(struct ADAPTER *prAdapter);
	void (*processRxInterrupt)(struct ADAPTER *prAdapter);
	void (*processAbnormalInterrupt)(struct ADAPTER *prAdapter);
	void (*lowPowerOwnInit)(struct ADAPTER *prAdapter);
	void (*lowPowerOwnRead)(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
	void (*lowPowerOwnSet)(struct ADAPTER *prAdapter, u_int8_t *pfgResult);
	void (*lowPowerOwnClear)(struct ADAPTER *prAdapter,
		u_int8_t *pfgResult);
	void (*wakeUpWiFi)(struct ADAPTER *prAdapter);
	bool (*isValidRegAccess)(struct ADAPTER *prAdapter,
				 uint32_t u4Register);
	void (*getMailboxStatus)(struct ADAPTER *prAdapter, uint32_t *pu4Val);
	void (*setDummyReg)(struct GLUE_INFO *prGlueInfo);
	void (*recordWFDMAIdx)(struct ADAPTER *prAdapter);
	void (*checkIdxMismatch)(u_int32_t u4Idx,
		struct RTMP_TX_RING *prTxRing);
	void (*checkDummyReg)(struct GLUE_INFO *prGlueInfo);
	void (*tx_ring_ext_ctrl)(struct GLUE_INFO *prGlueInfo,
		struct RTMP_TX_RING *tx_ring, uint32_t index);
	void (*rx_ring_ext_ctrl)(struct GLUE_INFO *prGlueInfo,
		struct RTMP_RX_RING *rx_ring, uint32_t index);
	void (*wfdmaManualPrefetch)(struct GLUE_INFO *prGlueInfo);
	void (*processSoftwareInterrupt)(struct ADAPTER *prAdapter);
	void (*softwareInterruptMcu)(struct ADAPTER *prAdapter,
		u_int32_t intrBitMask);
	uint32_t (*getMdSwIntSta)(struct ADAPTER *prAdapter);
	void (*hifRst)(struct GLUE_INFO *prGlueInfo);
	void (*initPcieInt)(struct GLUE_INFO *prGlueInfo);
	void (*powerOffPcieMac)(struct ADAPTER *prAdapter);
	void (*hwControlVote)(struct ADAPTER *prAdapter,
		uint8_t enable, uint32_t u4WifiUser);
	void (*checkFwOwnMsiStatus)(struct ADAPTER *prAdapter);
	void (*dumpPcieMsiStatus)(struct ADAPTER *prAdapter);
	void (*recoveryMsiStatus)(struct ADAPTER *prAdapter, u_int8_t fgForce);
	void (*recoverSerStatus)(struct ADAPTER *prAdapter);
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	int (*wf_power_dump_start)(void *priv_data, unsigned int force_dump);
	int (*wf_power_dump_end)(void *priv_data);
#endif
#if CFG_SUPPORT_PCIE_ASPM
	uint32_t (*configPcieAspm)(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn,
		u_int enable_role);
	void (*updatePcieAspm)(struct GLUE_INFO *prGlueInfo, u_int8_t fgEn);
	void (*keepPcieWakeup)(struct GLUE_INFO *prGlueInfo, u_int8_t fgWakeup);
	u_int8_t fgWifiEnL1_2;
	u_int8_t fgMDEnL1_2;
	u_int8_t fgWifiRstEnL1_2;
#endif
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
#if CFG_PCIE_LTR_UPDATE
	void (*pcieLTRValue)(struct ADAPTER *prAdapter,
		uint8_t ucState);
#endif
	void (*disableDevice)(struct GLUE_INFO *prGlueInfo);
	void (*clearEvtRingTillCmdRingEmpty)(struct ADAPTER *prAdapter);

	struct SW_WFDMA_INFO rSwWfdmaInfo;
#if CFG_MTK_WIFI_SW_EMI_RING
	struct SW_EMI_RING_INFO rSwEmiRingInfo;
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	void (*setPcieSpeed)(struct GLUE_INFO *prGlueInfo, uint32_t speed);
	u_int8_t pcie_current_speed;
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
	uint32_t (*setWfdmaCoalescingInt)(struct ADAPTER *prAdapter,
		u_int8_t fgEnable);
#endif

#if CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE
	u_int8_t (*checkPortForRxEventFromPse)(struct ADAPTER *prAdapter,
		uint8_t u2Port);
#endif
	struct timespec64 rHifIntTs;
	uint32_t u4EnHifIntTs;
	uint32_t u4HifIntTsCnt;

	u_int8_t fgUpdateWfdmaTh;
	uint32_t u4WfdmaTh;
	unsigned long ulRecoveryMsiCheckTime;
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

#define wifi_resource_start(d, v)  (0x18000000)
#define wifi_resource_len(d, v)    (0x100000)
#define wifi_name(d)               ("WLAN")

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
uint32_t glRegisterShutdownCB(remove_card pfShutdown);
#endif

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
void halPciePreSuspendDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void halPciePreSuspendTimeout(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo);
void halPcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser);
int32_t glBusFuncOn(void);
void glBusFuncOff(void);
uint32_t glReadPcieCfgSpace(int offset, uint32_t *value);
uint32_t glWritePcieCfgSpace(int offset, uint32_t value);
void glNotifyPciePowerDown(void);

void mtk_pci_disable_device(struct GLUE_INFO *prGlueInfo);
void mtk_pci_msi_enable_irq(uint32_t u4Irq, uint32_t u4Bit);
void mtk_pci_msi_disable_irq(uint32_t u4Irq, uint32_t u4Bit);
struct GLUE_INFO *get_glue_info_isr(void *dev_instance, int irq, int idx);
irqreturn_t mtk_pci_isr(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_thread(int irq, void *dev_instance);
uint32_t mtk_pci_read_msi_mask(struct GLUE_INFO *prGlueInfo);
void mtk_pci_msi_unmask_all_irq(struct GLUE_INFO *prGlueInfo);
void mtk_pci_enable_irq(struct GLUE_INFO *prGlueInfo);
void mtk_pci_disable_irq(struct GLUE_INFO *prGlueInfo);
uint8_t pcie_backup_config_space_settings(
	struct ADAPTER *prAdapter);
uint8_t pcie_restore_config_space_settings(
	struct ADAPTER *prAdapter);
irqreturn_t pcie_sw_int_top_handler(int irq, void *dev_instance);
irqreturn_t pcie_sw_int_thread_handler(int irq, void *dev_instance);
#if CFG_MTK_WIFI_FW_LOG_MMIO || CFG_MTK_WIFI_FW_LOG_EMI
irqreturn_t pcie_fw_log_top_handler(int irq, void *dev_instance);
irqreturn_t pcie_fw_log_thread_handler(int irq, void *dev_instance);
#endif
irqreturn_t mtk_pci_isr_tx_data0_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_tx_data1_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_tx_free_done_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_rx_data0_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_rx_data1_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_rx_event_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_tx_cmd_thread(int irq, void *dev_instance);
irqreturn_t mtk_pci_isr_lump_thread(int irq, void *dev_instance);
irqreturn_t pcie_drv_own_top_handler(int irq, void *dev_instance);
irqreturn_t pcie_drv_own_thread_handler(int irq, void *dev_instance);
#if (CFG_MTK_MDDP_SUPPORT || CFG_MTK_CCCI_SUPPORT)
irqreturn_t mtk_md_dummy_pci_interrupt(int irq, void *dev_instance);
#endif

#if CFG_SUPPORT_PCIE_ASPM
bool glBusConfigASPM(struct pci_dev *dev, int val);
bool glBusConfigASPML1SS(struct pci_dev *dev, int enable);
#endif

#if CFG_MTK_WIFI_PCIE_SUPPORT
#if CFG_SUPPORT_PCIE_GEN_SWITCH
int mtk_pcie_speed(struct pci_dev *dev, int speed);
int mtk_pcie_retrain(struct pci_dev *dev);
#endif
#endif
#if CFG_MTK_WIFI_PCIE_SR
int mtk_pcie_enter_L2(struct pci_dev *dev);
int mtk_pcie_exit_L2(struct pci_dev *dev);
#endif
#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
extern int mtk_pcie_enable_cfg_dump(int port);
extern int mtk_pcie_disable_cfg_dump(int port);
#endif /* CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG */
uint8_t halPcieIsPcieProbed(void);
u_int8_t pcie_check_status_is_linked(void);
u_int8_t mtk_get_aer_triggered(void);
void mtk_trigger_aer_slot_reset(void);
void glUpdateRxCopyMemOps(
	struct HIF_MEM_OPS *prMemOps);
int halSetMemOps(
	struct platform_device *prPlatDev,
	struct HIF_MEM_OPS *prMemOps);

#if (CFG_PCIE_GEN_SWITCH == 1)
irqreturn_t pcie_gen_switch_top_handler(int irq, void *dev_instance);
irqreturn_t pcie_gen_switch_thread_handler(int irq, void *dev_instance);

irqreturn_t pcie_gen_switch_end_top_handler(int irq, void *dev_instance);
irqreturn_t pcie_gen_switch_end_thread_handler(int irq, void *dev_instance);
void pcie_check_gen_switch_timeout(struct ADAPTER *prAdapter, uint32_t u4Reg);
void pcie_gen_switch_polling_rx_done(struct ADAPTER *prAdapter);
uint32_t *pcie_gen_switch_get_emi_add(struct ADAPTER *prAdapter);
void pcie_gen_switch_recover(struct ADAPTER *prAdapter);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#endif /* _HIF_H */
