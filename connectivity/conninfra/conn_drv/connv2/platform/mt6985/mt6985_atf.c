// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../../../../include/conninfra.h"
#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "include/mt6985.h"
#include "include/mt6985_atf.h"
#include "include/mt6985_consys_reg_offset.h"
#include "include/mt6985_pos.h"
#include "include/mt6985_soc.h"

int consys_init_atf_data_mt6985_atf(void)
{
	struct arm_smccc_res res;
	int platform_config;
	static bool initialized = 0;
	int ret;

	if (initialized == 1)
		return 0;

	platform_config = consys_hw_get_platform_config();
	arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, SMC_CONNSYS_INIT_ATF_DATA_OPID,
		platform_config, consys_co_clock_type_mt6985(), 0, 0, 0, 0, &res);
	ret = res.a0;
	initialized = 1;
	return ret;
}

int consys_polling_chipid_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_POLLING_CHIPID_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_d_die_cfg_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_D_DIE_CFG_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_spi_master_cfg_mt6985_atf(unsigned int curr_status, unsigned int next_statuss)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_MASTER_CFG_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_subsys_pll_initial_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SUBSYS_PLL_INITIAL_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_low_power_setting_mt6985_atf(unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_LOW_POWER_SETTING_OPID,
			     curr_status, next_status, 0, 0, 0, 0, ret);
	return ret;
}

int consys_conninfra_wakeup_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_CONNINFRA_WAKEUP_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_conninfra_sleep_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_CONNINFRA_SLEEP_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_spi_read_mt6985_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
	unsigned int *data)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, SMC_CONNSYS_SPI_READ_OPID,
		subsystem, addr, 0, 0, 0, 0, &res);
	*data = res.a1;
	ret = res.a0;
	return ret;
}

int consys_spi_write_mt6985_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
				unsigned int data)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_WRITE_OPID, subsystem, addr, data, 0, 0, 0, ret);
	return ret;
}

int consys_spi_update_bits_mt6985_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
	unsigned int data, unsigned int mask)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_UPDATE_BITS_OPID,
			     subsystem, addr, data, mask, 0, 0, ret);
	return ret;
}

int consys_spi_clock_switch_mt6985_atf(enum connsys_spi_speed_type type)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_CLOCK_SWITCH_OPID, type, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_subsys_status_update_mt6985_atf(bool on, int radio)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SUBSYS_STATUS_UPDATE_OPID,
			     on, radio, 0, 0, 0, 0, ret);
	return ret;
}

int consys_reset_power_state_mt6985_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_RESET_POWER_STATE_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

/*
 * This is to get sleep count related information from ATF.
 * But it cannot get string from ATF.
 * Separate into several SMC call
 * Getting all data as integer value and then combine it into string.
 */
#define POWER_STATE_DUMP_DATA_SIZE	25
int consys_power_state_dump_mt6985_atf(char *buf, unsigned int size)
{
	int	i, op_id;
	struct arm_smccc_res res;
	unsigned long power_state_dump_data[POWER_STATE_DUMP_DATA_SIZE];
	int ret;

	for (op_id = SMC_CONNSYS_POWER_STATE_DUMP_START_OPID, i = 0;
	     op_id < SMC_CONNSYS_POWER_STATE_DUMP_END_OPID;
	     op_id++, i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, op_id,
			      0, 0, 0, 0, 0, 0, &res);
		ret = res.a0;
		if (ret) {
			pr_notice("[%s][%d], Get power state dump fail at opid=%d\n",
			       __func__, __LINE__, op_id);
			return -1;
		}
		power_state_dump_data[0 + 3 * i] = res.a1;
		power_state_dump_data[1 + 3 * i] = res.a2;
		power_state_dump_data[2 + 3 * i] = res.a3;
	}
	if (op_id == SMC_CONNSYS_POWER_STATE_DUMP_END_OPID) {
		arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, op_id,
			      0, 0, 0, 0, 0, 0, &res);
		ret = res.a0;
		if (ret) {
			pr_notice("[%s][%d], Get power state dump fail\n", __func__, __LINE__);
			return -1;
		}
		power_state_dump_data[0 + 3 * i] = res.a1;
	}

	if (buf == NULL || size <= 0)
		return ret;

	if(snprintf(buf, size,"[consys_power_state][round:%llu]"
		"conninfra:%lu.%03lu,%lu;gps:%lu.%03lu,%lu;"
		"[total]conninfra:%llu.%03llu,%llu;gps:%llu.%03llu,%llu;",
		power_state_dump_data[0],
		power_state_dump_data[1],
		power_state_dump_data[2],
		power_state_dump_data[3],
		power_state_dump_data[10],
		power_state_dump_data[11],
		power_state_dump_data[12],
		power_state_dump_data[13],
		power_state_dump_data[14],
		power_state_dump_data[15],
		power_state_dump_data[22],
		power_state_dump_data[23],
		power_state_dump_data[24]) > 0)
		pr_info("%s", buf);
	return ret;
}

void consys_set_mcu_control_mt6985_atf(int type, bool onoff)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_SET_MCU_CONTROL_OPID, 0, 0, 0, 0, 0, 0);
}
