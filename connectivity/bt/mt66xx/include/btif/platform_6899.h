/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _PLATFORM_6899_H
#define _PLATFORM_6899_H

#include "btmtk_define.h"
#include "platform_base.h"
#include "conninfra.h"

/*********************************************************************
*
*  Connsys Control Register Definition
*
**********************************************************************
*/

/*
 * EMI region
 */
#define BT_EMI_BASE_OFFSET				0x000000000

/*
 * ConnInfra Control Register Region:
 *	  (CONN_INFRA_BASE) ~ (CONN_INFRA_BASE + CONN_INFRA_LENGTH)
 */
#define CONN_INFRA_BASE					0x18000000 /* variable */

/*
 * Conninfra CFG AO
 *
 */
#define CONN_INFRA_CFG_AO_BASE				CON_REG_INFRACFG_AO_ADDR
#define BGF_PAD_EINT					(CONN_INFRA_CFG_AO_BASE + 0xF00)

/*
 * ConnInfra RGU Region
 * 0x18000000
 */
#define CONN_INFRA_RGU_START				CON_REG_INFRA_RGU_ADDR

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL  		(CONN_INFRA_RGU_START + 0x0020)
#define BGF_PWR_CTL_B					BIT(7)

#define CONN_INFRA_RGU_BGFSYS_CPU_SW_RST		(CONN_INFRA_RGU_START + 0x0124)
#define BGF_CPU_SW_RST_B				BIT(0)

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST		(CONN_INFRA_RGU_START + 0x0414)
#define BGF_ON_PWR_ACK_B				BITS(24, 25)

#define CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST	(CONN_INFRA_RGU_START + 0x0424)
#define BGF_OFF_PWR_ACK_B				BIT(24)
#define BGF_OFF_PWR_ACK_S				BIT(0)

#define CONN_INFRA_RGU_ON_SEMA_M1_SW_RST_B	(CONN_INFRA_RGU_START + 0x015C)
#define SEMA_M1_SW_RST_B			BIT(0)

/*
 * ConnInfra CFG Region
 * 0x18001000
 */
#define CONN_INFRA_CFG_START				CON_REG_INFRA_CFG_ADDR

#define CONN_INFRA_CFG_VERSION				(0x18011000)
#define CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1	(CONN_INFRA_CFG_START + 0x210)
#define CONN_INFRA_RDY					BIT(16)

#define CONN_INFRA_CONN2BT_GALS_SLP_CTL			(CONN_INFRA_CFG_START + 0x0450)
#define CONN_INFRA_CONN2BT_GALS_SLP_STATUS		(CONN_INFRA_CFG_START + 0x0454)
#define CONN2BT_SLP_PROT_TX_EN_B			BIT(0)
#define CONN2BT_SLP_PROT_TX_ACK_B			BIT(23)
#define CONN2BT_SLP_PROT_RX_EN_B			BIT(4)
#define CONN2BT_SLP_PROT_RX_ACK_B			BIT(22)

#define CONN_INFRA_BT2CONN_GALS_SLP_CTL			(CONN_INFRA_CFG_START + 0x0460)
#define CONN_INFRA_BT2CONN_GALS_SLP_STATUS		(CONN_INFRA_CFG_START + 0x0464)
#define BT2CONN_SLP_PROT_TX_EN_B			BIT(0)
#define BT2CONN_SLP_PROT_TX_ACK_B			BIT(23)
#define BT2CONN_SLP_PROT_RX_EN_B			BIT(4)
#define BT2CONN_SLP_PROT_RX_ACK_B			BIT(22)

#define CONN_INFRA_CFG_BT_PWRCTLCR0			(CONN_INFRA_CFG_START + 0x0208)
#define BT_FUNC_EN_B					BIT(0)

#define CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT		(CONN_INFRA_CFG_START + 0x0418)
#define BT_EMI_CTRL_BIT					BIT(0)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT				BIT(1)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT				BIT(2)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT				BIT(3)
#define BT_INFRA_CTRL_BIT				BIT(5)

#define CONN_INFRA_CFG_BT_MANUAL_CTRL			(CONN_INFRA_CFG_START + 0x0108)
#define BT_MTCMOS_FSM_PWR_ACK				BIT(27)

#define CONN_INFRA_CFG_HOST_CSR				(CONN_INFRA_CFG_START + 0x0600)
#define BT_CFG_HOST_CSR_CLR				(0x3FF)

/*
 * Connsys Host CSR Top Region
 * 0x18060000
 */
#define CONN_HOST_CSR_TOP_START				CON_REG_SPM_BASE_ADDR

#define CONN_INFRA_WAKEUP_BT				(CONN_HOST_CSR_TOP_START + 0x01A8)
#define CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL		(CONN_HOST_CSR_TOP_START + 0x0184)
#define CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES		(CONN_HOST_CSR_TOP_START + 0x0A10)
#define BGFSYS_ON_ISO_EN				BITS(18, 19)
#define BGFSYS_ON_ISO_EN_VALUE				BIT(18)

#define CONN_REMAP_ADDR					(CONN_HOST_CSR_TOP_START + 0x01C4)

#define CONN_INFRA_ON2OFF_SLP_PROT_ACK			BIT(5)

#define CONN_MCU_PC					(CONN_HOST_CSR_TOP_START + 0x22C)

