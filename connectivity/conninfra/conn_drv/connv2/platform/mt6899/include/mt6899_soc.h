/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef MT6899_SOC_H
#define MT6899_SOC_H

#define PLATFORM_SOC_CHIP 0x6899
#define FACTORY_TC_SIZE 16

int consys_get_co_clock_type_mt6899(void);
int consys_clk_get_from_dts_mt6899(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6899(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6899(void);
unsigned long long consys_soc_timestamp_get_mt6899(void);
int consys_conninfra_on_power_ctrl_mt6899(unsigned int enable);
void consys_set_if_pinmux_mt6899(unsigned int enable,
            unsigned int curr_status, unsigned int next_status);
int consys_is_consys_reg_mt6899(unsigned int addr);
int consys_factory_testcase_mt6899(char *buf, unsigned int size);
int consys_reg_init_mt6899(struct platform_device *pdev);
int consys_reg_deinit_mt6899(void);

#endif /* MT6899_SOC_H */

