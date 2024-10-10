// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "../include/consys_hw.h" /* for semaphore index */
#include "../include/consys_reg_util.h" /* macro for read/write cr */
#include "../include/plat_def.h"
#include "../include/plat_library.h"

#include "include/mt6878.h" /* For function declaration */
#include "include/mt6878_consys_reg.h" /* cr base address */
#include "include/mt6878_consys_reg_offset.h" /* cr offset */
#include "include/mt6878_pos.h"
#include "include/mt6878_pos_gen.h"

/*******************************************************************************
 *                                 M A C R O S
 ********************************************************************************
 */
#define CONSYS_SLEEP_MODE_1	(0x1 << 0)
#define CONSYS_SLEEP_MODE_2	(0x1 << 1)
#define CONSYS_SLEEP_MODE_3	(0x1 << 2)

#define MT6637E1 0x66378A00
#define MT6637E2 0x66378A01

#define SEMA_HOLD_TIME_THRESHOLD 10 //10 ms

#define CONN_SW_SEMA_NUM_MAX 8
/*******************************************************************************
 *                             D A T A   T Y P E S
 ********************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 ********************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ********************************************************************************
 */
static u64 sema_get_time[CONN_SEMA_NUM_MAX];
static u64 sw_sema_get_time[CONN_SW_SEMA_NUM_MAX];
static unsigned long g_sema_irq_flags = 0;
static unsigned long g_sw_sema_irq_flags = 0;

#ifndef CONFIG_FPGA_EARLY_PORTING
static const char *get_spi_sys_name(enum sys_spi_subsystem subsystem);
#endif
static int connsys_adie_clock_buffer_setting(unsigned int curr_status, unsigned int next_status);

unsigned int consys_emi_set_remapping_reg_mt6878(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	return consys_emi_set_remapping_reg_mt6878_gen(con_emi_base_addr, md_shared_emi_base_addr,
							gps_emi_base_addr, 16);
}

int consys_conninfra_wakeup_mt6878(void)
{
	return consys_conninfra_wakeup_mt6878_gen();
}

int consys_conninfra_sleep_mt6878(void)
{
	return consys_conninfra_sleep_mt6878_gen();
}

int consys_polling_chipid_mt6878(void)
{
	return consys_polling_chipid_mt6878_gen(NULL);
}

int connsys_d_die_cfg_mt6878(void)
{
	unsigned int d_die_efuse = 0;

	/* Reset conninfra sysram */
	consys_init_conninfra_sysram_mt6878_gen();

	/* Read D-die Efuse
	 * AP2CONN_EFUSE_DATA 0x1801_1020
	 * Write to conninfra sysram: CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE(0x1805_3820)
	 */
	connsys_get_d_die_efuse_mt6878_gen(&d_die_efuse);
	CONSYS_REG_WRITE(
		CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE, d_die_efuse);

	connsys_d_die_cfg_mt6878_gen();

#if defined(CONNINFRA_PLAT_BUILD_MODE)
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE, CONNINFRA_PLAT_BUILD_MODE);
	pr_info("[%s] Write CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE to 0x%08x\n",
		__func__, CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE));
#endif

	return 0;
}

static int connsys_adie_clock_buffer_setting(unsigned int curr_status,
					     unsigned int next_status)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	unsigned int hw_version;
	int ret = 0;

	hw_version = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID);
	ret = connsys_adie_clock_buffer_setting_mt6878_gen(
		curr_status, next_status, hw_version, CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT);

	if (ret)
		return -1;
#endif
	return 0;
}

int connsys_spi_master_cfg_mt6878(unsigned int curr_status, unsigned int next_status)
{
	if (curr_status != 0)
		return 0;

	if (consys_get_adie_chipid_mt6878() == ADIE_6637) {
		connsys_wt_slp_top_ctrl_adie6637_mt6878_gen();
	} else if (consys_get_adie_chipid_mt6878() == ADIE_6631) {
		connsys_wt_slp_top_ctrl_adie6631_mt6878_gen();
	} else if (consys_get_adie_chipid_mt6878() == ADIE_6686) {
		connsys_wt_slp_top_ctrl_adie6631_mt6878_gen();
		connsys_wt_slp_top_ctrl_adie6686_mt6878_gen();
	}

	return 0;
}