#define BGF_LPCTL					(CONN_HOST_CSR_TOP_START + 0x0030)
#define BGF_HOST_SET_FW_OWN_B				BIT(0)
#define BGF_HOST_CLR_FW_OWN_B				BIT(1)
#define BGF_OWNER_STATE_SYNC_B				BIT(2)

#define BGF_IRQ_STAT					(CONN_HOST_CSR_TOP_START + 0x0034)
#define BGF_IRQ_FW_OWN_CLR_B				BIT(0)

#define BGF_IRQ_EN					(CONN_HOST_CSR_TOP_START + 0x0038)
#define BGF_IRQ_DRV_OWN_EN_B				BIT(0)
#define BGF_IRQ_FW_OWN_EN_B				BIT(1)

#define BGF_IRQ_STAT2					(CONN_HOST_CSR_TOP_START + 0x003C)
#define BGF_IRQ_FW_OWN_SET_B				BIT(0)

#define CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT		(CONN_HOST_CSR_TOP_START + 0xC00)
#define CONNINFRA_READY_B				BIT(15)

#define CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL	(CONN_HOST_CSR_TOP_START + 0xC04)


/*
 * BGFSYS Control Register Region:
 *     (BGFSYS_BASE) ~ (BGFSYS_BASE + BGFSYS_LENGTH)
 * 0x18800000
 */
#define BGF_MCCR					(BGF_REG_BASE_ADDR + 0x0100)
#define BGF_CON_CR_AHB_AUTO_DIS				BIT(31)

#define BGF_MCCR_SET					(BGF_REG_BASE_ADDR + 0x0104)
#define BGF_CON_CR_AHB_STOP				(0x304)

#define BGF_SW_IRQ_RESET_ADDR				(0x1803F014)
#define BGF_SW_IRQ_STATUS				(0x1803F010)
#define BGF_FW2AP_NOTIFY				BIT(3)
#define BGF_WHOLE_CHIP_RESET				BIT(2)
#define BGF_SUBSYS_CHIP_RESET				BIT(1)
#define BGF_FW_LOG_NOTIFY				BIT(0)

#define BGF_MCU_CFG_SW_DBG_CTL				(BGF_REG_BASE_ADDR + 0x016C)

#define BGF_IP_VERSION					(BGF_REG_INFO_BASE_ADDR)
#define BGF_IP_VER_ID					0x020A0000

#define BGF_MCUCIRQ 					(0x1803F00C)
#define BGF_BTIF0_WAKEUP_OUT_B				BIT(0) | BIT(1)
#define BEIF_EMI_OFFSET					(0xBA0000)
#define BEIF_EMI_SIZE					(1024*32)

/*
 * BGF Info base address
 * 0x18810000
 */
#define BGF_BT_CFG_BGF_CKGEN0_HCLK_SRC_SEL_FRZ_WIN_EN	(BGF_REG_INFO_BASE_ADDR + 0x100)
#define HLCK_SWITCH_FRZ_WINDOW_B			BIT(2)

/*
 * Unmapping address
 *
 */
#define BGF_MCUSYS_DLY_CHAIN_CTL			(0x18820284)

#define BGF_DRIVER_DUMP_BASE				(0x18023A00)

#define BGF2AP_CONN_INFRA_ON_CCIF4_BGF2AP_PCCIF_ACK_ADDR       (0x1803E014)

/*********************************************************************
*
* CR/Constant value for specific project
*
**********************************************************************
*/
#define BIN_NAME_MCU			"soc7_0_ram_mcu_1"
#define BIN_NAME_BT			"soc7_0_ram_bt_1"
#define CONN_INFRA_CFG_ID		(0x02050403)

#define MET_EMI_ADDR	(0x2BC00)
#define BT_SSPM_TIMER	(0x1c011008)

/*********************************************************************
*
* Utility APIs
*
**********************************************************************
*/
static inline void beif_notify_fw(void)
{
#if (CFG_BT_ATF_SUPPORT == 1)
	beif_notify_fw_smc(SMC_BT_SET_BEIF_REG);
#else
	bt_write_cr(BGF_MCUCIRQ, BGF_BTIF0_WAKEUP_OUT_B, FALSE);
#endif
}

/* bgfsys_ccif_on
 *
 *    MD coex indication, BT driver should set relative control register to info
 *    MD that BT is on
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *    N/A
 *
 * Todo:
 *    Use pre-mapped memory, don't know why it doesn't work
 */
static inline void bgfsys_ccif_on(void)
{

	uint8_t *ccif_base = ioremap(0x10001BF0, 0x10);

	if (ccif_base == NULL) {
		BTMTK_ERR("%s: remapping ccif_base fail", __func__);
		return;
	}

	/* CONSYS_BGF_PWR_ON, 0x10001BF0[31:24] = 8'b1 */
	REG_WRITEL(ccif_base, BITS(24, 31));
	iounmap(ccif_base);
}

/* bgfsys_ccif_off
 *
 *    MD coex indication, BT driver should set relative control register to info
 *    MD that BT is off
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *    N/A
 *
 * Todo:
 *    Use pre-mapped memory, don't know why it doesn't work
 */
