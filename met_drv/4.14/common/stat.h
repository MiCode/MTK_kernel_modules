/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _STAT_H_
#define _STAT_H_

#include <linux/device.h>

extern struct metdevice met_stat;

int stat_reg(struct kobject *parent);
void stat_unreg(void);

void stat_start(void);
void stat_stop(void);
void stat_polling(unsigned long long stamp, int cpu);

unsigned int get_ctrl_flags(void);

#endif				/* _STAT_H_ */
