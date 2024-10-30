/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _PLATFORM_6893_H
#define _PLATFORM_6893_H

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
#define CONN_INFRA_CFG_AO_BASE			CON_REG_INFRACFG_AO_ADDR
#define BGF_PAD_EINT					(CONN_INFRA_CFG_AO_BASE + 0xF00)

#if 0
/*
 * CCIF Base
 *
 */
#define CONN_INFRA_CFG_CCIF_BASE		CON_REG_INFRA_CCIF_ADDR
#define CCIF_CONSYS_BGF_ADDR			(CONN_INFRA_CFG_CCIF_BASE + 0x318)

/*
 * BGF2MD Base
 *
 */
#define CONN_BGF2MD_BASE			BGF2MD_BASE_ADDR
#define CONN_PCCIF5_RX_ACK			(BGF2MD_BASE_ADDR + 0x14)
#endif

/*
 * ConnInfra RGU Region
 */
#define CONN_INFRA_RGU_START						CON_REG_INFRA_RGU_ADDR

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL  		(CONN_INFRA_RGU_START + 0x0008)
#define BGF_PWR_CTL_B                         		BIT(7)

#define CONN_INFRA_RGU_BGFSYS_CPU_SW_RST     		(CONN_INFRA_RGU_START + 0x0014)
#define BGF_CPU_SW_RST_B                     		BIT(0)

#define CONN_INFRA_RGU_BGFSYS_SW_RST_B				(CONN_INFRA_RGU_START + 0x001C)
#define BGF_SW_RST_B								BIT(0)

#define CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST		(CONN_INFRA_RGU_START + 0x0114)
#define BGF_ON_PWR_ACK_B							BITS(24, 25)

#define CONN_INFRA_RGU_BGFSYS_OFF_TOP_PWR_ACK_ST	(CONN_INFRA_RGU_START + 0x0124)
#define BGF_OFF_PWR_ACK_B							BIT(24)
#define BGF_OFF_PWR_ACK_S							BITS(0, 1)

/*
 * ConnInfra CFG Region
 */
#define CONN_INFRA_CFG_START						CON_REG_INFRA_CFG_ADDR

#define CONN_INFRA_CFG_VERSION						(CONN_INFRA_CFG_START)

#define CONN_INFRA_CONN2BT_GALS_SLP_CTL				(CONN_INFRA_CFG_START + 0x0610)
#define CONN2BT_SLP_PROT_TX_EN_B					BIT(0)
#define CONN2BT_SLP_PROT_TX_ACK_B					BIT(3)
#define CONN2BT_SLP_PROT_RX_EN_B					BIT(4)
#define CONN2BT_SLP_PROT_RX_ACK_B					BIT(7)

#define CONN_INFRA_BT2CONN_GALS_SLP_CTL				(CONN_INFRA_CFG_START + 0x0614)
#define BT2CONN_SLP_PROT_TX_EN_B					BIT(0)
#define BT2CONN_SLP_PROT_TX_ACK_B					BIT(3)
#define BT2CONN_SLP_PROT_RX_EN_B					BIT(4)
#define BT2CONN_SLP_PROT_RX_ACK_B					BIT(7)

#define CONN_INFRA_CFG_BT_PWRCTLCR0					(CONN_INFRA_CFG_START + 0x0874)
#define BT_FUNC_EN_B								BIT(0)

#define CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT		(CONN_INFRA_CFG_START + 0x0C18)
#define BT_EMI_CTRL_BIT								BIT(0)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT				BIT(1)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT				BIT(2)
#define CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT				BIT(3)

/*
 * Connsys Host CSR Top Region
 */
#define CONN_HOST_CSR_TOP_START						CON_REG_SPM_BASE_ADDR

#define CONN_INFRA_WAKEUP_BT						(CONN_HOST_CSR_TOP_START + 0x01A8)
#define CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL		(CONN_HOST_CSR_TOP_START + 0x0184)

#define CONN_REMAP_ADDR								(CONN_HOST_CSR_TOP_START + 0x01C4)

#define CONN_INFRA_ON2OFF_SLP_PROT_ACK				BIT(5)

