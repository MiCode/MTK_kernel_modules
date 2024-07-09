/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*! \file  cmm_asic_connac2x.h
*    \brief This file contains the info of CONNAC2X
*/

#ifndef _CMM_ASIC_CONNAC2X_H
#define _CMM_ASIC_CONNAC2X_H

#if (CFG_SUPPORT_CONNAC2X == 1)

#define CONN_INFRA_REMAPPING_OFFSET                    0x64000000
#define CONNAC2X_WFDMA_DISP_MAX_CNT_MASK               0x000000FF
#define CONNAC2X_WFDMA_DISP_BASE_PTR_MASK               0xFFFF0000

#define CONNAC2X_MCU_WPDMA_0_BASE                      0x54000000
#define CONNAC2X_MCU_WPDMA_1_BASE                      0x55000000
#define CONNAC2X_HOST_WPDMA_0_BASE                     0x7c024000
#define CONNAC2X_HOST_WPDMA_1_BASE                     0x7c025000
#define CONNAC2X_HOST_DMASHDL                          0x7c026000

#define CONNAC2X_WPDMA_GLO_CFG(__BASE)                ((__BASE) + 0x0208)
#define CONNAC2X_WPDMA_GLO_CFG_EXT0(__BASE)           ((__BASE) + 0x02B0)
#define CONNAC2X_TX_RING_EXT_CTRL_BASE(__BASE)        ((__BASE) + 0x0600)
#define CONNAC2X_TX_RING_DISP_MAX_CNT	  4
#define CONNAC2X_RX_RING_DISP_MAX_CNT	  4

#define CONNAC2X_RX_RING_CIDX(__BASE)                 ((__BASE) + 0x0508)
#define CONNAC2X_HOST_DMASHDL_SW_CONTROL(__BASE)      ((__BASE) + 0x0004)

/* OMIT_TX_INFO[28]*/
#define CONNAC2X_WPDMA1_GLO_CFG_OMIT_TX_INFO           0x10000000
/* OMIT_RX_INFO[27]*/
#define CONNAC2X_WPDMA1_GLO_CFG_OMIT_RX_INFO           0x08000000
/* OMIT_RX_INFO_PFET2[21]*/
#define CONNAC2X_WPDMA1_GLO_CFG_OMIT_RX_INFO_PFET2     0x00200000
/* FW_DWLD_Bypass_dmashdl[9] */
#define CONNAC2X_WPDMA1_GLO_CFG_FW_DWLD_Bypass_dmashdl 0x00000200
/* RX_DMA_BUSY[3] */
#define CONNAC2X_WPDMA1_GLO_CFG_RX_DMA_BUSY   0x00000008
/* RX_DMA_EN[2] */
#define CONNAC2X_WPDMA1_GLO_CFG_RX_DMA_EN     0x00000004
/* TX_DMA_BUSY[1] */
#define CONNAC2X_WPDMA1_GLO_CFG_TX_DMA_BUSY   0x00000002
/* TX_DMA_EN[0] */
#define CONNAC2X_WPDMA1_GLO_CFG_TX_DMA_EN     0x00000001
/* TX_DMASHDL_ENABLE[6] */
#define CONNAC2X_WPDMA1_GLO_CFG_EXT0_TX_DMASHDL_EN     0x00000040
/* DMASHDL_BYPASS[28] */
#define CONNAC2X_HIF_DMASHDL_BYPASS_EN                 0x10000000

#define CONNAC2X_NIC_TX_PSE_HEADER_LENGTH		8
#define CONNAC2X_RX_INIT_EVENT_LENGTH                  8
#define CONNAC2X_RX_EVENT_HDR_LENGTH                   12

#define CONNAC2X_WFDMA_DUMMY_CR		(CONNAC2X_MCU_WPDMA_0_BASE + 0x120)
#define CONNAC2X_WFDMA_NEED_REINIT_BIT	BIT(1)

#define CONNAC2X_HOST_CSR_TOP_BASE     (0x7c060000)
#define CONNAC2X_BN0_LPCTL_ADDR        (CONNAC2X_HOST_CSR_TOP_BASE + 0x10)
#define CONNAC2X_BN0_IRQ_STAT_ADDR     (CONNAC2X_HOST_CSR_TOP_BASE + 0x14)
#define CONNAC2X_BN0_IRQ_ENA_ADDR      (CONNAC2X_HOST_CSR_TOP_BASE + 0x18)
#define CONNAC2X_HOST_MAILBOX_WF_ADDR  (CONNAC2X_HOST_CSR_TOP_BASE + 0xfc)

/* For support mcu debug mechanism. +*/
#define PC_IDX_SWH(val, idx, mask, shift) \
	((val & (~(mask << shift))) | ((mask & idx) << shift))

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#define CONNAC2X_CONN_HIF_ON_ADDR_REMAP23              0x7010
#define CONNAC2X_HOST_EXT_CONN_HIF_WRAP                0x7c027000
#define CONNAC2X_MCU_INT_CONN_HIF_WRAP                 0x57000000
#define CONNAC2X_WFDMA_COUNT                           2

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define CONNAC2X_WPDMA_EXT_INT_STA(__BASE)            ((__BASE) + 0x0010)
#define CONNAC2X_WPDMA_EXT_INT_MASK(__BASE)           ((__BASE) + 0x0014)
#define CONNAC2X_WPDMA_HIF_RST(__BASE)                ((__BASE) + 0x0100)
#define CONNAC2X_WPDMA_HOST2MCU_SW_INT_SET(__BASE)    ((__BASE) + 0x0108)
#define CONNAC2X_WPDMA_MCU2HOST_SW_INT_STA(__BASE)    ((__BASE) + 0x01F0)
#define CONNAC2X_WPDMA_MCU2HOST_SW_INT_MASK(__BASE)   ((__BASE) + 0x01F4)
#define CONNAC2X_WPDMA_INT_STA(__BASE)                ((__BASE) + 0x0200)
#define CONNAC2X_WPDMA_INT_MASK(__BASE)               ((__BASE) + 0x0204)
#define CONNAC2X_WPDMA_RST_DTX_PTR(__BASE)            ((__BASE) + 0x020C)
#define CONNAC2X_WPDMA_RST_DRX_PTR(__BASE)            ((__BASE) + 0x0280)


#define CONNAC2X_TX_RING_BASE(__BASE)                 ((__BASE) + 0x0300)
#define CONNAC2X_TX_RING_PTR(__BASE)                  ((__BASE) + 0x0300)
#define CONNAC2X_TX_RING_CNT(__BASE)                  ((__BASE) + 0x0304)
#define CONNAC2X_TX_RING_CIDX(__BASE)                 ((__BASE) + 0x0308)
#define CONNAC2X_TX_RING_DIDX(__BASE)                 ((__BASE) + 0x030C)

#define CONNAC2X_RX_RING_BASE(__BASE)                 ((__BASE) + 0x0500)
#define CONNAC2X_RX_RING_PTR(__BASE)                  ((__BASE) + 0x0500)
#define CONNAC2X_RX_RING_CNT(__BASE)                  ((__BASE) + 0x0504)
#define CONNAC2X_RX_RING_DIDX(__BASE)                 ((__BASE) + 0x050C)
#define CONNAC2X_RX_RING_EXT_CTRL_BASE(__BASE)        ((__BASE) + 0x0680)

#define CONNAC2X_WFDMA1_RX_RING_BASE(__BASE)          ((__BASE) + 0x0500)
#define CONNAC2X_WFDMA1_RX_RING_PTR(__BASE)           ((__BASE) + 0x0500)
#define CONNAC2X_WFDMA1_RX_RING_CNT(__BASE)           ((__BASE) + 0x0504)
#define CONNAC2X_WFDMA1_RX_RING_CIDX(__BASE)          ((__BASE) + 0x0508)
#define CONNAC2X_WFDMA1_RX_RING_DIDX(__BASE)          ((__BASE) + 0x050C)
#define CONNAC2X_WFDMA1_RX_RING_EXT_CTRL_BASE(__BASE) ((__BASE) + 0x0680)

#define CONNAC2X_FWDL_TX_RING_IDX         16
#define CONNAC2X_CMD_TX_RING_IDX          17 /* Direct to WM */
#define CONNAC2X_DATA0_TXD_IDX            18 /* Band_0 TXD to WA */
#define CONNAC2X_DATA1_TXD_IDX            19 /* Band_1 TXD to WA */
#define CONNAC2X_CMD_TX_WA_RING_IDX       20 /* WA relay to WM */