static inline void bgfsys_ccif_off(void)
{
	uint8_t *ccif_base = NULL, *bgf2md_base = NULL;

	ccif_base = ioremap(0x10001BF0, 0x10);
	if (ccif_base == NULL) {
		BTMTK_ERR("%s: remapping ccif_base fail", __func__);
		return;
	}

	/* CONSYS_BGF_PWR_ON, 0x10001BF0[31:24] = 8'b1 */
	REG_WRITEL(ccif_base, BITS(24, 31));
	iounmap(ccif_base);

	bgf2md_base = ioremap(0x1025C000, 0x100);
	if (bgf2md_base == NULL) {
		BTMTK_ERR("%s: remapping bgf2md_base fail", __func__);
		return;
	}

	/* set PCCIF5 RX ACK, 0x1025C014[0:7] = 8'b1 */
	REG_WRITEL(bgf2md_base + 0x14, 0xFF);
	iounmap(bgf2md_base);
}

/* bgfsys_check_conninfra_ready
 *
 *    wakeup conninfra and check its ready status
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *     0 if success, otherwise error code
 *
 */
static int32_t bgfsys_check_conninfra_ready(void)
{
	int32_t i = 0, retry = 10, hang_ret = 0, retry_wakeup_count = 3;
	uint32_t value = 0;
	uint8_t* conninfra_cfg_version_base = NULL;
	u_int8_t conninfra_cfg_id_rdy = FALSE;

	conninfra_cfg_version_base = ioremap(CONN_INFRA_CFG_VERSION, 0x10);
	if (conninfra_cfg_version_base == NULL) {
		BTMTK_ERR("CONNINFRA_CFG_VERSION ioremap fail");
		return -1;
	}

	do {
#if (CFG_BT_ATF_SUPPORT == 1)
		bt_conn_infra_on_off_smc(SMC_BT_CONN_INFRA_FORCE_ON_OFF_OPID, 1);
#else
		/* wake up conn_infra off */
		SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
#endif
		/* wait 200 us to avoid fake ready */
		udelay(200);
		for (i = 0; i < retry; i++) {
			value = REG_READL(CONN_INFRA_WAKEUP_BT);
			if (value)
				break;
			else
				BTMTK_INFO("CONN_INFRA_WAKEUP_BT value = [%02d][0x%08x]", i, value);
			/* retry */
			SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
			usleep_range(USLEEP_1MS_L, USLEEP_1MS_H);
		}

		/* polling conninfra version id */
		for (i = 0; i < retry; i++) {
			value = REG_READL(conninfra_cfg_version_base);
			if (value == CONN_INFRA_CFG_ID) {
				conninfra_cfg_id_rdy = TRUE;
				break;
			}

			BTMTK_DBG("connifra cfg version = 0x%08x", value);
			value = REG_READL(CONN_INFRA_WAKEUP_BT);
			if (!value)
				BTMTK_INFO("CONN_INFRA_WAKEUP_BT value = [0x%08x]", value);

			usleep_range(USLEEP_1MS_L, USLEEP_1MS_H);
		}

		if (conninfra_cfg_id_rdy) {
			for (i = 0; i < retry; i++) {
				value = REG_READL(CONN_INFRA_CFG_ON_CONN_INFRA_CFG_PWRCTRL1) &
					CONN_INFRA_RDY;
				BTMTK_DBG("connifra cfg power control = 0x%08x", value);
				if (value == CONN_INFRA_RDY) {
					iounmap(conninfra_cfg_version_base);
					return 0;
				}

				usleep_range(500, 550);
			}
		} else {
			value = REG_READL(CONN_INFRA_WAKEUP_BT);
			BTMTK_INFO("dump CONN_INFRA_WAKEUP_BT value = [0x%08x]", value);
			/* Check conninfra bus */
			if (!conninfra_reg_readable()) {
				hang_ret = conninfra_is_bus_hang();
				if (hang_ret == CONNINFRA_AP2CONN_CLK_ERR)
					BTMTK_WARN("Possible turn off by other module, try wakeup again");
				else if (hang_ret > 0) {
					BTMTK_ERR("conninfra bus is hang, needs reset");
					conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "bus hang");
				}
				BTMTK_ERR("conninfra not readable, but not bus hang ret = %d", hang_ret);
			}
		}
		retry_wakeup_count--;
	} while (retry_wakeup_count > 0);

	iounmap(conninfra_cfg_version_base);
	return -1;
}

static inline int32_t bgfsys_clr_host_csr(void)
{
        if (bgfsys_check_conninfra_ready()) {
		BTMTK_WARN("conninfra wakeup fail");
		return -1;
	}

        REG_WRITEL(CONN_INFRA_CFG_HOST_CSR, BT_CFG_HOST_CSR_CLR);

#if (CFG_BT_ATF_SUPPORT == 1)
        bt_conn_infra_on_off_smc(SMC_BT_CONN_INFRA_FORCE_ON_OFF_OPID, 0);
#else
        /* release conn_infra force on */
        CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
#endif
	return 0;
}

static inline u_int8_t bt_is_bgf_bus_timeout(void)
{
	int32_t mailbox_status = 0;

	/*
	 * Conn-infra bus timeout : 160ms
	 * Bgf bus timeout : 80ms
	 *
	 * There's still a case that we pass conninfra check but still get
	 * bus hang, so we have to check mail box for sure
	 */
	mailbox_status = REG_READL(CON_REG_SPM_BASE_ADDR + 0x268);
	if (mailbox_status != 0 &&
		(mailbox_status & 0xFF000000) != 0x87000000) {
		BTMTK_INFO("mailbox_status = 0x%08x", mailbox_status);
		return TRUE;
	}

	return FALSE;
}

