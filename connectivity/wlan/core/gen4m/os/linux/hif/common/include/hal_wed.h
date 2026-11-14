/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   "hal_wed.h"
 *  \brief
 *
 */

#ifndef _HAL_WED_H
#define _HAL_WED_H

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
#define WARP_PROXY_NAME_MAX 128
#define TX_RING_DATA_NUM	2
#define RX_RING_DATA_NUM	2
#define WARP_RXBM_CB_NUM 12288

#define FOE_INFO_LEN	12 /* hwnat dependency */
#define MAX_RXD_SIZE	0x60
#define WED_SET_PPE_TYPE(_p, _idx)   GLUE_RX_SET_PKT_PPE_TYPE(_p, _idx)
#define WED_GET_PPE_TYPE(_p)         GLUE_RX_GET_PKT_PPE_TYPE(_p)

	/* RxDmad DW1 */
#define RXDMAD_TO_HOST (1 << 8)
#define RXDMAD_RING_INFO (1 << 9)
#define RXDMAD_TO_HOST_A (1 << 12)
#define RXDMAD_RXD_ERR (1 << 13)
#define RXDMAD_RXD_DROP (1 << 14)
#define RXDMAD_M_DONE (1 << 15)

	/* RxDmad DW2 */
#define RXDMAD_CSRN_EXT_SHIFT 11
#define RXDMAD_CSRN_EXT_MASK (0x1 << 11)

	/* RxDmad DW3 */
#define RXDMAD_PPE_VLD 31
#define RXDMAD_CSRN_MASK (0x1f << 11)
#define RXDMAD_CSRN_SHIFT 11
#define RXDMAD_PPE_ENTRY_MASK (0x7fff << 16)
#define RXDMAD_PPE_ENTRY_SHIFT 16
#define RXDMAD_TOKEN_ID_MASK (0xffff << 16)
#define RXDMAD_TOKEN_ID_SHIFT 16
#define WPDMA_OFFSET	0xd4000

#define WIFI_RING_OFFSET    0x10

#define WF_WFDMA_HOST_DMA0_PCI_BASE(addr)  \
	(addr - WF_WFDMA_HOST_DMA0_BASE + WPDMA_OFFSET)
#define WF_WFDMA_HOST_DMA1_PCI_BASE(addr)  \
	(addr - WF_WFDMA_HOST_DMA1_BASE + 0xd5000)

/*WFDMA EXT_CR*/
#define WIFI_INT_STA  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR)
#define WIFI_INT_MSK  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR)

#define WIFI_HOST_DMA0_WPDMA_GLO_CFG  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR)

#define WIFI_TX_RING0_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR)
#define WIFI_TX_RING0_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL1_ADDR)
#define WIFI_TX_RING0_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL2_ADDR)
#define WIFI_TX_RING0_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL3_ADDR)

#define WIFI_TX_RING1_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR)
#define WIFI_TX_RING1_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL1_ADDR)
#define WIFI_TX_RING1_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL2_ADDR)
#define WIFI_TX_RING1_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL3_ADDR)

#define WIFI_RX_RING2_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL0_ADDR)
#define WIFI_RX_RING2_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL1_ADDR)
#define WIFI_RX_RING2_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL2_ADDR)
#define WIFI_RX_RING2_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL3_ADDR)

#define WIFI_RX_RING3_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL0_ADDR)
#define WIFI_RX_RING3_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL1_ADDR)
#define WIFI_RX_RING3_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL2_ADDR)
#define WIFI_RX_RING3_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL3_ADDR)

#define WIFI_RX_RING4_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR)
#define WIFI_RX_RING4_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL1_ADDR)
#define WIFI_RX_RING4_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL2_ADDR)
#define WIFI_RX_RING4_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL3_ADDR)

#define WIFI_RX_RING5_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR)
#define WIFI_RX_RING5_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL1_ADDR)
#define WIFI_RX_RING5_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL2_ADDR)
#define WIFI_RX_RING5_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL3_ADDR)

#define WIFI_RX_RING6_BASE  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR)
#define WIFI_RX_RING6_CNT   \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL1_ADDR)
#define WIFI_RX_RING6_CIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL2_ADDR)
#define WIFI_RX_RING6_DIDX  \
	WF_WFDMA_HOST_DMA0_PCI_BASE(\
	WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL3_ADDR)

