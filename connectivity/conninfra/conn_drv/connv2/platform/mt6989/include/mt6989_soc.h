/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef MT6989_SOC_H
#define MT6989_SOC_H

#include <linux/irqreturn.h>

#define PLATFORM_SOC_CHIP_MT6989 0x6989

int consys_co_clock_type_mt6989(void);
int consys_clk_get_from_dts_mt6989(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6989(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6989(void);
int consys_platform_spm_conn_ctrl_mt6989(unsigned int enable);
void consys_set_if_pinmux_mt6989(unsigned int enable);
int consys_register_irq_mt6989(struct platform_device *pdev);
void consys_unregister_irq_mt6989(void);
irqreturn_t consys_irq_handler_mt6989(int irq, void* data);

#endif /* MT6989_SOC_H */