/* bt_dump_cpucpr
 *
 *    Dump cpu counter for a period of time
 *
 * Arguments:
 *     [IN] times  	how many times to dump CR
 *     [IN] sleep_ms	sleep interval b/w each dump
 *
 * Return Value:
 *     N/A
 *
 */
static void inline bt_dump_cpupcr(uint32_t times, uint32_t sleep_ms)
{
	uint32_t i = 0;
	uint32_t value = 0;

	for(i = 0; i < times; i++) {
		value = REG_READL(CONN_MCU_PC);
		BTMTK_DBG("%s: bt pc=0x%08x", __func__, value);
		if (sleep_ms > 0)
			msleep(sleep_ms);
	}
}

/* DE Defined: dump all BGF host csr */
static void inline bt_dump_bgfsys_host_csr(void)
{
	uint32_t value = 0, i = 0, cr_count = 4;
	uint8_t *base = NULL;

	/* E20 + 4*/
	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE);
		return;
	}

	BT_DUMP_CR_INIT("[BGF host csr]", cr_count);
	/* 18023A00 - 18023A08 */
	for (i = 0; i <= 8; i+=4) {
		value = REG_READL(base + i);
		if (BT_DUMP_CR_PRINT(value)) {
			iounmap(base);
			return;
		}
	}
	/* 18023E20*/
	value = bt_read_cr(0x18023E20);
	BT_DUMP_CR_PRINT(value);
	iounmap(base);
}

/* DE Defined: dump all BGF MCU core debug flag */
/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_mcu_core_flag(void)
{
	uint32_t value = 0, i = 0, cr_count = 38;
	uint8_t *base = NULL;

	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE);
		return;
	}

	BT_DUMP_CR_INIT("[BGF MCU core debug flag]", cr_count);
	/* write 0x18023A04, read 0x18023A00 */
	for (i = 0xC0015D00; i <= 0xC0015D25; i++) {
		REG_WRITEL(base + 0x04, i);
		value = REG_READL(base);
		if (BT_DUMP_CR_PRINT(value))
			break;
	}
	iounmap(base);
}

/* DE Defined: dump all BGF MCU PC log */
/* new name: dump all BGF MCU pc/lr log record */
static inline void bt_dump_bgfsys_mcu_pc_log(void)
{
	uint32_t value = 0, i = 0, cr_count = 85;
	uint8_t *base = NULL;

	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE);
		return;
	}

	BT_DUMP_CR_INIT("[BGF MCU PC/LR log]", cr_count);
	/* write 0x18023A04, read 0x18023A00 */
	for (i = 0xC0010D00; i <= 0xC0010D54; i++) {
		REG_WRITEL(base + 0x04, i);
		value = REG_READL(base);
		if (BT_DUMP_CR_PRINT(value))
			break;
	}
	iounmap(base);
}

/* DE Defined: dump all BGF BUS debug flag */
/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_bus_flag(void)
{
	uint32_t value = 0, i = 0, cr_count = 10;
	uint8_t *base = NULL;

	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE);
		return;
	}

	BT_DUMP_CR_INIT("[BGF BUS debug flag]", cr_count);
	/* write 0x18023A04, read 0x18023A00 */
	for (i = 0xC0012510; i <= 0xC00125A0; i += 0x10) {
		REG_WRITEL(base + 0x04, i);
		value = REG_READL(base);
		if (BT_DUMP_CR_PRINT(value))
			break;
	}
	iounmap(base);
}

/* DE Defined: dump all BGF_MCU_DMA debug_flag */
/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgf_mcu_dma_flag(void)
{
	uint32_t value = 0, i = 0, cr_count = 8;
	uint8_t *base = NULL;

	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE);
		return;
	}

	BT_DUMP_CR_INIT("[BGF MCU DMA debug flag]", cr_count);
	/* write 0x18023A04, read 0x18023A00 */
	for (i = 0xC0011700; i <= 0xC0011707; i++) {
		REG_WRITEL(base + 0x04, i);
		value = REG_READL(base);
		if (BT_DUMP_CR_PRINT(value))
			break;
	}
	iounmap(base);
}

/* DE Defined: dump all BGF MCUSYS debug flag */
/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_mcusys_flag(void)
{
	uint32_t value = 0, i = 0, cr_count = 43;	/* 47-4, cuz skip 4*/
	uint8_t *base = NULL;

	base = ioremap(BGF_DRIVER_DUMP_BASE, 0x10);
	if (base == NULL) {
		BTMTK_ERR("ioremap [0x%08x] fail", BGF_DRIVER_DUMP_BASE + 0xA00);
		return;
	}

	BT_DUMP_CR_INIT("[BGF MCUSYS debug flag]", cr_count);
	/* write 0x18023A04, read 0x18023A00 */
	for (i = 0xC0010100; i <= 0xC0015B00; i += 0x200) {
		if (i == 0xC0010D00 || i == 0xC0011700 || i == 0xC0012500)
			continue;
		REG_WRITEL(base + 0x04, i);
		value = REG_READL(base);
		if (BT_DUMP_CR_PRINT(value))
			break;
	}
	iounmap(base);
}

