/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MTK_AOV_ULPOSC_H
#define MTK_AOV_ULPOSC_H

#include <linux/regmap.h>

#define CAL_FACTOR1_MIN_VAL	(0)
#define CAL_FACTOR1_MAX_VAL	(4)
#define CAL_FACTOR2_MIN_VAL	(0)
#define CAL_FACTOR2_MAX_VAL	(0x80)
#define CALI_DIV_VAL	(512)
#define CALI_FACTOR1_REG_MASK	(0x180)
#define CALI_FACTOR1_REG_SHIFT	(7)
#define CALI_FACTOR2_REG_MASK	(0x7f)
#define CALI_FACTOR2_REG_SHIFT	(0)
#define CALI_MIS_RATE	(2)

struct aov_ulposc_info {
	bool ulposc_support;
	bool ulposc_cali_done;
	unsigned int ulposc_cali_config_num;
	unsigned int ulposc_config_register[3];
	unsigned int ulposc_cali_config_setting[3];
	unsigned int ulposc_cali_result[3];
	unsigned int ulposc_cali_target;
	unsigned int ulposc_cali_result_target;
	unsigned int ulposc_cali_fmeter_id;
	unsigned int ulposc_result_fmeter_id;
	void __iomem *vlp_base;
};

int aov_ulposc_cali(struct mtk_aov *aov_dev);
void aov_ulposc_dts_init(struct mtk_aov *aov_dev);
int aov_ulposc_check_cali_result(struct mtk_aov *aov_dev);
#endif /* MTK_AOV_ULPOSC_H */