int consys_get_sleep_mode_mt6878(void)
{
	int platform_config;

	platform_config = consys_hw_get_platform_config();
	if (platform_config & CONSYS_SLEEP_MODE_1)
		return 1;
	else if (platform_config & CONSYS_SLEEP_MODE_2)
		return 2;
	else if (platform_config & CONSYS_SLEEP_MODE_3)
		return 3;

	if (conn_hw_env.adie_hw_version == MT6637E1)
		return 1;

	return 3;
}

int connsys_a_die_cfg_mt6878(unsigned int curr_status, unsigned int next_status)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_info("[%s] not support on FPGA", __func__);
#else /* CONFIG_FPGA_EARLY_PORTING */
	unsigned int adie_id = 0;
	unsigned int hw_ver_id = 0;
	int check = 0;
	struct consys_plat_thermal_data_mt6878 input;
	mapped_addr sysram_efuse_list[16] = { 0 };
	unsigned int sleep_mode = 0;
	unsigned int clock_type = 0;
	unsigned int sysram_clock_type = 0;
	unsigned int curr_bt = 0;
	unsigned int curr_wifi = 0;
	unsigned int curr_fm = 0;
	unsigned int next_bt = 0;
	unsigned int next_wifi = 0;
	unsigned int next_fm = 0;
	bool bt_wifi_fm_on = false;

	curr_bt = (curr_status & (0x1U << CONNDRV_TYPE_BT)) >> CONNDRV_TYPE_BT;
	curr_wifi = (curr_status & (0x1U << CONNDRV_TYPE_WIFI)) >> CONNDRV_TYPE_WIFI;
	curr_fm = (curr_status & (0x1U << CONNDRV_TYPE_FM)) >> CONNDRV_TYPE_FM;

	next_bt = (next_status & (0x1U << CONNDRV_TYPE_BT)) >> CONNDRV_TYPE_BT;
	next_wifi = (next_status & (0x1U << CONNDRV_TYPE_WIFI)) >> CONNDRV_TYPE_WIFI;
	next_fm = (next_status & (0x1U << CONNDRV_TYPE_FM)) >> CONNDRV_TYPE_FM;

	if (((curr_bt + curr_wifi + curr_fm) == 0) && ((next_bt + next_wifi + next_fm) != 0))
		bt_wifi_fm_on = true;

	if ((consys_get_adie_chipid_mt6878() == ADIE_6686) && (bt_wifi_fm_on == false)) {
		return 0;
	} else if ((consys_get_adie_chipid_mt6878() != ADIE_6686) && (curr_status != 0)) {
		return 0;
	}

	clock_type = consys_co_clock_type_mt6878();
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

	/*
	 * Write clock type to conninfra sysram
	 * 0: default
	 * 1: co-clock 26M
	 * 2: co_clock 52M
	 * 3: tcxo 26M
	 */
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_CLOCK_TYPE, sysram_clock_type);
	memset(&input, 0, sizeof(struct consys_plat_thermal_data_mt6878));
	if (consys_get_adie_chipid_mt6878() == ADIE_6637) {
		connsys_a_die_switch_to_gpio_mode_mt6878_gen();
		connsys_adie_top_ck_en_ctl_mt6878_gen(1);

		if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
			== CONN_SEMA_GET_FAIL) {
			connsys_adie_top_ck_en_ctl_mt6878_gen(0);
			pr_err("[SPI READ] Require semaphore fail\n");
			return CONNINFRA_SPI_OP_FAIL;
		}
	} else if (consys_get_adie_chipid_mt6878() == ADIE_6631) {
		connsys_a_die_switch_to_gpio_mode_mt6878_gen();
		connsys_adie_top_ck_en_ctl_mt6878_gen(1);

		if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
			== CONN_SEMA_GET_FAIL) {
			connsys_adie_top_ck_en_ctl_mt6878_gen(0);
			pr_err("[SPI READ] Require semaphore fail\n");
			return CONNINFRA_SPI_OP_FAIL;
		}
	} else if (consys_get_adie_chipid_mt6878() == ADIE_6686) {
		connsys_a_die_switch_to_gpio_mode_mt6878_gen();
		connsys_adie_top_ck_en_ctl_mt6878_gen(1);
		consys_m10_srclken_cfg_mt6878_gen(1);

		if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
			== CONN_SEMA_GET_FAIL) {
			connsys_adie_top_ck_en_ctl_mt6878_gen(0);
			consys_m10_srclken_cfg_mt6878_gen(0);
			pr_err("[SPI READ] Require semaphore fail\n");
			return CONNINFRA_SPI_OP_FAIL;
		}
		if (consys_sema_m4_acquire_timeout_mt6878(0, CONN_SEMA_TIMEOUT)
			== CONN_SEMA_GET_FAIL) {
			connsys_adie_top_ck_en_ctl_mt6878_gen(0);
			consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
			consys_m10_srclken_cfg_mt6878_gen(0);
			pr_err("[SPI READ] Require semaphore fail\n");
			return CONNINFRA_SPI_OP_FAIL;
		}
	}

	connsys_a_die_cfg_deassert_adie_reset_mt6878_gen(curr_status, next_status);

	if (consys_get_adie_chipid_mt6878() == ADIE_6686)
		consys_sema_m4_release_mt6878(0);

	check = connsys_a_die_cfg_read_adie_id_mt6878_gen(curr_status, next_status, &adie_id, &hw_ver_id);
	if (check) {
		consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
		return -1;
	}
	pr_info("[%s] A-die chip id: 0x%08x\n", __func__, adie_id);

	conn_hw_env.adie_hw_version = adie_id;
	/* Write to conninfra sysram */
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_A_DIE_CHIP_ID, adie_id);

	sysram_efuse_list[0] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0;
	sysram_efuse_list[1] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1;
	sysram_efuse_list[2] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2;
	sysram_efuse_list[3] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3;
	if (consys_get_adie_chipid_mt6878() != ADIE_6637) {
		sysram_efuse_list[4] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_4;
		sysram_efuse_list[5] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_5;
		sysram_efuse_list[6] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_6;
		sysram_efuse_list[7] = (mapped_addr)CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_7;
	}
	connsys_a_die_efuse_read_get_efuse_info_mt6878_gen(curr_status, next_status,
			sysram_efuse_list, &(input.slop_molecule), &(input.thermal_b), &(input.offset));
	pr_info("slop_molecule=[%d], thermal_b =[%d], offset=[%d]",
		input.slop_molecule, input.thermal_b, input.offset);
	update_thermal_data_mt6878(&input);

	connsys_a_die_cfg_PART2_mt6878_gen(curr_status, next_status, hw_ver_id);

	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);

	if (consys_get_adie_chipid_mt6878() == ADIE_6686)
		consys_m10_srclken_cfg_mt6878_gen(0);

	if (consys_get_adie_chipid_mt6878() == ADIE_6637)
		connsys_a_die_switch_to_conn_mode_mt6878_gen();

	conn_hw_env.is_rc_mode = consys_is_rc_mode_enable_mt6878();

	if (consys_get_adie_chipid_mt6878() == ADIE_6637) {
		sleep_mode = consys_get_sleep_mode_mt6878();
		pr_info("sleep_mode = %d\n", sleep_mode);
		connsys_wt_slp_top_power_saving_ctrl_adie6637_mt6878_gen(adie_id, sleep_mode);
	} else {
		connsys_wt_slp_top_power_saving_ctrl_adie6631_mt6878_gen();
	}
	connsys_adie_top_ck_en_ctl_mt6878_gen(0);