/* WPDMA_INT_STA (0x50000000+0x200) */
#define CONNAC2X_WFDMA_TX_DONE_INT17             BIT(27)
#define CONNAC2X_WFDMA_TX_DONE_INT16             BIT(26)
#define CONNAC2X_WFDMA_RX_DONE_INT5              BIT(23)
#define CONNAC2X_WFDMA_RX_DONE_INT4              BIT(22)
#define CONNAC2X_TX_COHERENT_INT                 BIT(21)
#define CONNAC2X_RX_COHERENT_INT                 BIT(20)
#define CONNAC2X_WFDMA_TX_DONE_INT6              BIT(10)
#define CONNAC2X_WFDMA_TX_DONE_INT5              BIT(9)
#define CONNAC2X_WFDMA_TX_DONE_INT4              BIT(8)
#define CONNAC2X_WFDMA_TX_DONE_INT3              BIT(7)
#define CONNAC2X_WFDMA_TX_DONE_INT2              BIT(6)
#define CONNAC2X_WFDMA_TX_DONE_INT1              BIT(5)
#define CONNAC2X_WFDMA_TX_DONE_INT0              BIT(4)
#define CONNAC2X_WFDMA_RX_DONE_INT3              BIT(3)
#define CONNAC2X_WFDMA_RX_DONE_INT2              BIT(2)
#define CONNAC2X_WFDMA_RX_DONE_INT1              BIT(1)
#define CONNAC2X_WFDMA_RX_DONE_INT0              BIT(0)

/* EXT WPDMA_INT_STA (0x53000000+0x10) */
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT19        BIT(31)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT18        BIT(30)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT17        BIT(27)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT16        BIT(26)
#define CONNAC2X_EXT_WFDMA0_TX_COHERENT_INT      BIT(23)
#define CONNAC2X_EXT_WFDMA0_RX_COHERENT_INT      BIT(22)
#define CONNAC2X_EXT_WFDMA1_TX_COHERENT_INT      BIT(21)
#define CONNAC2X_EXT_WFDMA1_RX_COHERENT_INT      BIT(20)
#define CONNAC2X_EXT_WFDMA0_RX_DONE_INT3         BIT(19)
#define CONNAC2X_EXT_WFDMA0_RX_DONE_INT2         BIT(18)
#define CONNAC2X_EXT_WFDMA0_RX_DONE_INT1         BIT(17)
#define CONNAC2X_EXT_WFDMA0_RX_DONE_INT0         BIT(16)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT20        BIT(15)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT6         BIT(10)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT5         BIT(9)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT4         BIT(8)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT3         BIT(7)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT2         BIT(6)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT1         BIT(5)
#define CONNAC2X_EXT_WFDMA1_TX_DONE_INT0         BIT(4)
#define CONNAC2X_EXT_WFDMA1_RX_DONE_INT2         BIT(2)
#define CONNAC2X_EXT_WFDMA1_RX_DONE_INT1         BIT(1)
#define CONNAC2X_EXT_WFDMA1_RX_DONE_INT0         BIT(0)

/* For support mcu debug mechanism. +*/
#define PCIE_CTRL_EN		(1 << 28)
#define CONNAC2X_PCIE_CONDBGCR_CTRL		(0xE009C)
#define CONNAC2X_PCIE_CONDBGCR_DATA		(0xE0204)
#define CONNAC2X_PCIE_CONDBGCR_LR_DATA		(0xE0208)
#define CONNAC2X_PCIE_CONDBGCR_SEL		(0xE0090)
#define CONNAC2X_PCIE_MCU_PC_LOG_MASK	(0x3F)
#define CONNAC2X_PCIE_MCU_PC_LOG_SHIFT	(2)
#define CONNAC2X_PCIE_MCU_LR_LOG_MASK	(0x3F)
#define CONNAC2X_PCIE_MCU_LR_LOG_SHIFT	(8)
/* For support mcu debug mechanism. -*/

/* For support CFG_SUPPORT_DEBUG_SOP +*/
#define CONNAC2X_PCIE_DEBUG_STATUS           (0xE021C)
#define CONNAC2X_PCIE_DEBUG_SEL              (0xE0094)
#define CONNAC2X_PCIE_BT_DBG_STATUS          (0xE0238)
#define CONNAC2X_PCIE_BT_DBG_SEL             (0xE00b4)
#define CONNAC2X_PCIE_CONN_INFRA_STATUS      (0xE0288)
#define CONNAC2X_PCIE_CONN_INFRA_SEL         (0xE00A4)
#define CONNAC2X_PCIE_CONN_INFRA_BUS_STATUS  (0xE0150)
#define CONNAC2X_PCIE_CONN_INFRA_BUS_SEL     (0xE0138)

/* For CONNAC 2.5 project */
#define CONNAC25_PCIE_WF_DEBUG_STATUS         (0xE0168)
#define CONNAC25_PCIE_WF_DEBUG_SEL            (0xE0164)
#define CONNAC25_PCIE_POWER_STATUS            (0xE0294)
#define CONNAC25_PCIE_BGF_DBG_SEL             (0xE00AC)
#define CONNAC25_PCIE_BGF_DBG_STATUS          (0xE023C)
#define CONNAC25_PCIE_BGF_BUS_SEL             (0xE00A8)
#define CONNAC25_PCIE_BGF_BUS_STATUS          (0xE022C)
#define CONNAC25_PCIE_CONNINFRA_CLK_SEL       (0xE015C)
#define CONNAC25_PCIE_CONNINFRA_CLK_STATUS    (0xE02C8)
#define CONNAC25_PCIE_CONNINFRA_STRAP_STATUS  (0xE0294)


#define CONNAC2X_PCIE_DBG_BGF_MCU_SEL_CR \
	(0xE00A0)
#define CONNAC2X_PCIE_BGF_MCU_PC_DBG_STS \
	(0xE022C)
#define CONNAC2X_PCIE_MCU_BGF_ON_DBG_STS \
	(0xE0234)
#define BGF_MCU_PC_LOG_SEL	0xFFFC
/* For support CFG_SUPPORT_DEBUG_SOP -*/

#endif /* _HIF_PCIE || _HIF_AXI */

#if defined(_HIF_USB)
#define CONNAC2X_UDMA_BASE        0x74000000
#define CONNAC2X_UDMA_TX_QSEL     (CONNAC2X_UDMA_BASE + 0x08) /* 0008 */
#define CONNAC2X_UDMA_RESET       (CONNAC2X_UDMA_BASE + 0x14) /* 0014 */
#define CONNAC2X_UDMA_WLCFG_1     (CONNAC2X_UDMA_BASE + 0x0C) /* 000c */
#define CONNAC2X_UDMA_WLCFG_0     (CONNAC2X_UDMA_BASE + 0x18) /* 0018 */

/* For support mcu debug mechanism. +*/
#define USB_CTRL_EN		(1 << 31)
#define CONNAC2X_UDMA_CONDBGCR_DATA	(CONNAC2X_UDMA_BASE + 0xA18) /* 0A18 */
#define CONNAC2X_UDMA_CONDBGCR_SEL	(CONNAC2X_UDMA_BASE + 0xA1C) /* 0A1C */
#define CONNAC2X_UDMA_WM_MONITER_SEL	(~(0x40000000))
#define CONNAC2X_UDMA_PC_MONITER_SEL	(~(0x20000000))
#define CONNAC2X_UDMA_LR_MONITER_SEL    (0x20000000)
#define CONNAC2X_UDMA_MCU_PC_LOG_MASK	(0x3F)
#define CONNAC2X_UDMA_MCU_PC_LOG_SHIFT	(16)
/* For support mcu debug mechanism. -*/

/* For support CFG_SUPPORT_DEBUG_SOP +*/
#define CONNAC2X_UDMA_BT_DBG_STATUS	(CONNAC2X_UDMA_BASE + 0xA00) /* 0A00 */
#define CONNAC2X_UDMA_BT_DBG_SEL	(CONNAC2X_UDMA_BASE + 0xA04) /* 0A04 */
#define CONNAC2X_UDMA_DBG_STATUS	(CONNAC2X_UDMA_BASE + 0xA10) /* 0A10 */
#define CONNAC2X_UDMA_DBG_SEL		(CONNAC2X_UDMA_BASE + 0xA14) /* 0A14 */
/* For support CFG_SUPPORT_DEBUG_SOP -*/
#define CONNAC2X_UDMA_CONN_INFRA_STATUS (CONNAC2X_UDMA_BASE + 0xA20)  /* 0A20 */
#define CONNAC2X_UDMA_CONN_INFRA_STATUS_SEL \
	(CONNAC2X_UDMA_BASE + 0xA24) /* 0A24 */
