// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/pinctrl/consumer.h>
#include <linux/irqflags.h>
#include <connectivity_build_in_adapter.h>
#include <linux/pm_runtime.h>

#include "consys_hw.h" /* for semaphore index */
/* platform dependent */
#include "plat_def.h"
/* macro for read/write cr */
#include "consys_reg_util.h"
#include "plat_def.h"
/* cr base address */
#include "mt6983_consys_reg.h"
/* cr offset */
#include "mt6983_consys_reg_offset.h"
/* For function declaration */
#include "mt6983.h"
#include "mt6983_pos.h"
#include "mt6983_pos_gen.h"

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CONSYS_SLEEP_MODE_1	(0x1 << 0)
#define CONSYS_SLEEP_MODE_2	(0x1 << 1)

#define MT6637E1 0x66378A00
#define MT6637E2 0x66378A01

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

#ifndef CONFIG_FPGA_EARLY_PORTING
static const char* get_spi_sys_name(enum sys_spi_subsystem subsystem);
#endif
static int connsys_adie_clock_buffer_setting(unsigned int curr_status, unsigned int next_status);

unsigned int consys_emi_set_remapping_reg_mt6983(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	return consys_emi_set_remapping_reg_mt6983_gen(con_emi_base_addr, md_shared_emi_base_addr,
							gps_emi_base_addr, 16);
}

int consys_conninfra_on_power_ctrl_mt6983(unsigned int enable)
{
	int ret = 0;

#if MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE
	ret = consys_platform_spm_conn_ctrl_mt6983(enable);
#else
	ret = consys_conninfra_on_power_ctrl_mt6983_gen(enable);
#endif /* MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE */
	if (enable)
		consys_update_ap2conn_hclk_mt6983_gen();

	return ret;
}

int consys_conninfra_wakeup_mt6983(void)
{
	return consys_conninfra_wakeup_mt6983_gen();
}

int consys_conninfra_sleep_mt6983(void)
{
	return consys_conninfra_sleep_mt6983_gen();
}

void consys_set_if_pinmux_mt6983(unsigned int enable)
{
#ifndef CFG_CONNINFRA_ON_CTP
	struct pinctrl_state *tcxo_pinctrl_set;
	struct pinctrl_state *tcxo_pinctrl_clr;
	int ret = -1;
#endif
	int clock_type = consys_co_clock_type_mt6983();

	if (enable) {
		consys_set_if_pinmux_mt6983_gen(1);
		/* if(TCXO mode)
		 * 	Set GPIO135 pinmux for TCXO mode (Aux3)(CONN_TCXOENA_REQ)
		 */

		if (clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO ||
			clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
			consys_set_gpio_tcxo_mode_mt6983_gen(1, 1);
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
		consys_set_if_pinmux_mt6983_gen(0);

		if (clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO ||
			clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO) {
	#if defined(CFG_CONNINFRA_ON_CTP)
			consys_set_gpio_tcxo_mode_mt6983_gen(1, 0);
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

int consys_polling_chipid_mt6983(void)
{
	return consys_polling_chipid_mt6983_gen(NULL);
}

int connsys_d_die_cfg_mt6983(void)
{
	unsigned int d_die_efuse = 0;

	/* Reset conninfra sysram */
	consys_init_conninfra_sysram_mt6983_gen();

	/* Read D-die Efuse
	 * AP2CONN_EFUSE_DATA 0x1801_1020
	 * Write to conninfra sysram: CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE(0x1805_3820)
	 */
	connsys_get_d_die_efuse_mt6983_gen(&d_die_efuse);
	CONSYS_REG_WRITE(
		CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE, d_die_efuse);

	connsys_d_die_cfg_mt6983_gen();

#if defined(CONNINFRA_PLAT_BUILD_MODE)
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE, CONNINFRA_PLAT_BUILD_MODE);
	pr_info("[%s] Write CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE to 0x%08x\n",
		__func__, CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE));
#endif

	return 0;
}

static int connsys_adie_clock_buffer_setting(unsigned int curr_status, unsigned int next_status)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	unsigned int hw_version;
	int ret = 0;

	hw_version = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID);
	ret = connsys_adie_clock_buffer_setting_mt6983_gen(
		curr_status, next_status, hw_version, CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT);

	if (ret)
		return -1;
#endif
	return 0;
}

int connsys_spi_master_cfg_mt6983(unsigned int next_status)
{
	connsys_wt_slp_top_ctrl_adie6637_mt6983_gen();
	return 0;
}

