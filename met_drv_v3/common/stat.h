/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
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
