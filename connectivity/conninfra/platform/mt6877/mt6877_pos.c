// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/pinctrl/consumer.h>
#include <linux/irqflags.h>
#include <connectivity_build_in_adapter.h>

#include "consys_hw.h" /* for semaphore index */
/* platform dependent */
#include "plat_def.h"
/* macro for read/write cr */
#include "consys_reg_util.h"
#include "plat_def.h"
/* cr base address */
#include "mt6877_consys_reg.h"
/* cr offset */
#include "mt6877_consys_reg_offset.h"
/* For function declaration */
#include "mt6877.h"
#include "mt6877_pos.h"
#include "pmic_mng.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define SEMA_HOLD_TIME_THRESHOLD 10 //10 ms

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
struct a_die_reg_config {
	unsigned int reg;
	unsigned int mask;
	unsigned int config;
};

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static u64 sema_get_time[CONN_SEMA_NUM_MAX];

static int consys_spi_read_nolock(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data);
static int consys_spi_write_nolock(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data);
#ifndef CONFIG_FPGA_EARLY_PORTING
static const char* get_spi_sys_name(enum sys_spi_subsystem subsystem);
static void consys_spi_write_offset_range_nolock(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size);
#endif
static int connsys_adie_clock_buffer_setting(bool bt_only);

unsigned int consys_emi_set_remapping_reg_mt6877(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	CONSYS_REG_WRITE_OFFSET_RANGE(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_ADDR,
		con_emi_base_addr, 0, 16, 20);
	CONSYS_REG_WRITE_MASK(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_WF_PERI_BASE_ADDR_ADDR,
		0x01000, 0xFFFFF);
	CONSYS_REG_WRITE_MASK(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_BT_PERI_BASE_ADDR_ADDR,
		0x01000, 0xFFFFF);
	CONSYS_REG_WRITE_MASK(
		CONN_HOST_CSR_TOP_CONN2AP_REMAP_GPS_PERI_BASE_ADDR_ADDR,
		0x01000, 0xFFFFF);

	if (md_shared_emi_base_addr) {
		CONSYS_REG_WRITE_OFFSET_RANGE(
			CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_ADDR,
			md_shared_emi_base_addr, 0, 16, 20);
	}
	pr_info("connsys_emi_base=[0x%llx] mcif_emi_base=[0x%llx] remap cr: connsys=[0x%08x] mcif=[0x%08x]\n",
		con_emi_base_addr, md_shared_emi_base_addr,
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN2AP_REMAP_MCU_EMI_BASE_ADDR_ADDR),
		CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN2AP_REMAP_MD_SHARE_EMI_BASE_ADDR_ADDR));
	return 0;

}

int consys_conninfra_on_power_ctrl_mt6877(unsigned int enable)
{
#if MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE
	return consys_platform_spm_conn_ctrl_mt6877(enable);
#else
	int check;

	if (enable) {
		/* Turn on SPM clock (apply this for SPM's CONNSYS power control related CR accessing)
		 * Address:
		 * 	POWERON_CONFIG_EN[0] = 1'b1
		 * 	POWERON_CONFIG_EN[31:16] = 16'h0b16 (key)
		 * 	0x1000_6000[0][31:16]
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		CONSYS_REG_WRITE_MASK(SPM_POWERON_CONFIG_EN, 0x0b160001, 0xffff0001);

#endif
		/* Power on connsys MTCMOS */
		/* Assert "conn_infra_on" primary part power on, set "connsys_on_domain_pwr_on"=1
		 * CONN_PWR_CON[2]=1'b1 (0x1000_6E04[2] = 1'b1)
		 */
		CONSYS_SET_BIT(SPM_CONN_PWR_CON, (0x1 << 2));
		/* Check "conn_infra_on" primary part power status, check "connsys_on_domain_pwr_ack"=1
		 * (polling "10 times" and each polling interval is "0.5ms")
		 * Address:
		 * 	OTHER_PWR_STATUS[0] (0x1000_6178[0])
		 * Value: 1'b1
		 * Action: polling
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		check = 0;
		CONSYS_REG_BIT_POLLING(SPM_OTHER_PWR_STATUS, 0, 0x1, 10, 500, check);
		if (check != 0)
			pr_err("check 'connsys_on_domain_pwr_ack=1' fail. 0x1000_6178[0] should be 1b'1. Get:0x%08x\n",
				CONSYS_REG_READ(SPM_OTHER_PWR_STATUS));
#endif

		/* Assert "conn_infra_on" secondary part power on, set "connsys_on_domain_pwr_on_s"=1
		 * CONN_PWR_CON[3] (0x1000_6E04[3]) = 1'b1
		 */
		CONSYS_SET_BIT(SPM_CONN_PWR_CON, (0x1 << 3));

		/* Check "conn_infra_on" secondary part power status, check "connsys_on_domain_pwr_ack_s"=1
		 * (polling "10 times" and each polling interval is "0.5ms")
		 * Address:
		 * 	PWR_STATUS_2ND[1] (0x1000_6EF4[1])
		 * Value: 1'b1
		 * Action: polling
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		check = 0;
		CONSYS_REG_BIT_POLLING(SPM_PWR_STATUS_2ND, 1, 0x1, 10, 500, check);
		if (check != 0)
			pr_err("Check 'connsys_on_domain_pwr_ack_s=1' fail. 0x1000_6EF4[1] should be 1b'1. Get: 0x%08x\n",
				CONSYS_REG_READ(SPM_PWR_STATUS_2ND));
#endif
		/* Checkpoint "connsys MTCMOS pwr on" */
		/* Turn on AP-to-CONNSYS bus clock, set "conn_clk_dis"=0
		 * (apply this for bus clock toggling)
		 * CONN_PWR_CON[4](0x1000_6E04[4]) = 1'b0
		 */
		CONSYS_CLR_BIT(SPM_CONN_PWR_CON, (0x1 << 4));
		/* wait 1us */
		udelay(1);

		/* De-assert "conn_infra_on" isolation, set "connsys_iso_en"=0
		 * CONN_PWR_CON[1](0x1000_6E04[1]) = 1'b0
		 */
		CONSYS_CLR_BIT(SPM_CONN_PWR_CON, (0x1 << 1));

		/* De-assert CONNSYS S/W reset (TOP RGU CR), set "ap_sw_rst_b"=1
		 * WDT_SWSYSRST[9]=1'b0
		 * WDT_SWSYSRST[31:24]=8'h88 (key)
		 * (0x1000_7200[9][31:24])
		 */
		CONSYS_REG_WRITE_MASK(TOPRGU_WDT_SWSYSRST, 0x88000000, 0xff000200);

		/* De-assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=1
		 * CONN_PWR_CON[0](0x1000_6E04[0])=1'b1
		 */
		CONSYS_SET_BIT(SPM_CONN_PWR_CON, (0x1 << 0));

		/* wait 0.5ms */
		udelay(500);

		/* conn2ap/ap2conn slpprot disable */
		/* Turn off AP2CONN AHB RX bus sleep protect
		 * (apply this for INFRA AXI bus accessing when CONNSYS had been turned on)
		 * INFRA_TOPAXI_PROTECTEN[19](0x1000_1220[19])=1'b0
		 */
		CONSYS_CLR_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 19));

		/* Check AP2CONN AHB RX bus sleep protect turn off
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * If AP2CONN (TX/RX) protect turn off fail, power on fail.
		 * (DRV access connsys CR will get 0)
		 * Address:INFRA_TOPAXI_PROTECTEN_STA1[19](0x1000_1228[19])
		 * Value: 1'b0
		 * Action: polling
		 */
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 19, 0x0, 100, 500, check);
		if (check)
			pr_err("Check AP2CONN AHB RX bus sleep protect turn off fail. 0x1000_1228[19] should be 1b'0. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));

		/* Turn off AP2CONN AHB TX bus sleep protect
		 * (apply this for INFRA AXI bus accessing when CONNSYS had been turned on)
		 * INFRA_TOPAXI_PROTECTEN[13](0x1000_1220[13])=1'b0
		 */
		CONSYS_CLR_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 13));

		/* Check AP2CONN AHB TX bus sleep protect turn off
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * If AP2CONN (TX/RX) protect turn off fail, power on fail.
		 * (DRV access connsys CR will get 0 )
		 * Address: INFRA_TOPAXI_PROTECTEN_STA1[13](0x1000_1228[13])
		 * Value: 1'b0
		 * Action: polling
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 13, 0x0, 100, 500, check);
		if (check)
			pr_err("Check AP2CONN AHB TX bus sleep protect turn off fail. 0x1000_1228[13] should be 1b'0. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));
#endif
		/* Turn off CONN2AP AXI RX bus sleep protect
		 * (disable sleep protection when CONNSYS had been turned on)
		 * Note : Should turn off AXI Rx sleep protection first.
		 * INFRA_TOPAXI_PROTECTEN[14](0x1000_1220[14])=1'b0
		 */
		CONSYS_CLR_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 14));

		/* Turn off CONN2AP AXI TX bus sleep protect
		 * (disable sleep protection when CONNSYS had been turned on)
		 * Note : Should turn off AXI Tx sleep protection after AXI Rx sleep protection has been turn off.
		 * INFRA_TOPAXI_PROTECTEN[18](0x1000_1220[18]_=1'b0
		 */
		CONSYS_CLR_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 18));

		/* Checkpoint "AP2CONN/CONN2AP slpprot release */
		/* Wait 5ms (apply this for CONNSYS XO clock ready)
		 * [NOTE] this setting could be changed at different design architecture and
		 * the characteristic of AFE WBG)
		 */
		msleep(5);

	} else {
		/* conn2ap/ap2conn slpprot enable */
		/* Turn on AP2CONN AHB TX bus sleep protect
		 * INFRA_TOPAXI_PROTECTEN[13](0x1000_1220[13])=1'b1
		 */
		CONSYS_SET_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 13));
		/* Check  AP2CONN AHB TX bus sleep protect turn off
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * Address: INFRA_TOPAXI_PROTECTEN_STA1[13](0x1000_1228[13])
		 * value: 1'b1
		 * Action: polling
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 13, 0x1, 100, 500, check);
		if (check)
			pr_err("Check AP2CONN AHB TX bus sleep protect turn on fail. 0x1000_1228[13] should be 1b'1. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));
#endif
		/* Turn on AP2CONN AHB RX bus sleep protect
		 * INFRA_TOPAXI_PROTECTEN[15](0x1000_1220[19]) = 1'b1
		 */
		CONSYS_SET_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 19));

		/* Check AP2CONN AHB RX bus sleep protect turn on
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * Address: INFRA_TOPAXI_PROTECTEN_STA1[19](0x1000_1228[19])
		 * Value: 1'b1
		 * Action: polling
		 */
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 19, 0x1, 100, 500, check);
		if (check)
			pr_err("Check AP2CONN AHB RX bus sleep protect turn on fail. 0x1000_1228[19] should be 1b'1. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));

		/* Turn on CONN2AP AXI TX bus sleep protect
		 * (disable sleep protection when CONNSYS had been turned on)
		 * INFRA_TOPAXI_PROTECTEN[18](0x1000_1220[18]) = 1'b1
		 */
		CONSYS_SET_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 18));

		/* Check CONN2AP AXI TX bus sleep protect turn on
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * Address: INFRA_TOPAXI_PROTECTEN_STA1[18](0x1000_1228[18])
		 * Value: 1'b1
		 * Action: polling
		 */
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 18, 0x1, 100, 500, check);
		if (check)
			pr_err("Check CONN2AP AXI TX bus sleep protect turn on fail. 0x1000_1228[18] should be 1b'1. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));

		/* Turn on CONN2AP AXI RX bus sleep protect
		 * (disable sleep protection when CONNSYS had been turned on)
		 * Note : Should turn off AXI Rx sleep protection first.
		 * INFRA_TOPAXI_PROTECTEN[14](0x1000_1220[14]) = 1'b1
		 */
		CONSYS_SET_BIT(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN, (0x1 << 14));

		/* Check  CONN2AP AXI RX bus sleep protect turn off
		 * (polling ""100 times"" and each polling interval is ""0.5ms"")
		 * Address: INFRA_TOPAXI_PROTECTEN_STA1[14](0x1000_1228[14])
		 * Value: 1'b1
		 * Action: polling
		 */
#ifndef CONFIG_FPGA_EARLY_PORTING
		check = 0;
		CONSYS_REG_BIT_POLLING(
			INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1, 14, 0x1, 100, 500, check);
		if (check)
			pr_err("Check CONN2AP AXI RX bus sleep protect turn on fail. 0x1000_1228[14] should be 1b'1. But get 0x%08x\n",
				CONSYS_REG_READ(INFRACFG_AO_INFRA_TOPAXI_PROTECTEN_STA1));
#endif
		/* Power off connsys MTCMOS */
		/* Assert "conn_infra_on" isolation, set "connsys_iso_en"=1
		 * CONN_PWR_CON[1](0x1000_6E04[1]) = 1'b1
		 */
		CONSYS_SET_BIT(SPM_CONN_PWR_CON, (0x1 << 1));

		/* Assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=0
		 * CONN_PWR_CON[0](0x1000_6E04[0]) = 1'b0
		 */
		CONSYS_CLR_BIT(SPM_CONN_PWR_CON, (0x1 << 0));

		/* Assert CONNSYS S/W reset(TOP RGU CR), set "ap_sw_rst_b"=0
		 * WDT_SWSYSRST(0x1000_7200)[9] = 1'b1
		 * WDT_SWSYSRST(0x1000_7200)[31:24]=8'h88 (key)
		 */
		CONSYS_REG_WRITE_MASK(TOPRGU_WDT_SWSYSRST, 0x88000200, 0xff000200);

		/* Turn off AP-to-CONNSYS bus clock,
		 * set "conn_clk_dis"=1 (apply this for bus clock gating)
		 * CONN_PWR_CON[4](0x1000_6E04[4] = 1'b1
		 */
		CONSYS_SET_BIT(SPM_CONN_PWR_CON, (0x1 << 4));
		/* Wait 1us */
		udelay(1);

		/* De-assert "conn_infra_on" primary part power on, set "connsys_on_domain_pwr_on"=0
		 * CONN_PWR_CON[2](0x1000_6E04[2]) = 1'b0
		 */
		CONSYS_CLR_BIT(SPM_CONN_PWR_CON, (0x1 << 2));

		/* De-assert "conn_infra_on" secondary part power on, set "connsys_on_domain_pwr_on_s"=0
		 * CONN_PWR_CON[3](0x1000_6E04[3]) = 1'b0
		 */
		CONSYS_CLR_BIT(SPM_CONN_PWR_CON, (0x1 << 3));
	}
	return 0
