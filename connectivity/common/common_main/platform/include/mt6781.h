/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MTK_MT6781_H_
#define _MTK_MT6781_H_

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

#define CONSYS_BT_WIFI_SHARE_V33	0
#define CONSYS_PMIC_CTRL_ENABLE		1
#define CONSYS_PWR_ON_OFF_API_AVAILABLE	1

#define	PRIMARY_ADIE	0x6631
#define	SECONDARY_ADIE	0x6635

/* if clock of TCXO is controlled by GPIO, CLK_CTRL_TCXOENA_REQ should be 1. */
#define CLK_CTRL_TCXOENA_REQ 0

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*tag start:new platform need to make sure these define */
#define PLATFORM_SOC_CHIP 0x6781
/*tag end*/

/*device tree mode*/
/* A-Die interface pinmux base */
#define CONSYS_IF_PINMUX_REG_BASE	0x10005000

/* VCN13 ext ctrl */
#define CONSYS_VCN13_REG_BASE		0x10005000
#define CONSYS_VCN13_MODE_OFFSET	0x00000310
#define CONSYS_VCN13_MODE_MASK		0xFFF0FFFF
#define CONSYS_VCN13_MODE_VALUE		0x00000000
#define CONSYS_VCN13_DIR_OFFSET		0x00000000
#define CONSYS_VCN13_DIR_BIT		(0x1 << 12)        /* bit 12 */
#define CONSYS_VCN13_DOUT_OFFSET	0x00000100
#define CONSYS_VCN13_DOUT_BIT		(0x1 << 12)        /* bit 12 */

/*TOPCKGEN_BASE*/
#define CONSYS_AP2CONN_OSC_EN_OFFSET	0x00000f00
#define CONSYS_EMI_MAPPING_OFFSET	0x00000380
#define CONSYS_EMI_PERI_MAPPING_OFFSET	0x00000388
#define CONSYS_EMI_AP_MD_OFFSET		0x0000039C

/*CONN_HIF_ON_BASE*/
#define CONSYS_CLOCK_CHECK_VALUE        0x30000
#define CONSYS_HCLK_CHECK_BIT           (0x1 << 16)
#define CONSYS_OSCCLK_CHECK_BIT         (0x1 << 17)
#define CONSYS_SLEEP_CHECK_BIT          (0x1 << 18)

#define CONSYS_AP2CONN_WAKEUP_BIT	(0x1 << 9)

/* EMI part mapping & ctrl*/
#define CONSYS_EMI_COREDUMP_OFFSET	(0x88000)
#define CONSYS_EMI_COREDUMP_MEM_SIZE	(0xc8000)
#define CONSYS_EMI_AP_PHY_OFFSET	(0x00000)
#define CONSYS_EMI_AP_PHY_BASE		(0x80088000)
#define CONSYS_EMI_FW_PHY_BASE		(0xf0088000)
#define CONSYS_EMI_PAGED_TRACE_OFFSET	(0x400)
#define CONSYS_EMI_PAGED_DUMP_OFFSET	(0x8400)
#define CONSYS_EMI_FULL_DUMP_OFFSET	(0x10400)
#define CONSYS_EMI_MET_DATA_OFFSET	(0x50000)

#define CONSYS_EMI_RAM_BT_BUILDTIME_OFFSET	(0x300)
#define CONSYS_EMI_RAM_WIFI_BUILDTIME_OFFSET	(0x310)
#define CONSYS_EMI_RAM_MCU_BUILDTIME_OFFSET	(0x320)
#define CONSYS_EMI_PATCH_MCU_BUILDTIME_OFFSET	(0x330)

/* AP_PCCIF4_BASE Register */
#define INFRASYS_COMMON_AP2MD_PCCIF4_AP_PERI_AP_CCU_CONFIG (0xC14)
#define INFRASYS_COMMON_AP2MD_PCCIF4_AP_PCCIF_ACK_OFFSET (0x14)
#define INFRASYS_COMMON_AP2MD_CON_PWR_ON_CON_SW_READY_MASK (0x3 << 0)

/**********************************************************************/
/* Base: mcu_base (0x1800_2000) */
/**********************************************************************/
#define CONSYS_CLOCK_CONTROL		0x00000100
#define CONSYS_BUS_CONTROL		0x00000110
#define EMI_CONTROL_DBG_PROBE		0x00000144
#define CONN2AP_SW_IRQ_CLR_OFFSET	0x0000014c
#define CONN2AP_SW_IRQ_OFFSET		0x00000150
#define CONN_MCU_EMI_CONTROL		0x00000150
#define CONSYS_SW_DBG_CTL		(0x16c)
#define CONSYS_DEBUG_SELECT		0x00000400
#define CONSYS_DEBUG_STATUS		0x0000040c
#define CONSYS_EMI_BT_ENTRY_OFFSET	(0x504)
#define CONSYS_EMI_WIFI_ENTRY_OFFSET	(0x508)
#define CONSYS_COM_REG0			(0x600)

/**********************************************************************/
/* Base: conn_hif_pdma_base (0x1800_4000) */
/**********************************************************************/
#define CONSYS_HIF_TOP_MISC             0x00000104
#define CONSYS_HIF_DBG_IDX              0x0000012C
#define CONSYS_HIF_DBG_PROBE            0x00000130
#define CONSYS_HIF_BUSY_STATUS          0x00000138
#define CONSYS_HIF_PDMA_BUSY_STATUS     0x00000168
#define CONSYS_HIF_PDMA_AXI_RREADY      0x00000154
#define CONSYS_HIF_PDMA_AXI_RREADY_MASK     (0x1 << 1)        /* bit 1 */