#define CONN_MCU_PC									(CONN_HOST_CSR_TOP_START + 0x22C)

#define BGF_LPCTL									(CONN_HOST_CSR_TOP_START + 0x0030)
#define BGF_HOST_SET_FW_OWN_B						BIT(0)
#define BGF_HOST_CLR_FW_OWN_B						BIT(1)
#define BGF_OWNER_STATE_SYNC_B						BIT(2)

#define BGF_IRQ_STAT								(CONN_HOST_CSR_TOP_START + 0x0034)
#define BGF_IRQ_FW_OWN_CLR_B						BIT(0)

#define BGF_IRQ_EN									(CONN_HOST_CSR_TOP_START + 0x0038)
#define BGF_IRQ_DRV_OWN_EN_B						BIT(0)
#define BGF_IRQ_FW_OWN_EN_B							BIT(1)

#define BGF_IRQ_STAT2								(CONN_HOST_CSR_TOP_START + 0x003C)
#define BGF_IRQ_FW_OWN_SET_B						BIT(0)

/*
 * BGFSYS Control Register Region:
 *     (BGFSYS_BASE) ~ (BGFSYS_BASE + BGFSYS_LENGTH)
 */
#define BGFSYS_BASE								0x18800000

#define BGF_MCCR								(BGF_REG_BASE_ADDR + 0x0100)
#define BGF_CON_CR_AHB_AUTO_DIS					BIT(31)

#define BGF_MCCR_SET							(BGF_REG_BASE_ADDR + 0x0104)
#define BGF_CON_CR_AHB_STOP						BIT(2)

#define BGF_SW_IRQ_RESET_ADDR					(BGF_REG_BASE_ADDR + 0x014C)
#define BGF_SW_IRQ_STATUS						(BGF_REG_BASE_ADDR + 0x0150)
#define BGF_WHOLE_CHIP_RESET					BIT(26)
#define BGF_SUBSYS_CHIP_RESET					BIT(25)
#define BGF_FW_LOG_NOTIFY						BIT(24)

#define BGF_MCU_CFG_SW_DBG_CTL					(BGF_REG_BASE_ADDR + 0x016C)

#define BGF_HW_VERSION							BGF_REG_INFO_BASE_ADDR
#define BGF_HW_VER_ID							0x00008A00

#define BGF_FW_VERSION							(BGF_REG_INFO_BASE_ADDR + 0x04)
#define BGF_FW_VER_ID							0x00008A00

#define BGF_HW_CODE								(BGF_REG_INFO_BASE_ADDR + 0x08)
#define BGF_HW_CODE_ID							0x00000000

#define BGF_IP_VERSION							(BGF_REG_INFO_BASE_ADDR + 0x10)
#define BGF_IP_VER_ID							0x20010000

/*********************************************************************
*
* CR/Constant value for specific project
*
**********************************************************************
*/
#define BIN_NAME_MCU			"soc3_0_ram_mcu_1"
#define BIN_NAME_BT			"soc3_0_ram_bt_1"

#if (CONNAC20_CHIPID == 6885)
	#define CONN_INFRA_CFG_ID		(0x20010000)
#elif (CONNAC20_CHIPID == 6893)
	#define CONN_INFRA_CFG_ID		(0x20010101)
#endif

#define MET_EMI_ADDR	(0x27000)
#define BT_SSPM_TIMER	(0)