#endif /* MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE */
}

int consys_conninfra_wakeup_mt6877(void)
{

	/* Wake up conn_infra
	 * CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_CONN_INFRA_WAKEPU_TOP(0x180601a0)=1'b1
	 */
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR, 0x1);

	return consys_polling_chipid_mt6877();
}

int consys_conninfra_sleep_mt6877(void)
{
	/* Release conn_infra force on
	 * CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_CONN_INFRA_WAKEPU_TOP(0x180601a0) = 1'b0
	 */
	CONSYS_REG_WRITE(CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR, 0x0);
	return 0;
}

void consys_set_if_pinmux_mt6877(unsigned int enable)
{
#ifndef CFG_CONNINFRA_ON_CTP
	struct pinctrl_state *tcxo_pinctrl_set;
	struct pinctrl_state *tcxo_pinctrl_clr;
	int ret = -1;
#endif

	if (enable) {
		/* set pinmux for the interface between D-die and A-die (Aux1)
		 * 	CONN_HRST_B(0x1000_5480[26:24])
		 * 	CONN_TOP_CLK(0x1000_5480[30:28])/CONN_TOP_DATA(0x1000_5490[2:0])
		 * 	CONN_WB_PTA(0x1000_5490[6:4])
		 * 	CONN_BT_CLK(0x1000_5480[18:16])/CONN_BT_DATA(0x1000_5480[22:20])
		 * 	CONN_WF_CTRL0/CONN_WF_CTRL1/CONN_WF_CTRL2/CONN_WF_CTRL3/CONN_WF_CTRL4
		 * 	(0x1000_5490[10:8]/0x1000_5490[14:12]/0x1000_5490[18:16]/0x1000_5490[22:20]/0x1000_5490[26:24])
		 * Write to 3b'001
		 */
		CONSYS_REG_WRITE_MASK(GPIO_MODE24, 0x11110000, 0x77770000);
		CONSYS_REG_WRITE_MASK(GPIO_MODE25, 0x01111111, 0x07777777);

		/* Set pinmux driving to 2mA setting
		 * conn(PAT_CONN_TOP_DATA/PAD_CONN_TOP_WB_PTA/PAD_CONN_WF_CTRL0~4)
		 * conn_bt_clk(PAC_CONN_BT_CLK)
		 * conn_bt_DATA(PAC_CONN_BT_DATA)
		 * conn_hrst_b(PAD_CONN_HRST_B)
		 * conn_top_clk(PAD_CONN_TOP_CLK)
		 * 0x11EA_0000[29:27]=3'b000
		 */
		CONSYS_REG_WRITE_MASK(IOCFG_RT_DRV_CFG0, 0x0, 0x38000000);

		/* Set pinmux PUPD setting
		 * Clear CONN_TOP_DATA/CONN_BT_DATA PD setting
		 * 0x11EA_0078[14][9]=2'b11
		 */
		CONSYS_SET_BIT(IOCFG_RT_PD_CFG0_CLR, ((0x1 << 14) | (0x1 << 9)));
		/* Set pinmux PUPD setting
		 * CONN_TOP_DATA/CONN_BT_DATA as PU
		 * 0x11EA_0094[14][9]= 2'b11
		 */
		CONSYS_SET_BIT(IOCFG_RT_PU_CFG0_SET, ((0x1 << 14) | (0x1 << 9)));
		/* if(TCXO mode)
		 * 	Set GPIO135 pinmux for TCXO mode (Aux3)(CONN_TCXOENA_REQ)
		 */

		if (consys_co_clock_type_mt6877() == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
			/* For CTP load, write CR directly */
			/* 0x1000_5400[30:28]=3'b011 */
			CONSYS_REG_WRITE_MASK(GPIO_MODE16, 0x30000000, 0x70000000);
	#else
			if (!IS_ERR(g_conninfra_pinctrl_ptr)) {
				tcxo_pinctrl_set = pinctrl_lookup_state(g_conninfra_pinctrl_ptr, "conninfra_tcxo_set");
				if (!IS_ERR(tcxo_pinctrl_set)) {
					ret = pinctrl_select_state(g_conninfra_pinctrl_ptr, tcxo_pinctrl_set);
					if (ret)
						pr_err("[%s] set TCXO mode error: %d\n", __func__, ret);
				}
			}
	#endif /* defined(CFG_CONNINFRA_ON_CTP) */
		}
	} else {
		/* Set pinmux for the interface between D-die and A-die (Aux0)
		 * 	CONN_HRST_B(0x1000_5480[26:24])
		 * 	CONN_TOP_CLK(0x1000_5480[30:28])/CONN_TOP_DATA(0x1000_5490[2:0])
		 * 	CONN_WB_PTA(0x1000_5490[6:4])
		 * 	CONN_BT_CLK(0x1000_5480[18:16])/CONN_BT_DATA(0x1000_5480[22:20])
		 * 	CONN_WF_CTRL0/CONN_WF_CTRL1/CONN_WF_CTRL2/CONN_WF_CTRL3/CONN_WF_CTRL4
		 * 	(0x1000_5490[10:8]/0x1000_5490[14:12]/0x1000_5490[18:16]/0x1000_5490[22:20]/0x1000_5490[26:24])
		 * Write 3'b000
		 */
		CONSYS_REG_WRITE_MASK(GPIO_MODE24, 0x0, 0x77770000);
		CONSYS_REG_WRITE_MASK(GPIO_MODE25, 0x0, 0x07777777);

		/* Set pinmux PUPD setting
		 * Clear CONN_TOP_DATA/CONN_BT_DATA PU setting
		 * 0x11EA_0098[14][9]=2'b11
		 */
		CONSYS_SET_BIT(IOCFG_RT_PU_CFG0_CLR, ((0x1 << 14) | (0x1 << 9)));

		/* Set pinmux PUPD setting
		 * CONN_TOP_DATA/CONN_BT_DATA as PD
		 * 0x11EA_0074[14][9]=2'b11
		 */
		CONSYS_SET_BIT(IOCFG_RT_PD_CFG0_SET, ((0x1 << 14) | (0x1 << 9)));

		if (consys_co_clock_type_mt6877() == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
		/* For CTP load, write CR directly */
			/* if(TCXO mode)
			 * 	Set GPIO135 pinmux to GPIO mode (Aux0)(CONN_TCXOENA_REQ)
			 * 0x1000_5400[30:28]=3'b000
			 */
			CONSYS_REG_WRITE_MASK(GPIO_MODE16, 0, 0x70000000);
	#else
			if (!IS_ERR(g_conninfra_pinctrl_ptr)) {
				tcxo_pinctrl_clr = pinctrl_lookup_state(g_conninfra_pinctrl_ptr, "conninfra_tcxo_clr");
				if (!IS_ERR(tcxo_pinctrl_clr)) {
					ret = pinctrl_select_state(g_conninfra_pinctrl_ptr, tcxo_pinctrl_clr);
					if (ret)
						pr_err("[%s] clear TCXO mode error: %d\n", __func__, ret);
				}
			}
	#endif /* defined(CFG_CONNINFRA_ON_CTP) */
		}
	}
}

