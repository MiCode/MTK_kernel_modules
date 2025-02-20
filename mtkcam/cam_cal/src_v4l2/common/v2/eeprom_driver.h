/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __EEPROM_DRIVER_H
#define __EEPROM_DRIVER_H

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#include "cam_cal_define.h"

#define DEV_NAME_STR_LEN_MAX 50

struct CAM_CAL_CONFIG_INFO {
	unsigned char *mp_eeprom_preload;
	unsigned char *mp_layout_preload;
};

struct EEPROM_DRV {
	dev_t dev_no;
	struct cdev cdev;
	struct class *pclass;
	char class_name[DEV_NAME_STR_LEN_MAX];

	struct i2c_client *pi2c_client;
	struct mutex eeprom_mutex;
	struct CAM_CAL_CONFIG_INFO config_info;
};

struct EEPROM_DRV_FD_DATA {
	struct EEPROM_DRV *pdrv;
	struct CAM_CAL_SENSOR_INFO sensor_info;
};

#endif