/*********************************************************************
*
* Utility APIs
*
**********************************************************************
*/

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
	uint8_t *ccif_base = ioremap(0x10003300, 0x100);

	if (ccif_base == NULL) {
		BTMTK_ERR("%s: remapping ccif_base fail", __func__);
		return;
	}

	/* CONSYS_BGF_PWR_ON, 0x10003318[0] = 1'b1 */
	SET_BIT(ccif_base + 0x18, BIT(0));
	/* CONSYS_BGF_READY, 0x10003318[1] = 1'b0 */
	CLR_BIT(ccif_base + 0x18, BIT(1));
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

	ccif_base = ioremap(0x10003300, 0x100);
	if (ccif_base == NULL) {
		BTMTK_ERR("%s: remapping ccif_base fail", __func__);
		return;
	}

	/* CONSYS_BGF_PWR_ON, 0x10003318[0] = 1'b0 */
	CLR_BIT(ccif_base + 0x18, BIT(0));
	/* CONSYS_BGF_READY, 0x10003318[1] = 1'b0 */
	CLR_BIT(ccif_base + 0x18, BIT(1));
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
	int32_t i = 0, hang_ret = 0;
	int32_t retry = POS_POLLING_RTY_LMT;
	uint32_t value = 0;

	/* wake up conn_infra off */
	SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));

	/* check ap2conn slpprot_rdy */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN_INFRA_ON2OFF_SLP_PROT_ACK &
			REG_READL(CONN_HOST_CSR_TOP_CONN_SLP_PROT_CTRL);
		BTMTK_DBG("ap2conn slpprot_rdy = 0x%08x", value);
		usleep_range(1000, 1100);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		return -1;

	/* polling conninfra version id */
	retry = 10;
	for (i = 0; i < retry; i++) {
		value = REG_READL(CONN_INFRA_CFG_VERSION);
		if (value == CONN_INFRA_CFG_ID)
			return 0; // success

		BTMTK_DBG("connifra cfg version = 0x%08x", value);
		usleep_range(USLEEP_1MS_L, USLEEP_1MS_H);
	}

	/* Check conninfra bus */
	if (!conninfra_reg_readable()) {
		hang_ret = conninfra_is_bus_hang();
		if (hang_ret > 0) {
			BTMTK_ERR("conninfra bus is hang, needs reset");
			conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "bus hang");
		}
		BTMTK_ERR("conninfra not readable, but not bus hang ret = %d", hang_ret);
	}

	return -1;
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

static void inline bt_dump_bgfsys_host_csr(void)
{
	uint32_t value = 0;
	uint32_t i = 0;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;

	BTMTK_INFO("[BGF host csr] Count = 11");
	for (i = 0x22C; i <= 0x244; i+=4) {
		value = REG_READL(CON_REG_SPM_BASE_ADDR + i);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;
	}

	for (i = 0x264; i <= 0x270; i+=4) {
		value = REG_READL(CON_REG_SPM_BASE_ADDR + i);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;
	}

	BTMTK_INFO("%s", g_dump_cr_buffer);
}

/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_mcusys_flag(void)
{
	uint32_t value = 0;
	uint32_t i = 0, count = 1, cr_count = 56;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;
	uint16_t switch_flag = 0x2A;

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0xA0);
	value &= 0xFFFFFF03; /* ignore [7:2] */
	value |= (switch_flag << 2);
	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA0, value);

	BTMTK_INFO("[BGF MCUSYS debug flag] Count = (%d)", cr_count);
	for (i = 0x00020101; i <= 0x00706F01; i+= 0x20200, count++) {
		REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA8, i);
		value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x22C);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;

		if ((count & 0xF) == 0 || count == cr_count) {
			BTMTK_INFO("%s", g_dump_cr_buffer);
			memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
			pos = &g_dump_cr_buffer[0];
			end = pos + BT_CR_DUMP_BUF_SIZE - 1;
		}
	}
}

/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_bus_flag(void)
{
	uint32_t value = 0;
	uint32_t i = 0, j = 0, count = 1, cr_count = 20;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;
	uint16_t switch_flag = 0x2A;


	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0xA0);
	value &= 0xFFFFFF03; /* ignore [7:2] */
	value |= (switch_flag << 2);
	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA0, value);

	if (conninfra_reg_readable() && !bt_is_bgf_bus_timeout()) {
		value = bt_read_cr(0x1881E000);
		value &= 0xFFFFFFEF;
		value |= (1 << 4);
		bt_write_cr(0x1881E000, value, FALSE);
	} else
		BTMTK_ERR("conninfra is not readable");

	BTMTK_INFO("[BGF BUS debug flag] Count = (%d)", cr_count);
	for (i = 0x104A4901; i <= 0xA04A4901; i += 0x10000000) {
		for (j = i; j <= i + 0x20200; j += 0x20200) {
			REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA8, j);
			value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x22C);
			ret = snprintf(pos, (end - pos + 1), "%08x ", value);
			if (ret < 0 || ret >= (end - pos + 1))
				break;
			pos += ret;
			count++;

			if ((count & 0xF) == 0 || count == cr_count) {
				BTMTK_INFO("%s", g_dump_cr_buffer);
				memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
				pos = &g_dump_cr_buffer[0];
				end = pos + BT_CR_DUMP_BUF_SIZE - 1;
			}
		}
	}
}

