/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  mt6655.h
*    \brief This file contains the info of mt6655
*/

#ifdef MT6655

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
#define MT6655_TX_DESC_APPEND_LENGTH		32
#define MT6655_HIF_TX_DESC_APPEND_LENGTH	44
#define MT6655_RX_INIT_DESC_LENGTH		32
#define MT6655_RX_DESC_LENGTH			32
#define MT6655_CHIP_ID				0x6655
#if defined(_HIF_USB) /* TODO */
#define CONNAC3X_TOP_HCR			0x70010200
#define CONNAC3X_TOP_HVR			0x70010204
#else
#define CONNAC3X_TOP_HCR			0x88000000
#define CONNAC3X_TOP_HVR			0x88000000
#endif
#define CONNAC3X_TOP_FVR			0x88000004
#define MT6655_TOP_CFG_BASE			NIC_CONNAC_CFG_BASE
#define MT6655_PATCH_START_ADDR			0x00900000
#define MT6655_ARB_AC_MODE_ADDR			(0x820E315C)
#define RX_DATA_RING_BASE_IDX			2
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xF0)
#define CONNAC3X_CONN_CFG_ON_CONN_ON_EMI_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xD0C)

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x010C)

#define MT6655_PCIE2AP_REMAP_BASE_ADDR		0x60000
#define MT6655_REMAP_BASE_ADDR			0x7c500000

#define MT6655_ROM_VERSION			1

extern struct PLE_TOP_CR rMt6655PleTopCr;
extern struct PSE_TOP_CR rMt6655PseTopCr;
extern struct PP_TOP_CR rMt6655PpTopCr;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt6655_show_wfdma_info(struct ADAPTER *prAdapter);
void mt6655_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void mt6655_show_pse_info(struct ADAPTER *prAdapter);
bool mt6655_show_host_csr_info(struct ADAPTER *prAdapter);
void mt6655_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt6655_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

void mt6655_icapRiseVcoreClockRate(void);
void mt6655_icapDownVcoreClockRate(void);

#endif  /* mt6655 */
