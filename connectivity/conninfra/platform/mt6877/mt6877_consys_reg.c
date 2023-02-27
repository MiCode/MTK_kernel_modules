// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "mt6877_consys_reg.h"
#include "mt6877_consys_reg_offset.h"
#include "mt6877_pos.h"

#define LOG_TMP_BUF_SZ 256

static int consys_reg_init(struct platform_device *pdev);
static int consys_reg_deinit(void);
static int consys_check_reg_readable(void);
static int __consys_check_reg_readable(void);
static int consys_check_reg_readable_for_coredump(void);
static int consys_is_consys_reg(unsigned int addr);
static int consys_is_bus_hang(void);

struct consys_base_addr conn_reg;

struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6877 = {
	.consys_reg_mng_init = consys_reg_init,
	.consys_reg_mng_deinit = consys_reg_deinit,
	.consys_reg_mng_check_reable = consys_check_reg_readable,
	.consys_reg_mng_check_reable_for_coredump = consys_check_reg_readable_for_coredump,
	.consys_reg_mng_is_bus_hang = consys_is_bus_hang,
	.consys_reg_mng_is_consys_reg = consys_is_consys_reg,

};


static const char* consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"conn_infra_rgu",
	"conn_infra_cfg",
	"conn_infra_clkgen_on_top",
	"conn_infra_bus_cr",
	"conn_host_csr_top",
	"infracfg_ao",
	"GPIO",
	"spm",
	"top_rgu",
	"conn_wt_slp_ctl_reg",
	"conn_infra_sysram_sw_cr",
	"conn_afe_ctl",
	"conn_semaphore",
	"conn_rf_spi_mst_reg",
	"IOCFG_RT",
	"conn_therm_ctl",
	"conn_bcrm_on",
};

struct consys_base_addr* get_conn_reg_base_addr()
{
	return &conn_reg;
}

int consys_reg_get_reg_symbol_num(void)
{
	return CONSYS_BASE_ADDR_MAX;
}

int consys_is_consys_reg(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

static void consys_bus_hang_dump_a_rc(void)
{
	unsigned int i;
	char tmp[LOG_TMP_BUF_SZ] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	void __iomem *addr = NULL;

	addr = ioremap(0x1000F900, 0x100);
	if (!addr) {
		pr_info("[%s] remap 0x1000F100 fail", __func__);
		return;
	}
	for (i = 0x50; i <= 0x94; i+= 4) {
		if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
			CONSYS_REG_READ(addr + i)) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[rc_trace] %s", tmp_buf);

	memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
	for (i = 0x98; i <= 0xd4; i += 4) {
		if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
			CONSYS_REG_READ(addr + i)) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[rc_timer] %s rc status=[0x%08x]", tmp_buf, CONSYS_REG_READ(addr + 0x4));

	iounmap(addr);
}

static void consys_bus_hang_dump_a(void)
{
	unsigned int r_rx, r_tx;
	unsigned int a0, a5, a6, a7, a8, a9, a10, a11, a12;
	unsigned int a1, a2, a3, a4;
	void __iomem *addr = NULL;

	/* A0	Read	0x10006110
	 */
	a0 = CONSYS_REG_READ(SPM_MD32PCM_SCU_STA0);
	/* A5	Read	0x10006E04
	 * A6	Read	0x10006178
	 * A7	Read	0x10006EF4
	 * A8	Read	0x10000180
	 * A9	Read	0x1000694C
	 * A10	Read	0x10006100
	 * A11	Read	0x10006928
	 * A12	Read	0x10006938
	 */
	a5 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0xE04);
	a6 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0x178);
	a7 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0xEF4);
	a9 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0x94c);
	a10 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0x100);
	a11 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0x928);
	a12 = CONSYS_REG_READ(CONN_REG_SPM_ADDR + 0x938);

	addr = ioremap(0x10000180, 0x20);
	if (addr != NULL) {
		a8 = CONSYS_REG_READ(addr);
		iounmap(addr);
	} else {
		pr_info("[%s] remap 0x10000180 error", __func__);
		a8 = 0xdeaddead;
	}

	/* RC Mode
	 * A1	Read	0x1000F928
	 * A2	Read	0x1000F92C
	 * A3	Read	0x1000F930
	 * A4	Read	0x1000F934
	 */
	addr = ioremap(0x1000F900, 0x40);
	if (addr != NULL) {
		a1 = CONSYS_REG_READ(addr + 0x28);
		a2 = CONSYS_REG_READ(addr + 0x2c);
		a3 = CONSYS_REG_READ(addr + 0x30);
		a4 = CONSYS_REG_READ(addr + 0x34);
		iounmap(addr);
	} else {
		pr_info("[%s] remap 0x1000F900 fail", __func__);
		a1 = 0xdeaddead;
		a2 = 0xdeaddead;
		a3 = 0xdeaddead;
		a4 = 0xdeaddead;
	}

	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1228 [19] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0(CR at ap side)
	 */
	r_rx = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 19));
	r_tx = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	pr_info("[CONN_BUS_A][%s][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
		(consys_is_rc_mode_enable_mt6877()? "RC" : "Legacy"),
		a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12,
		r_rx, r_tx);

	consys_bus_hang_dump_a_rc();

}