/**********************************************************************/
/* Base: conn_hif_on_base (0x1800_7000) */
/**********************************************************************/
#define CONN_HIF_ON_BASE_ADDR				(0x18007000)
#define CONN_HOST_CR_SLEEP_CNT_OFFSET			(0x58)
#define CONN_SLEEP_INFO_CTRL_OFFSET			(0x5c)
#define CONN_SLEEP_INFO_READ_CTRL_TIMER_OFFSET		(0x60)
#define CONN_SLEEP_INFO_READ_CTRL_COUNT_OFFSET		(0x64)
#define CONSYS_CPUPCR_OFFSET				(0x00000104)
#define CONN_MCU_MAILBOX_DBG				(0x0120)

/**********************************************************************/
/* Base: conn_top_misc_on_base (0x180c_1000) */
/**********************************************************************/
#define CONSYS_ACCESS_EMI_HW_MODE_OFFSET		(0x168)
#define CONN_CFG_ON_DBGSEL_ADDR_OFFSET			(0x310)
#define CONN_CFG_ON_MON_CTL_ADDR_OFFSET			(0x320)
#define CONN_CFG_ON_MON_SEL0_ADDR_OFFSET		(0x328)
#define CONN_CFG_ON_MON_SEL1_ADDR_OFFSET		(0x32C)
#define CONN_CFG_ON_MON_SEL2_ADDR_OFFSET		(0x330)
#define CONN_CFG_ON_MON_SEL3_ADDR_OFFSET		(0x334)
#define CONN_CFG_ON_MON_FLAG_RECORD_ADDR_OFFSET		(0x340)
#define CONN_ON_ADIE_CTL_OFFSET				(0x500)

/**********************************************************************/
/* Base: conn_rf_spi_base (0x180c_6000) */
/**********************************************************************/
#define CONN_RF_SPI_BASE	0x180c6000
#define SPI_TOP_ADDR	0x50
#define SPI_TOP_WDAT	0x54
#define SPI_TOP_RDAT	0x58

/**********************************************************************/
/* Base: conn_mcu_cfg_on_base (0x180a_3000) */
/**********************************************************************/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

#ifdef CONSYS_BT_WIFI_SHARE_V33
struct bt_wifi_v33_status {
	UINT32 counter;
	UINT32 flags;
	spinlock_t lock;
};

extern struct bt_wifi_v33_status gBtWifiV33;
#endif

/*******************************************************************************
*                            P U B L I C   D A T A
*******************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
extern INT32 dump_conn_mcu_pc_log_mt6781(const char *trg_str);
extern INT32 dump_conn_debug_dump_mt6781(const char *);
extern INT32 dump_conn_mcu_debug_flag_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_bus_hang_layer1_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_bus_hang_layer2_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_bus_hang_layer3_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_bus_hang_layer4_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_timeout_info_mt6781(const char *);
extern INT32 dump_conn_bus_hang_debug_mt6781(const char *);
extern INT32 dump_conn_mcu_apb_timeout_info_mt6781(const char *);
extern INT32 dump_conn_apb_bus0_hang_mt6781(const char *);
extern INT32 dump_conn_apb_bus1_hang_mt6781(const char *);
extern INT32 dump_conn_apb_bus2_hang_mt6781(const char *);
extern INT32 dump_conn_emi_ctrl_host_csr_mt6781(const char *);
extern INT32 dump_conn_mcu_confg_emi_ctrl_mt6781(const char *);
extern INT32 dump_conn_mcu_cpu_probe_mt6781(const char *);
extern INT32 dump_conn_mcu_ahb_probe_mt6781(const char *);
extern INT32 dump_conn_mcu_idlm_prot_prob_mt6781(const char *);
extern INT32 dump_conn_mcu_wf_cmdbt_ram_prob_mt6781(const char *);
extern INT32 dump_conn_mcu_pda_dbg_flag_mt6781(const char *);
extern INT32 dump_conn_mcu_sysram_prb_mt6781(const char *);
extern INT32 dump_conn_mcu_confg_mt6781(const char *);
extern INT32 dump_conn_mcu_i_eidlm_mt6781(const char *);
extern INT32 dump_conn_mcu_dma_mt6781(const char *);
extern INT32 dump_conn_mcu_tcm_prob_mt6781(const char *);
extern INT32 dump_conn_mcu_met_prob_mt6781(const char *);
extern INT32 dump_conn_mcusys_n9_mt6781(const char *);
extern INT32 dump_conn_mcu_uart_dbg_loop_mt6781(const char *);
extern INT32 dump_conn_cfg_on_Debug_Signal_mt6781(const char *);
extern INT32 dump_conn_cfg_on_register_mt6781(const char *);
extern INT32 dump_conn_cmdbt_debug_signal_mt6781(const char *);
extern INT32 dump_conn_cmdbt_register_mt6781(const char *);
extern INT32 dump_conn_emi_detect_mt6781(const char *);
extern INT32 dump_conn_cmdbt_debug_mt6781(const char *);
extern INT32 dump_conn_hif_reg_debug_mt6781(const char *);
extern INT32 dump_conn_mcu_confg_bus_hang_reg_mt6781(const char *);
extern INT32 dump_wf_pdma_reg_debug_mt6781(const char *);
extern INT32 dump_conn_to_EMI_bus_path_mt6781(const char *trg_str);

INT32 consys_is_rc_mode_enable(VOID);

#endif /* _MTK_MT6781_H_ */