#define CONNAC2X_UDMA_CONN_INFRA_CLK_SEL (CONNAC2X_UDMA_BASE + 0xA4C)/* 0A4C */
#define CONNAC2X_UDMA_CONN_INFRA_CLK_STATUS \
	(CONNAC2X_UDMA_BASE + 0xA48) /* 0A48 */
#define CONNAC25_UDMA_BGF_DBG_SEL (CONNAC2X_UDMA_BASE + 0xA44)  /* 0A44 */
#define CONNAC25_UDMA_BGF_DBG_STATUS \
	(CONNAC2X_UDMA_BASE + 0xA40) /* 0A40 */


#define CONNAC2X_UDMA_WLCFG_0_WL_TX_BUSY_MASK           (0x1 << 31)
#define CONNAC2X_UDMA_WLCFG_0_WL_TX_EN(p)               (((p) & 0x1) << 23)
#define CONNAC2X_UDMA_WLCFG_0_WL_RX_EN(p)               (((p) & 0x1) << 22)
#define CONNAC2X_UDMA_WLCFG_0_TICK_1US_EN_MASK          (0x1 << 20)
#define CONNAC2X_UDMA_WLCFG_0_TICK_1US_EN(p)            (((p) & 0x1) << 20)
#define CONNAC2X_UDMA_WLCFG_0_WL_RX_FLUSH_MASK          (0x1 << 19)
#define CONNAC2X_UDMA_WLCFG_0_WL_RX_MPSZ_PAD0(p)        (((p) & 0x1) << 18)
#define CONNAC2X_UDMA_WLCFG_0_WL_TX_TMOUT_FUNC_EN_MASK  (0x1 << 16)
#define CONNAC2X_UDMA_WLCFG_1_WL_TX_TMOUT_LMT_MASK      (0xFFFFF << 8)
#define CONNAC2X_UDMA_WLCFG_1_WL_TX_TMOUT_LMT(p)        (((p) & 0xFFFFF) << 8)

#define CONNAC2X_UDMA_TX_TIMEOUT_LIMIT			(50000)

#define CONNAC2X_UDMA_WL_STOP_DP_OUT_ADDR      (CONNAC2X_UDMA_BASE + 0x080)
#define CONNAC2X_UDMA_WL_STOP_DP_OUT_DROP_MASK BITS(20, 25)

#define CONNAC2X_UDMA_WL_TX_SCH_ADDR           (CONNAC2X_UDMA_BASE + 0x100)
#define CONNAC2X_UDMA_WL_TX_SCH_MASK           (BITS(28, 31) | BITS(14, 15))
#define CONNAC2X_UDMA_WL_TX_SCH_IDLE           (BIT(14) | BIT(28))

#define CONNAC2X_UDMA_AR_CMD_FIFO_ADDR         (CONNAC2X_UDMA_BASE + 0x154)
#define CONNAC2X_UDMA_AR_CMD_FIFO_MASK         BIT(26)

#define CONNAC2X_U3D_RX4CSR0 0x74011240
#define CONNAC2X_U3D_RX5CSR0 0x74011250
#define CONNAC2X_U3D_RX6CSR0 0x74011260
#define CONNAC2X_U3D_RX7CSR0 0x74011270
#define CONNAC2X_U3D_RX8CSR0 0x74011280
#define CONNAC2X_U3D_RX9CSR0 0x74011290
#define CONNAC2X_U3D_RX_FIFOEMPTY (0x1<<17)

#define WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_CSR_TX_DROP_MODE_ADDR 0x7C0242B8
#define WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_EXT2_CSR_TX_DROP_MODE_MASK 0x00030000

#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_HIF_BUSY_ADDR 0x7C027044
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HIF_MISC_HIF_BUSY_MASK 0x00000001

#define CONNAC2X_WFDMA_HOST_CONFIG_ADDR                 0x7c027030
/*
 * 0: command packet forward to TX ring 17 (WMCPU)
 * 1: forward to TX ring 20 (WACPU)
 */
#define CONNAC2X_WFDMA_HOST_CONFIG_USB_CMDPKT_DST_MASK  (0x1 << 7)
#define CONNAC2X_WFDMA_HOST_CONFIG_USB_CMDPKT_DST(p)    (((p) & 0x1) << 7)
#define CONNAC2X_USB_CMDPKT2WM	0
#define CONNAC2X_USB_CMDPKT2WA	1

#define CONNAC2X_WFDMA_HOST_CONFIG_USB_RXEVT_EP4_EN  (0x1 << 6)

#define CONNAC2X_LEN_USB_RX_PADDING_CSO          (4)	/*HW design spec */
#endif /* _HIF_USB */

#if defined(_HIF_SDIO)
#define SDIO_HIF_TXD_LEN		sizeof(struct SDIO_HIF_TX_HEADER)
#define SDIO_HIF_TXD_PKG_TYPE_SHIFT		(0)
#define SDIO_HIF_TXD_PKG_TYPE_MASK		(0x3)

/* For support mcu debug mechanism. +*/
#define SDIO_CTRL_EN                    (1 << 31)
#define CONNAC2X_SDIO_WM_MONITER_SEL    (~(0x40000000))
#define CONNAC2X_SDIO_PC_MONITER_SEL    (~(0x20000000))
#define CONNAC2X_SDIO_LR_MONITER_SEL    (0x20000000)
#define CONNAC2X_SDIO_MCU_PC_LOG_MASK	(0x3F)
#define CONNAC2X_SDIO_MCU_PC_LOG_SHIFT	(16)
/* For support mcu debug mechanism. -*/
#endif /* _HIF_SDIO */

/*------------------------------------------------------------------------*/
/* Rx descriptor field related information                                */
/*------------------------------------------------------------------------*/
/* DW 0 */
#define CONNAC2X_RXD_RX_BYTE_COUNT_MASK           BITS(0, 15)
#define CONNAC2X_RXD_RX_BYTE_COUNT_OFFSET         0

#define CONNAC2X_RXD_PKT_TYPE_MASK                BITS(27, 31)
#define CONNAC2X_RXD_PKT_TYPE_OFFSET              27

#define HAL_CONNAC2X_RXD_GET_PKT_TYPE(_prHwMacRxDesc)	\
	(((_prHwMacRxDesc)->u2DW0 &\
	CONNAC2X_RXD_PKT_TYPE_MASK) >> CONNAC2X_RXD_PKT_TYPE_OFFSET)

/*------------------------------------------------------------------------------
 * MACRO for WTBL INFO GET
 *------------------------------------------------------------------------------
 */

/* CONNAC2X */
#define CONNAC2X_WIFI_LWTBL_BASE 0x820d4000
#define CONNAC2X_WIFI_LWTBL_GROUP_MASK 0x00000007 /* GROUP[2..0] */
#define CONNAC2X_WIFI_LWTBL_GROUP_SHFT 0

#define CONNAC2X_WIFI_UWTBL_BASE 0x820c4000
#define CONNAC2X_WIFI_UWTBL_TARGET_MASK 0x80000000 /* TARGET[31] */
#define CONNAC2X_WIFI_UWTBL_TARGET_SHFT 31
#define CONNAC2X_WIFI_UWTBL_GROUP_MASK 0x0000000F /* GROUP[3..0] */
#define CONNAC2X_WIFI_UWTBL_GROUP_SHFT 0


/* UWTBL DW 5 */
#define CONNAC2X_WTBL_KEY_LINK_DW_KEY_LOC0_MASK BITS(0, 10)
#define CONNAC2X_WTBL_KEY_LINK_DW_KEY_LOC0_OFFSET 0

#define CONNAC2X_WTBL_KEY_LINK_DW_KEY_LOC1_MASK BITS(16, 26)
#define CONNAC2X_WTBL_KEY_LINK_DW_KEY_LOC1_OFFSET 16



#define CONNAC2X_LWTBL_CONFIG(_pAd, _lmacWtblDUAddr, _wlanIdx) \
	HAL_MCR_WR(_pAd, _lmacWtblDUAddr, \
		((_wlanIdx >> 7) & CONNAC2X_WIFI_LWTBL_GROUP_MASK) \
		<< CONNAC2X_WIFI_LWTBL_GROUP_SHFT)

