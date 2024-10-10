/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef MT6985_SOC_H
#define MT6985_SOC_H

#define PLATFORM_SOC_CHIP_MT6985 0x6985

int consys_co_clock_type_mt6985(void);
int consys_clk_get_from_dts_mt6985(struct platform_device *pdev);
int consys_clock_buffer_ctrl_mt6985(unsigned int enable);
unsigned int consys_soc_chipid_get_mt6985(void);
int consys_platform_spm_conn_ctrl_mt6985(unsigned int enable);
void consys_set_if_pinmux_mt6985(unsigned int enable,
					unsigned int curr_status, unsigned int next_status);

#endif /* MT6985_SOC_H */
