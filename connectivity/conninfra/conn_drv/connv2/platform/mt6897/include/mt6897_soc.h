/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef MT6897_SOC_H
#define MT6897_SOC_H

#define PLATFORM_SOC_CHIP 0x6897

int consys_get_co_clock_type_mt6897(void);
int consys_clk_get_from_dts_mt6897(struct platform_device *pdev);
unsigned int consys_soc_chipid_get_mt6897(void);
unsigned long long consys_soc_timestamp_get_mt6897(void);
int consys_conninfra_on_power_ctrl_mt6897(unsigned int enable);
void consys_set_if_pinmux_mt6897(unsigned int enable,
            unsigned int curr_status, unsigned int next_status);
int consys_is_consys_reg_mt6897(unsigned int addr);
int consys_reg_init_mt6897(struct platform_device *pdev);
int consys_reg_deinit_mt6897(void);

#endif /* MT6897_SOC_H */

