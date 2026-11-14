/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file  mt7999.h
 *  \brief This file contains the info of mt7999
 */

#ifdef MT7999

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
#define CONNAC5X_CONN_CFG_ON_BASE		0x20060000
#define MCU_SW_CR_BASE				0x7C05B100
#define MT7999_TX_DESC_APPEND_LENGTH		32
#define MT7999_HIF_TX_DESC_APPEND_LENGTH	44
#define MT7999_RX_INIT_DESC_LENGTH		32
#define MT7999_RX_DESC_LENGTH			32
#define MT7999_CHIP_ID				0x7999
#define MT7999_CONNINFRA_VERSION_ID		0x05010000
#define MT7999_WF_VERSION_ID			0x05010000
#define USB_VND_PWR_ON_ADDR			(MCU_SW_CR_BASE + 0x20)
#define USB_VND_PWR_ON_ACK_BIT			BIT(0)
#define CONNAC5X_TOP_HCR			0x88000000
#define CONNAC5X_TOP_HVR			0x88000000
#define CONNAC5X_TOP_FVR			0x88000004
#define CONNAC5X_PLAT_CFG_ADDR			0x7C05B28C
#define CONNAC5X_PLAT_CFG_SIZE			64
#define MT7999_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define MT7999_PATCH_START_ADDR			0x00900000
#define MT7999_ARB_AC_MODE_ADDR			(0x820E315C)
#define RX_DATA_RING_BASE_IDX			2
#define CONNAC5X_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC5X_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC5X_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC5X_CONN_CFG_ON_CONN_ON_EMI_ADDR \
	(CONNAC5X_CONN_CFG_ON_BASE + 0xD1C)
#define MT7999_EMI_SIZE_ADDR \
	(MCU_SW_CR_BASE + 0x01E0)
#define MT7999_MCIF_MD_STATE_WHEN_WIFI_ON_ADDR	(MCU_SW_CR_BASE + 0x01E8)
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
#define MT7999_WIFI_OFF_MAGIC_NUM		0x10ff8A55U
#endif /* CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI */

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x010C)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x0154)
#define WF_PP_TOP_DBG_CS_3_ADDR    (WF_PP_TOP_BASE + 0x0158)

#define MT7999_PCIE2AP_REMAP_BASE_ADDR		0x60000
#define MT7999_PCIE2AP_REMAP_CBTOP_BASE_ADDR	0x1c0000
#define MT7999_REMAP_BASE_ADDR			0x7c500000

#define MT7999_ROM_VERSION			1

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
#define WFDMA_MD_MSI_NUM		1
#endif

extern struct PLE_TOP_CR rMt7999PleTopCr;
extern struct PSE_TOP_CR rMt7999PseTopCr;
extern struct PP_TOP_CR rMt7999PpTopCr;

#define CONN_AON_WF_NAPPING_ENABLE	0
#define CONN_AON_WF_NAPPING_DISABLE	1


/* Security Coredump EMI base */
#define COREDUMP_EMI_BASE		0x78000000
#define SEC_COREDUMP_EMI_OFFSET		0x18000
#define SEC_COREDUMP_EMI_BASE \
	(COREDUMP_EMI_BASE+SEC_COREDUMP_EMI_OFFSET)

#if (CFG_SUPPORT_PHY_ICS == 1)
#define PHYICS_EMI_BASE_ADDR 0x78231C00
#define PHYICS_EMI_SIZE (16 * 1024)
#define WIFI_MCU_MEMORY_PART_1        1
#define WIFI_MCU_MEMORY_PART_2        2
#if (CFG_SUPPORT_PHY_ICS_V3 == 1)
#define PHYICS_TOTAL_CNT (16 * 256) // 4096
#define PHYICS_BUF_SIZE (16 * 1024) // 8192
#elif (CFG_SUPPORT_PHY_ICS_V4 == 1)
#define PHYICS_TOTAL_CNT (8 * 256) // 2048
#define PHYICS_BUF_SIZE (8 * 1024) // 8192
#endif
#endif


/*------------------------------------------------------------------------------
 * MACRO for MT7999 RXVECTOR Parsing
 *------------------------------------------------------------------------------
 */