static void inline bt_dump_bgfsys_top_common_flag(void)
{
	uint32_t value = 0;
	uint32_t i = 0, count = 1, cr_count = 20;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;

	BTMTK_INFO("[BGF TOP common/bt part debug flag] Count = (%d)", cr_count);
	for (i = 0x80; i <= 0x93; i++, count++) {
		REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xAC, i);
		value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x23C);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;

		if ((count & 0xF) == 0 || count == cr_count) {
			BTMTK_INFO("%s", g_dump_cr_buffer);
			memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
			pos = &g_dump_cr_buffer[0];
			end = pos + BT_CR_DUMP_BUF_SIZE - 1;
		}
	}
}

/* please make sure check bus hang before calling this dump */
static void inline bt_dump_bgfsys_mcu_core_flag(void)
{
	uint32_t value = 0;
	uint32_t i = 0, count = 1, cr_count = 38;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;
	uint16_t switch_flag = 0x2B;

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0xA0);
	value &= 0xFFFFFF03; /* ignore [7:2] */
	value |= (switch_flag << 2);
	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA0, value);

	BTMTK_INFO("[BGF MCU core debug flag] Count = (%d)", cr_count);

	/* gpr0 ~ ipc */
	for (i = 0x3; i <= 0x25000003; i+= 0x1000000, count++) {
		REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA8, i);
		value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x22C);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;

		if ((count & 0xF) == 0 || count == cr_count) {
			BTMTK_INFO("%s", g_dump_cr_buffer);
			memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
			pos = &g_dump_cr_buffer[0];
			end = pos + BT_CR_DUMP_BUF_SIZE - 1;
		}
	}
}

static inline void bt_dump_bgfsys_mcu_pc_log(void)
{
	uint32_t value = 0;
	uint32_t i = 0, count = 1, cr_count = 45;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;


	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;

	BTMTK_INFO("[BGF MCU PC log] Count = (%d)", cr_count);
	for (i = 0x00; i <= 0x2C; i++, count++) {
		value = REG_READL(CON_REG_SPM_BASE_ADDR + 0xA0);
		value &= 0xFFFFFF03; /* ignore [7:2] */
		value |= (i << 2);
		REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA0, value);

		value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x22C);
		ret = snprintf(pos, (end - pos + 1), "%08x ", value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;

		if ((count & 0xF) == 0 || count == cr_count) {
			BTMTK_INFO("%s", g_dump_cr_buffer);
			memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
			pos = &g_dump_cr_buffer[0];
			end = pos + BT_CR_DUMP_BUF_SIZE - 1;
		}
	}
}

