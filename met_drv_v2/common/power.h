/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _POWER_H_
#define _POWER_H_

#define POWER_LOG_ALL	-1
void force_power_log(int cpu);
void force_power_log_val(unsigned int frequency, int cpu);

#endif				/* _POWER_H_ */
