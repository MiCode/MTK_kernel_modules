/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONNV3_PMIC_MNG_H_
#define _CONNV3_PMIC_MNG_H_

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

struct connv3_platform_pmic_ops {
	int (*pmic_initial_setting) (struct platform_device *pdev, struct connv3_dev_cb* dev_cb);
	int (*pmic_common_power_ctrl) (u32 enable);
	int (*pmic_vsel_ctrl) (u32 enable);
	int (*pmic_parse_state) (char *buffer, int buf_sz);
	int (*pmic_antenna_power_ctrl) (u32 radio, u32 enable);
	int (*pmic_get_connsys_chip_info) (char *connsys_ecid, int connsys_ecid_size);
	int (*pmic_get_connsys_adie_chip_info) (char *connsys_adie_info, int connsys_adie_info_size);
	int (*pmic_get_pmic_chip_info) (char *pmic_ecid, int pmic_ecid_size);
	int (*pmic_pwr_rst) (void);
};


/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

#if COMMON_KERNEL_PMIC_SUPPORT
extern struct regmap *g_connv3_regmap_mt6373;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int connv3_pmic_mng_init(struct platform_device *pdev,
			struct connv3_dev_cb* dev_cb,
			const struct connv3_plat_data* plat_data);
int connv3_pmic_mng_deinit(void);

int connv3_pmic_mng_common_power_ctrl(unsigned int enable);
int connv3_pmic_mng_vsel_ctrl(u32 enable);
int connv3_pmic_mng_parse_state(char *buffer, int buf_sz);
int connv3_pmic_mng_set_pmic_state(void);
int connv3_pmic_mng_antenna_power_ctrl(u32 radio, u32 enable);
int connv3_pmic_mng_get_connsys_chip_info(char *connsys_ecid, int connsys_ecid_size);
int connv3_pmic_mng_get_connsys_adie_chip_info(char *connsys_adie_info, int connsys_adie_info_size);
int connv3_pmic_mng_get_pmic_chip_info(char *pmic_ecid, int pmic_ecid_size);
int connv3_pmic_mng_pwr_rst(void);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif	/* _CONNV3_PMIC_MNG_H_ */