/* WED SER status */
#define WIFI_ERR_RECOV_STOP_IDLE		0x00
#define WIFI_ERR_RECOV_STOP_PDMA0		0x01
#define WIFI_ERR_RECOV_RESET_PDMA0		0x02
#define WIFI_ERR_RECOV_STOP_IDLE_DONE		0x03
#define WIFI_ERR_RECOV_HIF_INIT			0x04
#define WIFI_ERR_RECOV_DETACH			0x10
#define WIFI_ERR_RECOV_ATTACH			0x11
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct proxy_wlan_hook_ops {
	const char name[WARP_PROXY_NAME_MAX];
	uint32_t (*fun)(uint16_t hook, void *WedInfo, void *priv);
	uint32_t hooks;
};

enum MAC_TYPE_VER {
	MAC_TYPE_NONE,
	MAC_TYPE_FMAC,	/* Falcon */
	MAC_TYPE_BMAC,	/* Besra */
	MAC_TYPE_MAX
};

enum PROXY_WLAN_BUS_TYPE {
	BUS_TYPE_PCIE,
	BUS_TYPE_AXI,
	BUS_TYPE_MAX
};

enum PROXY_WLAN_DMA_TRX {
	DMA_TX,
	DMA_RX,
	DMA_TX_RX,
};

enum PROXY_WLAN_HOOK_PT {
	PROXY_WLAN_HOOK_HIF_INIT = 0,
	PROXY_WLAN_HOOK_HIF_EXIT,		/* 1 */
	PROXY_WLAN_HOOK_TX,			/* 2 */
	PROXY_WLAN_HOOK_RX,			/* 3 */
	PROXY_WLAN_HOOK_SYS_UP,			/* 4 */
	PROXY_WLAN_HOOK_SYS_DOWN,		/* 5 */
	PROXY_WLAN_HOOK_ISR,			/* 6 */
	PROXY_WLAN_HOOK_DMA_SET,		/* 7 */
	PROXY_WLAN_HOOK_SER,			/* 8 */
	PROXY_WLAN_HOOK_SUSPEND,		/* 9 */
	PROXY_WLAN_HOOK_RESUME,			/* 10 */
	PROXY_WLAN_HOOK_READ,			/* 11 */
	PROXY_WLAN_HOOK_WRITE,			/* 12 */
	PROXY_WLAN_HOOK_SEND_CMD,		/* 13 */
	PROXY_WLAN_HOOK_SWAP_IRQ,		/* 14 */
	PROXY_WLAN_HOOK_DEBUG,			/* 15 */
	PROXY_WLAN_HOOK_END			/* 16 */
};

enum WO_CMD_ID {
	WO_CMD_WED_START = 0x0000,
	WO_CMD_WED_CFG  = WO_CMD_WED_START,
	WO_CMD_WED_RX_STAT = 0x0001,
	WO_CMD_RRO_SER = 0x0002,
	WO_CMD_DBG_INFO = 0x0003,
	WO_CMD_DEV_INFO = 0x0004,
	WO_CMD_BSS_INFO = 0x0005,
	WO_CMD_STA_REC = 0x0006,
	WO_CMD_DEV_INFO_DUMP = 0x0007,
	WO_CMD_BSS_INFO_DUMP = 0x0008,
	WO_CMD_STA_REC_DUMP = 0x0009,
	WO_CMD_BA_INFO_DUMP = 0x000A,
	WO_CMD_FBCMD_Q_DUMP = 0x000B,
	WO_CMD_FW_LOG_CTRL = 0x000C,
	WO_CMD_LOG_FLUSH = 0x000D,
	WO_CMD_CHANGE_STATE = 0x000E,
	WO_CMD_CPU_STATS_ENABLE = 0x000F,
	WO_CMD_CPU_STATS_DUMP = 0x0010,
	WO_CMD_EXCEPTION_INIT = 0x0011,
	WO_CMD_PROF_CTRL = 0x0012,
	WO_CMD_STA_BA_DUMP = 0x0013,
	WO_CMD_BA_CTRL_DUMP = 0x0014,
	WO_CMD_RXCNT_CTRL = 0x0015,
	WO_CMD_RXCNT_INFO = 0x0016,
	WO_CMD_SET_CAP = 0x0017,
	WO_CMD_WED_END
};

enum BA_SESSION_TYPE {
	BA_SESSION_INV = 0,
	BA_SESSION_ORI = 1,
	BA_SESSION_RECP = 2,
};

/* PPE Valid type */
enum PPE_VLD_TYPE {
	RX_PPE_UNVALID,
	RX_PPE_VALID,
	PACKET_TYPE_NUM,
};

/* WED Attach type */
enum WED_ATTACH_TYPE {
	WED_ATTACH_IFON,    /* Interface on */
	WED_ATTACH_RESUME,  /* Resume */
};