int consys_polling_chipid_mt6877(void)
{
	unsigned int retry = 11;
	unsigned int consys_hw_ver = 0;
	int ret = -1;

	/* check CONN_INFRA IP Version
	 * (polling "10 times" for specific project code and each polling interval is "1ms")
	 * Address: CONN_INFRA IP Version(0x1800_1000[31:0])
	 * Value: 32'h0206_0002
	 * Action: polling
	 */
	while (--retry > 0) {
		consys_hw_ver = CONSYS_REG_READ(CONN_CFG_IP_VERSION_ADDR);
		if (consys_hw_ver == CONN_HW_VER) {
			pr_info("Consys HW version id(0x%x)\n", consys_hw_ver);
			ret = 0;
			break;
		}
		msleep(1);
	}

	if (retry == 0) {
		pr_err("Read CONSYS version id fail. Expect 0x%x but get 0x%x\n",
			CONN_HW_VER, consys_hw_ver);
#if defined(KERNEL_clk_buf_show_status_info)
		connectivity_export_clk_buf_show_status_info();
#endif
		return -1;
	}

	return ret;
}

int connsys_d_die_cfg_mt6877(void)
{
	/* Reset conninfra sysram */
#ifdef CFG_CONNINFRA_ON_CTP
	memset_io(CONN_INFRA_SYSRAM_BASE, 0x0, CONN_INFRA_SYSRAM_SIZE);
#else
	void __iomem *addr = NULL;
	addr = ioremap(CONN_INFRA_SYSRAM_BASE, CONN_INFRA_SYSRAM_SIZE);
	if (addr != NULL) {
		memset_io(addr, 0x0, CONN_INFRA_SYSRAM_SIZE);
		iounmap(addr);
	} else
		pr_err("[%s] remap 0x%08x fail", __func__, CONN_INFRA_SYSRAM_BASE);
#endif
	/* Set infra top emi address range
	 * CONN_INFRA_BUS_CR_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_START_CR_CONN2AP_EMI_PATH_ADDRESS_START
	 * 0x1800_E360=0x0400_0000
	 * CONN_INFRA_BUS_CR_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_END_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_END
	 * 0x1800_E364=0x43FF_FFFF
	 */
	CONSYS_REG_WRITE(CONN_BUS_CR_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_START_ADDR, 0x4000000);
	CONSYS_REG_WRITE(CONN_BUS_CR_CONN_INFRA_CONN2AP_EMI_PATH_ADDR_END_ADDR, 0x43ffffff);

	/* Read D-die Efuse
	 * AP2CONN_EFUSE_DATA 0x1800_1020
	 * Write to conninfra sysram: CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE(0x1805_2820)
	 */
	CONSYS_REG_WRITE(
		CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE, CONSYS_REG_READ(CONN_CFG_EFUSE_ADDR));

	/* conn_infra sysram hw control setting -> disable hw power down
	 * CONN_INFRA_RGU_SYSRAM_HWCTL_PDN_SYSRAM_HWCTL_PDN(0x1800_0050)=32'h0
	 */
	CONSYS_REG_WRITE(CONN_RGU_SYSRAM_HWCTL_PDN_ADDR, 0x0);

	/* conn_infra sysram hw control setting -> enable hw sleep
	 * CONN_INFRA_RGU_SYSRAM_HWCTL_SLP_SYSRAM_HWCTL_SLP(0x1800_0054=32'h0000_0001)
	 */
	CONSYS_REG_WRITE(CONN_RGU_SYSRAM_HWCTL_SLP_ADDR, 0x1);

	/* co-ext memory  hw control setting -> disable hw power down
	 * CONN_INFRA_RGU_CO_EXT_MEM_HWCTL_PDN_CO_EXT_MEM_HWCTL_PDN(0x1800_0070)=32'h0
	 */
	CONSYS_REG_WRITE(CONN_RGU_CO_EXT_MEM_HWCTL_PDN_ADDR, 0x0);

	/* co-ext memory  hw control setting -> enable hw sleep
	 * CONN_INFRA_RGU_CO_EXT_MEM_HWCTL_SLP_CO_EXT_MEM_HWCTL_SLP(0x1800_0074)=32'h0000_0001
	 */
	CONSYS_REG_WRITE(CONN_RGU_CO_EXT_MEM_HWCTL_SLP_ADDR, 0x1);

#if defined(CONNINFRA_PLAT_BUILD_MODE)
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE, CONNINFRA_PLAT_BUILD_MODE);
	pr_info("[%s] Write CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE to 0x%08x\n",
		__func__, CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE));
#endif

	return 0;
}

int connsys_adie_top_ck_en_ctl_mt6877(bool onoff)
{
	int check = 0;

	/* 1. CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0
	 * 	0x18005120[0] = 1'b1/1'b0 (Enable/Disable)
	 * 	dig_top_ck in Adie (Address in Adie: 12'hA00)
	 * 2. Polling CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_WB_SLP_TOP_CK_0_BSY
	 * 	POLLING	0x18005120[1] == 1'b0
	 */
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR, onoff, 0x1);
	CONSYS_REG_BIT_POLLING(
		CONN_WT_SLP_CTL_REG_WB_SLP_TOP_CK_0_ADDR, 1, 0, 100, 5, check);
	if (check == -1) {
		pr_err("[%s] op=%d fail\n", __func__, onoff);
	}
	return check;

}


static int connsys_adie_clock_buffer_setting(bool bt_only)
{
#ifndef CONFIG_FPGA_EARLY_PORTING

#define ADIE_CONFIG_NUM	2
	// E1 WF/GPS/FM on(default)
	const static struct a_die_reg_config adie_e1_default[ADIE_CONFIG_NUM] =
	{
		{ATOP_RG_TOP_XTAL_01, 0xc180, 0xc180},
		{ATOP_RG_TOP_XTAL_02, 0xf0ff0080, 0xd0550080},
	};
	const static struct a_die_reg_config adie_e1_bt_only[ADIE_CONFIG_NUM] =
	{
		{ATOP_RG_TOP_XTAL_01, 0xc180, 0x100},
		{ATOP_RG_TOP_XTAL_02, 0xf0ff0080, 0xf0ff0000},
	};
	const static struct a_die_reg_config adie_e2_default[ADIE_CONFIG_NUM] =
	{
		{ATOP_RG_TOP_XTAL_01, 0xc180, 0xc180},
		{ATOP_RG_TOP_XTAL_02, 0xf0ff0080, 0x50550080},
	};
	const static struct a_die_reg_config adie_e2_bt_only[ADIE_CONFIG_NUM] =
	{
		{ATOP_RG_TOP_XTAL_01, 0xc180, 0x100},
		{ATOP_RG_TOP_XTAL_02, 0xf0ff0080, 0x70ff0000},
	};

	int hw_version;
	const struct a_die_reg_config* config = NULL;
	unsigned int ret, i;

	hw_version = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID);

	if (bt_only) {
		/* E1 */
		if (hw_version == 0x66358A00) {
			config = adie_e1_bt_only;
		} else if (hw_version == 0x66358A10 || hw_version == 0x66358A11) {
			config = adie_e2_bt_only;
		} else
			pr_err("[%s] wrong adie version (0x%08x)\n", __func__, hw_version);
	} else {
		if (hw_version == 0x66358A00) {
			config = adie_e1_default;
		} else if (hw_version == 0x66358A10 || hw_version == 0x66358A11) {
			config = adie_e2_default;
		} else
			pr_err("[%s] wrong adie version (0x%08x)\n", __func__, hw_version);
	}

	if (config == NULL)
		return -1;

	connsys_adie_top_ck_en_ctl_mt6877(true);
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[EFUSE READ] Require semaphore fail\n");
		connsys_adie_top_ck_en_ctl_mt6877(false);
		return -1;
	}

	for (i = 0; i < ADIE_CONFIG_NUM; i++) {
		consys_spi_read_nolock(SYS_SPI_TOP, config[i].reg, &ret);
		ret &= (~config[i].mask);
		ret |= config[i].config;
		consys_spi_write_nolock(SYS_SPI_TOP, config[i].reg, ret);
	}

	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
	connsys_adie_top_ck_en_ctl_mt6877(false);
#endif
	return 0;
}

