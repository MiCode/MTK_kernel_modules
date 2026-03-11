// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "cam_cal_config.h"

#define pr_debug_if(cond, ...)      do { if ((cond)) pr_debug(__VA_ARGS__); } while (0)
#define pr_debug_err(...)    pr_debug("error: " __VA_ARGS__)

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000208, 0xFFFFFFE9, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000001, 0x00000201, 0x0000000F, xiaomi_do_module_version}, //module Information
		{0x00000001, 0x00000210, 0x00000010, xiaomi_do_part_number}, //Fuse ID
		{0x00000001, 0x0000095F, 0x0000074C, xiaomi_do_single_lsc},
		{0x00000001, 0x00000947, 0x00000010, xiaomi_do_2a_gain},
		{0x00000000, 0x00000000, 0x00000000, xiaomi_do_pdaf},
		{0x00000000, 0x00000000, 0x00000000, NULL},
		{0x00000000, 0x00000000, 0x00000000, xiaomi_do_dump_all},
		{0x00000000, 0x00000000, 0x00000000, NULL}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT turnerov08f10front_mtk_eeprom_aac = {
	.name = "turnerov08f10front_mtk_eeprom_aac",
	.check_layout_function = layout_check,
	.read_function = TURNER_OV08F_OTP_read_region,
	.layout = &cal_layout_table,
	.sensor_id = TURNEROV08F10FRONT_SENSOR_ID,
	.i2c_write_id = 0x20,
	.max_size = 0x4000,
	.enable_preload = 1,
	.preload_size = 0x2000,
	.has_stored_data = 1,
};
