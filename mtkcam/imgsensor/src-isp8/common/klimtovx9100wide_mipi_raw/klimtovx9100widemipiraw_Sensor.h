/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     klimtovx9100widemipiraw_Sensor.h
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _KLIMTOVX9100WIDEMIPI_SENSOR_H
#define _KLIMTOVX9100WIDEMIPI_SENSOR_H

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"

#include "klimtovx9100wide_ana_gain_table.h"
#include "klimtovx9100wide_Sensor_setting.h"
#include "klimtovx9100wide_Sensor_setting_aac.h"

#include "adaptor-subdrv-ctrl.h"
#include "adaptor-i2c.h"
#include "adaptor.h"

#endif