int connsys_spi_master_cfg_mt6877(unsigned int next_status)
{
	unsigned int bt_only = 0;

	if ((next_status & (~(0x1 << CONNDRV_TYPE_BT))) == 0)
		bt_only = 1;

	/* WF_CK_ADDR		0x18005070[11:0]	0xA04
	 * WF_B1_CK_ADDR	0x18005070[27:16]	0xAF4
	 * WF_WAKE_ADDR		0x18005074[11:0]	0x090
	 * WF_B1_WAKE_ADDR	0x18005074[27:16]	0x0A0
	 * WF_ZPS_ADDR		0x18005078[11:0]	0x08C
	 * WF_B1_ZPS_ADDR	0x18005078[27:16]	0x09C
	 * BT_CK_ADDR		0x1800507C[11:0]	0xA08
	 * BT_WAKE_ADDR		0x18005080[11:0]	0x094
	 * TOP_CK_ADDR		0x18005084[11:0]	0x02C 
	 * GPS_CK_ADDR		0x18005088[11:0]	0xA0C
	 * GPS_L5_CK_ADDR	0x18005088[27:16]	0xAFC
	 * WF_B0_CMD_ADDR	0x1800508c[11:0]	0x0F0
	 * WF_B1_CMD_ADDR	0x18005090[11:0]	0x0F4
	 * GPS_RFBUF_ADR	0x18005094[11:0]	0x0FC
	 * GPS_L5_EN_ADDR	0x18005098[11:0]	0x0F8
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_CK_ADDR, 0xa04);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_CK_ADDR_WB_WF_B1_CK_ADDR, 0xaf4);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_WAKE_ADDR, 0x090);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_WAKE_ADDR_WB_WF_B1_WAKE_ADDR, 0x0a0);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_ZPS_ADDR, 0x08c);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_ZPS_ADDR_WB_WF_B1_ZPS_ADDR, 0x09c);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_BT_CK_ADDR_WB_BT_CK_ADDR, 0xa08);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_BT_WAKE_ADDR_WB_BT_WAKE_ADDR, 0x094);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_TOP_CK_ADDR_WB_TOP_CK_ADDR, 0x02c);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_CK_ADDR, 0xa0c);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_GPS_CK_ADDR_WB_GPS_L5_CK_ADDR, 0xafc);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_B0_CMD_ADDR_WB_WF_B0_CMD_ADDR, 0x0f0);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_WF_B1_CMD_ADDR_WB_WF_B1_CMD_ADDR, 0x0f4);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_GPS_RFBUF_ADR_WB_GPS_RFBUF_ADR, 0x0fc);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_GPS_L5_EN_ADDR_WB_GPS_L5_EN_ADDR, 0x0f8);

	/* CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH(0x004[4:0]) = 0x8
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_WB_BG_ADDR1(0x10[15:0]) = 0xA03C
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_WB_BG_ADDR2(0x14[15:0]) = 0xA03C
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_WB_BG_ADDR3(0x18[15:0]) = 0xAA18
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_WB_BG_ADDR4(0x1c[15:0]) = 0xAA18
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_WB_BG_ADDR5(0x20[15:0]) = 0xA0C8
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_WB_BG_ADDR6(0x24[15:0]) = 0xAA00
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_WB_BG_ADDR7(0x28[15:0]) = 0xA0B4
	 * CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_WB_BG_ADDR8(0x2c[15:0]) = 0xA34C
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON1_WB_BG_ON1(0x30)     = 0x00000000
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON2_WB_BG_ON2(0x34)     = 0x00000000
	 * if (BT_only) {
	 *    CONN_WT_SLP_CTL_REG_WB_BG_ON3_WB_BG_ON3(0x38)     = 0x74E03F75
	 *    CONN_WT_SLP_CTL_REG_WB_BG_ON4_WB_BG_ON4(0x3c)     = 0x76E83F75
	 * } else {
	 *    CONN_WT_SLP_CTL_REG_WB_BG_ON3_WB_BG_ON3(0x38)     = 0x74E0FFF5
	 *    CONN_WT_SLP_CTL_REG_WB_BG_ON4_WB_BG_ON4(0x3c)     = 0x76E8FFF5
	 * }
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON5_WB_BG_ON5(0x40)     = 0x00000000
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON6_WB_BG_ON6(0x44)     = 0xFFFFFFFF
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON7_WB_BG_ON7(0x48)     = 0x00000019
	 * CONN_WT_SLP_CTL_REG_WB_BG_ON8_WB_BG_ON8(0x4c)     = 0x00010400
	 *
	 * CONN_WT_SLP_CTL_REG_WB_BG_OFF1_WB_BG_OFF1(0x50)   = 0x57400000
	 * CONN_WT_SLP_CTL_REG_WB_BG_OFF2_WB_BG_OFF2(0x54)   = 0x57400000
	 * if (BT only) {
	 *    CONN_WT_SLP_CTL_REG_WB_BG_OFF3_WB_BG_OFF3(0x58)   = 0x44E03F75
	 *    CONN_WT_SLP_CTL_REG_WB_BG_OFF4_WB_BG_OFF4(0x5c)   = 0x44E03F75
	 * } else {
	 *    CONN_WT_SLP_CTL_REG_WB_BG_OFF3_WB_BG_OFF3(0x58)   = 0x44E0FFF5
	 *    CONN_WT_SLP_CTL_REG_WB_BG_OFF4_WB_BG_OFF4(0x5c)   = 0x44E0FFF5
	 * }
	 * CONN_WT_SLP_CTL_REG_WB_BG_OFF5_WB_BG_OFF5(0x60)   = 0x00000001
	 * CONN_WT_SLP_CTL_REG_WB_BG_OFF6_WB_BG_OFF6(0x64)   = 0x00000000
	 * CONN_WT_SLP_CTL_REG_WB_RG_OFF7_WB_BG_OFF7(0x68)   = 0x00040019
	 * CONN_WT_SLP_CTL_REG_WB_RG_OFF8_WB_BG_OFF8(0x6c)   = 0x00410440
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_WT_SLP_CTL_REG_WB_SLP_CTL_CMD_LENGTH, 0x8);

	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR1_ADDR, 0xa03c, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR2_ADDR, 0xa03c, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR3_ADDR, 0xaa18, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR4_ADDR, 0xaa18, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR5_ADDR, 0xa0c8, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR6_ADDR, 0xaa00, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR7_ADDR, 0xa0b4, 0xffff);
	CONSYS_REG_WRITE_MASK(CONN_WT_SLP_CTL_REG_WB_BG_ADDR8_ADDR, 0xa34c, 0xffff);

	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON1_ADDR, 0x00000000);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON2_ADDR, 0x00000000);
	if (bt_only) {
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON3_ADDR, 0x74E03F75);
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON4_ADDR, 0x76E83F75);
	} else {
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON3_ADDR, 0x74E0fff5);
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON4_ADDR, 0x76E8FFF5);
	}
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON5_ADDR, 0x00000000);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON6_ADDR, 0xFFFFFFFF);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON7_ADDR, 0x00000019);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_ON8_ADDR, 0x00010400);

	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF1_ADDR, 0x57400000);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF2_ADDR, 0x57400000);
	if (bt_only) {
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF3_ADDR, 0x44E03F75);
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF4_ADDR, 0x44E03F75);
	} else {
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF3_ADDR, 0x44e0fff5);
		CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF4_ADDR, 0x44e0fff5);
	}
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF5_ADDR, 0x00000001);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF6_ADDR, 0x00000000);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF7_ADDR, 0x00040019);
	CONSYS_REG_WRITE(CONN_WT_SLP_CTL_REG_WB_BG_OFF8_ADDR, 0x00410440);

	return 0;
}

#ifndef CONFIG_FPGA_EARLY_PORTING
static bool connsys_a_die_efuse_read_nolock(
	unsigned int efuse_addr,
	unsigned int *efuse0, unsigned int *efuse1, unsigned int *efuse2, unsigned int *efuse3)
{
	bool efuse_valid = false;
	int retry = 0;
	int ret0, ret1, ret2, ret3;
	unsigned int ret;

	if (!efuse0 || !efuse1 || !efuse2 || !efuse3) {
		pr_err("[%s] invalid input", __func__);
		return false;
	}

	/* Efuse control clear, clear Status /trigger
	 * Address: ATOP EFUSE_CTRL_write_efsrom_kick_and_read_kick_busy_flag (0x108[30])
	 * Data: 1'b0
	 * Action: TOPSPI_WR
	 */
	consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, &ret);
	ret &= ~(0x1 << 30);
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, ret);

	/* Efuse Read 1st 16byte
	 * Address:
	 *    ATOP EFUSE_CTRL_efsrom_mode (0x108[7:6]) = 2'b00
	 *    ATOP EFUSE_CTRL_efsrom_ain (0x108[25:16]) = efuse_addr (0)
	 *    ATOP EFUSE_CTRL_write_efsrom_kick_and_read_kick_busy_flag (0x108[30]) = 1'b1
	 * Action: TOPSPI_WR
	 */
	consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, &ret);
	ret &= ~(0x43ff00c0);
	ret |= (0x1 << 30);
	ret |= ((efuse_addr << 16) & 0x3ff0000);
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, ret);

	/* Polling EFUSE busy = low
	 * (each polling interval is "30us" and polling timeout is 2ms)
	 * Address:
	 *    ATOP EFUSE_CTRL_write_efsrom_kick_and_read_kick_busy_flag (0x108[30]) = 1'b0
	 * Action: TOPSPI_Polling
	 */
	consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, &ret);
	while ((ret & (0x1 << 30)) != 0 && retry < 70) {
		retry++;
		udelay(30);
		consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, &ret);
	}
	if ((ret & (0x1 << 30)) != 0) {
		pr_info("[%s] EFUSE busy, retry failed(%d)\n", __func__, retry);
	}

	/* Check efuse_valid & return
	 * Address: ATOP EFUSE_CTRL_csri_efsrom_dout_vld_sync_1_ (0x108[29])
	 * Action: TOPSPI_RD
	 */
	/* if (efuse_valid == 1'b1)
	 *     Read Efuse Data to global var
	 */
	consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_CTRL, &ret);
	if (((ret & (0x1 << 29)) >> 29) == 1) {
		ret0 = consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_RDATA0, efuse0);
		CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0, *efuse0);

		ret1 = consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_RDATA1, efuse1);
		CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1, *efuse1);

		ret2 = consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_RDATA2, efuse2);
		CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2, *efuse2);

		ret3 = consys_spi_read_nolock(SYS_SPI_TOP, ATOP_EFUSE_RDATA3, efuse3);
		CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3, *efuse3);

		pr_info("efuse = [0x%08x, 0x%08x, 0x%08x, 0x%08x]", *efuse0, *efuse1, *efuse2, *efuse3);
		if (ret0 || ret1 || ret2 || ret3)
			pr_err("efuse read error: [%d, %d, %d, %d]", ret0, ret1, ret2, ret3);
		efuse_valid = true;
	} else
		pr_err("EFUSE is invalid\n");

	return efuse_valid;
}

static int connsys_a_die_thermal_cal_nolock(
	int efuse_valid,
	unsigned int efuse0, unsigned int efuse1, unsigned int efuse2, unsigned int efuse3)
{
	struct consys_plat_thermal_data_mt6877 input;
	memset(&input, 0, sizeof(struct consys_plat_thermal_data_mt6877));

	if (efuse_valid) {
		if (efuse1 & (0x1 << 7)) {
			consys_spi_write_offset_range_nolock(
				SYS_SPI_TOP, ATOP_RG_TOP_THADC_BG, efuse1, 12, 3, 4);
			consys_spi_write_offset_range_nolock(
				SYS_SPI_TOP, ATOP_RG_TOP_THADC, efuse1, 23, 0, 3);
		}
		if(efuse1 & (0x1 << 15)) {
			consys_spi_write_offset_range_nolock(
				SYS_SPI_TOP, ATOP_RG_TOP_THADC, efuse1, 26, 13, 2);
			input.slop_molecule = (efuse1 & 0x1f00) >> 8;
			pr_info("slop_molecule=[%d]", input.slop_molecule);
		}
		if (efuse1 & (0x1 << 23)) {
			/* [22:16] */
			input.thermal_b = (efuse1 & 0x7f0000) >> 16;
			pr_info("thermal_b =[%d]", input.thermal_b);
		}
		if (efuse1 & (0x1 << 31)) {
			input.offset = (efuse1 & 0x7f000000) >> 24;
			pr_info("offset=[%d]", input.offset);
		}
	}
	update_thermal_data_mt6877(&input);
	return 0;
}
#endif /* ndef(CONFIG_FPGA_EARLY_PORTING) */

