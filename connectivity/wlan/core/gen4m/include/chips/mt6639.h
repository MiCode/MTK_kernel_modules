/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  mt6639.h
*    \brief This file contains the info of mt6639
*/

#ifdef MT6639

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
#define CONNAC3X_CONN_CFG_ON_BASE		0x7C060000
#define MCU_SW_CR_BASE				0x7C05B100
#define MT6639_TX_DESC_APPEND_LENGTH		32
#define MT6639_HIF_TX_DESC_APPEND_LENGTH	44
#define MT6639_RX_INIT_DESC_LENGTH		32
#define MT6639_RX_DESC_LENGTH			32
#define MT6639_CHIP_ID				0x6639
#define MT6639_CONNINFRA_VERSION_ID		0x03010001
#define MT6639_CONNINFRA_VERSION_ID_E2		0x03010002
#define MT6639_WF_VERSION_ID			0x03010001
#define USB_VND_PWR_ON_ADDR			(MCU_SW_CR_BASE + 0x20)
#define USB_VND_PWR_ON_ACK_BIT			BIT(0)
#if defined(_HIF_USB) /* TODO */
#define CONNAC3X_TOP_HCR			0x70010200
#define CONNAC3X_TOP_HVR			0x70010204
#else
#define CONNAC3X_TOP_HCR			0x88000000
#define CONNAC3X_TOP_HVR			0x88000000
#endif
#define CONNAC3X_TOP_FVR			0x88000004
#define CONNAC3X_PLAT_CFG_ADDR			0x7C05BA38
#define CONNAC3X_PLAT_CFG_SIZE			64
#define MT6639_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define MT6639_PATCH_START_ADDR			0x00900000
#define MT6639_ARB_AC_MODE_ADDR			(0x820E315C)
#define RX_DATA_RING_BASE_IDX			2
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC3X_CONN_CFG_ON_CONN_ON_EMI_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xD0C)
#define MT6639_EMI_SIZE_ADDR \
	(MCU_SW_CR_BASE + 0x01E0)
#define MT6639_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR	(MCU_SW_CR_BASE + 0x01E8)

#define WF_PP_TOP_BASE				0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x010C)

#define MT6639_PCIE2AP_REMAP_BASE_ADDR		0x60000
#define MT6639_REMAP_BASE_ADDR			0x7c500000

#define MT6639_ROM_VERSION			1

#define MTK_CUSTOM_OID_INTERFACE_VERSION     0x00000200	/* for WPDWifi DLL */
#define MTK_EM_INTERFACE_VERSION		0x0001

#ifdef CFG_WFDMA_AP_MSI_NUM
#define WFDMA_AP_MSI_NUM		(CFG_WFDMA_AP_MSI_NUM)
#else
#define WFDMA_AP_MSI_NUM		1
#endif
#define WFDMA_MD_MSI_NUM		8

#if CFG_PCIE_LTR_UPDATE
#define LTR_SNOOP_LATENCY_REQUIREMENT BIT(15)
#define LTR_NONSNOOP_LATENCY_REQUIREMENT BIT(31)
#define PCIE_LOW_LATENCY_LTR_VALUE  \
	(LTR_NONSNOOP_LATENCY_REQUIREMENT | LTR_SNOOP_LATENCY_REQUIREMENT)
#define PCIE_HIGH_LATENCY_LTR_VALUE \
	(LTR_NONSNOOP_LATENCY_REQUIREMENT | LTR_SNOOP_LATENCY_REQUIREMENT \
	| 0x10011001) /* 1 ms */
#endif

#define MT6639_MEMOEY_REPAIR_CHECK_MASK 0xFFFF

extern struct PLE_TOP_CR rMt6639PleTopCr;
extern struct PSE_TOP_CR rMt6639PseTopCr;
extern struct PP_TOP_CR rMt6639PpTopCr;

extern u_int8_t fgIsMcuOff;
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
extern u_int8_t fgTriggerDebugSop;
#endif

/*------------------------------------------------------------------------------
 * MACRO for MT6639 RXVECTOR Parsing
 *------------------------------------------------------------------------------
 */
