/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MTK_MT6873_H_
#define _MTK_MT6873_H_

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

#define CONSYS_BT_WIFI_SHARE_V33	0
#define CONSYS_PMIC_CTRL_ENABLE		1
#define CONSYS_PWR_ON_OFF_API_AVAILABLE	1
#define CONSYS_AFE_REG_SETTING		0
#define CONSYS_RC_MODE_ENABLE		1

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
#define COMMON_KERNEL_EMI_MPU_SUPPORT	1
#define COMMON_KERNEL_PMIC_SUPPORT	1
#else
#define COMMON_KERNEL_EMI_MPU_SUPPORT	0
#define COMMON_KERNEL_PMIC_SUPPORT	0
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#define COMMON_KERNEL_CLK_SUPPORT	1
#else
#define COMMON_KERNEL_CLK_SUPPORT	0
#endif

#if CONSYS_PMIC_CTRL_ENABLE
#if COMMON_KERNEL_PMIC_SUPPORT
#include <linux/mfd/mt6359p/registers.h>
#endif
#endif

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*tag start:new platform need to make sure these define */
#define PLATFORM_SOC_CHIP 0x6873
/*tag end*/

/*device tree mode*/
/* A-Die interface pinmux base */
#define CONSYS_IF_PINMUX_REG_BASE	0x10005000
#define CONSYS_IF_PINMUX_01_OFFSET	0x00000450
#define CONSYS_IF_PINMUX_01_MASK	0x0000FFFF
#define CONSYS_IF_PINMUX_01_VALUE	0x11110000
#define CONSYS_IF_PINMUX_02_OFFSET	0x00000460
#define CONSYS_IF_PINMUX_02_MASK	0xF0000000
#define CONSYS_IF_PINMUX_02_VALUE	0x01111111
#define CONSYS_CLOCK_TCXO_MODE_OFFSET	0x400
#define CONSYS_CLOCK_TCXO_MODE_MASK	0x8FFFFFFF
#define CONSYS_CLOCK_TCXO_MODE_VALUE	0x50000000

/* A-Die interface pinmux driving base */
#define CONSYS_IF_PINMUX_DRIVING_BASE	0x11EA0000
#define CONSYS_IF_PINMUX_DRIVING_OFFSET_1	0x0
#define CONSYS_IF_PINMUX_DRIVING_MASK_1		0xC0000FFF
#define CONSYS_IF_PINMUX_DRIVING_VALUE_1	0x01209000 /* --00 0001 0010 0000 1001 ---- */
#define CONSYS_IF_PINMUX_DRIVING_OFFSET_2	0x10
#define CONSYS_IF_PINMUX_DRIVING_MASK_2		0xFFFF8000
#define CONSYS_IF_PINMUX_DRIVING_VALUE_2	0x0

/* CONN_WF_CTRL2 */
#define CONSYS_WF_CTRL2_01_OFFSET	0x00000050
#define CONSYS_WF_CTRL2_01_MASK	0xFFEFFFFF
#define CONSYS_WF_CTRL2_01_VALUE	0x00100000
#define CONSYS_WF_CTRL2_02_OFFSET	0x00000150
#define CONSYS_WF_CTRL2_02_MASK	0xFFEFFFFF
#define CONSYS_WF_CTRL2_02_VALUE	0x00100000
#define CONSYS_WF_CTRL2_03_OFFSET	0x00000460
#define CONSYS_WF_CTRL2_03_MASK	0xFFF8FFFF
#define CONSYS_WF_CTRL2_GPIO_MODE	0x00000000
#define CONSYS_WF_CTRL2_CONN_MODE	0x00010000

