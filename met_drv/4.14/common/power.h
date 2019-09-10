/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef _POWER_H_
#define _POWER_H_

#define POWER_LOG_ALL	-1
void force_power_log(int cpu);
void force_power_log_val(unsigned int frequency, int cpu);

#endif				/* _POWER_H_ */
