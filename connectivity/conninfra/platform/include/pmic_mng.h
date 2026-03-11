/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _PLATFORM_PMIC_MNG_H_
#define _PLATFORM_PMIC_MNG_H_

#include <linux/platform_device.h>

#include "consys_hw.h"

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#define COMMON_KERNEL_PMIC_SUPPORT	1
#else
#define COMMON_KERNEL_PMIC_SUPPORT	0
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

typedef int(*CONSYS_PMIC_GET_FROM_DTS) (
	struct platform_device *pdev,
	struct conninfra_dev_cb* dev_cb);

typedef int(*CONSYS_PMIC_COMMON_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_COMMON_POWER_LOW_POWER_MODE) (unsigned int enable);

typedef int(*CONSYS_PMIC_WIFI_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_BT_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_GPS_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_FM_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_EVENT_NOTIFIER) (unsigned int id, unsigned int event);
typedef int(*CONSYS_PMIC_RAISE_VOLTAGE) (unsigned int, bool, bool);

struct consys_platform_pmic_ops {
	CONSYS_PMIC_GET_FROM_DTS consys_pmic_get_from_dts;
	/* vcn 18 */
	CONSYS_PMIC_COMMON_POWER_CTRL consys_pmic_common_power_ctrl;
	CONSYS_PMIC_COMMON_POWER_LOW_POWER_MODE consys_pmic_common_power_low_power_mode;
	CONSYS_PMIC_WIFI_POWER_CTRL consys_pmic_wifi_power_ctrl;
	CONSYS_PMIC_BT_POWER_CTRL consys_pmic_bt_power_ctrl;
	CONSYS_PMIC_GPS_POWER_CTRL consys_pmic_gps_power_ctrl;
	CONSYS_PMIC_FM_POWER_CTRL consys_pmic_fm_power_ctrl;
	CONSYS_PMIC_EVENT_NOTIFIER consys_pmic_event_notifier;
	CONSYS_PMIC_RAISE_VOLTAGE consys_pmic_raise_voltage;
};


/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if COMMON_KERNEL_PMIC_SUPPORT
extern struct regmap *g_regmap;
extern struct regmap *g_regmap_mt6363;
extern struct regmap *g_regmap_mt6373;
extern struct regmap *g_regmap_mt6368;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int pmic_mng_init(struct platform_device *pdev, struct conninfra_dev_cb* dev_cb, const struct conninfra_plat_data* plat_data);
int pmic_mng_deinit(void);
int pmic_mng_register_device(void);

int pmic_mng_common_power_ctrl(unsigned int enable);
int pmic_mng_common_power_low_power_mode(unsigned int enable);
int pmic_mng_wifi_power_ctrl(unsigned int enable);
int pmic_mng_bt_power_ctrl(unsigned int enable);
int pmic_mng_gps_power_ctrl(unsigned int enable);
int pmic_mng_fm_power_ctrl(unsigned int enable);
int pmic_mng_event_cb(unsigned int id, unsigned int event);
int pmic_mng_raise_voltage(unsigned int, bool, bool);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif				/* _PLATFORM_PMIC_MNG_H_ */