/*TOPCKGEN_BASE*/
#define CONSYS_AP2CONN_OSC_EN_OFFSET	0x00000f00
#define CONSYS_EMI_MAPPING_OFFSET	0x00000380
#define CONSYS_EMI_PERI_MAPPING_OFFSET	0x00000388
#define CONSYS_EMI_AP_MD_OFFSET		0x0000039C
/*AP_RGU_BASE*/
#define CONSYS_CPU_SW_RST_OFFSET	0x00000018
/*SPM_BASE*/
#define CONSYS_PWRON_CONFG_EN_OFFSET	0x00000000
#define CONSYS_TOP1_PWR_CTRL_OFFSET	0x00000304
#define CONSYS_PWR_CONN_ACK_OFFSET	0x0000016C
#define CONSYS_PWR_CONN_ACK_S_OFFSET	0x00000170
#define CONSYS_SPM_APSRC_OFFSET		0x000006f8
#define CONSYS_SPM_APSRC_VALUE		0x00000005
#define CONSYS_SPM_DDR_EN_OFFSET	0x000006fc
#define CONSYS_SPM_DDR_EN_VALUE		0x00050505
#define CONSYS_SPM_CON_TOP_OFF_CHECK_OFFSET	0x00000178
#define CONSYS_SPM_CON_TOP_OFF_CHECK_BIT	(0x1 << 1)
/*CONN_MCU_CONFIG_BASE*/
#define CONSYS_IP_VER_OFFSET		0x00000010
#define CONSYS_CONF_ID_OFFSET		0x0000001c
#define CONSYS_HW_ID_OFFSET		0x00000000
#define CONSYS_FW_ID_OFFSET		0x00000004
#define CONSYS_MCU_CFG_ACR_OFFSET	0x00000140
#define EMI_CONTROL_DBG_PROBE		0x00000144
#define CONSYS_SW_IRQ_OFFSET		0x00000148
#define CONN2AP_SW_IRQ_CLR_OFFSET	0x0000014c
#define CONN2AP_SW_IRQ_OFFSET		0x00000150
#define CONN_MCU_EMI_CONTROL		0x00000150

#define CONSYS_IP_VER_ID		0x10070000

#define CONSYS_HIF_TOP_MISC             0x00002104
#define CONSYS_HIF_DBG_IDX              0x0000212C
#define CONSYS_HIF_DBG_PROBE            0x00002130
#define CONSYS_HIF_BUSY_STATUS          0x00002138
#define CONSYS_HIF_PDMA_BUSY_STATUS     0x00002168
#define CONSYS_HIF_PDMA_AXI_RREADY      0x00000154
#define CONSYS_HIF_PDMA_AXI_RREADY_MASK (0x1 << 1)	/* bit 1 */
#define CONSYS_CLOCK_CONTROL            0x00000100
#define CONSYS_BUS_CONTROL              0x00000110
#define CONSYS_DEBUG_SELECT             0x00000400
#define CONSYS_DEBUG_STATUS             0x0000040c
#define CONSYS_EMI_CTRL_VALUE           (1 << 21)

/*CONN_HIF_ON_BASE*/
#define CONSYS_CLOCK_CHECK_VALUE        0x30000
#define CONSYS_HCLK_CHECK_BIT           (0x1 << 16)
#define CONSYS_OSCCLK_CHECK_BIT         (0x1 << 17)
#define CONSYS_SLEEP_CHECK_BIT          (0x1 << 18)

/*AXI bus*/
#define CONSYS_AHB_RX_PROT_EN_OFFSET	0x2AC
#define CONSYS_AHB_RX_PROT_STA_OFFSET	0x258
#define CONSYS_AXI_RX_PROT_EN_OFFSET	0x2A4
#define CONSYS_AXI_RX_PROT_STA_OFFSET	0x228
#define CONSYS_AHB_RX_PROT_MASK		(0x1<<10)	/* bit 10 */
#define CONSYS_AXI_RX_PROT_MASK		(0x1<<14)	/* bit 14 */
#define CONSYS_AXI_TX_PROT_MASK		(0x1<<18)	/* bit 18 */
#define CONSYS_AHB_TX_PROT_MASK		(0x1<<13)	/* bit 13 */
#define CONSYS_AHB_TIMEOUT_EN_ADDRESS	0x18002440
#define CONSYS_AHB_TIMEOUT_EN_VALUE	0x90000002