static inline unsigned int __consys_bus_hang_clock_detect(void)
{
	unsigned int count = 0;
	unsigned int r;

	while (count < 4) {
		CONSYS_SET_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, (0x1 << 0));
		udelay(20);
		r = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_BUS_MCU_STAT_ADDR, ((0x1 << 2) | (0x1 << 1)));
		if (r == 0x6)
			break;
		udelay(1000);
		count ++;
	}

	return r;
}

static void consys_bus_hang_dump_b(void)
{
	unsigned int b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10;
	unsigned int bus_clock, ip_version, irq_b, irq_vndr, irq_axi, irq_conninfra, wifi_irq;

	/* B0	Read	0x180602C0
	 */
	b0 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_0_ADDR);
	/* B1
	 * Write	0x1806015C[2:0]	3'b000
	 * Read		0x180602C8
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x0);
	b1 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);

	/* B2
	 * Write	0x1806015C[2:0]	3'b010
	 * Read		0x180602C8
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x2);
	b2 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);
	/* B3
	 * Write	0x1806015C[2:0]	3'b011
	 * Read		0x180602C8
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x3);
	b3 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);
	/* B4
	 * Write	0x1806015C[2:0]	3'b110
	 * Read		0x180602C8
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x6);
	b4 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);

	/* B5	Read	0x180602CC
	 */
	b5 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_3_ADDR);

	/* B6:	0x1806_01a0
	 * B7:	0x1806_01a4
	 * B8:	0x1806_01a8
	 * B9:	0x1806_01ac
	 * B10:	0x1806_01b0
	 */
	b6 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR);
	b7 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_WF_ADDR);
	b8 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_BT_ADDR);
	b9 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_GPS_ADDR);
	b10 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_FM_ADDR);

	/* On2Off check */
	/* 2. Check conn_infra off bus clock
	 * 	- write 0x1 to 0x1806_0000[0], reset clock detect
	 * 	- 0x1806_0000[2] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 	- 0x1806_0000[1] osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * 	- Read 0x1800_1000 = 0x02060002
	 * 4 Check conn_infra off domain bus hang irq status
	 * 	- 0x1806_02D4[0], should be 1'b1, or means conn_infra off bus might hang (conn_infra_bus_timeout_irq_b)
	 * 	1) 0x1806014C[0] should be 1'b0, or means conn_infra main bus timeout for VDNR timeout mechanism)
	 * 	2) 0x18060448[0] should be 1'b0, or means conn_infra axi layer bus timeout for VDNR timeout mechanism)
	 * 	3) 0x18060434[0] should be 1'b1, or means conn_infra_on timeout for AHB/APB timeout mechanism)
	 * 	4) 0x18060434[1] should be 1'b1, or means conn_infra_off timeout for AHB/APB timeout mechanism)
	 */
	bus_clock = __consys_bus_hang_clock_detect();
	ip_version = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	irq_b = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_DBG_DUMMY_5_ADDR, (0x1 << 0));
	irq_vndr = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_CONN_INFRA_ON_BUS_TIMEOUT_IRQ_ADDR, (0x1 << 0));
	irq_axi = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_TIMEOUT_IRQ, (0x1 << 0));
	irq_conninfra = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_TIMEOUT_IRQ_B_ADDR, 0x3);
	/* Dump WIFI IRQ status
	 * [AP2WF] readable step4. irq_status = 1b'1?
	 */
	wifi_irq = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_WF_MCUSY_VDNR_BUS_TIMOUT_ADDR, (0x1 << 0));

	pr_info("[CONN_BUS_B][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
		b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10,
		bus_clock, ip_version, irq_b, irq_vndr, irq_axi, irq_conninfra, wifi_irq);
}