#endif /* CONFIG_FPGA_EARLY_PORTING */
	return 0;
}

void connsys_afe_sw_patch_mt6878(void)
{
	connsys_afe_sw_patch_mt6878_gen();
}

int connsys_afe_wbg_cal_mt6878(void)
{
	static int first_cal = 1;

	/*
	 * DAC cal should be executed only once.
	 * The result will be stored in always-on domain.
	 */
	if (first_cal == 0)
		return 0;
	first_cal = 0;
	return connsys_afe_wbg_cal_mt6878_gen(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT);
}

int connsys_subsys_pll_initial_mt6878(void)
{
	return connsys_subsys_pll_initial_xtal_26000k_mt6878_gen();
}

int connsys_low_power_setting_mt6878(unsigned int curr_status, unsigned int next_status)
{
	if (consys_get_adie_chipid_mt6878() == ADIE_6637)
		connsys_adie_clock_buffer_setting(curr_status, next_status);

	if (curr_status == 0) {
		connsys_low_power_setting_mt6878_gen();
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
		(CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_STA_ADDR + index * 4), 0x1) == 0x1) {
		return CONN_SEMA_GET_SUCCESS;
	} else {
		return CONN_SEMA_GET_FAIL;
	}
}

static int consys_sema_m4_acquire(unsigned int index)
{
	if (index >= CONN_SW_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;

	if (CONSYS_REG_READ_BIT(
		(CONN_SEMAPHORE_CONN_SEMA00_M4_OWN_STA_SW_ADDR + index * 4), 0x1) == 0x1) {
		return CONN_SEMA_GET_SUCCESS;
	} else {
		return CONN_SEMA_GET_FAIL;
	}
}

int consys_sema_acquire_timeout_mt6878(unsigned int index, unsigned int usec)
{
	unsigned int i;

	if (index >= CONN_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;
	for (i = 0; i < usec; i++) {
		if (consys_sema_acquire(index) == CONN_SEMA_GET_SUCCESS) {
			sema_get_time[index] = get_jiffies();
			if (index == CONN_SEMA_RFSPI_INDEX)
				local_irq_save(g_sema_irq_flags);
			return CONN_SEMA_GET_SUCCESS;
		}
		udelay(1);
	}

	pr_err("Get semaphore 0x%x timeout, flags=%lu, dump status:\n", index, g_sema_irq_flags);
	pr_err("M0:[0x%x] M1:[0x%x] M2:[0x%x] M3:[0x%x]\n",
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M0_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M1_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M2_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M3_STA_REP_1_ADDR));
	/* Debug feature in Android, remove it in CTP */
	/*
	 * consys_reg_mng_dump_cpupcr(CONN_DUMP_CPUPCR_TYPE_ALL, 10, 200);
	 */
	return CONN_SEMA_GET_FAIL;
}

int consys_sema_m4_acquire_timeout_mt6878(unsigned int index, unsigned int usec)
{
	unsigned int i;

	if (index >= CONN_SW_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;
	for (i = 0; i < usec; i++) {
		if (consys_sema_m4_acquire(index) == CONN_SEMA_GET_SUCCESS) {
			sw_sema_get_time[index] = get_jiffies();
			return CONN_SEMA_GET_SUCCESS;
		}
		udelay(1);
	}

	pr_err("Get sw semaphore 0x%x timeout, dump status:\n", index);
	pr_err("M4:[0x%x] M5:[0x%x] M6:[0x%x] M7:[0x%x]\n",
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M4_STA_REP_SW_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M5_STA_REP_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M6_STA_REP_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M7_STA_REP_ADDR));

	/* Debug feature in Android, remove it in CTP */
	/*
	 * consys_reg_mng_dump_cpupcr(CONN_DUMP_CPUPCR_TYPE_ALL, 10, 200);
	 */
	return CONN_SEMA_GET_FAIL;
}

void consys_sema_release_mt6878(unsigned int index)
{
	unsigned long duration;

	if (index >= CONN_SEMA_NUM_MAX)
		return;
	CONSYS_REG_WRITE(
		(CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_REL_ADDR + index * 4), 0x1);

	duration = time_duration(sema_get_time[index]);
	if (index == CONN_SEMA_RFSPI_INDEX)
		local_irq_restore(g_sema_irq_flags);
	if (duration > SEMA_HOLD_TIME_THRESHOLD)
		pr_notice("%s hold semaphore (%d), flags=%lu for %lu ms\n",
			__func__, index, g_sema_irq_flags, duration);
}

void consys_sema_m4_release_mt6878(unsigned int index)
{
	unsigned long duration;

	if (index >= CONN_SW_SEMA_NUM_MAX)
		return;
	CONSYS_REG_WRITE(
		(CONN_SEMAPHORE_CONN_SEMA00_M4_OWN_REL_SW_ADDR + index * 4), 0x1);

	duration = time_duration(sw_sema_get_time[index]);
	if (duration > SEMA_HOLD_TIME_THRESHOLD)
		pr_notice("%s hold semaphore (%d), flags=%lu for %lu ms\n",
			__func__, index, g_sw_sema_irq_flags, duration);
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

int consys_spi_read_nolock_int_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int *data, mapped_addr spi_base_addr)
{
	/* Read action:
	 * 1. Polling busy_cr[polling_bit] should be 0
	 * 2. Write addr_cr with data being {read_addr_format | addr[11:0]}
	 * 3. Trigger SPI by writing write_data_cr as 0
	 * 4. Polling busy_cr[polling_bit] as 0
	 * 5. Read data_cr[data_mask]
	 */
	int check = 0;
	const struct spi_op *op = &spi_op_array[subsystem];

	if (!data) {
		pr_err("[%s] invalid data ptr\n", __func__);
		return CONNINFRA_SPI_OP_FAIL;
	}
#ifndef CONFIG_FPGA_EARLY_PORTING
	CONSYS_REG_BIT_POLLING(
		spi_base_addr + op->busy_cr, op->polling_bit,
		0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			spi_base_addr + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(spi_base_addr + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif

	CONSYS_REG_WRITE(spi_base_addr + op->addr_cr,
			 (op->read_addr_format | addr));

	CONSYS_REG_WRITE(spi_base_addr + op->write_data_cr, 0);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(
		spi_base_addr + op->busy_cr, op->polling_bit,
		0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%d][STEP4] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, subsystem, spi_base_addr + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(spi_base_addr + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	check = CONSYS_REG_READ_BIT(
		spi_base_addr + op->read_data_cr, op->read_data_mask);
	*data = check;

	return 0;
}

int consys_spi_read_nolock_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int *data)
{
	int ret = 0;
	/* If (Dual A-die) {RFSPI0(with base addr 0x18042XXX)
	 * & RFSPI1(with base addr 0x18046XXX)}(GPS with 6686)
	 */
	mapped_addr spi_base_addr = (mapped_addr)CONN_REG_CONN_RF_SPI_MST_REG_ADDR;

	ret = consys_spi_read_nolock_int_mt6878(subsystem, addr, data, spi_base_addr);
	return ret;
}

int consys_spi_read_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6878(subsystem, addr, data);

	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_1_read_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	int ret = 0;
	/* If (Dual A-die) {RFSPI0(with base addr 0x18042XXX)
	 * & RFSPI1(with base addr 0x18046XXX)}(GPS with 6686)
	 */
	mapped_addr spi_base_addr = (mapped_addr)CONN_REG_CONN_RF_SPI_1_MST_REG_ADDR;

	if ((subsystem != SYS_SPI_GPS) && (subsystem != SYS_SPI_TOP))
		return -1;

	consys_spi_read_nolock_int_mt6878(subsystem, addr, data, spi_base_addr);
	return ret;
}


int consys_spi_write_nolock_int_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int data, mapped_addr spi_base_addr)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	int check = 0;
#endif
	const struct spi_op *op = &spi_op_array[subsystem];

	/* Write action:
	 * 1. Wait busy_cr[polling_bit] as 0
	 * 2. Write addr_cr with data being {write_addr_format | addr[11:0]
	 * 3. Write write_data_cr ad data
	 * 4. Wait busy_cr[polling_bit] as 0
	 */
#ifndef CONFIG_FPGA_EARLY_PORTING
	CONSYS_REG_BIT_POLLING(
		spi_base_addr + op->busy_cr,
		op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			spi_base_addr + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(spi_base_addr + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	CONSYS_REG_WRITE(spi_base_addr + op->addr_cr,
			 (op->write_addr_format | addr));

	CONSYS_REG_WRITE(spi_base_addr + op->write_data_cr, data);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(spi_base_addr + op->busy_cr, op->polling_bit,
			       0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP4] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			spi_base_addr + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(spi_base_addr + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif

	return 0;
}

int consys_spi_write_nolock_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;
	/* If (Dual A-die) {RFSPI0(with base addr 0x18042XXX)
	 * & RFSPI1(with base addr 0x18046XXX)}(GPS with 6686)
	 */
	mapped_addr spi_base_addr = (mapped_addr)CONN_REG_CONN_RF_SPI_MST_REG_ADDR;

	ret = consys_spi_write_nolock_int_mt6878(subsystem, addr, data, spi_base_addr);
	return ret;
}


int consys_spi_write_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_write_nolock_mt6878(subsystem, addr, data);

	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_1_write_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;
	/* If (Dual A-die) {RFSPI0(with base addr 0x18042XXX)
	 * & RFSPI1(with base addr 0x18046XXX)}(GPS with 6686)
	 */
	mapped_addr spi_base_addr = (mapped_addr)CONN_REG_CONN_RF_SPI_1_MST_REG_ADDR;

	if ((subsystem != SYS_SPI_GPS) && (subsystem != SYS_SPI_TOP))
		return -1;

	ret = consys_spi_write_nolock_int_mt6878(subsystem, addr, data, spi_base_addr);
	return ret;
}


int consys_spi_update_bits_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int data, unsigned int mask)
{
	int ret = 0;
	unsigned int curr_val = 0;
	unsigned int new_val = 0;
	bool change = false;

	/* Get semaphore before updating bits */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6878(subsystem, addr, &curr_val);

	if (ret) {
		consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
#ifndef CONFIG_FPGA_EARLY_PORTING
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
#endif
		return CONNINFRA_SPI_OP_FAIL;
	}

	new_val = (curr_val & (~mask)) | (data & mask);
	change = (curr_val != new_val);

	if (change) {
		ret = consys_spi_write_nolock_mt6878(subsystem, addr, new_val);
	}

	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_spi_1_update_bits_mt6878(enum sys_spi_subsystem subsystem, unsigned int addr,
				  unsigned int data, unsigned int mask)
{
	int ret = 0;
	unsigned int curr_val = 0;
	unsigned int new_val = 0;
	bool change = false;

	ret = consys_spi_1_read_mt6878(subsystem, addr, &curr_val);

	new_val = (curr_val & (~mask)) | (data & mask);
	change = (curr_val != new_val);

	if (change) {
		ret = consys_spi_1_write_mt6878(subsystem, addr, new_val);
	}

	return ret;
}


int consys_spi_clock_switch_mt6878(enum connsys_spi_speed_type type)
{
	int check;
	int ret = 0;

	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
		== CONN_SEMA_GET_FAIL) {
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
			CONSYS_REG_WRITE_HW_ENTRY(
				CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_SPI_CK_CTL, 1);
			CONSYS_REG_WRITE_HW_ENTRY(
				CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_SET_FM_HS_EN, 1);
		} else {
			ret = -1;
			pr_info("[%s] BPLL enable fail: 0x%08x",
				__func__, CONSYS_REG_READ(CONN_CFG_PLL_STATUS_ADDR));
		}
	} else {
		pr_err("[%s] wrong parameter %d\n", __func__, type);
	}

	consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

