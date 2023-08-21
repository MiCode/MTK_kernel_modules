/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
/*! \file  soc5_0.h
*    \brief This file contains the info of soc5_0
*/

#ifdef SOC7_0

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
#define CONN_INFRA_CFG_BASE					0x830C0000
#define CONNAC2x_CONN_CFG_ON_BASE				0x7C060000
#define SOC7_0_TX_DESC_APPEND_LENGTH				32
#define SOC7_0_RX_DESC_LENGTH					24
#define SOC7_0_CHIP_ID						0x7961
#define CONNAC2X_TOP_HCR					0x88000000
#define CONNAC2X_TOP_HVR					0x88000000
#define CONNAC2X_TOP_FVR					0x88000004
#define SOC7_0_TOP_CFG_BASE					CONN_CFG_BASE
#define SOC7_0_PATCH_START_ADDR					0x00900000
#define RX_DATA_RING_BASE_IDX					2
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC2x_CONN_CFG_ON_CONN_ON_MISC_ADDR \
		(CONNAC2x_CONN_CFG_ON_BASE + 0xF0)

#define CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0120)

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR \
	(0x7C00E000 + 0x068)

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void soc7_0_show_wfdma_info(struct ADAPTER *prAdapter);
void soc7_0_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void soc7_0_show_pse_info(struct ADAPTER *prAdapter);
bool soc7_0_show_host_csr_info(struct ADAPTER *prAdapter);
void soc7_0_show_wfdma_dbg_probe_info(IN struct ADAPTER *prAdapter,
	IN enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void soc7_0_show_wfdma_wrapper_info(IN struct ADAPTER *prAdapter,
	IN enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

#endif  /* soc7_0 */
