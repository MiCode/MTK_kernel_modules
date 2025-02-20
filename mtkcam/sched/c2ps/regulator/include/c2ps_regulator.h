/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_H_
#define C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_H_

#include "c2ps_common.h"

int regulator_init(void);
void regulator_exit(void);

void send_regulator_req(struct regulator_req *req);
int calculate_uclamp_value(struct c2ps_task_info *tsk_info);
struct regulator_req *get_regulator_req(void);
void c2ps_regulator_flush(void);
void c2ps_regulator_init(void);


#endif  // C2PS_REGULATOR_INCLUDE_C2PS_REGULATOR_H_
