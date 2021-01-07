/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNINFRA_STEP_PARSE_ACT_H_
#define _CONNINFRA_STEP_PARSE_ACT_H_

#include "conninfra_step_base.h"


struct step_action *conninfra_step_create_emi_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_register_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_gpio_action(enum step_drv_type drv_type, int param_num, char *params[]);

struct step_action *conninfra_step_create_periodic_dump_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_show_string_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_sleep_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_condition_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_value_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_condition_emi_action(enum step_drv_type drv_type, int param_num, char *params[]);
struct step_action *conninfra_step_create_condition_register_action(enum step_drv_type drv_type, int param_num, char *params[]);


#endif /* end of _CONNINFRA_STEP_PARSE_ACT_H_ */

