/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef MT6991_SOC_H
#define MT6991_SOC_H

#include <linux/irqreturn.h>

#define PLATFORM_SOC_CHIP_MT6991 0x6991

int consys_co_clock_type_mt6991(void);
int consys_clk_get_from_dts_mt6991(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6991(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6991(void);
int consys_platform_spm_conn_ctrl_mt6991(unsigned int enable);
void consys_set_if_pinmux_mt6991(unsigned int enable);
int consys_register_irq_mt6991(struct platform_device *pdev);
void consys_unregister_irq_mt6991(void);
irqreturn_t consys_irq_handler_mt6991(int irq, void* data);

#endif /* MT6991_SOC_H */