#define CONNAC2X_LWTBL_IDX2BASE(_lmacWtblDUAddr, _wlanIdx, _DW) \
	((_lmacWtblDUAddr & 0xFFFF0000) | 0x8000 | \
	((_wlanIdx & 0x7F) << 8) | (_DW & 0x3F) << 2)

#define CONNAC2X_UWTBL_CONFIG(_pAd, _umacWtblDUAddr, _wlanIdx) \
	HAL_MCR_WR(_pAd, _umacWtblDUAddr, \
		((_wlanIdx >> 7) & CONNAC2X_WIFI_UWTBL_GROUP_MASK) \
		<< CONNAC2X_WIFI_UWTBL_GROUP_SHFT)

#define CONNAC2X_UWTBL_IDX2BASE(_umacWtblDUAddr, _wlanIdx, _DW) \
	((_umacWtblDUAddr & 0XFFFFC000) | 0x2000 | \
	((_wlanIdx & 0x7F) << 6) | (_DW & 0xF) << 2)

#define CONNAC2X_KEYTBL_CONFIG(_pAd, _umacWtblDUAddr, _key_loc) \
	HAL_MCR_WR(_pAd, _umacWtblDUAddr, \
		(CONNAC2X_WIFI_UWTBL_TARGET_MASK | \
		(((_key_loc >> 7) & CONNAC2X_WIFI_UWTBL_GROUP_MASK) \
		<< CONNAC2X_WIFI_UWTBL_GROUP_SHFT)))

#define CONNAC2X_KEYTBL_IDX2BASE(_umacWtblDUAddr, _key_loc, _DW) \
	((_umacWtblDUAddr & 0XFFFFC000) | 0x2000 | \
	((_key_loc & 0x7F) << 6) | (_DW & 0xF) << 2)

/*------------------------------------------------------------------------------
 * MACRO for decision of RXV source (RXD or RX_RPT)
 *------------------------------------------------------------------------------
 */
#define CONNAC2X_RXV_FROM_RX_RPT(_prAdapter)	\
	(((_prAdapter)->chip_info->get_rxv_from_rxrpt) ? \
	((_prAdapter)->chip_info->get_rxv_from_rxrpt(_prAdapter)) : FALSE)

/*------------------------------------------------------------------------------
 * MACRO for decision of RX debug source (Group3 or Group5)
 *------------------------------------------------------------------------------
 */
#define CONNAC2X_RX_DBG_INFO_GRP3(_prAdapter)	\
	(((_prAdapter)->chip_info->RxDbgInfoFromRxdGrp3) ? \
	((_prAdapter)->chip_info->RxDbgInfoFromRxdGrp3(_prAdapter)) : FALSE)

/*------------------------------------------------------------------------------
 * MACRO for CONNAC2X RXVECTOR Parsing
 *------------------------------------------------------------------------------
 */
/* P-RXVector, 1st Cycle */
#define CONNAC2X_RX_VT_RX_RATE_MASK         	BITS(0, 6)
#define CONNAC2X_RX_VT_RX_RATE_OFFSET       	0
#define CONNAC2X_RX_VT_NSTS_MASK            	BITS(7, 9)
#define CONNAC2X_RX_VT_NSTS_OFFSET          	7
#define CONNAC2X_RX_VT_LDPC                 	BIT(11)
#define CONNAC2X_RX_VT_MU			BIT(21)

/* P-RXVector, 1st Cycle, Modified */
#define CONNAC2X_RX_VT_FR_MODE_MASK_V2		BITS(12, 14)
#define CONNAC2X_RX_VT_FR_MODE_OFFSET_V2	12
#define CONNAC2X_RX_VT_SHORT_GI_MASK_V2		BITS(15, 16)
#define CONNAC2X_RX_VT_SHORT_GI_OFFSET_V2	15
#define CONNAC2X_RX_VT_DCM			BIT(17)
#define CONNAC2X_RX_VT_NUM_RX_MASK_V2		BITS(18, 20)
#define CONNAC2X_RX_VT_NUM_RX_OFFSET_V2		18
#define CONNAC2X_RX_VT_STBC_MASK_V2		BITS(22, 23)
#define CONNAC2X_RX_VT_STBC_OFFSET_V2		22
#define CONNAC2X_RX_VT_RX_MODE_MASK_V2		BITS(24, 27)
#define CONNAC2X_RX_VT_RX_MODE_OFFSET_V2	24

/* C-RXC Vector, 1st Cycle */
#define CONNAC2X_RX_VT_STBC_MASK            BITS(0, 1)
#define CONNAC2X_RX_VT_STBC_OFFSET          0
#define CONNAC2X_RX_VT_RX_MODE_MASK         BITS(4, 7)
#define CONNAC2X_RX_VT_RX_MODE_OFFSET       4
#define CONNAC2X_RX_VT_FR_MODE_MASK         BITS(8, 10)
#define CONNAC2X_RX_VT_FR_MODE_OFFSET       8
#define CONNAC2X_RX_VT_SHORT_GI_MASK		BITS(13, 14)
#define CONNAC2X_RX_VT_SHORT_GI_OFFSET		13

/* C-RXC Vector, 2nd Cycle */
#define CONNAC2X_RX_VT_GROUP_ID_MASK        BITS(22, 27)
#define CONNAC2X_RX_VT_GROUP_ID_OFFSET      22
#define CONNAC2X_RX_VT_NUM_RX_MASK		BITS(28, 30)
#define CONNAC2X_RX_VT_NUM_RX_OFFSET		28

/* C-RXC Vector, 4th Cycle */
#define CONNAC2X_RX_VT_RCPI0_MASK             BITS(0, 7)
#define CONNAC2X_RX_VT_RCPI0_OFFSET           0
#define CONNAC2X_RX_VT_RCPI1_MASK             BITS(8, 15)
#define CONNAC2X_RX_VT_RCPI1_OFFSET           8
#define CONNAC2X_RX_VT_RCPI2_MASK             BITS(16, 23)
#define CONNAC2X_RX_VT_RCPI2_OFFSET           16
#define CONNAC2X_RX_VT_RCPI3_MASK             BITS(24, 31)
#define CONNAC2X_RX_VT_RCPI3_OFFSET           24

#define CONNAC2X_HAL_RX_VECTOR_GET_RX_VECTOR(_prHwRxVector, _ucIdx) \
	((_prHwRxVector)->u4RxVector[_ucIdx])

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI0(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[6]) & CONNAC2X_RX_VT_RCPI0_MASK) >> \
	CONNAC2X_RX_VT_RCPI0_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI1(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[6]) & CONNAC2X_RX_VT_RCPI1_MASK) >> \
	CONNAC2X_RX_VT_RCPI1_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[6]) & CONNAC2X_RX_VT_RCPI2_MASK) >> \
	CONNAC2X_RX_VT_RCPI2_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI3(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[6]) & CONNAC2X_RX_VT_RCPI3_MASK) >> \
	CONNAC2X_RX_VT_RCPI3_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_NUM_RX(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[2]) & CONNAC2X_RX_VT_NUM_RX_MASK) >> \
	CONNAC2X_RX_VT_NUM_RX_OFFSET)

#define CONNAC2X_HAL_RXV_GET_RCPI0_RXRPT(_RxvDw6)	\
	(((_RxvDw6) & CONNAC2X_RX_VT_RCPI0_MASK) >> CONNAC2X_RX_VT_RCPI0_OFFSET)

#define CONNAC2X_HAL_RXV_GET_RCPI1_RXRPT(_RxvDw6)	\
	(((_RxvDw6) & CONNAC2X_RX_VT_RCPI1_MASK) >> CONNAC2X_RX_VT_RCPI1_OFFSET)

#define CONNAC2X_HAL_RXV_GET_RCPI2_RXRPT(_RxvDw6)	\
	(((_RxvDw6) & CONNAC2X_RX_VT_RCPI2_MASK) >> CONNAC2X_RX_VT_RCPI2_OFFSET)

#define CONNAC2X_HAL_RXV_GET_RCPI3_RXRPT(_RxvDw6)	\
	(((_RxvDw6) & CONNAC2X_RX_VT_RCPI3_MASK) >> CONNAC2X_RX_VT_RCPI3_OFFSET)