/* Group3[0] */
#define MT6639_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define MT6639_RX_VT_RX_RATE_OFFSET       0
#define MT6639_RX_VT_NSTS_MASK            BITS(7, 10)
#define MT6639_RX_VT_NSTS_OFFSET          7
#define MT6639_RX_VT_MUMIMO_MASK          BIT(21)
#define MT6639_RX_VT_MUMIMO_OFFSET        21

/* Group3[2] */
#define MT6639_RX_VT_FR_MODE_MASK         BITS(0, 2)
#define MT6639_RX_VT_FR_MODE_OFFSET       0
#define MT6639_RX_VT_GI_MASK              BITS(3, 4)
#define MT6639_RX_VT_GI_OFFSET            3
#define MT6639_RX_VT_DCM_MASK             BIT(5)
#define MT6639_RX_VT_DCM_OFFSET           5
#define MT6639_RX_VT_STBC_MASK            BITS(9, 10)
#define MT6639_RX_VT_STBC_OFFSET          9
#define MT6639_RX_VT_TXMODE_MASK          BITS(11, 14)
#define MT6639_RX_VT_TXMODE_OFFSET        11

#define RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_RX_RATE_MASK)	\
			 >> MT6639_RX_VT_RX_RATE_OFFSET)

#define RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_NSTS_MASK)	\
			 >> MT6639_RX_VT_NSTS_OFFSET)

#define RXV_GET_RX_MUMIMO(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_MUMIMO_MASK)	\
			 >> MT6639_RX_VT_MUMIMO_OFFSET)

#define RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_FR_MODE_MASK)	\
			 >> MT6639_RX_VT_FR_MODE_OFFSET)

#define RXV_GET_GI(_prRxVector)					\
		(((_prRxVector) & MT6639_RX_VT_GI_MASK)		\
			 >> MT6639_RX_VT_GI_OFFSET)

#define RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_STBC_MASK)	\
			 >> MT6639_RX_VT_STBC_OFFSET)

#define RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & MT6639_RX_VT_TXMODE_MASK)	\
			 >> MT6639_RX_VT_TXMODE_OFFSET)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt6639_get_rx_mode_mcs(struct SW_RFB *prSwRfb);

void mt6639_show_wfdma_info(struct ADAPTER *prAdapter);
void mt6639_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void mt6639_show_pse_info(struct ADAPTER *prAdapter);
void mt6639_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt6639_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

void mt6639_icapRiseVcoreClockRate(void);
void mt6639_icapDownVcoreClockRate(void);

#if defined(_HIF_PCIE)
void mt6639_dumpWfsyscpupcr(struct ADAPTER *ad);
void mt6639_DumpBusStatus(struct ADAPTER *ad);
void mt6639_dumpPcGprLog(struct ADAPTER *ad, u_int8_t fgIsDumpViaBt);
void mt6639_dumpN45CoreReg(struct ADAPTER *ad, u_int8_t fgIsDumpViaBt);
void mt6639_dumpWfTopReg(struct ADAPTER *ad, u_int8_t fgIsDumpViaBt);
void mt6639_dumpHostVdnrTimeoutInfo(struct ADAPTER *ad);
void mt6639_dumpWfBusReg(struct ADAPTER *ad, u_int8_t fgIsDumpViaBt);
void mt6639_dumpCbtopReg(struct ADAPTER *ad);

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
void mt6639_dumpPcieReg(void);
void mt6639_dumpPcieRegWithScanDump(void);
bool mt6639_CheckDumpViaBt(struct ADAPTER *prAdapter);
#endif

u_int8_t mt6639_is_ap2conn_off_readable(struct ADAPTER *ad);
u_int8_t mt6639_is_conn2wf_readable(struct ADAPTER *ad);
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
int mt6639_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo);
#endif

void mt6639_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t *pu4RxV);

#if CFG_SUPPORT_WIFI_SLEEP_COUNT
int mt6639PowerDumpStart(void *priv_data,
	unsigned int force_dump);
int mt6639PowerDumpEnd(void *priv_data);
#endif /* CFG_SUPPORT_WIFI_SLEEP_COUNT */

#endif  /* mt6639 */