static inline void bt_dump_bgfsys_suspend_wakeup_debug(void)
{
	uint32_t value = 0, cr_count = 3;

	BT_DUMP_CR_INIT("[BGF dump suspend/wakeup]", cr_count);
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x790);
	if (BT_DUMP_CR_PRINT(value))
		return;

	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x794);
	if (BT_DUMP_CR_PRINT(value))
		return;

	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xC04, 0x300508);
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0xC00);
	if (BT_DUMP_CR_PRINT(value))
		return;
}

static void bt_dump_bgfsys_all(void)
{
	/* these dump all belongs to host_csr */
	bt_dump_cpupcr(10, 0);
	bt_dump_bgfsys_host_csr();
	bt_dump_bgfsys_mcu_pc_log();
	bt_dump_bgfsys_mcu_core_flag();
	bt_dump_bgfsys_mcusys_flag();
	bt_dump_bgfsys_bus_flag();
	bt_dump_bgf_mcu_dma_flag();
}

/* bt_dump_bgfsys_debug_cr()
 *
 *    Dump all bgfsys debug cr for debuging bus hang
 *
 * Arguments:
 *     N/A
 *
 * Return Value:
 *     N/A
 *
 */
static inline void bt_dump_bgfsys_debug_cr(void)
{
	bt_dump_bgfsys_all();
}

/* bt_reg_readable_for_coredump()
 *
 *    Dump all bgfsys debug cr before coredump
 *
 * Arguments:
 *     N/A
 *
 * Return Value:
 *     N/A
 *
 */
static inline int32_t bt_reg_readable_for_coredump(void)
{
	bt_dump_bgfsys_debug_cr();
	return conninfra_reg_readable_for_coredump();
}

/* bt_cif_dump_own_cr
 *
 *    Dump fw/driver own relative cr (plus cpucpr) if
 *    driver own or fw own fail
 *
 * Arguments:
 *     N/A
 *
 * Return Value:
 *     N/A
 *
 */
static inline void bt_dump_cif_own_cr(void)
{
	bt_dump_bgfsys_all();
}

static inline int32_t bgfsys_get_sw_irq_status(void)
{
	int32_t value = 0;

	/* Check bgf bus status */
	if (bt_is_bgf_bus_timeout()) {
		bt_dump_bgfsys_all();
		return RET_SWIRQ_ST_FAIL;
	}
#if (CFG_BT_ATF_SUPPORT == 1)
        value = bgfsys_get_sw_irq_status_smc(SMC_BT_GET_SW_IRQ_STATUS);
#else
	/* read sw irq status*/
	value = bt_read_cr(BGF_SW_IRQ_STATUS);

	if (value & BGF_SUBSYS_CHIP_RESET){
		bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET, TRUE);
	} else if (value & BGF_FW_LOG_NOTIFY){
		bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY, TRUE);
	} else if (value &  BGF_WHOLE_CHIP_RESET){
		bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_WHOLE_CHIP_RESET, TRUE);
	} else if (value & BGF_FW2AP_NOTIFY){
		bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_FW2AP_NOTIFY, TRUE);
	}
#endif
	return value;
}

/*
static inline void bgfsys_ack_sw_irq_fwlog(void)
{
	bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY, TRUE);
}

static inline void bgfsys_ack_sw_irq_reset(void)
{
	bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET, TRUE);
}
*/

/* bgfsys_power_on_dump_cr
 *
 *    Dump CR for debug if power on sequence fail
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *    N/A
 *
 */
static inline void bgfsys_power_on_dump_cr(void)
{
#if 0
	uint32_t i;
	uint32_t val_w, val_r;
	int32_t is_bus_hang = 0;

	is_bus_hang = conninfra_is_bus_hang();
	BTMTK_INFO("%s: conninfra_is_bus_hang ret = %d", __func__, is_bus_hang);

	if (!CAN_DUMP_HOST_CSR(is_bus_hang)) {
		BTMTK_ERR("%s; host csr is not readable", __func__);
		return;
	}

	if(bgfsys_check_conninfra_ready())
		goto host_csr_only;

	BTMTK_INFO("%s: M0 - M3 semaphore status:", __func__);
	for (i = 0x18070400; i <= 0x18073400; i += 0x1000) {
		val_r = bt_read_cr(i);
		BTMTK_INFO("[0x%08x] = [0x%08x]", i, val_r);
	}

	BTMTK_INFO("%s: conninfra part:", __func__);
	val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x02CC);
	BTMTK_INFO("%s: REG[0x180602CC] read[0x%08x]", __func__, val_r);

	if (!is_bus_hang) {
		val_r = REG_READL(CONN_INFRA_CFG_START + 0x0610);
		BTMTK_INFO("%s: REG[0x18001610] read[0x%08x]", __func__, val_r);
		val_r = REG_READL(CONN_INFRA_CFG_START + 0x0614);
		BTMTK_INFO("%s: REG[0x18001614] read[0x%08x]", __func__, val_r);
		val_r = REG_READL(CONN_INFRA_CFG_START + 0x0150);
		BTMTK_INFO("%s: REG[0x18001150] read[0x%08x]", __func__, val_r);
		val_r = REG_READL(CONN_INFRA_CFG_START + 0x0170);
		BTMTK_INFO("%s: REG[0x18001170] read[0x%08x]", __func__, val_r);
	}