#define CONNAC2X_HAL_RXV_GET_NUM_RX_RXRPT(_RxvDw2)	\
	(((_RxvDw2) & CONNAC2X_RX_VT_NUM_RX_MASK) >>	\
	CONNAC2X_RX_VT_NUM_RX_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI0_V2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[1]) & CONNAC2X_RX_VT_RCPI0_MASK) >> \
	CONNAC2X_RX_VT_RCPI0_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI1_V2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[1]) & CONNAC2X_RX_VT_RCPI1_MASK) >> \
	CONNAC2X_RX_VT_RCPI1_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI2_V2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[1]) & CONNAC2X_RX_VT_RCPI2_MASK) >> \
	CONNAC2X_RX_VT_RCPI2_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_RCPI3_V2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[1]) & CONNAC2X_RX_VT_RCPI3_MASK) >> \
	CONNAC2X_RX_VT_RCPI3_OFFSET)

#define CONNAC2X_HAL_RX_VECTOR_GET_NUM_RX_V2(_prHwRxVector) \
	((((_prHwRxVector)->u4RxVector[0]) & CONNAC2X_RX_VT_NUM_RX_MASK_V2) >> \
	CONNAC2X_RX_VT_NUM_RX_OFFSET_V2)


#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#define HAL_IS_CONNAC2X_EXT_TX_DONE_INTR(u4IntrStatus, __u4IntrBits) \
	((u4IntrStatus & (__u4IntrBits)) ? TRUE : FALSE)

#define HAL_IS_CONNAC2X_EXT_RX_DONE_INTR(u4IntrStatus, __u4IntrBits) \
	((u4IntrStatus & (__u4IntrBits)) ? TRUE : FALSE)
#endif /* defined(_HIF_PCIE) || defined(_HIF_AXI) */


/*------------------------------------------------------------------------------
 * MACRO for CONNAC2X WTBL TX RATE
 *------------------------------------------------------------------------------
 */
#define CONNAC2X_HW_TX_RATE_TO_MODE(_x)		(((_x) & (0xf << 6)) >> 6)
#define CONNAC2X_HW_TX_RATE_TO_NSS(_x)		(((_x) & (0x7 << 10)) >> 10)
#define CONNAC2X_HW_TX_RATE_TO_STBC(_x)		(((_x) & (0x1 << 13)) >> 13)
#define CONNAC2X_HW_TX_RATE_TO_DCM(_x)		(((_x) & (0x1 << 4)) >> 4)
#define CONNAC2X_HW_TX_RATE_TO_106T(_x)		(((_x) & (0x1 << 5)) >> 5)
#define CONNAC2X_HW_TX_RATE_UNMASK_DCM(_x)	((uint8_t)(_x) & 0xef)
#define CONNAC2X_HW_TX_RATE_UNMASK_106T(_x)	((uint8_t)(_x) & 0xdf)

/*------------------------------------------------------------------------------
 * MACRO for CONNAC2X TXV
 *------------------------------------------------------------------------------
 */
#define CONNAC2X_TXV_GET_TX_RATE(_x)	((_x)->u4TxV[2] & 0x7f)
#define CONNAC2X_TXV_GET_TX_LDPC(_x)	(((_x)->u4TxV[2] & (0x1 << 7)) >> 7)
#define CONNAC2X_TXV_GET_TX_STBC(_x)	(((_x)->u4TxV[0] & (0x3 << 6)) >> 6)
#define CONNAC2X_TXV_GET_TX_FRMODE(_x)	(((_x)->u4TxV[0] & (0x7 << 8)) >> 8)
#define CONNAC2X_TXV_GET_TX_MODE(_x)	(((_x)->u4TxV[0] & (0xf << 12)) >> 12)
#define CONNAC2X_TXV_GET_TX_NSTS(_x)	(((_x)->u4TxV[2] & (0x7 << 8)) >> 8)
#define CONNAC2X_TXV_GET_TX_PWR(_x)	(((_x)->u4TxV[0] & (0xff << 16)) >> 16)
#define CONNAC2X_TXV_GET_TX_SGI(_x)	(((_x)->u4TxV[1] & (0x3 << 26)) >> 26)
#define CONNAC2X_TXV_GET_TX_DCM(_x)	(((_x)->u4TxV[2] & (0x1 << 4)) >> 4)
#define CONNAC2X_TXV_GET_TX_106T(_x)	(((_x)->u4TxV[2] & (0x1 << 5)) >> 5)
#define CONNAC2X_TXV_GET_TX_RATE_UNMASK_DCM(_r)		((uint8_t)(_r) & 0xef)
#define CONNAC2X_TXV_GET_TX_RATE_UNMASK_106T(_r)	((uint8_t)(_r) & 0xdf)

#if (CFG_SUPPORT_PERF_IND == 1)
#define PERF_IND_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define PERF_IND_RX_VT_RX_RATE_OFFSET       0
#define PERF_IND_RX_VT_NSTS_MASK            BITS(7, 9)
#define PERF_IND_RX_VT_NSTS_OFFSET          7
#define PERF_IND_RX_VT_BF_MASK              BIT(10)
#define PERF_IND_RX_VT_BF_OFFSET            10
#define PERF_IND_RX_VT_LDPC_MASK            BIT(11)
#define PERF_IND_RX_VT_LDPC_OFFSET          11
#define PERF_IND_RX_VT_FR_MODE_MASK         BITS(12, 14)
#define PERF_IND_RX_VT_FR_MODE_OFFSET       12
#define PERF_IND_RX_VT_GI_MASK              BITS(15, 16)
#define PERF_IND_RX_VT_GI_OFFSET            15
#define PERF_IND_RX_VT_DCM_MASK             BIT(17)
#define PERF_IND_RX_VT_DCM_OFFSET           17
#define PERF_IND_RX_VT_NUMRX_MASK           BITS(18, 20)
#define PERF_IND_RX_VT_NUMRX_OFFSET         18
#define PERF_IND_RX_VT_MUMIMO_MASK          BIT(21)
#define PERF_IND_RX_VT_MUMIMO_OFFSET        21
#define PERF_IND_RX_VT_STBC_MASK            BITS(22, 23)
#define PERF_IND_RX_VT_STBC_OFFSET          22
#define PERF_IND_RX_VT_TXMODE_MASK          BITS(24, 27)
#define PERF_IND_RX_VT_TXMODE_OFFSET        24

#define PERF_IND_RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_RX_RATE_MASK)	\
			 >> PERF_IND_RX_VT_RX_RATE_OFFSET)

#define PERF_IND_RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_NSTS_MASK)	\
			 >> PERF_IND_RX_VT_NSTS_OFFSET)

#define PERF_IND_RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_FR_MODE_MASK)	\
			 >> PERF_IND_RX_VT_FR_MODE_OFFSET)

#define PERF_IND_RXV_GET_GI(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_GI_MASK)	\
			 >> PERF_IND_RX_VT_GI_OFFSET)

#define PERF_IND_RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_STBC_MASK)	\
			 >> PERF_IND_RX_VT_STBC_OFFSET)

#define PERF_IND_RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & PERF_IND_RX_VT_TXMODE_MASK)	\
			 >> PERF_IND_RX_VT_TXMODE_OFFSET)
#endif

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

struct CONNAC2X_WIFI_CMD {
	struct HW_MAC_CONNAC2X_TX_DESC rWifiCmdTxD;

	uint16_t u2Length;
	uint16_t u2PqId;

	uint8_t ucCID;
	uint8_t ucPktTypeID;	/* Must be 0x20 (CMD Packet) */
	uint8_t ucSetQuery;
	uint8_t ucSeqNum;

	/* padding fields, hw may auto modify this field */
	uint8_t ucD2B0Rev;
	uint8_t ucExtenCID;	/* Extend CID */
	uint8_t ucS2DIndex;	/* Index for Src to Dst in CMD usage */
	uint8_t ucExtCmdOption;	/* Extend CID option */

	uint8_t ucCmdVersion;
	uint8_t ucReserved2[3];
	uint32_t au4Reserved3[4];	/* padding fields */

	uint8_t aucBuffer[0];
};

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
struct CONNAC2X_WIFI_UNI_CMD {
	struct HW_MAC_CONNAC2X_TX_DESC rWifiCmdTxD;

	uint16_t u2Length;
	uint16_t u2CID;

	uint8_t aucReserved[1];
	uint8_t ucPktTypeID;	/* Must be 0xA0 (CMD Packet) */
	uint8_t ucFragNum;
	uint8_t ucSeqNum;

	uint16_t u2Checksum;
	uint8_t ucS2DIndex;	/* Index for Src to Dst in CMD usage */
	uint8_t ucOption;	/* CID option */

