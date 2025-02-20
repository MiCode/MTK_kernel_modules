/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef PMIC_MNG_H
#define PMIC_MNG_H

#include <linux/platform_device.h>
#include <linux/version.h>

#include "consys_hw.h"

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

typedef int(*CONSYS_PMIC_COMMON_POWER_CTRL) (unsigned int enable, unsigned int curr_status,
					unsigned int next_status);
typedef int(*CONSYS_PMIC_COMMON_POWER_LOW_POWER_MODE) (unsigned int enable, unsigned int curr_status,
					unsigned int next_status);

typedef int(*CONSYS_PMIC_WIFI_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_BT_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_GPS_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_FM_POWER_CTRL) (unsigned int enable);
typedef int(*CONSYS_PMIC_EVENT_NOTIFIER) (unsigned int id, unsigned int event);
typedef int(*CONSYS_PMIC_RAISE_VOLTAGE) (unsigned int, bool, bool);
typedef int(*CONSYS_PMIC_LEAVE_LOW_POWER_MODE) (void);

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
	CONSYS_PMIC_LEAVE_LOW_POWER_MODE consys_pmic_leave_low_power_mode;
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
extern struct regmap *g_regmap_mt6369;
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

int pmic_mng_common_power_ctrl(unsigned int enable, unsigned int curr_status,
					unsigned int next_status);
int pmic_mng_common_power_low_power_mode(unsigned int enable, unsigned int curr_status,
					unsigned int next_status);
int pmic_mng_wifi_power_ctrl(unsigned int enable);
int pmic_mng_bt_power_ctrl(unsigned int enable);
int pmic_mng_gps_power_ctrl(unsigned int enable);
int pmic_mng_fm_power_ctrl(unsigned int enable);
int pmic_mng_event_cb(unsigned int id, unsigned int event);
int pmic_mng_raise_voltage(unsigned int, bool, bool);
bool pmic_mng_is_support_raise_voltage(void);
int pmic_mng_common_power_leave_low_power_mode(void);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif	/* PMIC_MNG_H */
