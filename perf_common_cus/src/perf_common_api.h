// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <linux/slab.h>
#include <linux/types.h>

void format_sbin_data(char *buf, u32 size, u32 *sbin_data, u32 lens);
extern void (*format_sbin_data_hook)(char*, u32, u32*, u32);