static inline void bt_dump_bgfsys_suspend_wakeup_debug(void)
{
	#define CR_CNT	7
	uint32_t value = 0;
	uint32_t i = 0;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;
	int32_t CR_ADDR[CR_CNT] = {0x22C, 0x268, 0x26C, 0x2E0, 0x2E4, 0x2E8, 0x2EC};
	uint8_t *CR_NAME[CR_CNT] = {"BGF_MCU_PC_DBG", "N9_MCU_MAILBOX_DBG", "HOST_MAILBOX_BT_ADDR", "CR", "CR", "CR", "CR"};

	memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
	pos = &g_dump_cr_buffer[0];
	end = pos + BT_CR_DUMP_BUF_SIZE - 1;

	for (i = 0; i < CR_CNT; i++) {
		value = REG_READL(CON_REG_SPM_BASE_ADDR + CR_ADDR[i]);
		ret = snprintf(pos, (end - pos + 1), "%s[0x%08x] read[0x%08x], ", CR_NAME[i], 0x18060000 + CR_ADDR[i], value);
		if (ret < 0 || ret >= (end - pos + 1))
			break;
		pos += ret;
	}
	BTMTK_INFO("%s", g_dump_cr_buffer);
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
	bt_dump_bgfsys_top_common_flag();
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
	uint32_t offset = 0x410, value = 0, i = 0;
	uint8_t *pos = NULL, *end = NULL;
	int32_t ret = 0;

	ret = conninfra_is_bus_hang();
	BTMTK_INFO("%s: conninfra_is_bus_hang ret = %d", __func__, ret);

	if (!CAN_DUMP_HOST_CSR(ret)) {
		BTMTK_ERR("%s; host csr is not readable", __func__);
		return;
	}

	if(bgfsys_check_conninfra_ready())
		goto host_csr_only;

	BTMTK_INFO("%s: M0 - M3 semaphore status:", __func__);
	for (i = 0x18070400; i <= 0x18073400; i += 0x1000) {
		value = bt_read_cr(i);
		BTMTK_INFO("[0x%08x] = [0x%08x]", i, value);
	}

	if (conninfra_reg_readable() && !bt_is_bgf_bus_timeout()) {
		BTMTK_INFO("[BGF Bus hang debug CR (18800410~18000444)] Count = (14)");

		memset(g_dump_cr_buffer, 0, BT_CR_DUMP_BUF_SIZE);
		pos = &g_dump_cr_buffer[0];
		end = pos + BT_CR_DUMP_BUF_SIZE - 1;
		for (offset = 0x410; offset <= 0x0444; offset += 4) {
			ret = snprintf(pos, (end - pos + 1), "%08x ",
					 REG_READL(BGF_REG_BASE_ADDR + offset));
			if (ret < 0 || ret >= (end - pos + 1))
				break;
			pos += ret;
		}
		BTMTK_INFO("%s", g_dump_cr_buffer);
	} else
		BTMTK_INFO("conninfra is not readable, skip [BGF Bus hang debug CR]");

host_csr_only:
	bt_dump_bgfsys_all();
	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
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
	uint32_t value = 0, i = 0;
	int32_t ret = 0;

	ret = conninfra_is_bus_hang();
	BTMTK_INFO("%s: conninfra_is_bus_hang ret = %d", __func__, ret);

	if (!CAN_DUMP_HOST_CSR(ret)) {
		BTMTK_ERR("%s; host csr is not readable", __func__);
		return;
	}

	if(bgfsys_check_conninfra_ready())
		goto host_csr_only;

	/* following CR only accessible while bus is not hang */
	if (!ret ) {
		value = REG_READL(CON_REG_INFRA_CFG_ADDR + 0x400);
		BTMTK_INFO("0x18001400 = [0x%08x]", value);

		value = REG_READL(CON_REG_INFRA_CFG_ADDR + 0x41C);
		BTMTK_INFO("0x1800141C = [0x%08x]", value);

		value = REG_READL(CON_REG_INFRA_CFG_ADDR + 0x420);
		BTMTK_INFO("0x18001420 = [0x%08x]", value);

		value = 0x87654321;
		REG_WRITEL(CON_REG_INFRA_CFG_ADDR + 0x10, value);
		value = REG_READL(CON_REG_INFRA_CFG_ADDR + 0x10);
		BTMTK_INFO("0x18001010 = [0x%08x]", value);

		value = REG_READL(CON_REG_INFRA_CFG_ADDR + 0x160);
		BTMTK_INFO("0x18001160 = [0x%08x]", value);

		value = REG_READL(CONN_INFRA_CFG_START + 0x0168);
		BTMTK_INFO("0x18001168 = [0x%08x]", value);

		value = REG_READL(CONN_INFRA_CFG_START + 0x0170);
		BTMTK_INFO("0x18001170 = [0x%08x]", value);

		value = REG_READL(CONN_INFRA_CFG_BT_PWRCTLCR0);
		BTMTK_INFO("0x18001874 = [0x%08x]", value);

		value = REG_READL(CONN_INFRA_CFG_START + 0x0C00);
		BTMTK_INFO("0x18001C00 = [0x%08x]", value);

		value = REG_READL(CONN_INFRA_CFG_START + 0x0C04);
		BTMTK_INFO("0x18001C04 = [0x%08x]", value);

		BTMTK_INFO("%s: M0 - M3 semaphore status:", __func__);
		for (i = 0x18070400; i <= 0x18073400; i += 0x1000) {
			value = bt_read_cr(i);
			BTMTK_INFO("[0x%08x] = [0x%08x]", i, value);
		}

		value = bt_read_cr(0x18071400);
		BTMTK_INFO("0x18070400 = [0x%08x]", value);

		value = bt_read_cr(0x18070400);
		BTMTK_INFO("0x18070400 = [0x%08x]", value);

		value = bt_read_cr(0x18070400);
		BTMTK_INFO("0x18070400 = [0x%08x]", value);
	}

host_csr_only:
	value = REG_READL(BGF_LPCTL);
	BTMTK_INFO("0x18060030 = [0x%08x]", value);

	value = REG_READL(BGF_IRQ_STAT2);
	BTMTK_INFO("0x1806003C = [0x%08x]", value);

	value = 0x12345678;
	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0x188, value);
	value = REG_READL(CON_REG_SPM_BASE_ADDR + 0x188);
	BTMTK_INFO("0x18060188 = [0x%08x]", value);

	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0xA8, 0x194C4BA7);
	BTMTK_INFO("Write [0x180600A8] = [0x194C4BA7]");

	bt_dump_bgfsys_all();
	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
}