int connsys_a_die_cfg_mt6877(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else /* CONFIG_FPGA_EARLY_PORTING */
	bool adie_26m = true;
	unsigned int adie_id = 0;
	int check;
	unsigned int efuse0, efuse1, efuse2, efuse3;
	bool efuse_valid;

	if (consys_co_clock_type_mt6877() == CONNSYS_CLOCK_SCHEMATIC_52M_COTMS) {
		pr_info("A-die clock 52M\n");
		adie_26m = false;
	}

	/* if(A-die XTAL = 26MHz ) {
	 * CONN_WF_CTRL2 switch to GPIO mode, GPIO output value
	 * before patch download switch back to CONN mode.
	 * }
	 */
	if (adie_26m) {
		/* 0x1000_5064=32'h0000_1000
		 * 0x1000_5164=32'h0000_1000
		 * 0x1000_5490[18:16]=3'b000"
		 */
		CONSYS_REG_WRITE(GPIO_DIR6_SET, 0x00001000);
		CONSYS_REG_WRITE(GPIO_DOUT6_SET, 0x00001000);
		CONSYS_REG_WRITE_MASK(GPIO_MODE25, 0x0, 0x70000);
	}

	/* Enable A-die top_ck_en_0 */
	connsys_adie_top_ck_en_ctl_mt6877(true);
	/* sub-task: a_die_cfg */
	/* De-assert A-die reset
	 * CONN_INFRA_CFG_ADIE_CTL_ADIE_RSTB
	 * 0x18001030[0]=1'b1
	 */
	CONSYS_SET_BIT(CONN_CFG_ADIE_CTL_ADDR, (0x1 << 0));

	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		connsys_adie_top_ck_en_ctl_mt6877(false);
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	/* Read MT6635 ID
	 * ATOP CHIP_ID
	 * 0x02C[31:16]: hw_code
	 * 0x02C[15:0]: hw_ver
	 * Action: TOPSPI_RD
	 * Note:
	 * 	MT6635 E1: 0x66358A00
	 * 	MT6635 E2: 0x66358A10
	 * 	MT6635 E3: 0x66358A11
	 */
	check = consys_spi_read_nolock(SYS_SPI_TOP, ATOP_CHIP_ID, &adie_id);
	if (check || (adie_id & 0xffff0000) != 0x66350000) {
		pr_err("[%s] get a-die fail, ret=%d adie_id=0x%x\n", check, adie_id);
		consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
		return -1;
	}
	pr_info("[%s] A-die chip id: 0x%08x\n", __func__, adie_id);

	conn_hw_env.adie_hw_version = adie_id;
	/* Write to conninfra sysram */
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID, adie_id);

	/* Patch to FW from 7761(WF0_WRI_SX_CAL_MAP/WF1_WRI_SX_CAL_MAP)
	 * ATOP WRI_CTR2 0x064=32'h00007700
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_WRI_CTR2, 0x00007700);

	/* Set WF low power cmd as DBDC mode & legacy interface
	 * ATOP SMCTK11 0x0BC=32'h00000021
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_SMCTK11, 0x00000021);

	/* Update spi fm read extra bit setting
	 * CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_EN
	 * 	0x1800400C[15]=1'b0
	 * CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_CNT
	 * 	0x1800400C[7:0]=8'h0
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_EN, 0x0);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_FM_CTRL_FM_RD_EXT_CNT, 0x0);

	/* Update Thermal addr for 6635
	 * CONN_TOP_THERM_CTL_THERM_AADDR
	 * 	0x18002018=32'h50305A00
	 */
	CONSYS_REG_WRITE(CONN_THERM_CTL_THERM_AADDR_ADDR, 0x50305A00);
	/* Read efuse Data(000)
	 * 	efuse_valid = a_die_efuse_read(000)
	 * Thermal Cal (TOP)
	 * 	a_die_thermal_cal(efuse_valid)
	 */
	efuse_valid = connsys_a_die_efuse_read_nolock(0, &efuse0, &efuse1, &efuse2, &efuse3);
	connsys_a_die_thermal_cal_nolock(efuse_valid, efuse0, efuse1, efuse2, efuse3);

	/* Set WF_PAD to HighZ
	 * ATOP RG_ENCAL_WBTAC_IF_SW(0x070) = 32'h80000000
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_ENCAL_WBTAC_IF_SW, 0x80000000);

	/* Disable CAL LDO
	 * ATOP RG_WF0_TOP_01(0X380)=32'h000E8002
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_WF0_TOP_01, 0x000E8002);

	/* Disable CAL LDO
	 * ATOP RG_WF1_TOP_01(0x390)=32'h000E8002
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_WF1_TOP_01, 0x000E8002);

	/* Increase XOBUF supply-V
	 * ATOP RG_TOP_XTAL_01(0xA18)=32'hF6E8FFF5
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_TOP_XTAL_01, 0xF6E8FFF5);

	/* Increase XOBUF supply-R for MT6635 E1
	 * ATOP RG_TOP_XTAL_02(0xA1C)
	 * if (MT6635 E1) //rf_hw_ver = 0x8a00
	 * 	32'hD5555FFF
	 * else
	 * 	32'h0x55555FFF
	 */
	if (adie_id == 0x66358A00)
		consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_TOP_XTAL_02, 0xD5555FFF);
	else
		consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_TOP_XTAL_02, 0x55555FFF);

	/* Initial IR value for WF0 THADC
	 * 	ATOP RG_WF0_BG(0x384)=0x00002008
	 * Initial IR value for WF1 THADC
	 * 	ATOP RG_WF1_BG(0x394)=0x00002008
	 */
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_WF0_BG, 0x00002008);
	consys_spi_write_nolock(SYS_SPI_TOP, ATOP_RG_WF1_BG, 0x00002008);
	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);

	/* if(A-die XTAL = 26MHz ) {
	 * 	CONN_WF_CTRL2 switch to CONN mode
	 * }
	 */
	if (adie_26m) {
		/* 0x1000_5490[18:16]=3'b001 */
		CONSYS_REG_WRITE_MASK(GPIO_MODE25, 0x10000, 0x70000);
	}

	conn_hw_env.is_rc_mode = consys_is_rc_mode_enable_mt6877();
#endif /* CONFIG_FPGA_EARLY_PORTING */
	return 0;
}

int connsys_afe_wbg_cal_mt6877(void)
{
	/* AFE WBG CAL SEQ1 (RC calibration)
	 * 1. AFE WBG RC calibration, set "AFE RG_WBG_EN_RCK" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_RCK
	 * 	0x18003000[0]=1'b1
	 * 2. AFE WBG RC calibration, delay 60us
	 * 	60us	wait
	 * 3. AFE WBG RC calibration, set "AFE RG_WBG_EN_RCK" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_RCK	0x18003000[0]=1'b0
	 */
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 0));
	udelay(60);
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 0));

	/* AFE WBG CAL SEQ2 (TX calibration)
	 * 0.Before DACK, downgrade VCM to 0.6V
	 * 	update RG TX value for BT0
	 * 		CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER	0x18003058[25:22]=4'b0000
	 * 	update RG TX value for WF0
	 * 		CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER	0x18003078[25:22]=4'b0000
	 * 	update RG TX value for WF1
	 * 		CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER	0x18003094[25:22]=4'b0000
	 * 1. AFE WBG TX calibration, set "AFE RG_WBG_EN_BPLL_UP" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_UP	0x18003008[21]=1'b1
	 * 2. AFE WBG TX calibration, delay 30us
	 * 3. AFE WBG TX calibration, set "AFE RG_WBG_EN_WPLL_UP" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_UP	0x18003008[20]=1'b1
	 * 4. AFE WBG TX calibration, delay 60us
	 * 5. AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_BT" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_BT	0x18003000[21]=1'b1
	 * 6. AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_WF0" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF0	0x18003000[20]=1'b1
	 * 7. AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_WF1" = 1
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF1	0x18003000[19]=1'b1
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER, 0x0);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER, 0x0);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER, 0x0);
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_03_ADDR, (0x1 << 21));
	udelay(30);
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_03_ADDR, (0x1 << 20));
	udelay(60);
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 21));
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 20));
	CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 19));
	/* AFE WBG TX calibration, delay 800us */
	udelay(800);

	/* AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_BT" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_BT	0x18003000[21]=1'b0
	 * AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_WF0" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF0	0x18003000[20]=1'b0
	 * AFE WBG TX calibration, set "AFE RG_WBG_EN_TXCAL_WF1" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_01_RG_WBG_EN_TXCAL_WF1	0x18003000[19]=1'b0
	 * AFE WBG TX calibration, set "AFE RG_WBG_EN_BPLL_UP" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_BPLL_UP	0x18003008[21]=1'b0
	 * AFE WBG TX calibration, set "AFE RG_WBG_EN_WPLL_UP" = 0
	 * 	CONN_AFE_CTL_RG_DIG_EN_03_RG_WBG_EN_WPLL_UP	0x18003008[20]=1'b0
	 */
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 21));
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 20));
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_01_ADDR, (0x1 << 19));
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_03_ADDR, (0x1 << 21));
	CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_03_ADDR, (0x1 << 20));

	/* After DACK, set VCM back to 0.65V (default)
	 * update RG TX value for BT0
	 * 	CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER	0x18003058[25:22]=4'b0100
	 * update RG TX value for WF0
	 * 	CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER	0x18003078[25:22]=4'b0100
	 * update RG TX value for WF1
	 * 	CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER	0x18003094[25:22]=4'b0100
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_BT0_TX_03_RG_BT0_BBTX_VCM_VGER, 0x4);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_WF0_TX_03_RG_W0_BBTX_VCM_VGER, 0x4);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_WBG_WF1_TX_03_RG_W1_BBTX_VCM_VGER, 0x4);


	/* Initial BT path if WF is in cal(need set this CR after WBG cal)
	 * 	ATOP RG_ENCAL_WBTAC_IF_SW	0x070=32'h00000005
	 */
	consys_spi_write_mt6877(SYS_SPI_TOP, ATOP_RG_ENCAL_WBTAC_IF_SW, 0x00000005);
	return 0;
}

int connsys_subsys_pll_initial_mt6877(void)
{
	/* Set BPLL stable time = 40us (value = 40 * 1000 *1.01 / 38.46ns)
	 * 	CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_BPLL_STB_TIME(0x180030F4[30:16])=0x314
	 * Sset WPLL stable time = 50us (value = 50 * 1000 *1.01 / 38.46ns)
	 * 	CONN_AFE_CTL_RG_PLL_STB_TIME_RG_WBG_WPLL_STB_TIME(0x180030F4[14:0])0x521
	 */
	CONSYS_REG_WRITE_MASK(CONN_AFE_CTL_RG_PLL_STB_TIME_ADDR, 0x03140521, 0x7FFF7FFF);

	/* BT pll_en will turn on BPLL only (may change in different XTAL option)
	 * 	CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL(0x18003004[7:6]=0x1
	 * WF pll_en will turn on WPLL only (may change in different XTAL option)
	 * 	CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL(0x18003004[1:0])=0x3
	 * MCU pll_en will turn on BPLL (may change in different XTAL option)
	 * 	CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF(0x18003004[3:2])=0x1
	 * MCU pll_en will turn on BPLL + WPLL (may change in different XTAL option)
	 * 	CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF(0x18003004[17:16])=0x3
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_BT_PLL, 0x1);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_WF_PLL, 0x3);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_BGF, 0x1);
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_DIG_EN_02_RG_WBG_EN_MCU_PLL_WF, 0x3);

	/* CONN_INFRA CLKGEN WPLL AND BPLL REQUEST
	 * CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2(0x1800300C[18:15])=0xD
	 */
	CONSYS_REG_WRITE_HW_ENTRY(CONN_AFE_CTL_RG_DIG_TOP_01_RG_DIG_RSV2, 0xd);

	return 0;
}