static void consys_bus_hang_dump_c(bool offclock)
{
	unsigned int c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15;
	unsigned int r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
	unsigned int timeout1, timeout2, timeout3, timeout4, timeout5, timeout6, timeout7;
	unsigned int i;
	void __iomem *addr = NULL;
	char tmp[LOG_TMP_BUF_SZ] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	const unsigned int debug_ctrl_setting_values[] = {
		0x00010001, 0x00020001, 0x00010002, 0x00020002, 0x00030002,
		0x00010003, 0x00020003, 0x00030003, 0x00010004, 0x00020004,
		0x00010005
	};
	unsigned int axi1, axi2, axi3, axi4, axi5, axi6;

	/* 1. Power check
	 * sheet1. Power status
	 */
	r0 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_DBG_ADDR);
	/* Power check fail
	 * Dump 4. CONN_INFRA status, sheet "conn_infra_cfg_clk parse"
	 * 	Write 0x1806_015c[2:0] from 3b'000 to 3b'101
	 * 	Dump 0x1806_02c8
	 */
	memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
	for (i = 0; i <= 0x5; i ++) {
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, i);
		if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
			CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR)) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[CONN_BUS_C]conn_infra_cfg_clk:%s systrap:[0x%08x]", tmp_buf, r0);

	/* 2-1. conn_infra_bus_debug (host csr)
	 * sheet:
	 * 	- "ahb_apb_timeout_dump" - debug log
	 * 	- "debug_ctrl_setting"
	 */
	/* ahb_apb_timeout_dump
	 * 	Read 0x1806_0414, 1806_0418, 1806_042C, 1806_0430, 1806_041C, 1806_0420
	 * 	Read 0x1806_0410
	 */
	timeout1 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_OFF_DBG_1_ADDR);
	timeout2 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_OFF_DBG_2_ADDR);
	timeout3 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_ON_TOP_DBG_APB_1_ADDR);
	timeout4 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_ON_TOP_DBG_APB_2_ADDR);
	timeout5 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_OFF_TOP_DBG_1_ADDR);
	timeout6 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_OFF_TOP_DBG_2_ADDR);
	timeout7 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_TIMEOUT_LOG_ADDR);
	pr_info("[CONN_BUS_C]ahb_apb_timeout:[0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
		timeout1, timeout2, timeout3, timeout4, timeout5, timeout6, timeout7);

	/* debug_ctrl_setting - table 1
	 * 	0x1800_F408, 0x1800_F40C, 0x1800_F410, 0x1800_F414, 0x1800_F418, 0x1800_F41C
	 * 	0x1800_F420, 0x1800_F424, 0x1800_F428, 0x1800_F42C, 0x1800_F430
	 */
	addr = ioremap(0x1800f400, 0x40);
	if (addr) {
		memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
		for (i = 0x8; i <= 0x30; i += 4) {
			if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
			    CONSYS_REG_READ(addr + i)) >= 0)
				strncat(tmp_buf, tmp, strlen(tmp));
		}
		pr_info("[CONN_BUS_C]debug_ctrl_setting-1:%s", tmp_buf);
		iounmap(addr);
	} else {
		pr_info("[CONN_BUS_C]debug_ctrl_setting-1: allocate fail");
	}

	/* debug_ctrl_setting - table 2
	 * WRITE	0x18060138	0x0001_0001	READ	0x1806_0150
	 * WRITE	0x18060138	0x0002_0001	READ	0x1806_0150
	 * WRITE	0x18060138	0x0001_0002	READ	0x1806_0150
	 * WRITE	0x18060138	0x0002_0002	READ	0x1806_0150
	 * WRITE	0x18060138	0x0003_0002	READ	0x1806_0150
	 * WRITE	0x18060138	0x0001_0003	READ	0x1806_0150
	 * WRITE	0x18060138	0x0002_0003	READ	0x1806_0150
	 * WRITE	0x18060138	0x0003_0003	READ	0x1806_0150
	 * WRITE	0x18060138	0x0001_0004	READ	0x1806_0150
	 * WRITE	0x18060138	0x0002_0004	READ	0x1806_0150
	 * WRITE	0x18060138	0x0001_0005	READ	0x1806_0150
	 */
	memset(tmp_buf, '\0', LOG_TMP_BUF_SZ);
	for (i = 0; i < ARRAY_SIZE(debug_ctrl_setting_values); i++) {
		CONSYS_REG_WRITE(
			CONN_HOST_CSR_TOP_CONN_INFRA_ON_DEBUG_AO_DEBUGSYS_ADDR,
			debug_ctrl_setting_values[i]);
		if (snprintf(tmp, LOG_TMP_BUF_SZ, "[0x%08x]",
		    CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_ON_DEBUG_CTRL_AO2SYS_OUT_ADDR))  >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[CONN_BUS_C]debug_ctrl_setting-2:%s", tmp_buf);

	/* File: conn_infra_bus_debug
	 * sheet: 11. axi_layerdebug_ctrl_setting
	 * 1. Debug log: 0x1801_D408, 0x1801_D40C, 0x1801_D410
	 * 2. Real time log
	 * 	WRITE	0x18060440	0x0001_0001	READ	0x1806_0458
	 * 	WRITE	0x18060440	0x0002_0001	READ	0x1806_0458
	 * 	WRITE	0x18060440	0x0003_0001	READ	0x1806_0458
	 */
	addr = ioremap(0x1801d400, 0x20);
	if (addr) {
		axi1 = CONSYS_REG_READ(addr + 0x08);
		axi2 = CONSYS_REG_READ(addr + 0x0c);
		axi3 = CONSYS_REG_READ(addr + 0x10);
		pr_info("[CONN_BUS_C]axi_layerdebug_ctrl_setting-1:[0x%08x][0x%08x][0x%08x]", axi1, axi2, axi3);
		iounmap(addr);
	} else {
		pr_info("[CONN_BUS_C]axi_layerdebug_ctrl_setting-1: allocate fail");
	}

	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_CTRL, 0x00010001);
	axi4 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_DEBUGSYS_OUT);
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_CTRL, 0x00020001);
	axi5 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_DEBUGSYS_OUT);
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_CTRL, 0x00030001);
	axi6 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_DEBUGSYS_OUT);
	pr_info("[CONN_BUS_C]axi_layerdebug_ctrl_setting-2:[0x%08x][0x%08x][0x%08x]", axi4, axi5, axi6);

	if (offclock) {
		/* 2-2. conn_infra_bus_debug
		 * sheet:
		 * - "conn_infra_sleep_protect_dump"
		 * 	- 0x18001514, 0x1800E2A0, 0x18001504, 0x1800E2A4, 0x18001534
		 * 	- 0x18001524, 0x18001544, 0x18001554, 0x1800E2AC, 0x18001564
		 * 	- 0x1800E2A8, 0x18001574, 0x1800E2B4, 0x18001584, 0x1800E2B0
		 */
		r1 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_AP2CONN_SLP_STATUS_ADDR);
		r2 = CONSYS_REG_READ(CONN_BUS_CR_GALS_AP2CONN_GALS_DBG_ADDR);
		r3 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_CONN2AP_SLP_STATUS_ADDR);
		r4 = CONSYS_REG_READ(CONN_BUS_CR_GALS_CONN2AP_GALS_DBG_ADDR);
		r5 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_OFF_BUS_SLP_STATUS_ADDR);
		r6 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_ON_BUS_SLP_STATUS_ADDR);
		r7 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_WF_SLP_STATUS_ADDR);
		r8 = CONSYS_REG_READ(CONN_CFG_GALS_CONN2BT_SLP_STATUS_ADDR);
		r9 = CONSYS_REG_READ(CONN_BUS_CR_GALS_CONN2BT_GALS_DBG_ADDR);
		r10 = CONSYS_REG_READ(CONN_CFG_GALS_BT2CONN_SLP_STATUS_ADDR);
		r11 = CONSYS_REG_READ(CONN_BUS_CR_GALS_BT2CONN_GALS_DBG_ADDR);
		r12 = CONSYS_REG_READ(CONN_CFG_GALS_CONN2GPS_SLP_STATUS_ADDR);
		r13 = CONSYS_REG_READ(CONN_BUS_CR_GALS_CONN2GPS_GALS_DBG_ADDR);
		r14 = CONSYS_REG_READ(CONN_CFG_GALS_GPS2CONN_SLP_STATUS_ADDR);
		r15 = CONSYS_REG_READ(CONN_BUS_CR_GALS_GPS2CONN_GALS_DBG_ADDR);
		pr_info("[CONN_BUS_C]slp_prot:[0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
			r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15);
		/* Power relative */
		/* C0	Read	0x18001320
		 * C1	Read	0x18009A00
		 * C2	Read	0x18000400
		 * C3	Read	0x18001384
		 * C4	Read	0x18000104
		 */
		c0 = CONSYS_REG_READ(CONN_CFG_PLL_STATUS_ADDR);
		c1 = CONSYS_REG_READ(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR);
		c2 = CONSYS_REG_READ(CONN_RGU_WFSYS_ON_TOP_PWR_ST_ADDR);
		c3 = CONSYS_REG_READ(CONN_CFG_CONN_INFRA_CFG_RC_STATUS_ADDR);
		c4 = CONSYS_REG_READ(CONN_RGU_WFSYS_WA_WDT_EN_ADDR);
		/* A-die power relative
		 * 0x1800_50A8, 0x1800_5120~0x1800_5134
		 */
		c5 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_CK_STA_ADDR);
		c6 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR);
		c7 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_1_ADDR);
		c8 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_2_ADDR);
		c9 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_3_ADDR);
		c10 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_4_ADDR);
		c11 = CONSYS_REG_READ(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_5_ADDR);
		/* EMI CTL
		 * C12	Write 0x1800_1400[22:21]=2'b00 Write 0x1806_015C[2:0]=3'b111 Read 0x1806_02C8
		 * C13	Write 0x1800_1400[22:21]=2'b01 Write 0x1806_015C[2:0]=3'b111 Read 0x1806_02C8
		 * C14	Write 0x1800_1400[22:21]=2'b10 Write 0x1806_015C[2:0]=3'b111 Read 0x1806_02C8
		 */
		CONSYS_REG_WRITE_HW_ENTRY(CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL, 0x0);
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x7);
		c12 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);

		CONSYS_REG_WRITE_HW_ENTRY(CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL, 0x1);
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x7);
		c13 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);

		CONSYS_REG_WRITE_HW_ENTRY(CONN_CFG_EMI_CTL_0_EMI_CTL_DEBUG_1_SEL, 0x2);
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_CONN_INFRA_CFG_DBG_SEL_CONN_INFRA_CFG_DBG_SEL, 0x7);
		c14 = CONSYS_REG_READ(CONN_HOST_CSR_TOP_DBG_DUMMY_2_ADDR);
		c15 = CONSYS_REG_READ(CONN_CFG_EMI_CTL_1_ADDR);
		pr_info("[CONN_BUS_C]power:[0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x][0x%08x]",
			c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15);
		/* File: conn_infra_bus_debug
		 * sheet: 12. low_power_layer_information
		 * Read: 0x1800_E370
		 */
		pr_info("[CONN_BUS_C]low_power_layer_information:[0x%08x]",
			CONSYS_REG_READ(CONN_BUS_CR_CONN_INFRA_LOW_POWER_LAYER_CTRL_ADDR));
	}
}

