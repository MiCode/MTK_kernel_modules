// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "../../../../include/conninfra.h"
#include "../include/connsys_smc.h"
#include "../include/consys_hw.h"
#include "include/mt6897.h"
#include "include/mt6897_atf.h"
#include "include/mt6897_connsyslog.h"
#include "include/mt6897_consys_reg_offset.h"
#include "include/mt6897_pos.h"
#include "include/mt6897_soc.h"

int consys_init_atf_data_mt6897_atf(void)
{
	struct arm_smccc_res res;
	int platform_config;
	static bool initialized = 0;
	int ret;

	if (initialized == 1)
		return 0;

	platform_config = consys_hw_get_platform_config();
	arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, SMC_CONNSYS_INIT_ATF_DATA_OPID,
		platform_config, conn_hw_env.clock_type, 0, 0, 0, 0, &res);
	ret = res.a0;
	initialized = 1;
	return ret;
}

int consys_polling_chipid_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_POLLING_CHIPID_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_d_die_cfg_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_D_DIE_CFG_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_spi_master_cfg_mt6897_atf(unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_MASTER_CFG_OPID, curr_status, next_status, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_a_die_cfg_mt6897_atf(unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;
	struct arm_smccc_res res;

	arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, SMC_CONNSYS_A_DIE_CFG_OPID,
		      curr_status, next_status, 0, 0, 0, 0, &res);
	ret = res.a0;
	conn_hw_env.adie_hw_version = res.a1;

	return ret;
}

int connsys_afe_wbg_cal_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_AFE_WBG_CAL_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

void connsys_afe_sw_patch_mt6897_atf(void)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_AFE_SW_PATCH_OPID, 0, 0, 0, 0, 0, 0);
}

int connsys_subsys_pll_initial_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SUBSYS_PLL_INITIAL_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int connsys_low_power_setting_mt6897_atf(unsigned int curr_status, unsigned int next_status)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_LOW_POWER_SETTING_OPID,
			     curr_status, next_status, 0, 0, 0, 0, ret);
	return ret;
}

int consys_conninfra_wakeup_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_CONNINFRA_WAKEUP_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_conninfra_sleep_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONSYS_CONNINFRA_SLEEP_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_spi_read_mt6897_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
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

int consys_spi_write_mt6897_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
				unsigned int data)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_WRITE_OPID, subsystem, addr, data, 0, 0, 0, ret);
	return ret;
}

int consys_spi_update_bits_mt6897_atf(enum sys_spi_subsystem subsystem, unsigned int addr,
	unsigned int data, unsigned int mask)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_UPDATE_BITS_OPID,
			     subsystem, addr, data, mask, 0, 0, ret);
	return ret;
}

int consys_spi_clock_switch_mt6897_atf(enum connsys_spi_speed_type type)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SPI_CLOCK_SWITCH_OPID, type, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_subsys_status_update_mt6897_atf(bool on, int radio)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_SUBSYS_STATUS_UPDATE_OPID,
			     on, radio, 0, 0, 0, 0, ret);
	return ret;
}

int consys_thermal_query_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_THERMAL_QUERY_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_reset_power_state_mt6897_atf(void)
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
int consys_power_state_dump_mt6897_atf(char *buf, unsigned int size)
{
#define POWER_STATE_BUF_SIZE 256
	int	i, op_id;
	struct arm_smccc_res res;
	unsigned long power_state_dump_data[POWER_STATE_DUMP_DATA_SIZE];
	int ret;
	char temp_buf[POWER_STATE_BUF_SIZE];
	char *buf_p = temp_buf;
	int buf_sz = POWER_STATE_BUF_SIZE;

	for (op_id = SMC_CONNSYS_POWER_STATE_DUMP_START_OPID, i = 0;
	     op_id < SMC_CONNSYS_POWER_STATE_DUMP_END_OPID;
	     op_id++, i++) {
		arm_smccc_smc(MTK_SIP_KERNEL_CONNSYS_CONTROL, op_id,
			      0, 0, 0, 0, 0, 0, &res);
		ret = res.a0;
		if (ret) {
			pr_err("%s[%d], Get power state dump fail at opid=%d\n",
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
			pr_err("%s[%d], Get power state dump fail\n", __func__, __LINE__);
			return -1;
		}
		power_state_dump_data[0 + 3 * i] = res.a1;
	}

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	if(snprintf(buf_p, buf_sz,"[consys_power_state][round:%llu]"
		"conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;gps:%lu.%03lu,%lu;"
		"[total]conninfra:%llu.%03llu,%llu;wf:%llu.%03llu,%llu;"
		"bt:%llu.%03llu,%llu;gps:%llu.%03llu,%llu;",
		power_state_dump_data[0],
		power_state_dump_data[1],
		power_state_dump_data[2],
		power_state_dump_data[3],
		power_state_dump_data[4],
		power_state_dump_data[5],
		power_state_dump_data[6],
		power_state_dump_data[7],
		power_state_dump_data[8],
		power_state_dump_data[9],
		power_state_dump_data[10],
		power_state_dump_data[11],
		power_state_dump_data[12],
		power_state_dump_data[13],
		power_state_dump_data[14],
		power_state_dump_data[15],
		power_state_dump_data[16],
		power_state_dump_data[17],
		power_state_dump_data[18],
		power_state_dump_data[19],
		power_state_dump_data[20],
		power_state_dump_data[21],
		power_state_dump_data[22],
		power_state_dump_data[23],
		power_state_dump_data[24]) > 0)
		pr_info("%s", buf_p);
	return ret;
}

void consys_set_mcu_control_mt6897_atf(int type, bool onoff)
{
	CONNSYS_SMC_CALL_VOID(SMC_CONNSYS_SET_MCU_CONTROL_OPID, 0, 0, 0, 0, 0, 0);
}

int consys_pre_cal_backup_mt6897_atf(unsigned int offset, unsigned int size)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_PRE_CAL_BACKUP_OPID,
			     offset, size, 0, 0, 0, 0, ret);
	return ret;
}

int consys_pre_cal_clean_data_mt6897_atf(void)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_PRE_CAL_CLEAN_DATA_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_adie_top_ck_en_on_mt6897_atf(enum consys_adie_ctl_type type)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_ADIE_CLK_ENABLE_OPID, 1, 0, 0, 0, 0, 0, ret);
	return ret;
}

int consys_adie_top_ck_en_off_mt6897_atf(enum consys_adie_ctl_type type)
{
	int ret = 0;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_ADIE_CLK_ENABLE_OPID, 0, 0, 0, 0, 0, 0, ret);
	return ret;
}

unsigned int consys_emi_set_remapping_reg_mt6897_atf(
	phys_addr_t con_emi_base_addr,
	phys_addr_t md_shared_emi_base_addr,
	phys_addr_t gps_emi_base_addr)
{
	int ret;
	CONNSYS_SMC_CALL_RET(SMC_CONNSYS_EMI_SET_REMAPPING_REG_OPID,
			     0, 0, 0, 0, 0, 0, ret);
	return ret;
}

