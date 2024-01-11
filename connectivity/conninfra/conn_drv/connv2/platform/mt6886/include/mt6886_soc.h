/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef MT6886_SOC_H
#define MT6886_SOC_H

#define PLATFORM_SOC_CHIP 0x6886

int consys_get_co_clock_type_mt6886(void);
int consys_clk_get_from_dts_mt6886(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6886(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6886(void);
void consys_clock_fail_dump_mt6886(void);
unsigned long long consys_soc_timestamp_get_mt6886(void);
int consys_conninfra_on_power_ctrl_mt6886(unsigned int enable);
void consys_set_if_pinmux_mt6886(unsigned int enable);
int consys_is_consys_reg_mt6886(unsigned int addr);
void consys_print_platform_debug_mt6886(void);
int consys_reg_init_mt6886(struct platform_device *pdev);
int consys_reg_deinit_mt6886(void);

#endif /* MT6886_SOC_H */