/*WPLL SETTING*/
#define CONSYS_WPLL_SETTING_ADDRESS	0x180b3034
#define CONSYS_WPLL_SETTING_MASK	0xFFE7FFFF	/* bit 19, bit 20 */
#define CONSYS_WPLL_SETTING_VALUE	0x00080000	/* bit 19, bit 20 */

/* toppose_restore_done rollback CR */
#define CONSYS_TOPPOSE_RESTORE_ADDRESS	0x180c1130
#define CONSYS_TOPPOSE_RESTORE_MASK	0xFBFFFFFF	/* bit 26 */
#define CONSYS_TOPPOSE_RESTORE_VALUE	(0x1 << 26)

/*SPM clock gating control register */
#define CONSYS_PWRON_CONFG_EN_VALUE	(0x0b160001)
#define CONSYS_PWRON_CONFG_DIS_VALUE	(0x0b160000)

#if CONSYS_AFE_REG_SETTING
#define CONSYS_AFE_REG_BASE			(0x180B6000)
#define CONSYS_AFE_RG_WBG_PLL_03_OFFSET		(0x00000038)
#define CONSYS_AFE_RG_WBG_PLL_03_VALUE		(0x000C1DF0)
#define CONSYS_AFE_RG_WBG_GPS_02_OFFSET		(0x00000054)
#define CONSYS_AFE_RG_WBG_GPS_02_VALUE		(0x110A2000)
#endif

#define CONSYS_AFE_WBG_REG_BASE			(0x180B3000)
#define CONSYS_AFE_WBG_REG_AFE_01_OFFSET	(0x10)
#define CONSYS_AFE_WBG_REG_RCK_01_OFFSET	(0x18)
#define CONSYS_AFE_WBG_REG_GL1_01_OFFSET	(0x40)
#define CONSYS_AFE_WBG_REG_BT_TX_03_OFFSET	(0x58)
#define CONSYS_AFE_WBG_REG_WF0_TX_03_OFFSET	(0x78)
#define CONSYS_AFE_WBG_REG_WF1_TX_03_OFFSET	(0x94)
#define CONSYS_AFE_WBG_REG_GL5_01_OFFSET	(0x100)
#define CONSYS_AFE_WBG_REG_AFE_01_VALUE		(0x00000000)
#define CONSYS_AFE_WBG_REG_RCK_01_VALUE		(0x144B0160)
#define CONSYS_AFE_WBG_REG_GL1_01_VALUE		(0x10990C13)
#define CONSYS_AFE_WBG_REG_BT_TX_03_VALUE	(0xCD258051)
#define CONSYS_AFE_WBG_REG_WF0_TX_03_VALUE	(0xC5258251)
#define CONSYS_AFE_WBG_REG_WF1_TX_03_VALUE	(0xC5258251)
#define CONSYS_AFE_WBG_REG_GL5_01_VALUE		(0x10990C13)

#define CONSYS_COCLOCK_STABLE_TIME_BASE         (0x180C1200)
#define CONSYS_COCLOCK_ACK_ENABLE_OFFSET        (0x4)
#define CONSYS_COCLOCK_ACK_ENABLE_BIT           (1 << 0)
#define CONSYS_COCLOCK_ACK_ENABLE_MAST          (0xffff00ff)
#define CONSYS_COCLOCK_ACK_ENABLE_VALUE         (0x600)
#define CONSYS_COCLOCK_STABLE_TIME              (0x708)
#define CONSYS_COCLOCK_STABLE_TIME_MASK         (0xffff0000)