/* Group3[0] */
#define MT7999_RX_VT_RX_RATE_MASK         BITS(0, 6)
#define MT7999_RX_VT_RX_RATE_OFFSET       0
#define MT7999_RX_VT_NSTS_MASK            BITS(7, 10)
#define MT7999_RX_VT_NSTS_OFFSET          7
#define MT7999_RX_VT_MUMIMO_MASK          BIT(21)
#define MT7999_RX_VT_MUMIMO_OFFSET        21

/* Group3[2] */
#define MT7999_RX_VT_FR_MODE_MASK         BITS(0, 2) /* Group3[2] */
#define MT7999_RX_VT_FR_MODE_OFFSET       0
#define MT7999_RX_VT_GI_MASK              BITS(3, 4)
#define MT7999_RX_VT_GI_OFFSET            3
#define MT7999_RX_VT_DCM_MASK             BIT(5)
#define MT7999_RX_VT_DCM_OFFSET           5
#define MT7999_RX_VT_STBC_MASK            BITS(9, 10)
#define MT7999_RX_VT_STBC_OFFSET          9
#define MT7999_RX_VT_TXMODE_MASK          BITS(11, 14)
#define MT7999_RX_VT_TXMODE_OFFSET        11

#define RXV_GET_RX_RATE(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_RX_RATE_MASK)	\
			 >> MT7999_RX_VT_RX_RATE_OFFSET)

#define RXV_GET_RX_NSTS(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_NSTS_MASK)	\
			 >> MT7999_RX_VT_NSTS_OFFSET)

#define RXV_GET_RX_MUMIMO(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_MUMIMO_MASK)	\
			 >> MT7999_RX_VT_MUMIMO_OFFSET)

#define RXV_GET_FR_MODE(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_FR_MODE_MASK)	\
			 >> MT7999_RX_VT_FR_MODE_OFFSET)

#define RXV_GET_GI(_prRxVector)					\
		(((_prRxVector) & MT7999_RX_VT_GI_MASK)		\
			 >> MT7999_RX_VT_GI_OFFSET)

#define RXV_GET_STBC(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_STBC_MASK)	\
			 >> MT7999_RX_VT_STBC_OFFSET)

#define RXV_GET_TXMODE(_prRxVector)				\
		(((_prRxVector) & MT7999_RX_VT_TXMODE_MASK)	\
			 >> MT7999_RX_VT_TXMODE_OFFSET)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt7999_show_wfdma_info(struct ADAPTER *prAdapter);
void mt7999_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void mt7999_show_pse_info(struct ADAPTER *prAdapter);
void mt7999_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt7999_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt7999ShowDebugInfo(struct GLUE_INFO *prGlueInfo);

void mt7999_icapRiseVcoreClockRate(void);
void mt7999_icapDownVcoreClockRate(void);

#if defined(_HIF_PCIE)
void mt7999_dumpWfsyscpupcr(struct ADAPTER *ad);
void mt7999_DumpBusStatus(struct ADAPTER *ad);
void mt7999_dumpPcGprLog(struct ADAPTER *ad, uint8_t fgIsDumpViaBt);
void mt7999_dumpRV55CoreReg(struct ADAPTER *ad);
void mt7999_dumpWfTopReg(struct ADAPTER *ad, uint8_t fgIsDumpViaBt);
void mt7999_dumpWfBusReg(struct ADAPTER *ad, uint8_t fgIsDumpViaBt);
void mt7999_dumpCbInfraReg(struct ADAPTER *ad, uint8_t fgIsDumpViaBt);
u_int8_t mt7999_is_ap2conn_off_readable(struct ADAPTER *ad);
u_int8_t mt7999_is_conn2wf_readable(struct ADAPTER *ad);
#if CFG_MTK_WIFI_MBU
uint8_t mt7999_get_mbu_timeout_status(void);
#endif
#endif

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
void mt7999_dumpPcieReg(void);
bool mt7999_CheckDumpViaBt(struct ADAPTER *prAdapter);
#endif

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
int mt7999_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo);
#endif

void mt7999_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t *pu4RxV);

#if CFG_SUPPORT_WIFI_SLEEP_COUNT
int mt7999PowerDumpStart(void *priv_data,
	unsigned int force_dump);
int mt7999PowerDumpEnd(void *priv_data);
#endif /* CFG_SUPPORT_WIFI_SLEEP_COUNT */

#endif  /* mt7999 */
