/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gl_os.h"

#ifndef _GL_PLAT_H
#define _GL_PLAT_H

#define AUTO_CPU_FREQ (0)
#define CPU_ALL_CORE (0xff)

void kalSetTaskUtilMinPct(int pid, unsigned int min);
void kalSetCpuFreq(int32_t freq, uint32_t set_mask);
uint32_t kalGetDramBwMaxIdx(void);
void kalSetDramBoost(struct ADAPTER *prAdapter, int32_t iLv);
int kalSetCpuMask(struct task_struct *task, uint32_t set_mask);
#endif /* _GL_PLAT_H */
