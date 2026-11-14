/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025 MediaTek Inc.
 */

#ifndef MT6858_SOC_H
#define MT6858_SOC_H

#define PLATFORM_SOC_CHIP 0x6858

int consys_get_co_clock_type_mt6858(void);
int consys_clk_get_from_dts_mt6858(struct platform_device *pdev);
int consys_clk_get_from_dts_mt6858_6686(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6858(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6858(void);
void consys_clock_fail_dump_mt6858(void);
unsigned long long consys_soc_timestamp_get_mt6858(void);
int consys_conninfra_on_power_ctrl_mt6858(unsigned int enable);
void consys_set_if_pinmux_mt6858(unsigned int enable, unsigned int curr_status, unsigned int next_status);
void consys_set_if_pinmux_mt6858_6686(unsigned int enable, unsigned int curr_status, unsigned int next_status);
int consys_is_consys_reg_mt6858(unsigned int addr);
void consys_print_platform_debug_mt6858(void);
int consys_reg_init_mt6858(struct platform_device *pdev);
int consys_reg_deinit_mt6858(void);
int consys_register_irq_mt6858(struct platform_device *pdev);
void consys_unregister_irq_mt6858(void);
irqreturn_t consys_irq_handler_mt6858(int irq, void* data);

#endif /* MT6858_SOC_H */

