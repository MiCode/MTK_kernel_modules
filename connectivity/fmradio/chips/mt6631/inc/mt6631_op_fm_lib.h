/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MT6631_OP_FM_LIB_H__
#define __MT6631_OP_FM_LIB_H__

#include <linux/types.h>

struct fm_patch_tbl *mt6631_get_patch_tbl(void);
signed int mt6631_TurnOn_RgTopGbLdo(void);
signed int mt6631_pwrup_clock_on_reg_op(unsigned char *buf, signed int buf_size);
signed int mt6631_pwrdown_reg_op(unsigned char *buf, signed int buf_size);
void mt6631_show_fm_reg(void);
signed int mt6631_set_freq_fine_tune_reg_op(unsigned char *buf, signed int buf_size);
unsigned short mt6631_chan_para_get(unsigned short freq);
bool mt6631_TDD_chan_check(unsigned short freq);

#endif /* __MT6631_OP_FM_LIB_H__ */