static int consys_is_bus_hang(void)
{
	unsigned int r1, r2;
	unsigned int ret = 0;
	bool offclk_ok = true;

	consys_bus_hang_dump_a();
	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1228 [19] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0(CR at ap side)
	 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 19));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	if (r1)
		ret = CONNINFRA_AP2CONN_RX_SLP_PROT_ERR;
	if (r2)
		ret = CONNINFRA_AP2CONN_TX_SLP_PROT_ERR;
	if (ret)
		return ret;
	consys_bus_hang_dump_b();

	/* AP2CONN_INFRA OFF
	 * 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * 	- write 0x1 to 0x1806_0000[0], reset clock detect
	 * 	- 0x1806_0000[2]: conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 	- 0x1806_0000[1]: osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * 	- Read 0x1800_1000 = 0x02060002
	 * 4 Check conn_infra off domain bus hang irq status
	 * 	- 0x1806_02D4[0], should be 1'b1, or means conn_infra off bus might hang (conn_infra_bus_timeout_irq_b)
	 * 	(which part is going to timeout?
	 * 	1) 0x1806014C[0] should be 1'b0, or means conn_infra main bus timeout for VDNR timeout mechanism)
	 * 	2) 0x18060448[0] should be 1'b0, or means conn_infra axi layer bus timeout for VDNR timeout mechanism)
	 * 	3) 0x18060434[0] should be 1'b1, or means conn_infra_on timeout for AHB/APB timeout mechanism)
	 * 	4) 0x18060434[1] should be 1'b1, or means conn_infra_off timeout for AHB/APB timeout mechanism)
	 */
	r1 = __consys_bus_hang_clock_detect();
	r2 = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	if ((r1 != 0x6) || r2 != CONN_HW_VER) {
		pr_info("conninfra off clock fail. 0x1806_0000[2:1]=[0x%08x] version=[0x%08x]", r1, r2);
		offclk_ok = false;
	}
	r1 = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_DBG_DUMMY_5_ADDR, (0x1 << 0));
	if (r1 != 0x1) {
		pr_err("conninfra off bus might hang irq_b=[0x%08x] [0x%08x][0x%08x][0x%08x]",
			r1,
			CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_ON_BUS_TIMEOUT_IRQ_ADDR),
			CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_AXI_LAYER_DEBUG_CTRL_AO_TIMEOUT_IRQ),
			CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_BUS_TIMEOUT_IRQ_B_ADDR));
		ret = CONNINFRA_INFRA_BUS_HANG_IRQ;
	}

	consys_bus_hang_dump_c(offclk_ok);
	return ret;
}

