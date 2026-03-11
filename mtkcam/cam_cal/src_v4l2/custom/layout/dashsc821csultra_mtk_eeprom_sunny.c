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

unsigned int xiaomi_do_single_lsc_dashsc821cs_sunny(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData);

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x000082A4, 0x00000088, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000001, 0x0000829D, 0x0000000F, xiaomi_do_module_version},
		{0x00000001, 0x000082AC, 0x00000010, xiaomi_do_part_number},
		{0x00000001, 0x00008AD3, 0x0000074C, xiaomi_do_single_lsc_dashsc821cs_sunny},
		{0x00000001, 0x00008ABB, 0x00000010, xiaomi_do_2a_gain},
		{0x00000000, 0x00000000, 0x00000000, xiaomi_do_pdaf},
		{0x00000000, 0x00000000, 0x00000000, NULL},
		{0x00000000, 0x00000000, 0x00000000, xiaomi_do_dump_all},
		{0x00000000, 0x00000000, 0x00000000, NULL}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT dashsc821csultra_mtk_eeprom_sunny = {
	.name = "dashsc821csultra_mtk_eeprom_sunny",
	.check_layout_function = layout_check,
	.read_function = DASH_SC821CS_OTP_read_region,
	.layout = &cal_layout_table,
	.sensor_id = DASHSC821CSULTRA_SENSOR_ID,
	.i2c_write_id = 0x6C,
	.max_size = 0xA400,
	.enable_preload = 1,
	.preload_size = 0x1080,
	.has_stored_data = 1,
	.base_address = 0x829C,
};
typedef struct {
	uint32_t start_sunnny;
	uint32_t end_sunny;
} AddressRange_sunny;
static const AddressRange_sunny original_ranges_sunny[] = {
	{0x8AD3, 0x8BFB},
	{0x8C68, 0x8FFB},
	{0x9068, 0x92F6}
};
static const AddressRange_sunny secondary_ranges_sunny[] = {
	{0x8AD3, 0x8B79},
	{0x8BE6, 0x8F79},
	{0x8FE6, 0x92F6}
};
unsigned int xiaomi_do_single_lsc_dashsc821cs_sunny(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	if (!pGetSensorCalData) {
	    error_log("pGetSensorCalData is NULL\n");
	    return false;
	}
	int read_data_size = 0;
	int table_size;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	int offset = getDashSc821csOffset(pdata->pdrv->pi2c_client);

	const AddressRange_sunny *ranges_sunny = (offset == 4226) ? secondary_ranges_sunny : original_ranges_sunny;

	uint32_t total_size = 0;
	for (int i = 0; i < 3; i++) {
		total_size += ranges_sunny[i].end_sunny - ranges_sunny[i].start_sunnny + 1;
	}

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	if (total_size != CAM_CAL_SINGLE_LSC_SIZE)
		error_log("total_size(%d) is not match (%d)\n",
				total_size, CAM_CAL_SINGLE_LSC_SIZE);

	pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType = 2;//mtk type
	pCamCalData->SingleLsc.LscTable.MtkLcsData.PixId = 8;
	pCamCalData->SingleLsc.LscTable.MtkLcsData.TableSize = total_size;

	if (total_size > 0) {
		pCamCalData->SingleLsc.TableRotation = 0;

		must_log("u4Offset=%d u4Length=%d offset=%d", start_addr, total_size, offset);
		must_log("sensorID : 0x%x TableRotation : %d\n", pCamCalData->sensorID,
			pCamCalData->SingleLsc.TableRotation);

		uint8_t *buffer_ptr = (uint8_t *)&pCamCalData->SingleLsc.LscTable.MtkLcsData.SlimLscType;\
		for (int i = 0; i < 3; i++) {
			uint32_t range_size = ranges_sunny[i].end_sunny - ranges_sunny[i].start_sunnny + 1;

			if (buffer_ptr != NULL) {
				table_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
								ranges_sunny[i].start_sunnny, range_size, buffer_ptr);
			}
			buffer_ptr += range_size;
			read_data_size += table_size;
		}

		if (total_size == read_data_size)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			error_log("Read Failed read_data_size = %d\n", read_data_size);
			err = CamCalReturnErr[pCamCalData->Command];
			show_cmd_error_log(pCamCalData->Command);
			return err;
		}
	}

	must_log("======================SingleLsc Data==================\n");
	must_log("[1st] = %x, %x, %x, %x\n",
		pCamCalData->SingleLsc.LscTable.Data[0],
		pCamCalData->SingleLsc.LscTable.Data[1],
		pCamCalData->SingleLsc.LscTable.Data[2],
		pCamCalData->SingleLsc.LscTable.Data[3]);
	must_log("[1st] = SensorLSC(1)?MTKLSC(2)?  %x\n",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType);
	must_log("CapIspReg =0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[0],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[1],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[2],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[3],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[4]);
	must_log("RETURN = 0x%x\n", err);
	must_log("======================SingleLsc Data==================\n");

	return err;
}