int connsys_low_power_setting_mt6877(unsigned int curr_status, unsigned int next_status)
{
	bool bt_only = false;
	void __iomem *addr = NULL;
	unsigned int curr_wifi = 0;
	unsigned int next_wifi = 0;
	unsigned int wifi_on = -1;

	if ((next_status & (~(0x1 << CONNDRV_TYPE_BT))) == 0)
		bt_only = true;

	connsys_adie_clock_buffer_setting(bt_only);

	curr_wifi = (curr_status & (0x1 << CONNDRV_TYPE_WIFI)) >> CONNDRV_TYPE_WIFI;
	next_wifi = (next_status & (0x1 << CONNDRV_TYPE_WIFI)) >> CONNDRV_TYPE_WIFI;
	wifi_on = (curr_wifi != next_wifi) ? next_wifi : -1;

	if (curr_status == 0) {
		/* 1st On */
		/* Unmask on2off/off2on slpprot_rdy enable checker @conn_infra off power off =>
		 * check slpprot_rdy = 1'b1 and go to sleep
		 * CONN_INFRA_CFG_PWRCTRL0_CONN_INFRA_CFG_SLP_RDY_MASK(0x18001200[15:12])=4'h1
		 */
		CONSYS_REG_WRITE_MASK(CONN_CFG_CONN_INFRA_CFG_PWRCTRL0_ADDR, 0x1000, 0xf000);

		/* RC mode check start */
		if (!consys_is_rc_mode_enable_mt6877()) {
			/* Legacy mode */
			/* Disable conn_top rc osc_ctrl_top
			 * CONN_INFRA_CFG_RC_CTL_0_CONN_INFRA_OSC_RC_EN(0x18001380[7])=1'b0
			 */
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 7));
			/* Legacy OSC control stable time
			 * 0x18001300[7:0] = 8'd6
			 * 0x18001300[15:8] = 8'd7
			 * 0x18001300[23:16] = 8'd8
			 */
			CONSYS_REG_WRITE_MASK(CONN_CFG_OSC_CTL_0_ADDR, 0x00080706, 0x00ffffff);
			/* Legacy OSC control unmask conn_srcclkena_ack
			 * CONN_INFRA_CFG_OSC_CTL_1_ACK_FOR_XO_STATE_MASK(0x18001304[16])=1'b0
			 */
			CONSYS_CLR_BIT(CONN_CFG_OSC_CTL_1_ADDR, (0x1 << 16));
		} else {
			/* RC mode */
			/* GPS RC OSC control stable time
			 * 	0x18001394[7:0]   = 8'd6
			 * 	0x18001394[15:8]  = 8'd7
			 * 	0x18001394[23:16] = 8'd8
			 * 	0x18001394[31:24] = 8'd2
			 * GPS RC OSC control unmask conn_srcclkena_ack
			 * 	0x18001390[15] = 1'b0
			 *
			 * BT RC OSC control stable time
			 * 	0x180013A4[7:0]   = 8'd6
			 * 	0x180013A4[15:8]  = 8'd7
			 * 	0x180013A4[23:16] = 8'd8
			 * 	0x180013A4[31:24] = 8'd2
			 * BT RC OSC control unmask conn_srcclkena_ack
			 * 	0x180013A0[15] = 1'b0
			 *
			 * WF RC OSC control stable time
			 * 	0x180013B4[7:0]   = 8'd6
			 * 	0x180013B4[15:8]  = 8'd7
			 * 	0x180013B4[23:16] = 8'd8
			 * 	0x180013B4[31:24] = 8'd2
			 * WF RC OSC control unmask conn_srcclkena_ack
			 * 	0x180013B0[15] = 1'b0
			 *
			 * TOP RC OSC control stable time
			 * 	0x180013C4[7:0]   = 8'd6
			 * 	0x180013C4[15:8]  = 8'd7
			 * 	0x180013C4[23:16] = 8'd8
			 * 	0x180013C4[31:24] = 8'd2
			 * TOP RC OSC control unmask conn_srcclkena_ack
			 * 	0x180013C0[15] = 1'b0
			 *
			 * Enable conn_infra rc osc_ctl_top output
			 * 	0x18001388[3] = 1'b1
			 */
			CONSYS_REG_WRITE(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_GPS_ADDR, 0x02080706);
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_GPS_ADDR, (0x1 << 15));

			CONSYS_REG_WRITE(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_BT_ADDR, 0x02080706);
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_BT_ADDR, (0x1 << 15));

			CONSYS_REG_WRITE(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_WF_ADDR, 0x02080706);
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_WF_ADDR, (0x1 << 15));

			CONSYS_REG_WRITE(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_TOP_ADDR, 0x02080706);
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_TOP_ADDR, (0x1 << 15));

			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR, (0x1 << 3));

			/* Wait 1 T 32K (30us) */
			udelay(30);

			/* Enable conn_top rc osc_ctrl_gps output
			 * 	0x18001388[0] = 1'b1
			 * Enable conn_top rc osc_ctrl_bt output
			 * 	0x18001388[1] = 1'b1
			 * Enable conn_top rc osc_ctrl_wf output
			 * 	0x18001388[2] = 1'b1
			 * Enable conn_top rc osc_ctrl_gps
			 * 	0x18001380[4] = 1'b1
			 * Enable conn_top rc osc_ctrl_bt
			 * 	0x18001380[5] = 1'b1
			 * Enable conn_top rc osc_ctrl_wf
			 * 	0x18001380[6] = 1'b1
			 * Set conn_srcclkena control by conn_infra_emi_ctl
			 * 	0x18001400[0] = 1'b1
			 * Disable legacy osc control output
			 * 	0x18001380[31] = 1'b0
			 * Disable legacy osc control
			 * 	0x18001380[0] = 1'b0
			 * Legacy OSC control stable time
			 * 	CONN_INFRA_CFG_OSC_CTL_0_XO_VCORE_RDY_STABLE_TIME
			 * 		0x18001300[7:0]=8'd6
			 * 	CONN_INFRA_CFG_OSC_CTL_0_XO_INI_STABLE_TIME
			 * 		0x18001300[15:8]=8'd7
			 * 	CONN_INFRA_CFG_OSC_CTL_0_XO_BG_STABLE_TIME
			 * 		0x18001300[23:16]=8'd8
			 * Legacy OSC control unmask conn_srcclkena_ack
			 * 	CONN_INFRA_CFG_OSC_CTL_1_ACK_FOR_XO_STATE_MASK
			 * 		0x18001304[16]=1'b0
			 * Enable osc_rc_en_bk
			 * 	0x18001380[30] = 1'b1
			 * Unmask osc_en for osc_en_rc
			 * 	0x18001388[7:4] = 4'b1101
			 * Enable conn_emi_bt_only_rc_en => conn_srcclkena = conn_srcclkena_cfg || conn_srcclkena_emi
			 * 	0x18001400[16] = 1'b1
			 */
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR, (0x1 << 0));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR, (0x1 << 1));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR, (0x1 << 2));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 4));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 5));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 6));
			CONSYS_SET_BIT(CONN_CFG_EMI_CTL_0_ADDR, (0x1 << 0));
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 31));
			CONSYS_CLR_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 0));
			CONSYS_REG_WRITE_MASK(CONN_CFG_OSC_CTL_0_ADDR, 0x00080706, 0x00ffffff);
			CONSYS_CLR_BIT(CONN_CFG_OSC_CTL_1_ADDR, (0x1 << 16));
			CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_RC_CTL_0_ADDR, (0x1 << 30));
			CONSYS_REG_WRITE_MASK(CONN_CFG_CONN_INFRA_CFG_RC_CTL_1_ADDR, 0xd0, 0xf0);
			CONSYS_SET_BIT(CONN_CFG_EMI_CTL_0_ADDR, (0x1 << 16));
			CONSYS_SET_BIT(CONN_CFG_EMI_CTL_1_ADDR, (0x1 << 1));
		}
		/* RC mode check end */
		/* Prevent subsys from power on/off in a short time interval
		 * 	CONN_INFRA_RGU_BGFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ACK_S_MASK
		 * 		0x18000020[7] = 1'b0 (with key wdata[31:15] = 0x4254)
		 * 	CONN_INFRA_RGU_WFFYS_ON_TOP_PWR_CTL_BGFSYS_ON_TOP_PWR_ACK_S_MASK
		 * 		0x18000010[7] = 1'b0 (with key wdata[31:15] = 0x5746)
		 */
		CONSYS_REG_WRITE_MASK(CONN_RGU_BGFYS_ON_TOP_PWR_CTL_ADDR, 0x42540000, 0xffff0040);
		CONSYS_REG_WRITE_MASK(CONN_RGU_WFSYS_ON_TOP_PWR_CTL_ADDR, 0x57460000, 0xffff0040);
		/* conn2ap sleep protect release bypass ddr_en_ack check
		 * CONN_INFRA_CFG_EMI_CTL_0_EMI_SLPPROT_BP_DDR_EN(0x18001400[18]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CFG_EMI_CTL_0_ADDR, (0x1 << 18));
		/* Enable ddr_en timeout, timeout value = 1023 T (Bus clock)
		 * CONN_INFRA_CFG_EMI_CTL_0_DDR_CNT_LIMIT(0x18001400[14:4]) = 11'd1023
		 */
		CONSYS_REG_WRITE_HW_ENTRY(CONN_CFG_EMI_CTL_0_DDR_CNT_LIMIT, 1023);
		/* Update ddr_en timeout value enable
		 * CONN_INFRA_CFG_EMI_CTL_0_DDR_EN_CNT_UPDATE(0x18001400[15]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CFG_EMI_CTL_0_ADDR, (0x1 << 15));
		/* Update ddr_en timeout value disable
		 * CONN_INFRA_CFG_EMI_CTL_0_DDR_EN_CNT_UPDATE(0x18001400[15]) = 1'b0
		 */
		CONSYS_CLR_BIT(CONN_CFG_EMI_CTL_0_ADDR, (0x1 << 15));
		/* Enable conn_infra bus dcm
		 * 	0x1802_E03C[23:19]=5'h01
		 * 	0x1802_E040[9:5]=5'h01
		 * 	0x1802_E03C[1:0]=2'b11
		 * 	0x1802_E03C[4:3]=2'b11
		 */
		CONSYS_REG_WRITE_MASK(CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_0_ADDR, 0x80000, 0xf80000);
		CONSYS_REG_WRITE_MASK(CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_1_ADDR, 0x20, 0x3e0);
		CONSYS_REG_WRITE_MASK(CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_0_ADDR, 0x3, 0x3);
		CONSYS_REG_WRITE_MASK(CONN_BCRM_ON_VDNR_DCM_TOP_CTRL_0_ADDR, 0x18, 0x18);
		/* Bus access protector
		 * CONN_INFRA_LIGHT_SECURITY_CTRL(0x1800_E238[4][3][1][0]) = 4'1111
		 */
		CONSYS_SET_BIT(CONN_BUS_CR_LIGHT_SECURITY_CTRL_ADDR, (0x1 << 0));
		CONSYS_SET_BIT(CONN_BUS_CR_LIGHT_SECURITY_CTRL_ADDR, (0x1 << 1));
		CONSYS_SET_BIT(CONN_BUS_CR_LIGHT_SECURITY_CTRL_ADDR, (0x1 << 3));
		CONSYS_SET_BIT(CONN_BUS_CR_LIGHT_SECURITY_CTRL_ADDR, (0x1 << 4));

		/* Set conn_infra_off bus timeout
		 * CONN_INFRA_OFF_TIMEOUT_LIMIT(0x1800_E300[14:7]) = 8'h12
		 */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_BUS_CR_CONN_INFRA_BUS_OFF_TIMEOUT_CTRL_CONN_INFRA_OFF_TIMEOUT_LIMIT,
			0x12);

		/* Enable conn_infra off bus  timeout feature
		 * CONN_INFRA_OFF_TIMEOUT_EN
		 * CONN_INFRA_OFF_AHB_MUX_EN
		 * CONN_INFRA_OFF_APB_MUX_EN
		 * 	0x1800_E300[2:0] = 3'b111
		 */
		CONSYS_REG_WRITE_MASK(CONN_BUS_CR_CONN_INFRA_BUS_OFF_TIMEOUT_CTRL_ADDR, 0x7, 0x7);

		/* Set conn_infra_on bus timeout
		 * CONN_INFRA_ON_TIMEOUT_LIMIT(0x1800_E31C[14:7]) = 8'h2
		 */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_BUS_CR_CONN_INFRA_BUS_ON_TIMEOUT_CTRL_CONN_INFRA_ON_TIMEOUT_LIMIT,
			0x2);

		/* Enable conn_infra_on bus  timeout feature
		 * CONN_INFRA_ON_TIMEOUT_EN
		 * CONN_INFRA_ON_AHB_MUX_EN
		 * CONN_INFRA_ON_APB_MUX_EN
		 * 	0x1800_E31C[3:0] = 4'hf
		 */
		CONSYS_REG_WRITE_MASK(
			CONN_BUS_CR_CONN_INFRA_BUS_ON_TIMEOUT_CTRL_ADDR, 0x7, 0x7);

		/* Enable debug control bus timeout feature
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[9]=1'b1
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[31:16]=16'h7f4
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[2]=1'b1
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[3]=1'b1
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[4]=1'b1
		 * CONN_INFRA_VDNR_GEN_ON_DEBUG_CTRL_AO_CONN_INFRA_VDNR_GEN_ON_U_DEBUG_CTRL_AO_CONN_INFRA_ON_CTRL0
		 * 	0x1800_F000[9]=1'b0
		 */
		addr = ioremap(0x1800f000, 0x10);
		if (addr) {
			CONSYS_SET_BIT(addr, (0x1 << 9));
			CONSYS_REG_WRITE_MASK(addr, 0x07f40000, 0xffff0000);
			CONSYS_SET_BIT(addr, (0x1 << 2));
			CONSYS_SET_BIT(addr, (0x1 << 3));
			CONSYS_SET_BIT(addr, (0x1 << 4));
			CONSYS_CLR_BIT(addr, (0x1 << 9));
		} else {
			pr_info("[%s] remap 0x1800f000 error", __func__);
			return -1;
		}
		iounmap(addr);

		/* The config is moved to WIFI driver and WIFI FW.
		 * 0x1801_d000 is on conninfra off domain. The config could not be kept after
		 * conninfra went to sleep.
		 */
