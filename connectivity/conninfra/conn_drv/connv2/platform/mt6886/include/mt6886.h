/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MT6886_H
#define MT6886_H

#include "../../include/plat_def.h"

#define	ADIE_6637	0x6637
#define	ADIE_6635	0x6635

enum conn_semaphore_type {
	CONN_SEMA_CHIP_POWER_ON_INDEX = 0,
	CONN_SEMA_CALIBRATION_INDEX = 1,
	CONN_SEMA_FW_DL_INDEX = 2,
	CONN_SEMA_CLOCK_SWITCH_INDEX = 3,
	CONN_SEMA_PINMUX_INDEX = 4,
	CONN_SEMA_CCIF_INDEX = 5,
	CONN_SEMA_COEX_INDEX = 6,
	CONN_SEMA_USB_EP0_INDEX = 7,
	CONN_SEMA_USB_SHARED_INFO_INDEX = 8,
	CONN_SEMA_USB_SUSPEND_INDEX = 9,
	CONN_SEMA_USB_RESUME_INDEX = 10,
	CONN_SEMA_PCIE_INDEX = 11,
	CONN_SEMA_RFSPI_INDEX = 12,
	CONN_SEMA_EFUSE_INDEX = 13,
	CONN_SEMA_THERMAL_INDEX = 14,
	CONN_SEMA_FLASH_INDEX = 15,
	CONN_SEMA_DEBUG_INDEX = 16,
	CONN_SEMA_WIFI_LP_INDEX = 17,
	CONN_SEMA_PATCH_DL_INDEX = 18,
	CONN_SEMA_SHARED_VAR_INDEX = 19,
	CONN_SEMA_BGF_UART0_INDEX = 20,
	CONN_SEMA_BGF_UART1_INDEX = 21,
	CONN_SEMA_SDIO_CLK_INDEX = 22,
	CONN_SEMA_CONN_INFRA_COMMON_SYSRAM_INDEX = 23,
	CONN_SEMA_NUM_MAX = 25 /* can't be omitted */
};

struct consys_plat_thermal_data_mt6886 {
	int thermal_b;
	int slop_molecule;
	int offset;
};

#define POWER_STATE_DUMP_DATA_SIZE	25
extern unsigned long mt6886_power_state_dump_data[];

unsigned int consys_get_hw_ver_mt6886(void);
int consys_thermal_query_mt6886(void);
/* Power state relative */
int consys_enable_power_dump_mt6886(void);
int consys_reset_power_state_mt6886(void);
int consys_power_state_dump_mt6886(char *buf, unsigned int size);

unsigned int consys_adie_detection_mt6886(void);
void consys_set_mcu_control_mt6886(int type, bool onoff);

int consys_pre_cal_backup_mt6886(unsigned int offset, unsigned int size);
int consys_pre_cal_clean_data_mt6886(void);

int consys_co_clock_type_mt6886(void);
void update_thermal_data_mt6886(struct consys_plat_thermal_data_mt6886 *input);
unsigned int consys_get_adie_chipid_mt6886(void);
int consys_pre_cal_restore_mt6886(void);

#endif /* MT6886_H */