int consys_get_sleep_mode_mt6983(void)
{
	int platform_config;

	platform_config = consys_hw_get_platform_config();
	if (platform_config & CONSYS_SLEEP_MODE_1)
		return 1;
	else if (platform_config & CONSYS_SLEEP_MODE_2)
		return 2;

	if (conn_hw_env.adie_hw_version == MT6637E1)
		return 1;

	return 1;
}

int connsys_a_die_cfg_mt6983(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else /* CONFIG_FPGA_EARLY_PORTING */
	unsigned int adie_id = 0;
	unsigned int hw_ver_id = 0;
	int check = 0;
	struct consys_plat_thermal_data_mt6983 input;
	void __iomem *sysram_efuse_list[16] = { 0 };
	unsigned int sleep_mode = 0;
	unsigned int clock_type = 0;
	unsigned int sysram_clock_type = 0;

	clock_type = consys_co_clock_type_mt6983();
	/* FW only cares 26M or 52M */
	if (clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_COTMS ||
		clock_type == CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO) {
		sysram_clock_type = 2;
	} else if (clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_COTMS ||
		clock_type == CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO) {
		sysram_clock_type = 1;
	} else {
		pr_notice("unexpected value\n");
	}

	// Write clock type to conninfra sysram
	// 0: default
	// 1: co-clock 26M
	// 2: co_clock 52M
	// 3: tcxo 26M
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_CLOCK_TYPE, sysram_clock_type);

	memset(&input, 0, sizeof(struct consys_plat_thermal_data_mt6983));

	connsys_a_die_switch_to_gpio_mode_mt6983_gen();
	connsys_adie_top_ck_en_ctl_mt6983_gen(1);
	connsys_a_die_cfg_deassert_adie_reset_mt6983_gen();

	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		connsys_adie_top_ck_en_ctl_mt6983_gen(0);
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	check = connsys_a_die_cfg_read_adie_id_mt6983_gen(&adie_id, &hw_ver_id);
	if (check) {
		consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
		return -1;
	}
	pr_info("[%s] A-die chip id: 0x%08x\n", __func__, adie_id);

	conn_hw_env.adie_hw_version = adie_id;
	/* Write to conninfra sysram */
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID, adie_id);

	sysram_efuse_list[0] = (void __iomem *)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0;
	sysram_efuse_list[1] = (void __iomem *)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1;
	sysram_efuse_list[2] = (void __iomem *)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2;
	sysram_efuse_list[3] = (void __iomem *)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3;
	connsys_a_die_efuse_read_get_efuse_info_mt6983_gen(sysram_efuse_list,
			&(input.slop_molecule), &(input.thermal_b), &(input.offset));
	pr_info("slop_molecule=[%d], thermal_b =[%d], offset=[%d]", input.slop_molecule, input.thermal_b, input.offset);
	update_thermal_data_mt6983(&input);

	connsys_a_die_cfg_PART2_mt6983_gen(hw_ver_id);

	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);

	connsys_a_die_switch_to_conn_mode_mt6983_gen();

	conn_hw_env.is_rc_mode = consys_is_rc_mode_enable_mt6983();
#endif /* CONFIG_FPGA_EARLY_PORTING */

	sleep_mode = consys_get_sleep_mode_mt6983();
	pr_info("sleep_mode = %d\n", sleep_mode);
	connsys_wt_slp_top_power_saving_ctrl_adie6637_mt6983_gen(adie_id, sleep_mode);
	return 0;
}

void connsys_afe_sw_patch_mt6983(void)
{
	connsys_afe_sw_patch_mt6983_gen();
}

int connsys_afe_wbg_cal_mt6983(void)
{
	return connsys_afe_wbg_cal_mt6983_gen(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT);
}

int connsys_subsys_pll_initial_mt6983(void)
{
	return connsys_subsys_pll_initial_xtal_26000k_mt6983_gen();
}