#define CONSYS_COCLOCK_RC_CTL_0_TOP_OSC_RC_EN_OFFSET	(0x40)
#define CONSYS_COCLOCK_RC_CTL_0_TOP_OSC_RC_EN_BIT	(0x1 << 7)
#define CONSYS_COCLOCK_RC_CTL_1_GPS_XO_OFFSET	(0x44)
#define CONSYS_COCLOCK_RC_CTL_0_GPS_ACK_OFFSET	(0x48)
#define CONSYS_COCLOCK_RC_CTL_1_BT_XO_OFFSET	(0x4C)
#define CONSYS_COCLOCK_RC_CTL_0_BT_ACK_OFFSET	(0x50)
#define CONSYS_COCLOCK_RC_CTL_1_WF_XO_OFFSET	(0x54)
#define CONSYS_COCLOCK_RC_CTL_0_WF_ACK_OFFSET	(0x58)
#define CONSYS_COCLOCK_RC_CTL_1_TOP_XO_OFFSET	(0x5C)
#define CONSYS_COCLOCK_RC_CTL_0_TOP_ACK_OFFSET	(0x60)
#define CONSYS_COCLOCK_RC_CTL_1_XO_VALUE	(0x02080706)
#define CONSYS_COCLOCK_RC_CTL_0_ACK_BIT		(0x1 << 16)
#define CONSYS_COCLOCK_RC_CTL_0_GPS_OSC_RC_EN_BIT	(0x1 << 4 | 0x1 << 12)
#define CONSYS_COCLOCK_RC_CTL_0_BT_OSC_RC_EN_BIT	(0x1 << 5 | 0x1 << 13)
#define CONSYS_COCLOCK_RC_CTL_0_WF_OSC_RC_EN_BIT	(0x1 << 6 | 0x1 << 14)
#define CONSYS_COCLOCK_RC_CTL_0_TOP_OSC_RC_EN_BIT_2	(0x1 << 7 | 0x1 << 15)
#define CONSYS_COCLOCK_RC_CTL_0_OSC_LEGACY_EN_BIT	(0x1 << 8)

#define CONSYS_IDENTIFY_ADIE_CR_ADDRESS		(0x180C1130)
#define CONSYS_IDENTIFY_ADIE_ENABLE_BIT	(0x1 << 8)

/*CONSYS_CPU_SW_RST_REG*/
#define CONSYS_CPU_SW_RST_BIT		(0x1 << 12)
#define CONSYS_CPU_SW_RST_CTRL_KEY	(0x88 << 24)
#define CONSYS_SW_RST_BIT		(0x1 << 9)

/*CONSYS_TOP1_PWR_CTRL_REG*/
#define CONSYS_SPM_PWR_RST_BIT		(0x1 << 0)
#define CONSYS_SPM_PWR_ISO_S_BIT	(0x1 << 1)
#define CONSYS_SPM_PWR_ON_BIT		(0x1 << 2)
#define CONSYS_SPM_PWR_ON_S_BIT		(0x1 << 3)
#define CONSYS_CLK_CTRL_BIT		(0x1 << 4)
#define CONSYS_SRAM_CONN_PD_BIT		(0x1 << 8)
#define CONSYS_SPM_PWR_ON_CLK_BIT	(0x1 << 0)
#define CONSYS_SPM_PWR_ON_CLK_CTRL_KEY	(0x0B16 << 16)
#define SPM_RC_CENTRAL_CFG1		(0x0504)
#define SPM_RC_CENTRAL_CFG1_BIT		(0x1)

/*CONSYS_PWR_CONN_ACK_REG*/
#define CONSYS_PWR_ON_ACK_BIT		(0x1 << 1)

/*CONSYS_PWR_CONN_ACK_S_REG*/
#define CONSYS_PWR_ON_ACK_S_BIT		(0x1 << 1)

/*CONSYS_PWR_CONN_TOP2_ACK_REG*/
#define CONSYS_TOP2_PWR_ON_ACK_BIT	(0x1 << 30)

/*CONSYS_PWR_CONN_TOP2_ACK_S_REG*/
#define CONSYS_TOP2_PWR_ON_ACK_S_BIT	(0x1 << 30)

/*CONSYS_WD_SYS_RST_REG*/
#define CONSYS_WD_SYS_RST_CTRL_KEY	(0x88 << 24)
#define CONSYS_WD_SYS_RST_BIT		(0x1 << 9)