host_csr_only:
	for(i = 0x0F; i >= 0x01; i--) {
		val_w = (i << 16) + 0x0001;
		REG_WRITEL(CONN_HOST_CSR_TOP_START + 0x0128, val_w);
		val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0148);
		BTMTK_INFO("%s: REG[0x18060128] write[0x%08x], REG[0x18060148] read[0x%08x]", __func__, val_w, val_r);
	}
	for(i = 0x03; i >= 0x01; i--) {
		val_w = (i << 16) + 0x0002;
		REG_WRITEL(CONN_HOST_CSR_TOP_START + 0x0128, val_w);
		val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0148);
		BTMTK_INFO("%s: REG[0x18060128] write[0x%08x], REG[0x18060148] read[0x%08x]", __func__, val_w, val_r);
	}
	for(i = 0x04; i >= 0x01; i--) {
		val_w = (i << 16) + 0x0003;
		REG_WRITEL(CONN_HOST_CSR_TOP_START + 0x0128, val_w);
		val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0148);
		BTMTK_INFO("%s: REG[0x18060128] write[0x%08x], REG[0x18060148] read[0x%08x]", __func__, val_w, val_r);
	}

	BTMTK_INFO("%s: bgf part:", __func__);
	val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x022C);
	BTMTK_INFO("%s: REG[0x1806022C] read[0x%08x]", __func__, val_r);
	val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0230);
	BTMTK_INFO("%s: REG[0x18060230] read[0x%08x]", __func__, val_r);
	val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0234);
	BTMTK_INFO("%s: REG[0x18060234] read[0x%08x]", __func__, val_r);
	val_r = REG_READL(CONN_HOST_CSR_TOP_START + 0x0238);
	BTMTK_INFO("%s: REG[0x18060238] read[0x%08x]", __func__, val_r);
	val_r = REG_READL(CON_REG_SPM_BASE_ADDR + 0x268);
	BTMTK_INFO("%s: REG[0x18060268] read[0x%08x]", __func__, val_r);
	val_r = REG_READL(CON_REG_SPM_BASE_ADDR + 0x26C);
	BTMTK_INFO("%s: REG[0x1806026C] read[0x%08x]", __func__, val_r);

	bt_dump_bgfsys_all();

	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
#else
	bt_dump_bgfsys_all();
#endif
}

static void bgfsys_dump_conn_wt_slp_ctrl_reg(void)
{
#if 0
	uint8_t *base = NULL;
	uint32_t i = 0;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;

	base = ioremap(0x18005100, 0x100);
	if (base) {
		for(i = 0x20; i <= 0x34; i+=4) {
			ret = snprintf(pos, (end - pos + 1)," 0x%08x = [0x%08x]", 0x18005100 + i, REG_READL(base + i));
			if (ret < 0 || ret >= (end - pos + 1)) {
				BTMTK_ERR("snprintf [0x%02x] fail", i);
				break;
			}
			pos += ret;
		}
		iounmap(base);
	} else
		BTMTK_ERR("%s: remapping 0x18005100 fail", __func__);

	base = ioremap(0x180050A8, 0x10);
	if (base) {
		ret = snprintf(pos, (end - pos + 1)," 0x180050A8 = [0x%08x]", REG_READL(base));
		if (ret < 0 || ret >= (end - pos + 1)) {
			BTMTK_ERR("snprintf 0x180050A8 fail");
		} else {
			pos += ret;
                }
		iounmap(base);
	} else
		BTMTK_ERR("%s: remapping 0x180050A8 fail", __func__);

	BTMTK_INFO("%s:%s",__func__,  g_dump_cr_buffer);
#endif
}

/*********************************************************************
*
* Power On Sequence
*
**********************************************************************
*/

/* bgfsys_power_on
 *
 *    BGF MCU power on sequence
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *     0 if success, otherwise error code
 *
 */