/* WED Detach type */
enum WED_DETACH_TYPE {
	WED_DETACH_IFDOWN,   /* Interface down */
	WED_DETACH_SUSPEND,  /* Suspend */
};

struct ring_ctrl {
	uint32_t base;
	uint32_t cnt;
	uint32_t cidx;
	uint32_t didx;
	uint32_t attr_mask;
	uint32_t lens; /* TODO: optimize structures between proxy and gen4m*/
	dma_addr_t cb_alloc_pa;
	bool attr_enable;
};

struct WED_DMABUF {
	uint8_t fgIsSKB;
	unsigned long AllocSize;
	void *pkt;            /* RxBuf virtual address */
	void *AllocVa;        /* RxBuf virtual address + headroom offset */
	phys_addr_t AllocPa;  /* RxBuf physical address */
};

struct dma_token_que {
	uint32_t u4MaxSize;
	uint32_t u4FreeIdx;
	struct WED_DMABUF *pkt_token;
};

struct WED_CR_ACCESS {
	uint32_t reg_addr;
	uint32_t reg_val;
};

struct WED_MSDU_INFO {
	void *pPacket;
	uint8_t ucWlanIndex;
	uint8_t ucBssIndex;
	uint32_t ringIdx;
	bool fgDrop;
	void *pMsduInfo;
	uint8_t ucPrior;
	uint8_t ucUserInfo;
};

struct WED_INFO {
	enum PROXY_WLAN_BUS_TYPE infType;
	uint32_t ChipID;
	uint32_t u4IrqId;
	enum MAC_TYPE_VER macVer;
	unsigned long base_addr;
	void *pci_dev;
	bool fgMsiEnable;

	/* ring information would be assign to wifi_hw */
	unsigned long dma_offset;
	uint32_t int_sta;
	uint32_t int_mask;
	uint32_t int_enable_mask;
	uint32_t ring_offset;
	uint32_t fbuf_size;
	uint32_t int_ser;
	uint32_t int_ser_value;

	/* tx pdma/wfdma ring */
	uint32_t tx_ring_size;
	uint32_t tx_pkt_size;
	uint32_t tx_dma_glo_cfg;
	uint32_t txd_size;
	struct ring_ctrl tx_ring[TX_RING_DATA_NUM];

	/* rx pdma/wfdma ring */
	uint32_t rx_ring_size;
	uint32_t rx_pkt_size;
	uint32_t rx_dma_glo_cfg;
	uint32_t rxd_size;
	struct ring_ctrl rx_ring[RX_RING_DATA_NUM];

	/* event */
	struct ring_ctrl event;
	uint8_t wfdma_tx_done_trig0_bit;
	uint8_t wfdma_tx_done_trig1_bit;
	uint8_t wfdma_tx_done_free_notify_trig_bit;
	uint8_t wfdma_rx_done_trig0_bit;
	uint8_t wfdma_rx_done_trig1_bit;
	uint8_t wed_dma_ctrl;
	uint32_t tx_token_nums;
	bool whnat_en;
	uint8_t wed_idx;
	bool fgMirrorEnable;
	struct proxy_wlan_hook_ops *proxy_ops;
	irq_handler_t irq_handler;
	irq_handler_t irq_handler_thread;
	void (*irq_swap)(int irq, void *dev_instance);
	uint32_t (*wedRxTokenInit)(void *priv, uint8_t fgIsSKB,
		void *pkt, unsigned long alloc_size,
		void *alloc_va, phys_addr_t alloc_pa);
	void *pAdAdapter;
	void *prGlueInfo;
	bool fgAttached;
	uint8_t wed_ver;
	uint8_t wed_subver;
	unsigned long pcie_msi_msg_addr_lo;
	unsigned long pcie_msi_msg_addr_hi;

	bool fgCsrnExt;
	bool fgCopyMode;
	/* ser */
	void (*wifi_reset)(void);
};

struct WO_CMD_INFO {
	void *pMsg;
	uint32_t u4MsgLen;
	enum WO_CMD_ID wo_cmd_id;
};

struct CMD_STAREC_UPDATE_WO {
	uint8_t	ucBssIndex;
	uint8_t	ucWlanIdx;
	uint16_t u2TotalElementNum;
	uint8_t	ucAppendCmdTLV;
	uint8_t ucMuarIdx;
	uint8_t ucWlanIdxHnVer;
	uint8_t	aucReserve;
	uint8_t	aucBuffer[];
} __KAL_ATTRIB_PACKED__;