/*CONSYS_MCU_CFG_ACR_REG*/
#define CONSYS_MCU_CFG_ACR_MBIST_BIT	(0x1 << 0 | 0x1 << 1)

/*control app2cnn_osc_en*/
#define CONSYS_AP2CONN_OSC_EN_BIT	(0x1 << 10)
#define CONSYS_AP2CONN_WAKEUP_BIT	(0x1 << 9)

/* EMI part mapping & ctrl*/
#define CONSYS_EMI_COREDUMP_OFFSET	(0x68000)
#define CONSYS_EMI_COREDUMP_MEM_SIZE	(0xa0000)
#define CONSYS_EMI_AP_PHY_OFFSET	(0x00000)
#define CONSYS_EMI_AP_PHY_BASE		(0x80068000)
#define CONSYS_EMI_FW_PHY_BASE		(0xf0068000)
#define CONSYS_EMI_PAGED_TRACE_OFFSET	(0x400)
#define CONSYS_EMI_PAGED_DUMP_OFFSET	(0x8400)
#define CONSYS_EMI_FULL_DUMP_OFFSET	(0x10400)
#define CONSYS_EMI_MET_DATA_OFFSET	(0x0)

#define CONSYS_EMI_RAM_BT_BUILDTIME_OFFSET	(0x68300)
#define CONSYS_EMI_RAM_WIFI_BUILDTIME_OFFSET	(0x68310)
#define CONSYS_EMI_RAM_MCU_BUILDTIME_OFFSET	(0x68320)
#define CONSYS_EMI_PATCH_MCU_BUILDTIME_OFFSET	(0x68330)

#define CONSYS_EMI_BT_ENTRY_OFFSET	(0x504)
#define CONSYS_EMI_BT_ENTRY_ADDRESS	(0xF0110000)
#define CONSYS_EMI_WIFI_ENTRY_OFFSET	(0x508)
#define CONSYS_EMI_WIFI_ENTRY_ADDRESS	(0xF01D0000)

/* AP_PCCIF4_BASE Register */
#define INFRASYS_COMMON_AP2MD_PCCIF4_AP_PERI_AP_CCU_CONFIG (0x314)
#define INFRASYS_COMMON_AP2MD_PCCIF4_AP_PCCIF_ACK_OFFSET (0x14)
#define INFRASYS_COMMON_AP2MD_CON_PWR_ON_CON_SW_READY_MASK (0x3 << 0)

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

/**********************************************************************/
/* Base: conn_mcu_cfg_on_base (0x180a_3000) */
/**********************************************************************/


/**********************************************************************/
/* PMIC mt6359P define for Quark project only*/
/**********************************************************************/
#ifndef MT6359_PMIC_REG_BASE

#define MT6359_PMIC_REG_BASE                 ((unsigned int)(0x0))

#define MT6359_BUCK_VS2_VOTER_SET            (MT6359_PMIC_REG_BASE+0x18ac)
#define MT6359_BUCK_VS2_VOTER_CLR            (MT6359_PMIC_REG_BASE+0x18ae)
#define MT6359_LDO_VCN33_1_CON0              (MT6359_PMIC_REG_BASE+0x1be2)
#define MT6359_LDO_VCN33_1_OP_EN_SET         (MT6359_PMIC_REG_BASE+0x1bea)
#define MT6359_LDO_VCN33_1_OP_CFG_SET        (MT6359_PMIC_REG_BASE+0x1bf0)
#define MT6359_LDO_VCN33_2_CON0              (MT6359_PMIC_REG_BASE+0x1c08)
#define MT6359_LDO_VCN33_2_OP_EN_SET         (MT6359_PMIC_REG_BASE+0x1c10)
#define MT6359_LDO_VCN33_2_OP_CFG_SET        (MT6359_PMIC_REG_BASE+0x1c16)
#define MT6359_LDO_VCN13_CON0                (MT6359_PMIC_REG_BASE+0x1c1c)
#define MT6359_LDO_VCN13_OP_EN_SET           (MT6359_PMIC_REG_BASE+0x1c24)
#define MT6359_LDO_VCN13_OP_CFG_SET          (MT6359_PMIC_REG_BASE+0x1c2a)
#define MT6359_LDO_VCN18_CON0                (MT6359_PMIC_REG_BASE+0x1c2e)
#define MT6359_LDO_VCN18_OP_EN_SET           (MT6359_PMIC_REG_BASE+0x1c36)
#define MT6359_LDO_VCN18_OP_CFG_SET          (MT6359_PMIC_REG_BASE+0x1c3c)
#define MT6359_VCN13_ANA_CON0                (MT6359_PMIC_REG_BASE+0x202e)