int __consys_check_reg_readable(void)
{
	unsigned int r, r1, r2;

	/* AP2CONN_INFRA ON
	 * 1. Check ap2conn gals sleep protect status
	 * 	- 0x1000_1228 [19] / 0x1000_1228 [13](rx/tx)
	 * 	(sleep protect enable ready)
	 * 	both of them should be 1'b0  (CR at ap side)
	 */
	r1 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 19));
	r2 = CONSYS_REG_READ_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, (0x1 << 13));
	if (r1 || r2) {
		pr_info("[%s] AP2CONN_INFRA ON fail: rx=0x%x tx=0x%x", __func__, r1, r2);
		return 0;
	}

	/* AP2CONN_INFRA OFF
	 * 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 * 	- write 0x1 to 0x1806_0000[0], reset clock detect
	 * 	- 0x1806_0000[2] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * 	- 0x1806_0000[1] osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * 	- Read 0x1800_1000 = 0x02060000
	 * 4 Check conn_infra off domain bus hang irq status
	 * 	- 0x1806_02D4[0], should be 1'b1, or means conn_infra off bus might hang (conn_infra_bus_timeout_irq_b)
	 */
	r = __consys_bus_hang_clock_detect();
	if (r != 0x6) {
		pr_info("[%s] Clock detect fail, r=[0x%x]", __func__, r);
		return 0;
	}
	r = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
	if (r != CONN_HW_VER) {
		pr_info("[%s] Check ip version fail, r=[0x%x]", __func__, r);
		return 0;
	}

	return 1;
}