static inline int32_t bgfsys_get_sw_irq_status(void)
{
	int32_t value = 0;

	/* 2. Check bgf bus status */
	if (bt_is_bgf_bus_timeout()) {
		bt_dump_bgfsys_all();
		return RET_SWIRQ_ST_FAIL;
	}

	/* read sw irq status*/
	value = REG_READL(BGF_SW_IRQ_STATUS);

	if (value & BGF_SUBSYS_CHIP_RESET){
		SET_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET);
	}else if (value & BGF_FW_LOG_NOTIFY){
		SET_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY);
	} else if (value &  BGF_WHOLE_CHIP_RESET){
		SET_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_WHOLE_CHIP_RESET);
	}

	return value;
}

/*
static inline void bgfsys_ack_sw_irq_fwlog(void)
{
	SET_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_FW_LOG_NOTIFY);
}

static inline void bgfsys_ack_sw_irq_reset(void)
{
	SET_BIT(BGF_SW_IRQ_RESET_ADDR, BGF_SUBSYS_CHIP_RESET);
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

	//REG_WRITEL(CONN_INFRA_CFG_START + 0x0610, BIT(1));
	//val_r = REG_READL(CONN_INFRA_CFG_START + 0x0610);
	//BTMTK_INFO("%s: REG[0x18001610] write[0x%08x], REG[0x18001610] read[0x%08x]", __func__, BIT(1), val_r);
		bt_write_cr(0x1800F000, 0x32C8001C, FALSE);
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
	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));
}

static inline void bgfsys_dump_uart_pta_pready_status(void)
{
	uint8_t *base = NULL;

	if (!conninfra_reg_readable()) {
		BTMTK_ERR("%s: conninfra bus is not readable, discard", __func__);
		return;
	}

	REG_WRITEL(CON_REG_SPM_BASE_ADDR + 0x128, 0x00020002);
	BTMTK_INFO("0x18060128 = [0x%08x]",
		REG_READL(CON_REG_SPM_BASE_ADDR + 0x128));

	BTMTK_INFO("0x18001A00 = [0x%08x]",
		REG_READL(CON_REG_INFRA_CFG_ADDR + 0xA00));

	base = ioremap(0x1800C00C, 4);
	if (base == NULL) {
		BTMTK_ERR("%s: remapping 0x18001A00 fail", __func__);
		return;
	}
	BTMTK_INFO("0x1800C00C = [0x%08x]", *base);
	iounmap(base);
}

static void bgfsys_dump_conn_wt_slp_ctrl_reg(void)
{
	void __iomem *vir_addr = NULL;
	unsigned int val1 = 0, val2 = 0;
	int32_t ret = 0;

	if (!conninfra_reg_readable()) {
		ret = conninfra_is_bus_hang();
		BTMTK_ERR("%s: conninfra bus is not readable, conninfra_is_bus_hang = %d", __func__, ret);
		return;
	}

	vir_addr = ioremap(0x18005008, 0x10);
	if (vir_addr) {
		val1 = (unsigned int)REG_READL(vir_addr);
		iounmap(vir_addr);
	} else
		BTMTK_WARN("ioremap fail");

	vir_addr = ioremap(0x180050A8, 0x10);
	if (vir_addr) {
		val2 = (unsigned int)REG_READL(vir_addr);
		iounmap(vir_addr);
	} else
		BTMTK_WARN("ioremap fail");
	BTMTK_INFO("[%s] 0x18005008=[%x] 0x180050A8=[%x]", __func__, val1, val2);
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
	uint32_t remap_addr;

	remap_addr = REG_READL(CONN_REMAP_ADDR);
	BTMTK_INFO("remap addr = 0x%08X", remap_addr);

	bgfsys_dump_uart_pta_pready_status();

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* bus clock ctrl */
	conninfra_bus_clock_ctrl(CONNDRV_TYPE_BT, CONNINFRA_BUS_CLOCK_WPLL | CONNINFRA_BUS_CLOCK_BPLL, 1);

	/* enable bt function en */
	SET_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B);

	if (!conninfra_reg_readable()) {
		if (conninfra_is_bus_hang() > 0) {
			BTMTK_ERR("%s: check conninfra status fail after set CONN_INFRA_CFG_BT_PWRCTLCR0!", __func__);
			goto error;
		}
	}

	/* polling bgfsys top on power ack bits until they are asserted */
	do {
		value = BGF_ON_PWR_ACK_B &
			REG_READL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST);
		BTMTK_DBG("bgfsys on power ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != BGF_ON_PWR_ACK_B && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bgfsys on power ack = 0x%08x", value);
		goto error;
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
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_DBG("bt2conn slp_prot tx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bt2conn slp_prot tx ack = 0x%08x", value);
		goto error;
	}
	usleep_range(400, 440);

	/* read bgfsys hw_version */
	retry = 10;
	do {
		value = REG_READL(BGF_HW_VERSION);
		BTMTK_DBG("bgfsys hw version id = 0x%08x", value);
		usleep_range(500, 550);
		retry--;
	} while (value != BGF_HW_VER_ID && retry > 0);

	if (0 == retry) {
		BTMTK_ERR("bgfsys hw version id = 0x%08x", value);
		goto error;
	}

	/* read bgfsys fw_version */
	value = REG_READL(BGF_FW_VERSION);
	BTMTK_INFO("bgfsys fw version id = 0x%08x", value);
	if (value != BGF_FW_VER_ID)
		goto error;

	/* read bgfsys hw_code */
	value = REG_READL(BGF_HW_CODE);
	BTMTK_INFO("bgfsys hw code = 0x%08x", value);
	if (value != BGF_HW_CODE_ID)
		goto error;

	/* read and check bgfsys version id */
	value = REG_READL(BGF_IP_VERSION);
	BTMTK_INFO("bgfsys version id = 0x%08x", value);
	if (value != BGF_IP_VER_ID)
		goto error;

	/* clear con_cr_ahb_auto_dis */
	CLR_BIT(BGF_MCCR, BGF_CON_CR_AHB_AUTO_DIS);

	/* set con_cr_ahb_stop */
	REG_WRITEL(BGF_MCCR_SET, BGF_CON_CR_AHB_STOP);

	/* reset bfgsys semaphore */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_SW_RST_B, BGF_SW_RST_B);

	/* release n10 cpu core */
	SET_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* trun off BPLL & WPLL (use common API) */
	conninfra_bus_clock_ctrl(CONNDRV_TYPE_BT, CONNINFRA_BUS_CLOCK_WPLL | CONNINFRA_BUS_CLOCK_BPLL, 0);

	/*
	 * enable conn_infra bus hang detect function &
	 * bus timeout value(use common API)
	 */
	conninfra_config_setup();

	bgfsys_dump_uart_pta_pready_status();

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));

	/*
	 * polling BGFSYS MCU sw_dbg_ctl cr to wait it becomes 0x1D1E,
	 * which indicates that the power-on part of ROM is completed.
	 */
	retry = IDLE_LOOP_RTY_LMT;
	do {
		if (conninfra_reg_readable()) {
			mcu_pc = REG_READL(CONN_MCU_PC);
			mcu_idle = REG_READL(BGF_MCU_CFG_SW_DBG_CTL);
			BTMTK_INFO("MCU pc = 0x%08x", mcu_pc);

			if (0x1D1E == mcu_idle) {
				BTMTK_INFO("MCU sw_dbg_ctl = 0x%08x", mcu_idle);
				break;
			}
		} else {
			bgfsys_power_on_dump_cr();
			return -1;
		}

		msleep(delay_ms);
		retry--;
	} while (retry > 0);

	if (retry == 0) {
		bt_dump_cif_own_cr();
		return -1;
	}

	return 0;