static inline int32_t bgfsys_power_on(void)
{
	uint32_t value;
	int32_t retry = POS_POLLING_RTY_LMT;
	uint32_t delay_ms = 5;
	uint32_t mcu_idle, mcu_pc;
	uint8_t *base = NULL;

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* power on bgfsys on */
	value = 0x42540080 |
		(REG_READL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL) & ~(BITS(16, 31) | BIT(7)));
	REG_WRITEL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);

	/* polling bgfsys_on iso_en and reset */
	do {
		value = BGFSYS_ON_ISO_EN &
			REG_READL(CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES);

		BTMTK_DBG("consys power states = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != BGFSYS_ON_ISO_EN_VALUE && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("consys power states = 0x%08x", value);
		goto error;
	}

	/* enable bt function en */
	SET_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B);
	if (!conninfra_reg_readable()) {
		if (conninfra_is_bus_hang() > 0) {
			BTMTK_ERR("%s: check conninfra status fail after set CONN_INFRA_CFG_BT_PWRCTLCR0!", __func__);
			goto error;
		}
	}
	/* polling bgfsys top off power ack bits until they are asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BGF_OFF_PWR_ACK_B &
			REG_READL(CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST);
		BTMTK_DBG("bgfsys off top power ack_b = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != BGF_OFF_PWR_ACK_B && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bgfsys off top power ack_b = 0x%08x", value);
		goto error;
	}

	retry = POS_POLLING_RTY_LMT;
	do {
		value = BGF_OFF_PWR_ACK_S &
			REG_READL(CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST);
		BTMTK_DBG("bgfsys off top power ack_s = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != BGF_OFF_PWR_ACK_S && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bgfsys off top power ack_s = 0x%08x", value);
		goto error;
	}

	/* disable conn2bt slp_prot rx en */
	CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_RX_EN_B);
	/* polling conn2bt slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_STATUS);
		BTMTK_DBG("conn2bt slp_prot rx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("conn2bt slp_prot rx ack = 0x%08x", value);
		goto error;
	}

	/* disable conn2bt slp_prot tx en */
	CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_TX_EN_B);
	/* polling conn2bt slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_STATUS);
		BTMTK_DBG("conn2bt slp_prot tx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("conn2bt slp_prot tx ack = 0x%08x", value);
		goto error;
	}

	/* disable bt2conn slp_prot rx en */
	CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_RX_EN_B);
	/* polling bt2conn slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_STATUS);
		BTMTK_DBG("bt2conn slp_prot rx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bt2conn slp_prot rx ack = 0x%08x", value);
		goto error;
	}

	/* disable bt2conn slp_prot tx en */
	CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_TX_EN_B);
	/* polling bt2conn slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_STATUS);
		BTMTK_DBG("bt2conn slp_prot tx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bt2conn slp_prot tx ack = 0x%08x", value);
		goto error;
	}

	usleep_range(400, 440);

	/* read and check bgfsys version id */
	value = REG_READL(BGF_IP_VERSION);
	BTMTK_INFO("bgfsys version id = 0x%08x", value);
	if (value != BGF_IP_VER_ID)
		goto error;

	/* clear con_cr_ahb_auto_dis */
	CLR_BIT(BGF_MCCR, BGF_CON_CR_AHB_AUTO_DIS);

	/* set con_cr_ahb_stop, con_cr_btif_stop[1:0] */
	REG_WRITEL(BGF_MCCR_SET, BGF_CON_CR_AHB_STOP);

	/* set mcu_mem_pdn_delay chain dummy cr for the timing */
	value = 0x80000 | (bt_read_cr(BGF_MCUSYS_DLY_CHAIN_CTL) & ~ BITS(16, 19));
	bt_write_cr(BGF_MCUSYS_DLY_CHAIN_CTL, value, FALSE);

	/* enable hclk switch freeze window */
	SET_BIT(BGF_BT_CFG_BGF_CKGEN0_HCLK_SRC_SEL_FRZ_WIN_EN, HLCK_SWITCH_FRZ_WINDOW_B);

	/* host_csr debug bgfsys_on_top select setting */
	value = 0x300001 |
		(REG_READL(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL) & BITS(22, 31));
	REG_WRITEL(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL, value);

	/* wait conn_infra cmdbt restore done */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONNINFRA_READY_B &
			REG_READL(CONN_HOST_CSR_TOP_BGF_MONFLG_ON_OUT);
		BTMTK_DBG("signal is conn_infra_rdy = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != CONNINFRA_READY_B && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("signal is conn_infra_rdy = 0x%08x", value);
		goto error;
	}

	/*clear fw own IRQ*/
	BTMTK_DBG("clear fw own IRQ");
	REG_WRITEL(BGF_IRQ_STAT, BGF_IRQ_FW_OWN_SET_B);
	REG_WRITEL(BGF_IRQ_STAT2, BGF_IRQ_FW_OWN_SET_B);

	/* ack AP2BGF CCIF */
	bt_write_cr(BGF2AP_CONN_INFRA_ON_CCIF4_BGF2AP_PCCIF_ACK_ADDR, 0xFF, FALSE);

	/* release n10 cpu core */
	SET_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* clear host_csr debug bgfsys_on_top select */
	value = REG_READL(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL) & BITS(22, 31);
	REG_WRITEL(CONN_HOST_CSR_TOP_CR_HOSTCSR2BGF_ON_DBG_SEL, value);

	/*
	 * polling BGFSYS MCU sw_dbg_ctl cr to wait it becomes 0x1D1E,
	 * which indicates that the power-on part of ROM is completed.
	 */
	base = ioremap(0x18023A00, 0x10);
	if (!base)
		BTMTK_WARN("ioremap 0x18023A00 fail");
	else
		REG_WRITEL(base + 4, 0xC0010D2A);

	retry = IDLE_LOOP_RTY_LMT;
	base = ioremap(0x18023A00, 0x10);
        if (!base)
                BTMTK_WARN("ioremap 0x18023A00 fail");

	do {
		if (conninfra_reg_readable()) {
			if (base) {
				mcu_pc = REG_READL(base);
				BTMTK_INFO("MCU pc = 0x%08x", mcu_pc);
			}
			mcu_idle = REG_READL(BGF_MCU_CFG_SW_DBG_CTL);

			if (0x1D1E == mcu_idle){
				BTMTK_INFO("MCU sw_dbg_ctl = 0x%08x", mcu_idle);
				break;
			}
		} else {
			if (base)
				iounmap(base);
			bgfsys_power_on_dump_cr();
			return -1;
		}

		msleep(delay_ms);
		retry--;
	} while (retry > 0);

	if (base)
		iounmap(base);

	if (retry == 0) {
		bt_dump_cif_own_cr();
		return -1;
	}

	/* reset BGF_SW_IRQ */
	bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY, TRUE);
	bt_write_cr(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET, TRUE);

	/* release conn_infra force on, force on at bgfsys_mcu_rom_patch_dl */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
	return 0;