int consys_check_reg_readable(void)
{
	unsigned int r;

	if (__consys_check_reg_readable() == 0)
		return 0;

	r = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_DBG_DUMMY_5_ADDR, (0x1 << 0));
	if (r != 0x1) {
		pr_info("[%s] conn_infra off domain bus timeout irq, r=[0x%x]", __func__, r);
		return 0;
	}

	return 1;
}

int consys_check_reg_readable_for_coredump(void)
{
	unsigned int r;

	if (__consys_check_reg_readable() == 0)
		return 0;

	r = CONSYS_REG_READ_BIT(CONN_HOST_CSR_TOP_DBG_DUMMY_5_ADDR, (0x1 << 0));
	if (r != 0x1) {
		pr_info("[%s] conn_infra off domain bus timeout irq, r=[0x%x]\n", __func__, r);
		return 1; // can ignore bus timeout irq status for coredump
	}

	return 1;
}

int consys_reg_init(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *node = NULL;
	struct consys_reg_base_addr *base_addr = NULL;
	struct resource res;
	int flag, i = 0;

	node = pdev->dev.of_node;
	pr_info("[%s] node=[%p]\n", __func__, node);
	if (node) {
		for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
			base_addr = &conn_reg.reg_base_addr[i];

			ret = of_address_to_resource(node, i, &res);
			if (ret) {
				pr_err("Get Reg Index(%d-%s) failed",
						i, consys_base_addr_index_to_str[i]);
				continue;
			}

			base_addr->phy_addr = res.start;
			base_addr->vir_addr =
				(unsigned long) of_iomap(node, i);
			of_get_address(node, i, &(base_addr->size), &flag);

			pr_info("Get Index(%d-%s) phy(0x%zx) baseAddr=(0x%zx) size=(0x%zx)",
				i, consys_base_addr_index_to_str[i], base_addr->phy_addr,
				base_addr->vir_addr, base_addr->size);
		}

	} else {
		pr_err("[%s] can't find CONSYS compatible node\n", __func__);
		return ret;
	}
	return 0;

}

static int consys_reg_deinit(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (conn_reg.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				conn_reg.reg_base_addr[i].vir_addr);
			iounmap((void __iomem*)conn_reg.reg_base_addr[i].vir_addr);
			conn_reg.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}