#if 0
		/* Enable debug control bus timeout feature (axi layer)
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[9]=1'b1
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[31:16]=16'h30E0
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[2]=1'b1
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[3]=1'b1
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[4]=1'b1
		 * CONN_INFRA_VDNR_AXI_LAYER_DEBUG_CTRL_AO_CONN_INFRA_VDNR_AXI_LAYER_U_DEBUG_CTRL_AO_CONN_INFRA_CTRL0
		 * 	0x1801_D000[9]=1'b0
		 */
		addr = ioremap(0x1801d000, 0x10);
		if (addr) {
			CONSYS_SET_BIT(addr, (0x1 << 9));
			CONSYS_REG_WRITE_MASK(addr, 0x30e00000, 0xffff0000);
			CONSYS_SET_BIT(addr, (0x1 << 2));
			CONSYS_SET_BIT(addr, (0x1 << 3));
			CONSYS_SET_BIT(addr, (0x1 << 4));
			CONSYS_CLR_BIT(addr, (0x1 << 9));
		} else {
			pr_info("[%s] remap 0x1801d000 error", __func__);
			return -1;
		}
		iounmap(addr);
#endif

		/* De-assert CONNSYS S/W reset (TOP RGU CR), set "ap_sw_rst_b"=1
		 * WDT_SWSYSRST[12] = 1'b0
		 * WDT_SWSYSRST[31:24] =8'h88 (key)
		 * 	0x1000_7200[31:24] = 8'h88 (key)
		 * 	0x1000_7200[12]    = 1'b0
		 */
		CONSYS_REG_WRITE_MASK(TOPRGU_WDT_SWSYSRST, 0x88000000, 0xff001000);

		/* Enable conn_infra bus bpll div_1
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_BUS_BPLL_DIV_1_BPLL_DIV_1_DIV_EN(0x1800_9000[0]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_BPLL_DIV_1_ADDR, (0x1 << 0));
		/* Enable conn_infra bus bpll div_2
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_BUS_BPLL_DIV_2_BPLL_DIV_2_DIV_EN(0x1800_9004[0]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_BPLL_DIV_2_ADDR, (0x1 << 0));
		/* Enable conn_infra bus wpll div_1
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_BUS_WPLL_DIV_1_WPLL_DIV_1_DIV_EN(0x1800_9008[0]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_WPLL_DIV_1_ADDR, (0x1 << 0));
		/* Enable conn_infra bus wpll div_2
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_BUS_WPLL_DIV_2_WPLL_DIV_2_DIV_EN(0x1800_900C[0]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_WPLL_DIV_2_ADDR, (0x1 << 0));

		/* Set rfspi pll to bpll div 13
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_BPLL_SEL
		 * 0x1800_9048[7]=1'b0
		 */
		CONSYS_CLR_BIT(CONN_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR, (0x1 << 7));
		/* Set rfsp bpll div 13 en
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_UCLK_PLL_SEL
		 * 0x1800_9048[6]=1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR, (0x1 << 6));
		/* Set rfspi pll to bpll
		 * CONN_INFRA_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_BPLL_DIV13_EN
		 * 0x1800_9048[4]=1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CLKGEN_RFSPI_CK_CTRL_ADDR, (0x1 << 4));

		/* Set conn_infra slee count to host side control
		 * CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_CTL_EN
		 * 0x1806_0380[31]=1'b1
		 */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_CTL_EN,
			0x1);

		/* Set conn_infra slee count enable (host side)
		 * CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_EN
		 * 0x1806_0380[0]=1'b1
		 */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_EN,
			0x1);

		/* Enable coex clock
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_ANT_PINMUX_OSC_CKEN		0x1800_9a00[0]=1'b1
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_CO_EXT_FDD_COEX_HCLKCKEN	0x1800_9a00[1]=1'b1
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_CO_EXT_PTA_HCLK_CKEN		0x1800_9a00[2]=1'b1
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_CO_EXT_PTA_OSC_CKEN		0x1800_9a00[3]=1'b1
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_CO_EXT_UART_PTA_HCLK_CKEN	0x1800_9a00[4]=1'b1
		 * CONN_INFRA_CLKGEN_ON_TOP_CKGEN_BUS_CONN_CO_EXT_UART_PTA_OSC_CKEN	0x1800_9a00[5]=1'b1
		 */
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 0));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 1));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 2));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 3));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 4));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 5));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 25));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 26));
		CONSYS_SET_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 27));

		/* 0x1800_1200[9]=1b'1
		 */
		CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_PWRCTRL0_ADDR, (0x1 << 9));

		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		/* !!!!!!!!!!!!!!!!!!!!!! CANNOT add code after HERE!!!!!!!!!!!!!!!!!!!!!!!!!! */
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

		/* Disable conn_infra bus clock sw control  ==> conn_infra bus clock hw control
		 * CONN_INFRA_CFG_CKGEN_BUS_HCLK_CKSEL_SWCTL(0x1800_9A00[23]) = 1'b0
		 */
		CONSYS_CLR_BIT(CONN_CLKGEN_ON_TOP_CKGEN_BUS_ADDR, (0x1 << 23));

		/* Conn_infra HW_CONTROL => conn_infra enter dsleep mode
		 * CONN_INFRA_CFG_PWRCTRL0_HW_CONTROL(0x1800_1200[0]) = 1'b1
		 */
		CONSYS_SET_BIT(CONN_CFG_CONN_INFRA_CFG_PWRCTRL0_ADDR, (0x1 << 0));

	} else {
		/* Not first on */
		if (wifi_on == 0) { // wifi turn off
			// Debug use for vcn13 oc
			pr_info("Debug use: wifi power off, dump a-die status\n");
			pmic_mng_event_cb(0, 7);
		}
	}
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* !!!!!!!!!!!!!!!!!!!!!! CANNOT add code after HERE!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

	return 0;
}