error:
	bgfsys_power_on_dump_cr();

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
	return -1;
}

/* bgfsys_power_off
 *
 *    BGF MCU power off sequence
 *
 * Arguments:
 *    N/A
 *
 * Return Value:
 *     0 if success, otherwise error code
 *
 */
static inline int32_t bgfsys_power_off(void)
{
	uint32_t value = 0;
	int32_t retry = POS_POLLING_RTY_LMT;
	int32_t ret = 0;
	uint32_t addr = 0;
	uint32_t *remap_addr = NULL;

	/* wake up conn_infra */
	ret = bgfsys_check_conninfra_ready();
	if (ret)
		return ret;

	/* Dump bysys_status */
	value = bt_read_cr(0x18053858);
	BTMTK_INFO("BTSYS DUMP 0x18053858: %08X", value);

	/* CONN_BT_RF_CONTROL_RF_STATE_GO2IDLE */
	bt_write_cr(0x1883401C, BIT(8), TRUE);
	usleep_range(10, 20);
	value = bt_read_cr(0x1883401C) & (~BIT(8));
	bt_write_cr(0x1883401C, value, FALSE);

	/* enable bt2conn slp_prot tx en */
	SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_TX_EN_B);
	/* polling bt2conn slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_STATUS);
		BTMTK_DBG("bt2conn slp_prot tx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -1;

	/* enable bt2conn slp_prot rx en */
	SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_RX_EN_B);
	/* polling bt2conn slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_STATUS);
		BTMTK_DBG("bt2conn slp_prot rx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -2;

	/* enable conn2bt slp_prot tx en */
	SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_TX_EN_B);
	/* polling conn2bt slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_STATUS);
		BTMTK_DBG("conn2bt slp_prot tx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -2;

	/* enable conn2bt slp_prot rx en */
	SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL, CONN2BT_SLP_PROT_RX_EN_B);
	/* polling conn2bt slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_STATUS);
		BTMTK_DBG("conn2bt slp_prot rx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -1;

	if (ret == -2)
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "Power off fail");

	/* disable bt function en */
	CLR_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B);

	/* power off bgfsys on */
	value = 0x42540000 |
		(REG_READL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL) & ~(BITS(16, 31) | BIT(7)));
	REG_WRITEL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL, value);

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* reset IRQ */
	usleep_range(1000, 1100);
	value = bgfsys_get_sw_irq_status();
	BTMTK_INFO("%s: bgf_status[0x%08x]", __func__, value);

	/* Disable A-die top_ck_en_2
	 * 0x18003128	WR	0x18003128[0] == 1'b0
	 * 0x18003128	POLLING 0x18003128[1] == 1'b0
	 */
	addr = 0x18003128;
	remap_addr = ioremap(addr, 4);
	if (remap_addr) {
		value = BIT(0) & REG_READL(remap_addr);
		BTMTK_INFO("CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2[0] = 0x%08x", value);
		usleep_range(50, 60);

		CLR_BIT(remap_addr, BIT(0));
		retry = POS_POLLING_RTY_LMT;
		do {
			value = BIT(1) & REG_READL(remap_addr);
			BTMTK_DBG("CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_WB_SLP_TOP_CK_2 = 0x%08x", value);
			usleep_range(500, 550);
			retry --;
		} while (value != 0 && retry > 0);

		if (retry == 0)
			ret = -1;
		iounmap(remap_addr);
	} else {
		BTMTK_ERR("ioremap [0x%08x] fail", addr);
		ret = -1;
	}

	for (addr = 0x18071200; addr <= 0x18071260; addr += 4)
		bt_write_cr(addr, 0x1, TRUE);

	/* release BTSYS semaphore (total 25 with 1 CR) */
	CLR_BIT(CONN_INFRA_RGU_ON_SEMA_M1_SW_RST_B, SEMA_M1_SW_RST_B);
	SET_BIT(CONN_INFRA_RGU_ON_SEMA_M1_SW_RST_B, SEMA_M1_SW_RST_B);

	/* clear bt_emi_req */
	addr = 0x18011118;
	remap_addr = ioremap(addr, 4);
	if (remap_addr) {
		SET_BIT(remap_addr, BT_EMI_CTRL_BIT);
		CLR_BIT(remap_addr, BT_EMI_CTRL_BIT);

		SET_BIT(remap_addr, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT);
		CLR_BIT(remap_addr, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT);
		CLR_BIT(remap_addr, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT);
		CLR_BIT(remap_addr, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT);

		SET_BIT(remap_addr, BT_INFRA_CTRL_BIT);
		CLR_BIT(remap_addr, BT_INFRA_CTRL_BIT);

	} else {
		BTMTK_ERR("ioremap [0x%08x] fail", addr);
		return -1;
	}

	if (ret)
		bgfsys_power_on_dump_cr();

	/* when bt turn off, a-die may still closing and OC happened, add dump to debug */
	udelay(50);
	bgfsys_dump_conn_wt_slp_ctrl_reg();

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
	return ret;
}

static inline void fwp_get_patch_names(void)
{
	uint8_t flavor = FLAVOR_NONE;
	u_int8_t has_flavor = fwp_has_flavor_bin(&flavor);

	compose_fw_name(has_flavor, flavor, BIN_NAME_MCU, BIN_NAME_BT);
}
#endif
