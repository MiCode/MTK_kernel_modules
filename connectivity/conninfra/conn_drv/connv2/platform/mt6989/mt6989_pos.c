// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/pinctrl/consumer.h>

#include "conninfra.h"
#include "consys_hw.h"
#include "consys_reg_util.h"
#include "plat_def.h"
#include "osal.h"

#include "mt6989_consys_reg.h"
#include "mt6989_consys_reg_offset.h"
#include "mt6989.h"
#include "mt6989_pos.h"
#include "mt6989_pos_gen.h"

static u64 g_sema_get_time[CONN_SEMA_NUM_MAX];

static int consys_sema_acquire_timeout_mt6989(unsigned int index, unsigned int usec);
static int consys_sema_acquire_mt6989(unsigned int index);
static void consys_sema_release_mt6989(unsigned int index);
#ifndef CONFIG_FPGA_EARLY_PORTING
static const char *get_spi_sys_name(enum sys_spi_subsystem subsystem);
#endif

unsigned int consys_emi_set_remapping_reg_mt6989(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	return consys_emi_set_remapping_reg_mt6989_gen(con_emi_base_addr, md_shared_emi_base_addr,
							gps_emi_base_addr, 16);
}

bool consys_is_rc_mode_enable_mt6989(void)
{
#ifdef CONFIG_FPGA_EARLY_PORTING
	return false;
#else /* CONFIG_FPGA_EARLY_PORTING */
	/* Base: 0x1c00d000
	 * Offset: 0x004 CENTRAL_CFG1
	 *	[0] srclken_rc_en
	 */
	bool ret = (bool)CONSYS_REG_READ_BIT(SRCLKENRC_RC_CENTRAL_CFG1, 0x1);
	return ret;
#endif /* CONFIG_FPGA_EARLY_PORTING */
}

int consys_conninfra_on_power_ctrl_mt6989(unsigned int enable)
{
	int ret = 0;

#if MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE
	ret = consys_platform_spm_conn_ctrl_mt6989(enable);
#else
	ret = consys_conninfra_on_power_ctrl_mt6989_gen(enable);
#endif /* MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE */

	if (enable)
		mdelay(5);

	return ret;
}

int consys_polling_chipid_mt6989(void)
{
	return consys_polling_chipid_mt6989_gen(NULL);
}

int connsys_d_die_cfg_mt6989(void)
{
	unsigned int d_die_efuse = 0;

	/* Reset conninfra sysram */
	consys_init_conninfra_sysram_mt6989_gen();

	/* Read D-die Efuse
	 * AP2CONN_EFUSE_DATA 0x1801_1020
	 * Write to conninfra sysram: CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE(0x1805_3820)
	 */
	connsys_get_d_die_efuse_mt6989_gen(&d_die_efuse);
	CONSYS_REG_WRITE(
		CONN_INFRA_SYSRAM_SW_CR_D_DIE_EFUSE, d_die_efuse);

	connsys_d_die_cfg_mt6989_gen();

#if defined(CONNINFRA_PLAT_BUILD_MODE)
	CONSYS_REG_WRITE(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE, CONNINFRA_PLAT_BUILD_MODE);
	pr_info("[%s] Write CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE to 0x%08x\n",
		__func__, CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_BUILD_MODE));
#endif

	return 0;
}

int connsys_spi_master_cfg_mt6989(unsigned int curr_status, unsigned int next_status)
{
	if (curr_status != 0)
		return 0;

	connsys_wt_slp_top_ctrl_adie6686_mt6989_gen();
	return 0;
}

void connsys_afe_sw_patch_mt6989(void)
{
	connsys_afe_sw_patch_mt6989_gen();
}

int connsys_subsys_pll_initial_mt6989(void)
{
	return connsys_subsys_pll_initial_xtal_26000k_mt6989_gen();
}

int connsys_low_power_setting_mt6989(unsigned int curr_status, unsigned int next_status)
{
	if (curr_status == 0)
		connsys_low_power_setting_mt6989_gen();

	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* !!!!!!!!!!!!!!!!!!!!!! CANNOT add code after HERE!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

	return 0;
}

