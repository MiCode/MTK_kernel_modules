// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef MT6985_H
#define MT6985_H

#define CONN_HW_VER_MT6985		0x02050300
#define CONNSYS_A_DIE_ID_MT6985		0x6686

enum conn_semaphore_type
{
	CONN_SEMA_CHIP_POWER_ON_INDEX = 0,
	CONN_SEMA_CALIBRATION_INDEX = 1,
	CONN_SEMA_CLOCK_SWITCH_INDEX = 2,
	CONN_SEMA_PINMUX_INDEX = 3,
	CONN_SEMA_CCIF_INDEX = 4,
	CONN_SEMA_COEX_INDEX = 5,
	CONN_SEMA_RFSPI_INDEX = 6,
	CONN_SEMA_EFUSE_INDEX = 7,
	CONN_SEMA_THERMAL_INDEX = 8,
	CONN_SEMA_DEBUG_INDEX = 9,
	CONN_SEMA_SHARED_VAR_INDEX = 10,
	CONN_SEMA_NUM_MAX = 13 /* can't be omitted */
};

int consys_platform_spm_conn_ctrl_mt6985(unsigned int enable);
unsigned int consys_get_adie_chipid_mt6985(unsigned int drv_type);
unsigned int consys_get_hw_ver_mt6985(void);
int consys_enable_power_dump_mt6985(void);
int consys_reset_power_state_mt6985(void);
int consys_power_state_dump_mt6985(char *buf, unsigned int size);
void consys_set_mcu_control_mt6985(int type, bool onoff);

#endif /* MT6985_H */