error:
	/* turn off clock ctrl */
	conninfra_bus_clock_ctrl(CONNDRV_TYPE_BT, CONNINFRA_BUS_CLOCK_WPLL | CONNINFRA_BUS_CLOCK_BPLL, 0);

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

	/* wake up conn_infra */
	ret = bgfsys_check_conninfra_ready();
	if (ret)
		return ret;

	bgfsys_dump_uart_pta_pready_status();

	/* enable bt2conn slp_prot tx en */
	SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL, BT2CONN_SLP_PROT_TX_EN_B);
	/* polling bt2conn slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
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
			REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
		BTMTK_DBG("conn2bt slp_prot rx ack = 0x%08x", value);
		usleep_range(500, 550);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		ret = -1;

	if (ret == -2)
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_BT, "Power off fail");

	/* trun off BPLL & WPLL (use common API) */
	conninfra_bus_clock_ctrl(CONNDRV_TYPE_BT, CONNINFRA_BUS_CLOCK_WPLL | CONNINFRA_BUS_CLOCK_BPLL, 0);

	/* disable bt function en */
	CLR_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0, BT_FUNC_EN_B);

	/* reset n10 cpu core */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/* Disable A-die top_ck_en (use common API) */
	conninfra_adie_top_ck_en_off(CONNSYS_ADIE_CTL_FW_BT);

	/* reset bfgsys semaphore */
	CLR_BIT(CONN_INFRA_RGU_BGFSYS_SW_RST_B, BGF_SW_RST_B);

	/* clear bt_emi_req */
	SET_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, BT_EMI_CTRL_BIT);
	CLR_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, BT_EMI_CTRL_BIT);
	CLR_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_SRCCLKENA_BT);
	CLR_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_AP_BUS_REQ_BT);
	CLR_BIT(CONN_INFRA_CFG_EMI_CTL_BT_EMI_REQ_BT, CONN_INFRA_CFG_EMI_CTL_BT_SW_CONN_APSRC_REQ_BT);

	if (ret)
		bgfsys_power_on_dump_cr();

	bgfsys_dump_uart_pta_pready_status();

	/* when bt turn off, a-die may still closing and OC happened, add dump to debug */
	//udelay(50);
	bgfsys_dump_conn_wt_slp_ctrl_reg();

	/* release conn_infra force on */
	CLR_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));

	return ret;
}

static inline void fwp_get_patch_names(void)
{
#if (CONNAC20_CHIPID == 6885) && defined(BT_CUS_FEATURE)
	uint8_t flavor = 'b';
	u_int8_t has_flavor = TRUE;
#else
	uint8_t flavor = FLAVOR_NONE;
	u_int8_t has_flavor = fwp_has_flavor_bin(&flavor);
#endif

	compose_fw_name(has_flavor, flavor, BIN_NAME_MCU, BIN_NAME_BT);
}
#endif