int consys_conninfra_wakeup_mt6989(void)
{
	return consys_conninfra_wakeup_mt6989_gen();
}

int consys_conninfra_sleep_mt6989(void)
{
	return consys_conninfra_sleep_mt6989_gen();
}

int consys_subsys_status_update_mt6989(bool on, int radio)
{
	if (radio < CONNDRV_TYPE_BT || radio > CONNDRV_TYPE_WIFI) {
		pr_err("[%s] wrong parameter: %d\n", __func__, radio);
		return -1;
	}

	if (on)
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1U << radio));
	else
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_RADIO_STATUS, (0x1U << radio));

	return 0;
}

static int consys_sema_acquire_mt6989(unsigned int index)
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

int consys_sema_acquire_timeout_mt6989(unsigned int index, unsigned int usec)
{
	unsigned int i;

	if (index >= CONN_SEMA_NUM_MAX)
		return CONN_SEMA_GET_FAIL;
	for (i = 0; i < usec; i++) {
		if (consys_sema_acquire_mt6989(index) == CONN_SEMA_GET_SUCCESS) {
			g_sema_get_time[index] = jiffies;
			return CONN_SEMA_GET_SUCCESS;
		}
		udelay(1);
	}

	pr_err("Get semaphore 0x%x timeout, dump status:\n", index);
	pr_err("M1:[0x%x] M2:[0x%x] M3:[0x%x]\n",
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M1_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M2_STA_REP_1_ADDR),
		CONSYS_REG_READ(CONN_SEMAPHORE_CONN_SEMA_OWN_BY_M3_STA_REP_1_ADDR));
	return CONN_SEMA_GET_FAIL;
}

void consys_sema_release_mt6989(unsigned int index)
{
#define SEMA_HOLD_TIME_THRESHOLD 10 //10 ms
	u64 duration;

	if (index >= CONN_SEMA_NUM_MAX)
		return;
	CONSYS_REG_WRITE(
		(CONN_SEMAPHORE_CONN_SEMA00_M2_OWN_REL_ADDR + index*4), 0x1);

	duration = jiffies_to_msecs(jiffies - g_sema_get_time[index]);
	if (duration > SEMA_HOLD_TIME_THRESHOLD)
		pr_notice("%s hold semaphore (%d) for %lu ms\n", __func__, index, duration);
}

struct spi_op {
	bool valid;
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
	{false, },
	/* SYS_SPI_WF */
	{false, },
	/* SYS_SPI_BT */
	{false, },
	/* SYS_SPI_FM */
	{false, },
	/* SYS_SPI_GPS */
	{
		true,
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 4,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_ADDR_OFFSET, 0x00009000, 0x00008000,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_GPS_GPS_RDAT_OFFSET, 0x0000ffff
	},
	/* SYS_SPI_TOP */
	{
		true,
		CONN_RF_SPI_MST_REG_SPI_STA_OFFSET, 5,
		CONN_RF_SPI_MST_REG_SPI_TOP_ADDR_OFFSET, 0x0000b000, 0x0000a000,
		CONN_RF_SPI_MST_REG_SPI_TOP_WDAT_OFFSET,
		CONN_RF_SPI_MST_REG_SPI_TOP_RDAT_OFFSET, 0xffffffff
	},
	/* SYS_SPI_WF2 */
	{false, },
	/* SYS_SPI_WF3 */
	{false, },
};

static int consys_spi_read_nolock_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
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
	if (op->valid == false) {
		pr_err("[%s] invalid parameter: subsystem=[%d]", __func__, subsystem);
		return CONNINFRA_SPI_OP_FAIL;
	}