int consys_subsys_status_update_mt6878(bool on, int radio)
{
	if (radio < CONNDRV_TYPE_BT || radio > CONNDRV_TYPE_WIFI) {
		pr_err("[%s] wrong parameter: %d\n", __func__, radio);
		return -1;
	}

	if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX,
					       CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[%s] acquire semaphore (%d) timeout\n",
			__func__, CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);
		return -1;
	}

	/* When FM power on, give priority of selecting spi clock */
	if (radio == CONNDRV_TYPE_FM) {
		if (consys_sema_acquire_timeout_mt6878(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT)
			== CONN_SEMA_GET_SUCCESS) {
			CONSYS_REG_WRITE_HW_ENTRY(CONN_RF_SPI_MST_REG_SPI_HSCK_CTL_FM_SPI_CK_CTL,
						  on);
			consys_sema_release_mt6878(CONN_SEMA_RFSPI_INDEX);
		}
	}

	if (on) {
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	} else {
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << radio));
	}

	consys_sema_release_mt6878(CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX);

	/* BT is on but wifi is not on */
	if (on && (radio == CONNDRV_TYPE_BT) &&
	    (CONSYS_REG_READ_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1 << CONNDRV_TYPE_WIFI))
	    == 0x0) && consys_get_adie_chipid_mt6878() == ADIE_6637)
		consys_pre_cal_restore_mt6878();

	return 0;
}

