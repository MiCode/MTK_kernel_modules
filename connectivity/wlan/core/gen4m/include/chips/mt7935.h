/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file  mt7935.h
*    \brief This file contains the info of mt7935
*/

#ifdef MT7935

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
#define CONN_INFRA_CFG_BASE			0x830C0000
#define CONNAC3X_CONN_CFG_ON_BASE		0x20060000
#define MCU_SW_CR_BASE				0x7C05B100
#define MT7935_TX_DESC_APPEND_LENGTH		32
#define MT7935_HIF_TX_DESC_APPEND_LENGTH	44
#define MT7935_RX_INIT_DESC_LENGTH		32
#define MT7935_RX_DESC_LENGTH			32
#define MT7935_CHIP_ID				0x7935
#define MT7935_CONNINFRA_VERSION_ID		0x03050100
#define MT7935_WF_VERSION_ID			0x03050100
#define MT7935_WIFI_PWR_ON_OFF_MODE		0
#define USB_VND_PWR_ON_ADDR			(MCU_SW_CR_BASE + 0x20)
#define USB_VND_PWR_ON_ACK_BIT			BIT(0)
#define CONNAC3X_TOP_HCR			0x88000000
#define CONNAC3X_TOP_HVR			0x88000000
#define CONNAC3X_TOP_FVR			0x88000004
#define MT7935_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define MT7935_PATCH_START_ADDR			0x00900000
#define MT7935_ARB_AC_MODE_ADDR			(0x820E315C)
#define RX_DATA_RING_BASE_IDX			2
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC3X_CONN_CFG_ON_CONN_ON_EMI_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xD1C)
#define MT7935_EMI_SIZE_ADDR \
	(MCU_SW_CR_BASE + 0x01E0)
#define MT7935_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR	(MCU_SW_CR_BASE + 0x01E8)
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
#define MT7935_WIFI_OFF_MAGIC_NUM		0x10ff8A55U
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x010C)

#define MT7935_PCIE2AP_REMAP_BASE_ADDR		0x60000
#define MT7935_REMAP_BASE_ADDR			0x7c500000

#define MTK_CUSTOM_OID_INTERFACE_VERSION     0x00000200	/* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001

#ifdef CFG_WFDMA_AP_MSI_NUM
#define WFDMA_AP_MSI_NUM		(CFG_WFDMA_AP_MSI_NUM)
#else
#define WFDMA_AP_MSI_NUM		1
#endif

#if CFG_ENABLE_MAWD_MD_RING
#define WFDMA_MD_MSI_NUM		1
#else
#define WFDMA_MD_MSI_NUM		8
#endif

extern struct PLE_TOP_CR rMt7935PleTopCr;
extern struct PSE_TOP_CR rMt7935PseTopCr;
extern struct PP_TOP_CR rMt7935PpTopCr;

#define CONN_AON_WF_NAPPING_ENABLE	0
#define CONN_AON_WF_NAPPING_DISABLE	1


#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
extern struct WLAN_IPC_INFO mt7935_ipc_info;
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */

/*------------------------------------------------------------------------------
 * MACRO for MT7935 RXVECTOR Parsing
 *------------------------------------------------------------------------------
 */
/* Group3[0] */
#define MT7935_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define MT7935_RX_VT_RX_RATE_OFFSET       0
#define MT7935_RX_VT_NSTS_MASK            BITS(7, 10)
#define MT7935_RX_VT_NSTS_OFFSET          7

/* Group3[2] */
#define MT7935_RX_VT_FR_MODE_MASK         BITS(0, 2) /* Group3[2] */
#define MT7935_RX_VT_FR_MODE_OFFSET       0
#define MT7935_RX_VT_GI_MASK              BITS(3, 4)
#define MT7935_RX_VT_GI_OFFSET            3
#define MT7935_RX_VT_DCM_MASK             BIT(5)
#define MT7935_RX_VT_DCM_OFFSET           5
#define MT7935_RX_VT_STBC_MASK            BITS(9, 10)
#define MT7935_RX_VT_STBC_OFFSET          9
#define MT7935_RX_VT_TXMODE_MASK          BITS(11, 14)
#define MT7935_RX_VT_TXMODE_OFFSET        11

#define RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & MT7935_RX_VT_RX_RATE_MASK)	\
			 >> MT7935_RX_VT_RX_RATE_OFFSET)

#define RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & MT7935_RX_VT_NSTS_MASK)	\
			 >> MT7935_RX_VT_NSTS_OFFSET)

#define RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & MT7935_RX_VT_FR_MODE_MASK)	\
			 >> MT7935_RX_VT_FR_MODE_OFFSET)

#define RXV_GET_GI(_prRxVector)					\
		(((_prRxVector) & MT7935_RX_VT_GI_MASK)		\
			 >> MT7935_RX_VT_GI_OFFSET)

#define RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & MT7935_RX_VT_STBC_MASK)	\
			 >> MT7935_RX_VT_STBC_OFFSET)

#define RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & MT7935_RX_VT_TXMODE_MASK)	\
			 >> MT7935_RX_VT_TXMODE_OFFSET)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt7935_show_wfdma_info(struct ADAPTER *prAdapter);
void mt7935_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void mt7935_show_pse_info(struct ADAPTER *prAdapter);
void mt7935_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt7935_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

void mt7935_icapRiseVcoreClockRate(void);
void mt7935_icapDownVcoreClockRate(void);
uint32_t mt7935_mcu_init(struct ADAPTER *ad);
void mt7935_mcu_deinit(struct ADAPTER *ad);

#if defined(_HIF_PCIE)
void mt7935_dumpWfsyscpupcr(struct ADAPTER *ad);
void mt7935_DumpBusStatus(struct ADAPTER *ad);
void mt7935_dumpPcGprLog(struct ADAPTER *ad);
void mt7935_dumpN45CoreReg(struct ADAPTER *ad);
void mt7935_dumpWfTopReg(struct ADAPTER *ad);
void mt7935_dumpWfBusReg(struct ADAPTER *ad);
void mt7935_dumpCbtopReg(struct ADAPTER *ad);
u_int8_t mt7935_is_ap2conn_off_readable(struct ADAPTER *ad);
u_int8_t mt7935_is_conn2wf_readable(struct ADAPTER *ad);
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
int mt7935_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo);
#endif

void mt7935_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t *pu4RxV);
#endif  /* mt7935 */
