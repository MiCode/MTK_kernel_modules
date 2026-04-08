/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  mt7990.h
*    \brief This file contains the info of mt7990
*/

#ifdef MT7990

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
#define CONNSYS_VERSION_ID				0x03000001
#define CONNAC3X_CONN_CFG_ON_BASE			0x7C060000
#define MT7990_TX_DESC_APPEND_LENGTH			32
#define MT7990_RX_INIT_DESC_LENGTH			32
#define MT7990_RX_DESC_LENGTH				32
#define MT7990_CHIP_ID					0x7990
#define CONNAC3X_TOP_HCR				0x88000000
#define CONNAC3X_TOP_HVR				0x88000000
#define CONNAC3X_TOP_FVR				0x88000004
#define MT7990_TOP_CFG_BASE				NIC_CONNAC_CFG_BASE
#define MT7990_ARB_AC_MODE_ADDR				(0x820E315C)
#define RX_DATA_RING_BASE_IDX				2
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_DRV_FM_STAT_SYNC_SHFT	0
#define CONNAC3X_CONN_CFG_ON_CONN_ON_MISC_ADDR \
	(CONNAC3X_CONN_CFG_ON_BASE + 0xF0)

#define CONN_BUS_CR_BASE				0x7c048000
#define CONN_BUS_CR_ADDR_MD_SHARED_BASE_ADDR_ADDR \
	(CONN_BUS_CR_BASE + 0x308)
#define CONN_BUS_CR_ADDR_CONN2AP_REMAP_BYPASS_ADDR \
	(CONN_BUS_CR_BASE + 0x320)

#define CONN_INFRA_MCU_SW_DEF_CR_BASE			0x7C054A00
#define CONN_INFRA_CR_SW_DEF_MCU_ENTER_IDLE_LOOP \
	(CONN_INFRA_MCU_SW_DEF_CR_BASE + 0x68)

#define CONN_INFRA_CFG_AP2WF_REMAP_1_ADDR \
	(CONN_INFRA_CFG_BASE + 0x0120)

#define CONN_INFRA_CFG_PCIE2AP_REMAP_2_ADDR \
	(0x7C00E000 + 0x068)

#define WF_PP_TOP_BASE             0x820CC000
#define WF_PP_TOP_DBG_CTRL_ADDR    (WF_PP_TOP_BASE + 0x00FC)
#define WF_PP_TOP_DBG_CS_0_ADDR    (WF_PP_TOP_BASE + 0x0104)
#define WF_PP_TOP_DBG_CS_1_ADDR    (WF_PP_TOP_BASE + 0x0108)
#define WF_PP_TOP_DBG_CS_2_ADDR    (WF_PP_TOP_BASE + 0x010C)

extern struct PLE_TOP_CR rMt7990PleTopCr;
extern struct PSE_TOP_CR rMt7990PseTopCr;
extern struct PP_TOP_CR rMt7990PpTopCr;

#define MT7990_FIRMWARE_ROM			"wf_rom.bin"
#define MT7990_FIRMWARE_ROM_SRAM		"wf_rom_sram.bin"

#define MT7990_FIRMWARE_ROM_ADDR		0x00800000
#define MT7990_FIRMWARE_ROM_SRAM_ADDR	0xE0048000

#define MT7990_REMAP_BASE_ADDR		0x7c500000

#define MT7990_ROM_VERSION			1
#define MT7990_FLAVOR_VERSION			1

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
void mt7990_show_wfdma_info(struct ADAPTER *prAdapter);
void mt7990_show_ple_info(struct ADAPTER *prAdapter, u_int8_t fgDumpTxd);
void mt7990_show_pse_info(struct ADAPTER *prAdapter);
bool mt7990_show_host_csr_info(struct ADAPTER *prAdapter);
void mt7990_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);
void mt7990_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

#endif  /* mt7990 */