bool consys_is_rc_mode_enable_mt6878(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	return false;
#else /* CONFIG_FPGA_EARLY_PORTING */
#ifdef CFG_CONNINFRA_ON_CTP
	bool ret = (bool)CONSYS_REG_READ_BIT(RC_CENTRAL_CFG1, 0x1);
	return ret;
#else /* CFG_CONNINFRA_ON_CTP */
	static bool ever_read;
	mapped_addr addr = 0;
	static bool ret;

	/* Base: SRCLEN_RC (0x1C00_D000)
	 * Offset: 0x004 CENTRAL_CFG1
	 *	[0] srclken_rc_en
	 */
	if (!ever_read) {
		addr = ioremap(RC_CENTRAL_CFG1, 0x4);
		if (addr != 0) {
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
void consys_spi_write_offset_range_nolock_mt6878(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int value,
	unsigned int reg_offset, unsigned int value_offset, unsigned int size)
{
	unsigned int data = 0, data2;
	unsigned int reg_mask;
	int ret;

	value = (value >> value_offset);
	value = GET_BIT_RANGE(value, size, 0);
	value = (value << reg_offset);
	ret = consys_spi_read_nolock_mt6878(subsystem, addr, &data);
	if (ret) {
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
		return;
	}
	reg_mask = GENMASK(reg_offset + size - 1, reg_offset);
	data2 = data & (~reg_mask);
	data2 = (data2 | value);
	consys_spi_write_nolock_mt6878(subsystem, addr, data2);
}

const char *get_spi_sys_name(enum sys_spi_subsystem subsystem)
{
	const static char *spi_system_name[SYS_SPI_MAX] = {
		"SYS_SPI_WF1",
		"SYS_SPI_WF",
		"SYS_SPI_BT",
		"SYS_SPI_FM",
		"SYS_SPI_GPS",
		"SYS_SPI_TOP",
		"SYS_SPI_WF2",
		"SYS_SPI_WF3",
	};
	if (subsystem < SYS_SPI_MAX)
		return spi_system_name[subsystem];
	return "UNKNOWN";
}
#endif

int connsys_adie_top_ck_en_ctl_mt6878(bool onoff)
{
	return connsys_adie_top_ck_en_ctl_mt6878_gen(onoff);
}