struct STAREC_COMMON_WO {
	/* Basic STA record (Group0) */
	uint16_t u2Tag;		/* Tag = 0x00 */
	uint16_t u2Length;
	uint32_t u4ConnectionType;
	uint8_t	ucConnectionState;
	uint8_t	ucIsQBSS;
	uint16_t u2AID;
	uint8_t	aucPeerMacAddr[6];
	uint16_t u2ExtraInfo;
} __KAL_ATTRIB_PACKED__;

struct STAREC_BA_WO {
	uint16_t u2Tag;       /* Tag = 0x06 */
	uint16_t u2Length;
	uint8_t  ucTid;
	uint8_t  ucBaDirection;
	uint8_t  ucAmsduCap;
	uint8_t  ucBaEenable;
	uint16_t u2BaStartSeq;
	uint16_t u2BaWinSize;
} __KAL_ATTRIB_PACKED__;

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
#define WARP_PROXY_IO_WRITE32(_G, _R, _V)\
{\
	wedProxyIoWrite(_G, _R, _V);\
}
#define WARP_PROXY_IO_READ32(_G, _R, _V)\
{\
	wedProxyIoRead(_G, _R, _V);\
}

#define WARP_TOKEN_TO_OFFSET(_prToken, _offset)\
{\
	(_prToken)->u4Token += (_prToken)->u4Token < \
	(_offset) ? (_offset) : 0;\
}
#define WARP_OFFSET_TO_TOKEN(_u4Token, _offset)\
{\
	(_u4Token) -= (_offset);\
}

#define INC_CNT(_idx)\
{\
	(_idx) = (_idx+1);\
}

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
int wedInitial(void);
int wedInitAdapterInfo(struct ADAPTER *prAdapter);
int wedProxyHookRegister(struct proxy_wlan_hook_ops *ops);
int wedProxyHookUnregister(struct proxy_wlan_hook_ops *ops);
void wedProxyIoRead(struct GLUE_INFO *prGlueInfo,
	uint32_t u4BusAddr, uint32_t *pu4Value);
void wedProxyIoWrite(struct GLUE_INFO *prGlueInfo,
	uint32_t u4BusAddr, uint32_t u4Value);
void wedAttachDetach(struct ADAPTER *prAdapter,
			struct net_device *prNetDev,
			u_int8_t fgIsAttach);
void wedSuspendResume(u_int8_t fgIsSuspend);
uint32_t wedMirrorRevert(void);
bool wedDevReadData(struct GLUE_INFO *prGlueInfo,
	uint16_t u2Port, struct SW_RFB *prSwRfb);
uint32_t wedHwTxRequest(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
uint32_t wedStaRecUpdate(struct ADAPTER *prAdapter,
	struct STA_RECORD *pStaRecCfg);
uint32_t wedStaRecRxAddBaUpdate(struct ADAPTER *prAdapter, void *ba);
void wedHwRxInfoFree(struct SW_RFB *prSwRfb);
uint32_t wedHwRxInfoWrapper(struct SW_RFB *prSwRfb);
uint32_t wedHwRxRequest(void *prSkb);
void wedSuspendTrigger(void);
void wedResumeTrigger(void);
bool IsWedAttached(void);
void wedHwRecoveryFromError(struct ADAPTER *prAdapter, uint32_t status);
int wedProxyHookCall(uint16_t hook, void *priv);
int wedShowDebugInfo(void);
uint32_t wedMirrorAddrCheck(uint32_t u4BusAddr);
int wedInfoSetup(struct ADAPTER *prAdapter);
int wedRxTokenInfoSetup(struct ADAPTER *prAdapter);

uint32_t wedRxTokenInit(void *priv, uint8_t fgIsSKB, void *pkt,
		unsigned long alloc_size, void *alloc_va, phys_addr_t alloc_pa);
struct WED_DMABUF *wedRxtokenGet(void *priv, uint32_t u4token_id);
bool wedRxBufferSwap(struct GLUE_INFO *prGlueInfo, uint32_t u4tokeID,
	struct WED_DMABUF *prWedDmaBuf, struct RXD_STRUCT *pRxD,
	struct RTMP_DMABUF *prDmaBuf, struct SW_RFB *prSwRfb);
bool wedRxSkbGen(struct GLUE_INFO *prGlueInfo,
	struct WED_DMABUF *prWedDmaBuf, void **prPacket);
void wedUpdateIntMask(uint32_t mask);
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

extern void (*ppe_dev_register_hook)(void *dev);
extern void (*ppe_dev_unregister_hook)(void *dev);
extern int (*ra_sw_nat_hook_rx)(struct sk_buff *skb);

#endif /* _HAL_WED_H */