#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_ADDR                  \
	MT6359_BUCK_VS2_VOTER_SET
#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_MASK                  0xFFF
#define PMIC_RG_BUCK_VS2_VOTER_EN_SET_SHIFT                 0
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_ADDR                  \
	MT6359_BUCK_VS2_VOTER_CLR
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_MASK                  0xFFF
#define PMIC_RG_BUCK_VS2_VOTER_EN_CLR_SHIFT                 0
#define PMIC_RG_LDO_VCN33_1_LP_ADDR                         \
	MT6359_LDO_VCN33_1_CON0
#define PMIC_RG_LDO_VCN33_1_LP_MASK                         0x1
#define PMIC_RG_LDO_VCN33_1_LP_SHIFT                        1
#define PMIC_RG_LDO_VCN33_1_OP_EN_SET_ADDR                  \
	MT6359_LDO_VCN33_1_OP_EN_SET
#define PMIC_RG_LDO_VCN33_1_OP_CFG_SET_ADDR                 \
	MT6359_LDO_VCN33_1_OP_CFG_SET
#define PMIC_RG_LDO_VCN33_2_LP_ADDR                         \
	MT6359_LDO_VCN33_2_CON0
#define PMIC_RG_LDO_VCN33_2_LP_MASK                         0x1
#define PMIC_RG_LDO_VCN33_2_LP_SHIFT                        1
#define PMIC_RG_LDO_VCN33_2_OP_EN_SET_ADDR                  \
	MT6359_LDO_VCN33_2_OP_EN_SET
#define PMIC_RG_LDO_VCN33_2_OP_CFG_SET_ADDR                 \
	MT6359_LDO_VCN33_2_OP_CFG_SET
#define PMIC_RG_LDO_VCN13_LP_ADDR                           \
	MT6359_LDO_VCN13_CON0
#define PMIC_RG_LDO_VCN13_LP_MASK                           0x1
#define PMIC_RG_LDO_VCN13_LP_SHIFT                          1
#define PMIC_RG_LDO_VCN13_OP_EN_SET_ADDR                    \
	MT6359_LDO_VCN13_OP_EN_SET
#define PMIC_RG_LDO_VCN13_OP_CFG_SET_ADDR                   \
	MT6359_LDO_VCN13_OP_CFG_SET
#define PMIC_RG_LDO_VCN18_LP_ADDR                           \
	MT6359_LDO_VCN18_CON0
#define PMIC_RG_LDO_VCN18_LP_MASK                           0x1
#define PMIC_RG_LDO_VCN18_LP_SHIFT                          1
#define PMIC_RG_LDO_VCN18_OP_EN_SET_ADDR                    \
	MT6359_LDO_VCN18_OP_EN_SET
#define PMIC_RG_LDO_VCN18_OP_CFG_SET_ADDR                   \
	MT6359_LDO_VCN18_OP_CFG_SET
#define PMIC_RG_VCN13_VOCAL_ADDR                            \
	MT6359_VCN13_ANA_CON0
#define PMIC_RG_VCN13_VOCAL_MASK                            0xF
#define PMIC_RG_VCN13_VOCAL_SHIFT                           0

#endif

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

#endif /* _MTK_MT6873_H_ */