	uint8_t aucReserved2[4];
	uint8_t aucBuffer[0];
};
#endif

union WTBL_LMAC_DW0 {
	struct {
		uint32_t addr_4:8;
		uint32_t addr_5:8;
		uint32_t muar_idx:6;
		uint32_t rc_a1:1;
		uint32_t kid:2;
		uint32_t rc_id:1;
		uint32_t fd:1;
		uint32_t td:1;
		uint32_t rv:1;
		uint32_t rc_a2:1;
		uint32_t wpi_flg:1;
		uint32_t pad:1;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW1 {
	struct {
		uint32_t addr_0;
	} field;
	uint32_t word;
};

enum WTBL_LMAC_CIPHER_SUIT {
	WTBL_CIPHER_NONE = 0,
	WTBL_CIPHER_WEP_40 = 1,
	WTBL_CIPHER_TKIP_MIC = 2,
	WTBL_CIPHER_TKIP_NO_MIC = 3,
	WTBL_CIPHER_CCMP_128_PMF = 4,
	WTBL_CIPHER_WEP_104 = 5,
	WTBL_CIPHER_BIP_CMAC_128 = 6,
	WTBL_CIPHER_WEP_128 = 7,
	WTBL_CIPHER_WPI_128 = 8,
	WTBL_CIPHER_CCMP_128_CCX = 9,
	WTBL_CIPHER_CCMP_256 = 10,
	WTBL_CIPHER_GCMP_128 = 11,
	WTBL_CIPHER_GCMP_256 = 12,
	WTBL_CIPHER_GCMP_WPI_128 = 13,
};

union WTBL_LMAC_DW2 {
	struct {
		uint32_t aid12:12;
		uint32_t gid_su:1;
		uint32_t spp_en:1;
		uint32_t wpi_even:1;
		uint32_t aad_om:1;
		uint32_t cipher_suit:5;
		uint32_t cipher_suit_igtk:2;
		uint32_t rsvd:1;
		uint32_t sw:1;
		uint32_t ul:1;
		uint32_t tx_ps:1;
		uint32_t qos:1;
		uint32_t ht:1;
		uint32_t vht:1;
		uint32_t he:1;
		uint32_t mesh:1;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW3 {
	struct {
		uint32_t wmm_q:2;
		uint32_t rxd_dup_mode:2;
		uint32_t vlan_2e_th:1;
		uint32_t pad:3;
		uint32_t pfmu_index:8;
		uint32_t bf_rsvd:8;
		uint32_t ribf:1;
		uint32_t bf_rsvd2:4;
		uint32_t tebf:1;
		uint32_t tebf_vht:1;
		uint32_t tebf_he:1;
	} field;

	struct {
		uint32_t wmm_q:2;
		uint32_t rxd_dup_mode:2;
		uint32_t vlan_2e_th:1;
		uint32_t beam_chg:1;
		uint32_t ba_mode:2;
		uint32_t pfmu_index:8;
		uint32_t ulpf_index:8;
		uint32_t ribf:1;
		uint32_t ulpf:1;
		uint32_t ign_fbk:1;
		uint32_t bf_rsvd2:2;
		uint32_t tebf:1;
		uint32_t tebf_vht:1;
		uint32_t tebf_he:1;
	} field_v2;
	uint32_t word;
};

union WTBL_LMAC_DW4 {
	struct {
		uint32_t ant_id_sts0:3;
		uint32_t ant_id_sts1:3;
		uint32_t ant_id_sts2:3;
		uint32_t ant_id_sts3:3;
		uint32_t ant_id_sts4:3;
		uint32_t ant_id_sts5:3;
		uint32_t ant_id_sts6:3;
		uint32_t ant_id_sts7:3;
		uint32_t cascad:1;
		uint32_t ldpc_ht:1;
		uint32_t ldpc_vht:1;
		uint32_t ldpc_he:1;
		uint32_t dis_rhtr:1;
		uint32_t all_ack:1;
		uint32_t drop:1;
		uint32_t ack_en:1;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW5 {
	struct {
		uint32_t af:3;
		uint32_t af_he:2;
		uint32_t rts:1;
		uint32_t smps:1;
		uint32_t dyn_bw:1;
		uint32_t mmss:3;
		uint32_t usr:1;
		uint32_t sr_r:2;
		uint32_t beam_chg:1;
		uint32_t sr_abort:1;
		uint32_t tx_power_offset:6;
		uint32_t mpdu_size:2;
		uint32_t pe:2;
		uint32_t doppl:1;
		uint32_t txop_ps_cap:1;
		uint32_t du_i_psm:1;
		uint32_t i_psm:1;
		uint32_t psm:1;
		uint32_t skip_tx:1;
	} field;

	struct {
		uint32_t af:3;
		uint32_t af_he:2;
		uint32_t rts:1;
		uint32_t smps:1;
		uint32_t dyn_bw:1;
		uint32_t mmss:3;
		uint32_t usr:1;
		uint32_t sr_r:3;
		uint32_t sr_abort:1;
		uint32_t tx_power_offset:6;
		uint32_t mpdu_size:2;
		uint32_t pe:2;
		uint32_t doppl:1;
		uint32_t txop_ps_cap:1;
		uint32_t du_i_psm:1;
		uint32_t i_psm:1;
		uint32_t psm:1;
		uint32_t skip_tx:1;
	} field_v2;
	uint32_t word;
};

union WTBL_LMAC_DW6 {
	struct {
		uint32_t ba_win_size_tid0:4;
		uint32_t ba_win_size_tid1:4;
		uint32_t ba_win_size_tid2:4;
		uint32_t ba_win_size_tid3:4;
		uint32_t ba_win_size_tid4:4;
		uint32_t ba_win_size_tid5:4;
		uint32_t ba_win_size_tid6:4;
		uint32_t ba_win_size_tid7:4;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW7 {
	struct {
		uint32_t cb_rn:3;
		uint32_t dbnss_en:1;
		uint32_t bafen:1;
		uint32_t rdg_ba:1;
		uint32_t r:1;
		uint32_t spe_idx:5;
		uint32_t g2:1;
		uint32_t g4:1;
		uint32_t g8:1;
		uint32_t g16:1;
		uint32_t g2_ltf:2;
		uint32_t g4_ltf:2;
		uint32_t g8_ltf:2;
		uint32_t g16_ltf:2;
		uint32_t g2_he:2;
		uint32_t g4_he:2;
		uint32_t g8_he:2;
		uint32_t g16_he:2;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW8 {
	struct {
		uint32_t rts_fail_cnt_ac0:5;
		uint32_t rts_fail_cnt_ac1:5;
		uint32_t rts_fail_cnt_ac2:5;
		uint32_t rts_fail_cnt_ac3:5;
		uint32_t partial_aid:9;
		uint32_t pad:2;
		uint32_t chk_per:1;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW9 {
	struct {
		uint32_t rx_avg_mpdu_size:14;
		uint32_t pad:4;
		uint32_t pritx_dcm:1;
		uint32_t pritx_er160:1;
		uint32_t pritx_ersu:1;
		uint32_t fcap:2;
		uint32_t mpdu_fail_cnt:3;
		uint32_t mpdu_ok_cnt:3;
		uint32_t rate_idx:3;
	} field;

	struct {
		uint32_t rx_avg_mpdu_size:14;
		uint32_t pad:2;
		uint32_t pritx_sw_mode:1;
		uint32_t pritx_plr:1;
		uint32_t pritx_dcm:1;
		uint32_t pritx_er160:1;
		uint32_t pritx_ersu:1;
		uint32_t fcap:2;
		uint32_t mpdu_fail_cnt:3;
		uint32_t mpdu_ok_cnt:3;
		uint32_t rate_idx:3;
	} field_v2;
	uint32_t word;
};

union WTBL_LMAC_DW10 {
	struct {
		uint32_t rate1:14;
		uint32_t pad:2;
		uint32_t rate2:14;
		uint32_t pad2:2;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW11 {
	struct {
		uint32_t rate3:14;
		uint32_t pad:2;
		uint32_t rate4:14;
		uint32_t pad2:2;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW12 {
	struct {
		uint32_t rate5:14;
		uint32_t pad:2;
		uint32_t rate6:14;
		uint32_t pad2:2;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW13 {
	struct {
		uint32_t rate7:14;
		uint32_t pad:2;
		uint32_t rate8:14;
		uint32_t pad2:2;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW14 {
	struct {
		uint32_t rate_1_tx_cnt:16;
		uint32_t rate_1_fail_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW15 {
	struct {
		uint32_t rate_2_ok_cnt:16;
		uint32_t rate_3_ok_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW16 {
	struct {
		uint32_t current_bw_tx_cnt:16;
		uint32_t current_bw_fail_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW17 {
	struct {
		uint32_t other_bw_tx_cnt:16;
		uint32_t other_bw_fail_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW18 {
	struct {
		uint32_t rts_ok_cnt:16;
		uint32_t rts_fail_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW19 {
	struct {
		uint32_t data_retry_cnt:16;
		uint32_t mgnt_retry_cnt:16;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW20 {
	uint32_t word;
};

union WTBL_LMAC_DW21 {
	uint32_t word;
};

union WTBL_LMAC_DW22 {
	uint32_t word;
};

union WTBL_LMAC_DW23 {
	uint32_t word;
};

union WTBL_LMAC_DW24 {
	uint32_t word;
};

union WTBL_LMAC_DW25 {
	uint32_t word;
};

union WTBL_LMAC_DW26 {
	uint32_t word;
};

union WTBL_LMAC_DW27 {
	uint32_t word;
};

union WTBL_LMAC_DW28 {
	struct {
		uint32_t usr_rssi:9;
		uint32_t usr_snr:6;
		uint32_t pad:1;
		uint32_t rapid_reaction_rate:11;
		uint32_t pad2:3;
		uint32_t ht_amsdu:1;
		uint32_t amsdu_cros_lg:1;
	} field;

	struct {
		uint32_t om_info:12;
		uint32_t rxd_dup_om_chg:1;
		uint32_t pad:19;
	} field_v2;
	uint32_t word;
};

union WTBL_LMAC_DW29 {
	struct {
		uint32_t resp_rcpi_0:8;
		uint32_t resp_rcpi_1:8;
		uint32_t resp_rcpi_2:8;
		uint32_t resp_rcpi_3:8;
	} field;

	struct {
		uint32_t usr_rssi:9;
		uint32_t usr_snr:6;
		uint32_t pad:1;
		uint32_t rapid_reaction_rate:11;
		uint32_t pad2:3;
		uint32_t ht_amsdu:1;
		uint32_t amsdu_cros_lg:1;
	} field_2;
	uint32_t word;
};

union WTBL_LMAC_DW30 {
	struct {
		uint32_t resp_rcpi_4:8;
		uint32_t resp_rcpi_5:8;
		uint32_t resp_rcpi_6:8;
		uint32_t resp_rcpi_7:8;
	} field;

	struct {
		uint32_t resp_rcpi_0:8;
		uint32_t resp_rcpi_1:8;
		uint32_t resp_rcpi_2:8;
		uint32_t resp_rcpi_3:8;
	} field_v2;
	uint32_t word;
};

union WTBL_LMAC_DW31 {
	struct {
		uint32_t snr_rx0:6;
		uint32_t snr_rx1:6;
		uint32_t snr_rx2:6;
		uint32_t snr_rx3:6;
		uint32_t pad:8;
	} field;
	uint32_t word;
};

union WTBL_LMAC_DW32 {
	struct {
		uint32_t snr_rx4:6;
		uint32_t snr_rx5:6;
		uint32_t snr_rx6:6;
		uint32_t snr_rx7:6;
		uint32_t pad:8;
	} field;
	uint32_t word;
};

struct wtbl_rx_stat {
	union WTBL_LMAC_DW28 wtbl_d28;
	union WTBL_LMAC_DW29 wtbl_d29;
	union WTBL_LMAC_DW30 wtbl_d30;
	union WTBL_LMAC_DW31 wtbl_d31;
	union WTBL_LMAC_DW32 wtbl_d32;
};

struct wtbl_adm_ctrl {
	union WTBL_LMAC_DW20 wtbl_d20;
	union WTBL_LMAC_DW21 wtbl_d21;
	union WTBL_LMAC_DW22 wtbl_d22;
	union WTBL_LMAC_DW23 wtbl_d23;
	union WTBL_LMAC_DW24 wtbl_d24;
	union WTBL_LMAC_DW25 wtbl_d25;
	union WTBL_LMAC_DW26 wtbl_d26;
	union WTBL_LMAC_DW27 wtbl_d27;
};

struct wtbl_ppdu_cnt {
	union WTBL_LMAC_DW19 wtbl_d19;
};

struct wtbl_auto_rate_cnt {
	union WTBL_LMAC_DW14 wtbl_d14;
	union WTBL_LMAC_DW15 wtbl_d15;
	union WTBL_LMAC_DW16 wtbl_d16;
	union WTBL_LMAC_DW17 wtbl_d17;
	union WTBL_LMAC_DW18 wtbl_d18;
};

struct wtbl_auto_rate_tb {
	union WTBL_LMAC_DW10 wtbl_d10;
	union WTBL_LMAC_DW11 wtbl_d11;
	union WTBL_LMAC_DW12 wtbl_d12;
	union WTBL_LMAC_DW13 wtbl_d13;
};

struct wtbl_tx_rx_cap {
	union WTBL_LMAC_DW2 wtbl_d2;
	union WTBL_LMAC_DW3 wtbl_d3;
	union WTBL_LMAC_DW4 wtbl_d4;
	union WTBL_LMAC_DW5 wtbl_d5;
	union WTBL_LMAC_DW6 wtbl_d6;
	union WTBL_LMAC_DW7 wtbl_d7;
	union WTBL_LMAC_DW8 wtbl_d8;
	union WTBL_LMAC_DW9 wtbl_d9;
};

struct wtbl_basic_info {
	union WTBL_LMAC_DW0 wtbl_d0;
	union WTBL_LMAC_DW1 wtbl_d1;
};

struct fwtbl_lmac_struct {
	struct wtbl_basic_info peer_basic_info;
	struct wtbl_tx_rx_cap trx_cap;
	struct wtbl_auto_rate_tb auto_rate_tb;
	struct wtbl_auto_rate_cnt auto_rate_counters;
	struct wtbl_ppdu_cnt ppdu_counters;
	struct wtbl_adm_ctrl adm_ctrl;
	struct wtbl_rx_stat rx_stat;
};

union WTBL_UMAC_DW0 {
	struct {
		uint32_t pn0;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW1 {
	struct {
		uint32_t pn1:16;
		uint32_t com_sn:12;
		uint32_t pad:4;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW2 {
	struct {
		uint32_t ac0_sn:12;
		uint32_t ac1_sn:12;
		uint32_t ac2_sn:8;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW3 {
	struct {
		uint32_t ac2_sn:4;
		uint32_t ac3_sn:12;
		uint32_t ac4_sn:12;
		uint32_t ac5_sn:4;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW4 {
	struct {
		uint32_t ac5_sn:8;
		uint32_t ac6_sn:12;
		uint32_t ac7_sn:12;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW5 {
	struct {
		uint32_t key_loc0:11;
		uint32_t pad:5;
		uint32_t key_loc1:11;
		uint32_t qos:1;
		uint32_t ht:1;
		uint32_t pad2:3;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW6 {
	struct {
		uint32_t hw_amsdu_cfg:10;
		uint32_t pad:22;
	} field;
	uint32_t word;
};

union WTBL_UMAC_DW7 {
	struct {
		uint32_t pad:32;
	} field;
	uint32_t word;
};

struct wtbl_key_tb {
	union WTBL_UMAC_DW7 wtbl_d7;
};

struct wtbl_keylink_amsdu {
	union WTBL_UMAC_DW5 wtbl_d5;
	union WTBL_UMAC_DW6 wtbl_d6;
};

struct wtbl_serial_num {
	union WTBL_UMAC_DW0 wtbl_d0;
	union WTBL_UMAC_DW1 wtbl_d1;
	union WTBL_UMAC_DW2 wtbl_d2;
	union WTBL_UMAC_DW3 wtbl_d3;
	union WTBL_UMAC_DW4 wtbl_d4;
};

struct fwtbl_umac_struct {
	struct wtbl_serial_num serial_no;
	struct wtbl_keylink_amsdu klink_amsdu;
	struct wtbl_key_tb key_tb;
};

#if defined(_HIF_SDIO)
struct SDIO_HIF_TX_HEADER {
	uint16_t     InfoBufLen;
	uint8_t      Type;
	uint8_t      Reserved;
};

#endif /* defined(_HIF_SDIO) */

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void asicConnac2xCapInit(
	struct ADAPTER *prAdapter);
void asicConnac2xFillInitCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	uint16_t *pu2BufInfoLen,
	uint8_t *pucSeqNum,
	void **pCmdBuf);
void asicConnac2xFillCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_CMD_INFO *prCmdInfo,
	uint8_t *pucSeqNum,
	void **pCmdBuf);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void asicConnac2xFillUniCmdTxd(
	struct ADAPTER *prAdapter,
	struct WIFI_UNI_CMD_INFO *prCmdInfo,
	uint8_t *pucSeqNum,
	void **pCmdBuf);
#endif

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
uint32_t asicConnac2xWfdmaCfgAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx);
uint32_t asicConnac2xWfdmaIntRstDtxPtrAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx);
uint32_t asicConnac2xWfdmaIntRstDrxPtrAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx);
uint32_t asicConnac2xWfdmaHifRstAddrGet(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx);
void asicConnac2xWpdmaConfig(
	struct GLUE_INFO *prGlueInfo,
	uint8_t enable);
void asicConnac2xWfdmaStop(
	struct GLUE_INFO *prGlueInfo,
	uint8_t enable);
uint8_t asicConnac2xWfdmaWaitIdle(
	struct GLUE_INFO *prGlueInfo,
	uint8_t index,
	uint32_t round,
	uint32_t wait_us);
u_int8_t asicConnac2xWfdmaPollingAllIdle(
	struct GLUE_INFO *prGlueInfo);
void asicConnac2xWfdmaTxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_TX_RING *tx_ring,
	uint32_t index);
void asicConnac2xWfdmaRxRingExtCtrl(
	struct GLUE_INFO *prGlueInfo,
	struct RTMP_RX_RING *rx_ring,
	uint32_t index);
void asicConnac2xWfdmaManualPrefetch(
	struct GLUE_INFO *prGlueInfo);
void asicConnac2xEnablePlatformIRQ(
	struct ADAPTER *prAdapter);
void asicConnac2xDisablePlatformIRQ(
	struct ADAPTER *prAdapter);
void asicConnac2xEnableExtInterrupt(
	struct ADAPTER *prAdapter);
void asicConnac2xDisableExtInterrupt(
	struct ADAPTER *prAdapter);
void asicConnac2xProcessTxInterrupt(
	struct ADAPTER *prAdapter);
void asicConnac2xLowPowerOwnRead(
	struct ADAPTER *prAdapter,
	uint8_t *pfgResult);
void asicConnac2xLowPowerOwnSet(
	struct ADAPTER *prAdapter,
	uint8_t *pfgResult);
void asicConnac2xLowPowerOwnClear(
	struct ADAPTER *prAdapter,
	uint8_t *pfgResult);
uint32_t asicConnac2xGetSoftwareInterrupt(
	struct ADAPTER *prAdapter);
void asicConnac2xClearSoftwareInterrupt(
	struct ADAPTER *prAdapter,
	uint32_t u4Status);
void asicConnac2xProcessSoftwareInterrupt(
	struct ADAPTER *prAdapter);
void asicConnac2xSoftwareInterruptMcu(
	struct ADAPTER *prAdapter, u_int32_t intrBitMask);
void asicConnac2xHifRst(
	struct GLUE_INFO *prGlueInfo);
void asicConnac2xReadExtIntStatus(
	struct ADAPTER *prAdapter,
	uint32_t *pu4IntStatus);
void asicConnac2xProcessRxInterrupt(
	struct ADAPTER *prAdapter);
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
void asicConnac2xWfdmaInitForUSB(
	struct ADAPTER *prAdapter,
	struct mt66xx_chip_info *prChipInfo);
void asicConnac2xUsbRxEvtEP4Setting(
	struct ADAPTER *prAdapter,
	u_int8_t fgEnable);
uint8_t asicConnac2xUsbEventEpDetected(
	struct ADAPTER *prAdapter);
void asicConnac2xEnableUsbCmdTxRing(
	struct ADAPTER *prAdapter,
	u_int8_t ucDstRing);
#if CFG_ENABLE_FW_DOWNLOAD
void asicConnac2xEnableUsbFWDL(
	struct ADAPTER *prAdapter,
	u_int8_t fgEnable);
#endif /* CFG_ENABLE_FW_DOWNLOAD */
u_int8_t asicConnac2xUsbResume(IN struct ADAPTER *prAdapter,
	IN struct GLUE_INFO *prGlueInfo);
void asicConnac2xUdmaRxFlush(
	struct ADAPTER *prAdapter,
	u_int8_t bEnable);
uint16_t asicConnac2xUsbRxByteCount(
	struct ADAPTER *prAdapter,
	struct BUS_INFO *prBusInfo,
	uint8_t *pRXD);
#endif /* _HIF_USB */

void fillConnac2xTxDescTxByteCount(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc);
void fillConnac2xTxDescAppendWithWaCpu(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint8_t *prTxDescBuffer);
void fillConnac2xTxDescAppendByWaCpu(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	uint16_t u4MsduId,
	dma_addr_t rDmaAddr,
	uint32_t u4Idx,
	u_int8_t fgIsLast,
	uint8_t *pucBuffer);
void fillTxDescTxByteCountWithWaCpu(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo,
	void *prTxDesc);
void asicConnac2xInitTxdHook(
	struct TX_DESC_OPS_T *prTxDescOps);
void asicConnac2xInitRxdHook(
	struct RX_DESC_OPS_T *prRxDescOps);
#if (CFG_SUPPORT_MSP == 1)
void asicConnac2xRxProcessRxvforMSP(IN struct ADAPTER *prAdapter,
	IN OUT struct SW_RFB *prRetSwRfb);
#endif /* CFG_SUPPORT_MSP == 1 */
uint8_t asicConnac2xRxGetRcpiValueFromRxv(
	IN uint8_t ucRcpiMode,
	IN struct SW_RFB *prSwRfb,
	IN struct ADAPTER *prAdapter);
#if (CFG_SUPPORT_PERF_IND == 1)
void asicConnac2xRxPerfIndProcessRXV(IN struct ADAPTER *prAdapter,
	IN struct SW_RFB *prSwRfb,
	IN uint8_t ucBssIndex);
#endif
void asicConnac2xWfdmaReInit(
	struct ADAPTER *prAdapter);
void asicConnac2xWfdmaDummyCrWrite(
	struct ADAPTER *prAdapter);

#if (CFG_CHIP_RESET_SUPPORT == 1) && (CFG_WMT_RESET_API_SUPPORT == 0)
u_int8_t conn2_rst_L0_notify_step2(void);
#endif

/*******************************************************************************
*                  D E B U G F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
uint32_t asic_connac2x_show_raw_wtbl_info(
	struct GLUE_INFO *prGlueInfo,
	int8_t *pcCommand,
	int32_t i4TotalLen,
	int32_t i4Argc,
	int32_t idx);

uint32_t dump_key_table(
	struct GLUE_INFO *prGlueInfo,
	int8_t *pcCommand,
	int32_t i4TotalLen,
	int16_t key_loc0,
	int16_t key_loc1);

char *asic_connac2x_hw_rate_ofdm_str(
	u_int16_t ofdm_idx);

char *asic_connac2x_fwtbl_hw_rate_str(
	uint8_t mode,
	uint16_t rate_idx);

uint32_t asic_connac2x_show_txd_info(
	struct ADAPTER *prAdapter,
	int8_t *pcCommand,
	int32_t i4TotalLen,
	int32_t i4Argc,
	int32_t idx);
uint32_t asic_connac2x_show_umac_wtbl_info(
	struct ADAPTER *prAdapter,
	int8_t *pcCommand,
	int32_t i4TotalLen,
	int32_t i4Argc,
	int32_t idx);
u_int32_t asic_connac2x_show_rx_rate_info(
	struct ADAPTER *prAdapter,
	char *pcCommand,
	int32_t i4TotalLen,
	uint8_t ucStaIdx);
u_int32_t asic_connac2x_show_rx_rssi_info(
	struct ADAPTER *prAdapter,
	char *pcCommand,
	int32_t i4TotalLen,
	uint8_t ucStaIdx);
void asicConnac2xWfdmaControl(
	struct GLUE_INFO *prGlueInfo,
	u_int8_t ucDmaIdx,
	u_int8_t enable);
#endif /* CFG_SUPPORT_CONNAC2X == 1 */
#endif /* _CMM_ASIC_CONNAC2X_H */