#ifndef CONFIG_FPGA_EARLY_PORTING
	CONSYS_REG_BIT_POLLING(
		CONN_RF_SPI_MST_REG_BASE + op->busy_cr, op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif

	CONSYS_REG_WRITE(CONN_RF_SPI_MST_REG_BASE + op->addr_cr, (op->read_addr_format | addr));

	CONSYS_REG_WRITE(CONN_RF_SPI_MST_REG_BASE + op->write_data_cr, 0);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(
		CONN_RF_SPI_MST_REG_BASE + op->busy_cr, op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%d][STEP4] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, subsystem, CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	check = CONSYS_REG_READ_BIT(
		CONN_RF_SPI_MST_REG_BASE + op->read_data_cr, op->read_data_mask);
	*data = check;

	return 0;
}

int consys_spi_read_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6989(
		CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI READ] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6989(subsystem, addr, data);

	consys_sema_release_mt6989(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

static int consys_spi_write_nolock_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
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
		CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
		op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP1] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif
	CONSYS_REG_WRITE(CONN_RF_SPI_MST_REG_BASE + op->addr_cr, (op->write_addr_format | addr));

	CONSYS_REG_WRITE(CONN_RF_SPI_MST_REG_BASE + op->write_data_cr, data);

#ifndef CONFIG_FPGA_EARLY_PORTING
	udelay(1);
	check = 0;
	CONSYS_REG_BIT_POLLING(CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
		op->polling_bit, 0, 100, 50, check);
	if (check != 0) {
		pr_err("[%s][%s][STEP4] polling 0x%08lx bit %d fail. Value=0x%08x\n",
			__func__, get_spi_sys_name(subsystem),
			CONN_RF_SPI_MST_REG_BASE + op->busy_cr,
			op->polling_bit,
			CONSYS_REG_READ(CONN_RF_SPI_MST_REG_BASE + op->busy_cr));
		return CONNINFRA_SPI_OP_FAIL;
	}
#endif

	return 0;
}

int consys_spi_write_mt6989(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	int ret = 0;
	/* Get semaphore before read */
	if (consys_sema_acquire_timeout_mt6989(
		CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_write_nolock_mt6989(subsystem, addr, data);

	consys_sema_release_mt6989(CONN_SEMA_RFSPI_INDEX);
	return ret;
}

int consys_spi_update_bits_mt6989(
	enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask)
{
	int ret = 0;
	unsigned int curr_val = 0;
	unsigned int new_val = 0;
	bool change = false;

	/* Get semaphore before updating bits */
	if (consys_sema_acquire_timeout_mt6989(
		CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[SPI WRITE] Require semaphore fail\n");
		return CONNINFRA_SPI_OP_FAIL;
	}

	ret = consys_spi_read_nolock_mt6989(subsystem, addr, &curr_val);

	if (ret) {
		consys_sema_release_mt6989(CONN_SEMA_RFSPI_INDEX);
#ifndef CONFIG_FPGA_EARLY_PORTING
		pr_err("[%s][%s] Get 0x%08x error, ret=%d",
			__func__, get_spi_sys_name(subsystem), addr, ret);
#endif
		return CONNINFRA_SPI_OP_FAIL;
	}

	new_val = (curr_val & (~mask)) | (data & mask);
	change = (curr_val != new_val);

	if (change)
		ret = consys_spi_write_nolock_mt6989(subsystem, addr, new_val);

	consys_sema_release_mt6989(CONN_SEMA_RFSPI_INDEX);

	return ret;
}

#ifndef CONFIG_FPGA_EARLY_PORTING
const char *get_spi_sys_name(enum sys_spi_subsystem subsystem)
{
	static const char *spi_system_name[SYS_SPI_MAX] = {
		"NS-SYS_SPI_WF1",
		"NS-SYS_SPI_WF",
		"NS-SYS_SPI_BT",
		"NS-SYS_SPI_FM",
		"SYS_SPI_GPS",
		"SYS_SPI_TOP",
		"NS-SYS_SPI_WF2",
		"NS-SYS_SPI_WF3",
	};
	if (subsystem >= SYS_SPI_WF1 && subsystem < SYS_SPI_MAX)
		return spi_system_name[subsystem];
	return "UNKNOWN";
}
#endif
