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

unsigned int dashsc532hswide_ofilm_do_pdaf(struct EEPROM_DRV_FD_DATA *pdata,
	unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData);

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000008, 0xFF814984, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000001, 0x00000001, 0x0000000F, xiaomi_do_module_version},
		{0x00000001, 0x00000010, 0x0000000C, xiaomi_do_part_number},
		{0x00000001, 0x0000078C, 0x0000074C, xiaomi_do_single_lsc},
		{0x00000001, 0x00000754, 0x00000010, xiaomi_do_2a_gain},
		{0x00000001, 0x00000EF1, 0x00000A28, dashsc532hswide_ofilm_do_pdaf},
		{0x00000000, 0x00000000, 0x00000001, NULL},
		{0x00000000, 0x00000000, 0x00003FFC, xiaomi_do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, NULL}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT dashsc532hswide_mtk_eeprom_ofilm = {
	.name = "dashsc532hswide_mtk_eeprom_ofilm",
	.check_layout_function = layout_check,
	.read_function = Common_read_region,
	.layout = &cal_layout_table,
	.sensor_id = DASHSC532HSWIDE_SENSOR_ID,
	.i2c_write_id = 0xA2,
	.max_size = 0x4000,
	.enable_preload = 1,
	.preload_size = 0x2000,
	.has_stored_data = 1,
};


unsigned int dashsc532hswide_ofilm_do_pdaf(struct EEPROM_DRV_FD_DATA *pdata,
	unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
			(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	int err =  CamCalReturnErr[pCamCalData->Command];

	(void) start_addr;
	(void) block_size;

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
					start_addr, 496, (unsigned char *)&pCamCalData->PDAF.Data[0]);
	if (read_data_size > 0) {
		pCamCalData->PDAF.Size_of_PDAF = 496;
		err = CAM_CAL_ERR_NO_ERR;
	} else {
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				(start_addr + pCamCalData->PDAF.Size_of_PDAF + 2), 1004,
				(u8 *)&pCamCalData->PDAF.Data[496]);
	if (read_data_size > 0) {
		pCamCalData->PDAF.Size_of_PDAF = 1500;
		err = CAM_CAL_ERR_NO_ERR;
	} else {
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
								0x14D8, 496, (unsigned char *)&pCamCalData->PDAF.Data[1500]);
	pCamCalData->PDAF.Size_of_PDAF = 1996;
	read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
					0x16CA, 1004, (u8 *)&pCamCalData->PDAF.Data[1996]);
	pCamCalData->PDAF.Size_of_PDAF = 3000;


	must_log("======================PDAF Data==================\n");
	must_log("6 percent PD First five %x, %x, %x, %x, %x\n",
		pCamCalData->PDAF.Data[0],
		pCamCalData->PDAF.Data[1],
		pCamCalData->PDAF.Data[2],
		pCamCalData->PDAF.Data[3],
		pCamCalData->PDAF.Data[4]);
	must_log("25 percent PD First five %x, %x, %x, %x, %x\n",
		pCamCalData->PDAF.Data[1500],
		pCamCalData->PDAF.Data[1501],
		pCamCalData->PDAF.Data[1502],
		pCamCalData->PDAF.Data[1503],
		pCamCalData->PDAF.Data[1504]);
	must_log("RETURN = 0x%x\n", err);
	must_log("======================PDAF Data==================\n");

	return err;

}