int connsys_low_power_setting_mt6983(unsigned int curr_status, unsigned int next_status)
{
	connsys_adie_clock_buffer_setting(curr_status, next_status);

	if (curr_status == 0) {
		connsys_low_power_setting_mt6983_gen();
	} else {
		/* Not first on */
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

int consys_sema_acquire_timeout_mt6983(unsigned int index, unsigned int usec)
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

void consys_sema_release_mt6983(unsigned int index)
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

int consys_spi_read_nolock_mt6983(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
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

int consys_spi_read_mt6983(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6983(subsystem, addr, data);

	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_write_nolock_mt6983(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
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

int consys_spi_write_mt6983(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_write_nolock_mt6983(subsystem, addr, data);

	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_update_bits_mt6983(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask)
{
	int ret = 0;
	unsigned int curr_val = 0;
	unsigned int new_val = 0;
	bool change = false;

	/* Get semaphore before updating bits */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6983(subsystem, addr, &curr_val);

	if (ret) {
		consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
#ifndef CONFIG_FPGA_EARLY_PORTING
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
#endif
		return CONNINFRA_SPI_OP_FAIL;
	}

	new_val = (curr_val & (~mask)) | (data & mask);
	change = (curr_val != new_val);

	if (change) {
		ret = consys_spi_write_nolock_mt6983(subsystem, addr, new_val);
	}

	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_spi_clock_switch_mt6983(enum connsys_spi_speed_type type)
{
	int check;
	int ret = 0;

	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI CLOCK SWITCH] Require semaphore fail\n");
		return -1;
	}

	if (type == CONNSYS_SPI_SPEED_26M) {
		/* SPI clock from HS to osc (source:BPLL) (FM) */
		/* 0x18042110 WR [4]:1'b1 => Disable hs clock of FM */
		/* 0x18042110 WR [5]:1'b1 => Disable hs clock mux to FM */
		/* 0x18012088 WR [4]:1'b1 => Disable SPI BPLL from FM */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_HS_EN, 1);
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_CLR_FM_SPI_CK_CTL, 1);
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_CLR_CONN_RFSPI_BPLL_REQ_M4, 1);

	} else if (type == CONNSYS_SPI_SPEED_64M) {
		/* SPI clock from osc_ck to HS (source:BPLL) (FM) */
		/* 0x18012084 WR      [4]:1'b1 => 1'b1: Enable SPI BPLL from FM */
		/* 0x18011030 POLLING [1] == 1'b1 */
		/* 0x1804210C WR      [5]:1'b1  => Select hs clock mux to FM */
		/* 0x1804210C WR      [4]:1'b1  => Select hs clock of FM */
		CONSYS_REG_WRITE_HW_ENTRY(
			CONN_CLKGEN_TOP_CKGEN_RFSPI_PLL_REQ_SET_CONN_RFSPI_BPLL_REQ_M4, 1);
		check = 0;
		CONSYS_REG_BIT_POLLING(CONN_CFG_PLL_STATUS_ADDR, 1, 1, 100, 50, check);
		if (check == 0) {
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_SPI_CK_CTL, 1);
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_HS_EN, 1);
		} else {
			ret = -1;
			pr_info("[%s] BPLL enable fail: 0x%08x",
				__func__, CONSYS_REG_READ(CONN_CFG_PLL_STATUS_ADDR));
		}
	} else {
		pr_err("[%s] wrong parameter %d\n", __func__, type);
	}

	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_subsys_status_update_mt6983(bool on, int radio)
{
	if (radio < CONNDRV_TYPE_BT || radio > CONNDRV_TYPE_WIFI) {
		pr_err("[%s] wrong parameter: %d\n", __func__, radio);
		return -1;
	}

	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[%s] acquire semaphore (%d) timeout\n",
			__func__, CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);
		return -1;
	}

	/* When FM power on, give priority of selecting spi clock */
	if (radio == CONNDRV_TYPE_FM) {
		if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_SUCCESS) {
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL, on);
			consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
		}
	}

	if (on) {
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	} else {
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	}

	consys_sema_release_mt6983(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);

	/* BT is on but wifi is not on */
	if (on && (radio == CONNDRV_TYPE_BT) &&
	    (CONSYS_REG_READ_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << CONNDRV_TYPE_WIFI)) == 0x0))
		consys_pre_cal_restore_mt6983();

	return 0;
}

bool consys_is_rc_mode_enable_mt6983(void)
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

	/* Base: SRCLEN_RC (0x1C00_D000)
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
void consys_spi_write_offset_range_nolock_mt6983(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size)
{
	unsigned int data = 0, data2;
	unsigned int reg_mask;
	int ret;

	value = (value >> value_offset);
	value = GET_BIT_RANGE(value, size, 0);
	value = (value << reg_offset);
	ret = consys_spi_read_nolock_mt6983(subsystem, addr, &data);
	if (ret) {
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
		return;
	}
	reg_mask = GENMASK(reg_offset + size - 1, reg_offset);
	data2 = data & (~reg_mask);
	data2 = (data2 | value);
	consys_spi_write_nolock_mt6983(subsystem, addr, data2);
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

int connsys_adie_top_ck_en_ctl_mt6983(bool onoff)
{
	return connsys_adie_top_ck_en_ctl_mt6983_gen(onoff);
}