static int consys_sema_acquire(unsigned int index)
{
	if (index >= CONN_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;

	if (CONSYS_REG_READ_BIT(
		(CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_STA_ADDR + index*4), 0x1) == 0x1) {
		return CONN_SEMA_GET_SUCCESS;
	} else {
		return CONN_SEMA_GET_FAIL;
	}
}

int consys_sema_acquire_timeout_mt6877(unsigned int index, unsigned int usec)
{
	int i;
	unsigned long flags = 0;

	if (index >= CONN_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;
	for (i = 0; i < usec; i++) {
		if (consys_sema_acquire(index) == CONN_SEMA_GET_SUCCESS) {
			sema_get_time[index] = jiffies;
			if (index == CONN_SEMA_RFSPI_INDEX)
				local_irq_save(flags);
			return CONN_SEMA_GET_SUCCESS;
		}
		udelay(1);
	}

	pr_err("Get semaphore 0x%x timeout, dump status:\n", index);
	pr_err("M0:[0x%x] M1:[0x%x] M2:[0x%x] M3:[0x%x]\n",
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M1_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M2_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M3_STA_REP_1_ADDR));
	/* Debug feature in Android, remove it in CTP */
#if 0
	consys_reg_mng_dump_cpupcr(CONN_DUMP_CPUPCR_TYPE_ALL, 10, 200);
#endif
	return CONN_SEMA_GET_FAIL;
}

void consys_sema_release_mt6877(unsigned int index)
{
	u64 duration;
	unsigned long flags = 0;

	if (index >= CONN_SEMA_NUM_MAX)
		return;
	CONSYS_REG_WRITE(
		(CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_REL_ADDR + index*4), 0x1);

	duration = jiffies_to_msecs(jiffies - sema_get_time[index]);
	if (index == CONN_SEMA_RFSPI_INDEX)
		local_irq_restore(flags);
	if (duration > SEMA_HOLD_TIME_THRESHOLD)
		pr_notice("%s hold semaphore (%d) for %llu ms\n", __func__, index, duration);
}

struct spi_op {
	unsigned int busy_cr;
	unsigned int polling_bit;
	unsigned int addr_cr;
	unsigned int read_addr_format;
	unsigned int write_addr_format;
	unsigned int write_data_cr;
	unsigned int read_data_cr;
	unsigned int read_data_mask;
};

static const struct spi_op spi_op_array[SYS_SPI_MAX] = {
	/* SYS_SPI_WF1 */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 1,
		CONN_RF_SPI_MST_REG_SPI_WF_ADDR_OFFSET, 0x00001000, 0x00000000,
		CONN_RF_SPI_MST_REG_SPI_WF_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_WF_RDAT_OFFSET, 0xffffffff
	},
	/* SYS_SPI_WF0 */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 1,
		CONN_RF_SPI_MST_REG_SPI_WF_ADDR_OFFSET, 0x00003000, 0x00002000,
		CONN_RF_SPI_MST_REG_SPI_WF_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_WF_RDAT_OFFSET, 0xffffffff
	},
	/* SYS_SPI_BT */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 2,
		CONN_RF_SPI_MST_REG_SPI_BT_ADDR_OFFSET, 0x00005000, 0x00004000,
		CONN_RF_SPI_MST_REG_SPI_BT_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_BT_RDAT_OFFSET, 0x000000ff
	},
	/* SYS_SPI_FM */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 3,
		CONN_RF_SPI_MST_REG_SPI_FM_ADDR_OFFSET, 0x00007000, 0x00006000,
		CONN_RF_SPI_MST_REG_SPI_FM_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_FM_RDAT_OFFSET, 0x0000ffff
	},
	/* SYS_SPI_GPS */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 4,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_OFFSET, 0x00009000, 0x00008000,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_OFFSET, 0x0000ffff
	},
	/* SYS_SPI_TOP */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 5,
		CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_OFFSET, 0x0000b000, 0x0000a000,
		CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_OFFSET, 0xffffffff
	},
	/* SYS_SPI_WF2 */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 1,
		CONN_RF_SPI_MST_REG_SPI_WF_ADDR_OFFSET, 0x0000d000, 0x0000c000,
		CONN_RF_SPI_MST_REG_SPI_WF_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_WF_RDAT_OFFSET, 0xffffffff
	},
	/* SYS_SPI_WF3 */
	{
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 1,
		CONN_RF_SPI_MST_REG_SPI_WF_ADDR_OFFSET, 0x0000f000, 0x0000e000,
		CONN_RF_SPI_MST_REG_SPI_WF_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_WF_RDAT_OFFSET, 0xffffffff
	},
};

static int consys_spi_read_nolock(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	/* Read action:
	 * 1. Polling busy_cr[polling_bit] should be 0
	 * 2. Write addr_cr with data being {read_addr_format | addr[11:0]}
	 * 3. Trigger SPI by writing write_data_cr as 0
	 * 4. Polling busy_cr[polling_bit] as 0
	 * 5. Read data_cr[data_mask]
	 */
	int check = 0;
	const struct spi_op* op = &spi_op_array[subsystem];

	if (!data) {
		pr_err("[%s] invalid data ptr\n", __func__);
		return CONNINFRA_SPI_OP_FAIL;
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
	CONSYS_REG_BIT_POLLING(
		CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr, op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08x bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem), CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif

	CONSYS_REG_WRITE(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->addr_cr, (op->read_addr_format | addr));

	CONSYS_REG_WRITE(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->write_data_cr, 0);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(
		CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr, op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%d][STEP4] polling 0x%08x bit %d fail. Value=0x%08x\n",
			__func__, subsystem, CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	check = CONSYS_REG_READ_BIT(
		CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->read_data_cr, op->read_data_mask);
	*data = check;

	return 0;
}

int consys_spi_read_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock(subsystem, addr, data);

	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

static int consys_spi_write_nolock(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	int check = 0;
#endif
	const struct spi_op* op = &spi_op_array[subsystem];

	/* Write action:
	 * 1. Wait busy_cr[polling_bit] as 0
	 * 2. Write addr_cr with data being {write_addr_format | addr[11:0]
	 * 3. Write write_data_cr ad data
	 * 4. Wait busy_cr[polling_bit] as 0
	 */
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (subsystem == SYS_SPI_TOP && addr == 0x380 && data == 0) {
		pr_info("check who writes 0x380 to 0\n");
		BUG_ON(1);
		return CONNINFRA_SPI_OP_FAIL;
	}

	CONSYS_REG_BIT_POLLING(
		CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr,
		op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08x bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem), CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	CONSYS_REG_WRITE(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->addr_cr, (op->write_addr_format | addr));

	CONSYS_REG_WRITE(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->write_data_cr, data);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr, op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP4] polling 0x%08x bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem), CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_REG_CONN_RF_SPI_MST_REG_ADDR + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	return 0;
}

int consys_spi_write_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;

	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_write_nolock(subsystem, addr, data);

	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_update_bits_mt6877(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask)
{
	int ret = 0;
	unsigned int curr_val = 0;
	unsigned int new_val = 0;
	bool change = false;

	/* Get semaphore before updating bits */
	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock(subsystem, addr, &curr_val);

	if (ret) {
		consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
		return CONNINFRA_SPI_OP_FAIL;
	}

	new_val = (curr_val & (~mask)) | (data & mask);
	change = (curr_val != new_val);

	if (change) {
		ret = consys_spi_write_nolock(subsystem, addr, new_val);
	}

	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_spi_clock_switch_mt6877(enum connsys_spi_speed_type type)
{
	int check;
	int ret = 0;

	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI CLOCK SWITCH] Require semaphore fail\n");
		return -1;
	}

	/* Refer: MT6635_FM_POS_Montrose_20210204 SPI-HOPPING
	 */
	/* Enable BPLL */
	/* Polling bpll rdy */
	/* Refer: SPI clock select hs clock or osc clock (FM)
	 * CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN_SHFT 0x18004108[4]=1'b1/1'b0
	 * 	=> Select hs clock
	 * CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL_SHFT 0x18004108[5]=1'b1 (must release)
	 * 	=> Give priority of selecting  hs clock to FM
	 */
	/* CONN_RF_SPI_MST_REG_SPI_CRTL_HS_CAP_NEXT_SHFT
	 * 0x18004004[5]: 1'b1/1'b0
	 */
	if (type == CONNSYS_SPI_SPEED_26M) {
		CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN, 0);
		CONSYS_CLR_BIT(CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR, (0x1 << 5));
		CONSYS_CLR_BIT(CONN_AFE_CTL_RG_DIG_EN_02_ADDR, (0x1 << 22));
	} else if (type == CONNSYS_SPI_SPEED_64M) {
		CONSYS_SET_BIT(CONN_AFE_CTL_RG_DIG_EN_02_ADDR, (0x1 << 22));
		check = 0;
		CONSYS_REG_BIT_POLLING(CONN_CFG_PLL_STATUS_ADDR, 1, 1, 100, 50, check);
		if (check == 0) {
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_HS_EN, 1);
			CONSYS_SET_BIT(CONN_RF_SPI_MST_REG_SPI_CRTL_ADDR, (0x1 << 5));
		} else {
			ret = -1;
			pr_info("[%s] BPLL enable fail: 0x%08x",
				__func__, CONSYS_REG_READ(CONN_CFG_PLL_STATUS_ADDR));
		}
	} else {
		pr_err("[%s] wrong parameter %d\n", __func__, type);
	}

	consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_subsys_status_update_mt6877(bool on, int radio)
{
	if (radio < CONNDRV_TYPE_BT || radio > CONNDRV_TYPE_WIFI) {
		pr_err("[%s] wrong parameter: %d\n", __func__, radio);
		return -1;
	}

	if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[%s] acquire semaphore (%d) timeout\n",
			__func__, CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);
		return -1;
	}

	/* When FM power on, give priority of selecting spi clock */
	if (radio == CONNDRV_TYPE_FM) {
		if (consys_sema_acquire_timeout_mt6877(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_SUCCESS) {
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL, on);
			consys_sema_release_mt6877(CONN_SEMA_RFSPI_INDEX);
		}
	}

	if (on) {
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	} else {
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	}

	consys_sema_release_mt6877(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);
	return 0;
}

bool consys_is_rc_mode_enable_mt6877(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	return false;
#else /* CONFIG_FPGA_EARLY_PORTING */
#ifdef CFG_CONNINFRA_ON_CTP
	bool ret = (bool)CONSYS_REG_READ_BIT(RC_CENTRAL_CFG1, 0x1);
	return ret;
#else /* CFG_CONNINFRA_ON_CTP */
	static bool ever_read = false;
	void __iomem *addr = NULL;
	static bool ret = false;

	/* Base: SRCLEN_RC (0x1000_F800)
	 * Offset: 0x004 CENTRAL_CFG1
	 * 	[0] srclken_rc_en
	 */
	if (!ever_read) {
		addr = ioremap(RC_CENTRAL_CFG1, 0x4);
		if (addr != NULL) {
			ret = (bool)CONSYS_REG_READ_BIT(addr, 0x1);
			iounmap(addr);
			ever_read = true;
		} else
			pr_err("[%s] remap 0x%08x fail\n", __func__, RC_CENTRAL_CFG1);
	}

	return ret;
#endif /* CFG_CONNINFRA_ON_CTP */
#endif /* CONFIG_FPGA_EARLY_PORTING */
}

#ifndef CONFIG_FPGA_EARLY_PORTING
void consys_spi_write_offset_range_nolock(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size)
{
	unsigned int data = 0, data2;
	unsigned int reg_mask;
	int ret;

	value = (value >> value_offset);
	value = GET_BIT_RANGE(value, size, 0);
	value = (value << reg_offset);
	ret = consys_spi_read_nolock(subsystem, addr, &data);
	if (ret) {
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
		return;
	}
	reg_mask = GENMASK(reg_offset + size - 1, reg_offset);
	data2 = data & (~reg_mask);
	data2 = (data2 | value);
	consys_spi_write_nolock(subsystem, addr, data2);
}

const char* get_spi_sys_name(enum sys_spi_subsystem subsystem)
{
	const static char* spi_system_name[SYS_SPI_MAX] = {
		"SYS_SPI_WF1",
		"SYS_SPI_WF",
		"SYS_SPI_BT",
		"SYS_SPI_FM",
		"SYS_SPI_GPS",
		"SYS_SPI_TOP",
		"SYS_SPI_WF2",
		"SYS_SPI_WF3",
	};
	if (subsystem >= SYS_SPI_WF1 && subsystem < SYS_SPI_MAX)
		return spi_system_name[subsystem];
	return "UNKNOWN";
}
#endif
