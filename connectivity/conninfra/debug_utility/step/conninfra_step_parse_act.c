/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <connectivity_build_in_adapter.h>

#include <linux/firmware.h>
#include <asm/delay.h>
#include <linux/slab.h>

#include "osal.h"
#include "conninfra_step.h"
#include "consys_hw.h"
#include "consys_reg_mng.h"

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************/


/*******************************************************************************
 *                           D E F I N E
********************************************************************************/

/*******************************************************************************
 *                           P R I V A T E   D A T A
********************************************************************************/

/*******************************************************************************
 *                      I N T E R N A L   F U N C T I O N S
********************************************************************************/

/*************** for debug ************************/
/*************** for debug ************************/

static int conninfra_step_parse_number_with_symbol(char *ptr, long *value)
{
	int ret = STEP_VALUE_INFO_UNKNOWN;

	if (*ptr == '#') {
		if (osal_strtol(ptr + 1, 10, value)) {
			pr_err("STEP failed: str to value %s\n", ptr);
			ret = STEP_VALUE_INFO_UNKNOWN;
		} else {
			ret = STEP_VALUE_INFO_SYMBOL_REG_BASE;
		}
	} else if (*ptr == '$') {
		if (osal_strtol(ptr + 1, 10, value)) {
			pr_err("STEP failed: str to value %s\n", ptr);
			ret = STEP_VALUE_INFO_UNKNOWN;
		} else {
			ret = STEP_VALUE_INFO_SYMBOL_TEMP_REG;
		}
	} else {
		if (osal_strtol(ptr, 0, value)) {
			pr_err("STEP failed: str to value %s\n", ptr);
			ret = STEP_VALUE_INFO_UNKNOWN;
		} else {
			ret = STEP_VALUE_INFO_NUMBER;
		}
	}

	return ret;
}

static enum step_condition_operator_id conninfra_step_parse_operator_id(char *ptr)
{
	if (osal_strcmp(ptr, ">") == 0)
		return STEP_OPERATOR_GREATER;
	else if (osal_strcmp(ptr, ">=") == 0)
		return STEP_OPERATOR_GREATER_EQUAL;
	else if (osal_strcmp(ptr, "<") == 0)
		return STEP_OPERATOR_LESS;
	else if (osal_strcmp(ptr, "<=") == 0)
		return STEP_OPERATOR_LESS_EQUAL;
	else if (osal_strcmp(ptr, "==") == 0)
		return STEP_OPERATOR_EQUAL;
	else if (osal_strcmp(ptr, "!=") == 0)
		return STEP_OPERATOR_NOT_EQUAL;
	else if (osal_strcmp(ptr, "&&") == 0)
		return STEP_OPERATOR_AND;
	else if (osal_strcmp(ptr, "||") == 0)
		return STEP_OPERATOR_OR;

	return STEP_OPERATOR_MAX;
}




static unsigned int conninfra_step_parse_temp_register_id(char *ptr)
{
	unsigned long res;
	int num_sym;

	num_sym = conninfra_step_parse_number_with_symbol(ptr, &res);

	if (num_sym == STEP_VALUE_INFO_SYMBOL_TEMP_REG)
		return res;
	else
		return STEP_TEMP_REGISTER_SIZE;
}

static char *conninfra_step_save_params_msg(int num, char *params[], char *buf, int buf_size)
{
	int i, len, temp;

	for (i = 0, len = 0; i < num; i++) {
		if (params[i] == NULL)
			break;

		temp = osal_strlen(params[i]) + 1;

		if ((len + temp) >= (buf_size - 1))
			break;

		len += temp;
		osal_strncat(buf, params[i], temp);
		osal_strncat(buf, " ", 1);
	}
	osal_strncat(buf, "\0", 1);

	return buf;
}

static int conninfra_step_parse_register_address(struct step_reg_addr_info *p_reg_addr, char *ptr, long offset)
{
	unsigned long res;
	unsigned int symbol;
	int num_sym;

	num_sym = conninfra_step_parse_number_with_symbol(ptr, &res);

	if (num_sym == STEP_VALUE_INFO_SYMBOL_REG_BASE) {
		symbol = (unsigned int) res;

		res = consys_reg_mng_validate_idx_n_offset(symbol-1, offset);
		if (res == 0) {
			pr_err("STEP failed: validate symbol(%d) and ofsset(%d) fail. %s\n",
						symbol, offset, ptr);
			return -1;
		}
		res = consys_reg_mng_get_phy_addr_by_idx(symbol - 1);
		if (res == 0) {
			pr_err("STEP failed: validate symbol(%d) and ofsset(%d) fail. %s\n",
						symbol, offset, ptr);
			return -1;
		}

		p_reg_addr->address = res;
		p_reg_addr->address_type = symbol;
	} else if (num_sym == STEP_VALUE_INFO_NUMBER) {
		p_reg_addr->address = res;
		p_reg_addr->address_type = 0;
	} else {
		pr_err("STEP failed: number with symbol parse fail %s\n", ptr);
		return -1;
	}

	return 0;
}




static void conninfra_step_create_emi_output_log(struct step_emi_info *p_emi_info, int write,
	unsigned int begin, unsigned int end)
{
	p_emi_info->is_write = write;
	p_emi_info->begin_offset = begin;
	p_emi_info->end_offset = end;
	p_emi_info->output_mode = STEP_OUTPUT_LOG;
}

static void conninfra_step_create_emi_output_register(struct step_emi_info *p_emi_info, int write,
	unsigned int begin, unsigned int mask, unsigned int reg_id)
{
	p_emi_info->is_write = write;
	p_emi_info->begin_offset = begin;
	p_emi_info->end_offset = begin + 0x4;
	p_emi_info->mask = mask;
	p_emi_info->temp_reg_id = reg_id;
	p_emi_info->output_mode = STEP_OUTPUT_REGISTER;
}

static int _conninfra_step_create_emi_action(struct step_emi_info *p_emi_info, int param_num, char *params[])
{
	long write, begin, end;
	unsigned int reg_id;
	char buf[128] = "";
	long mask = 0xFFFFFFFF;

	if (param_num < 3) {
		pr_err("STEP failed: Init EMI to log param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return -1;
	}

	if (osal_strtol(params[0], 0, &write) ||
		osal_strtol(params[1], 0, &begin)) {
		pr_err("STEP failed: str to value %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return -1;
	}

	if (*params[(param_num - 1)] == STEP_TEMP_REGISTER_SYMBOL) {
		reg_id = conninfra_step_parse_temp_register_id(params[(param_num - 1)]);
		if (reg_id >= STEP_PARAMETER_SIZE) {
			pr_err("STEP failed: register id failed: %s\n",
				conninfra_step_save_params_msg(param_num, params, buf, 128));
			return -1;
		}

		if (param_num > 3) {
			if (osal_strtol(params[2], 0, &mask)) {
				pr_err("STEP failed: str to value %s\n",
					conninfra_step_save_params_msg(param_num, params, buf, 128));
				return -1;
			}
		}

		conninfra_step_create_emi_output_register(p_emi_info, write, begin, mask, reg_id);
	} else {
		if (osal_strtol(params[2], 0, &end)) {
			pr_err("STEP failed: str to value %s\n",
				conninfra_step_save_params_msg(param_num, params, buf, 128));
			return -1;
		}
		conninfra_step_create_emi_output_log(p_emi_info, write, begin, end);
	}

	return 0;
}


/*
 * Support:
 * _EMI | R(0) | Begin offset | End offset
 * _EMI | R(0) | Begin offset | mask | Output temp register ID ($)
 * _EMI | R(0) | Begin offset | Output temp register ID ($)
 */
struct step_action *conninfra_step_create_emi_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_emi_action *p_emi_act = NULL;
	struct step_emi_info *p_emi_info = NULL;
	int ret;

	p_emi_act = kzalloc(sizeof(struct step_emi_action), GFP_KERNEL);
	if (p_emi_act == NULL) {
		pr_err("STEP failed: kzalloc emi fail\n");
		return NULL;
	}

	p_emi_info = &p_emi_act->info;
	ret = _conninfra_step_create_emi_action(p_emi_info, param_num, params);

	if (ret != 0) {
		kfree(p_emi_act);
		return NULL;
	}

	return &(p_emi_act->base);
}

static void conninfra_step_create_read_register_output_register(struct step_register_info *p_reg_info,
	struct step_reg_addr_info reg_addr_info, unsigned int offset, int mask, unsigned int reg_id)
{
	p_reg_info->is_write = 0;
	p_reg_info->address_type = reg_addr_info.address_type;
	p_reg_info->address = reg_addr_info.address;
	p_reg_info->offset = offset;
	p_reg_info->times = 1;
	p_reg_info->delay_time = 0;
	p_reg_info->mask = mask;
	p_reg_info->temp_reg_id = reg_id;
	p_reg_info->output_mode = STEP_OUTPUT_REGISTER;
}

static void conninfra_step_create_read_register_output_log(struct step_register_info *p_reg_info,
	struct step_reg_addr_info reg_addr_info, unsigned int offset, unsigned int times, unsigned int delay_time)
{
	p_reg_info->is_write = 0;
	p_reg_info->address_type = reg_addr_info.address_type;
	p_reg_info->address = reg_addr_info.address;
	p_reg_info->offset = offset;
	p_reg_info->times = times;
	p_reg_info->delay_time = delay_time;
	p_reg_info->output_mode = STEP_OUTPUT_LOG;
}

static void conninfra_step_create_write_register_action(struct step_register_info *p_reg_info,
	struct step_reg_addr_info reg_addr_info, unsigned int offset, int value, int mask)
{
	p_reg_info->is_write = 1;
	p_reg_info->address_type = reg_addr_info.address_type;
	p_reg_info->address = reg_addr_info.address;
	p_reg_info->offset = offset;
	p_reg_info->value = value;
	p_reg_info->mask = mask;
}

static int _conninfra_step_create_register_action(struct step_register_info *p_reg_info,
	int param_num, char *params[])
{
	long write;
	struct step_reg_addr_info reg_addr_info;
	long offset, value;
	unsigned int reg_id = 0;
	char buf[128] = "";
	long mask = 0xFFFFFFFF;
	long times = 1;
	long delay_time = 0;

	if (param_num < 4) {
		pr_err("STEP failed: Register no params\n");
		return -1;
	}

	if (osal_strtol(params[0], 0, &write)) {
		pr_err("STEP failed: str to value %s\n",
			params[0]);
		return -1;
	}

	if (osal_strtol(params[2], 0, &offset)) {
		pr_err("STEP failed: str to value %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return -1;
	}

	pr_info("[%s] == offset=[%x]", __func__, offset);

	if (conninfra_step_parse_register_address(&reg_addr_info, params[1], offset) == -1) {
		pr_err("STEP failed: init write register symbol: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return -1;
	}

	if (write == 0) {
		if (*params[(param_num - 1)] == STEP_TEMP_REGISTER_SYMBOL) {
			reg_id = conninfra_step_parse_temp_register_id(params[(param_num - 1)]);
			if (reg_id >= STEP_PARAMETER_SIZE) {
				pr_err("STEP failed: register id failed: %s\n",
					conninfra_step_save_params_msg(param_num, params, buf, 128));
				return -1;
			}

			if (param_num > 4) {
				if (osal_strtol(params[3], 0, &mask)) {
					pr_err("STEP failed: str to value %s\n",
						conninfra_step_save_params_msg(param_num, params, buf, 128));
					return -1;
				}
			}

			conninfra_step_create_read_register_output_register(p_reg_info, reg_addr_info, offset, mask, reg_id);
		} else {
			if (param_num < 5 ||
				osal_strtol(params[3], 0, &times) ||
				osal_strtol(params[4], 0, &delay_time)) {
				pr_err("STEP failed: str to value %s\n",
					conninfra_step_save_params_msg(param_num, params, buf, 128));
				return -1;
			}

			conninfra_step_create_read_register_output_log(p_reg_info, reg_addr_info, offset, times, delay_time);
		}
	} else {
		if (osal_strtol(params[3], 0, &value)) {
			pr_err("STEP failed: str to value %s\n",
				conninfra_step_save_params_msg(param_num, params, buf, 128));
			return -1;
		}

		if (param_num > 4) {
			if (osal_strtol(params[4], 0, &mask)) {
				pr_err("STEP failed: str to value %s\n",
					conninfra_step_save_params_msg(param_num, params, buf, 128));
				return -1;
			}
		}

		conninfra_step_create_write_register_action(p_reg_info, reg_addr_info, offset, value, mask);
	}

	return 0;
}


/*
 * Support:
 * _REG | R(0) | Pre-define base address ID | offset | times | delay time(ms)
 * _REG | R(0) | AP Physical address        | offset | times | delay time(ms)
 * _REG | R(0) | Pre-define base address ID | offset | mask  | Output temp register ID ($)
 * _REG | R(0) | AP Physical address        | offset | mask  | Output temp register ID ($)
 * _REG | R(0) | Pre-define base address ID | offset | Output temp register ID ($)
 * _REG | R(0) | AP Physical address        | offset | Output temp register ID ($)
 * _REG | W(1) | AP Physical address        | offset | value
 * _REG | W(1) | AP Physical address        | offset | value
 * _REG | W(1) | AP Physical address        | offset | value | mask
 * _REG | W(1) | AP Physical address        | offset | value | mask
 */
struct step_action *conninfra_step_create_register_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_register_action *p_reg_act = NULL;
	struct step_register_info *p_reg_info;
	int ret;

	p_reg_act = kzalloc(sizeof(struct step_register_action), GFP_KERNEL);
	if (p_reg_act == NULL) {
		pr_err("STEP failed: kzalloc register fail\n");
		return NULL;
	}

	p_reg_info = &p_reg_act->info;
	ret = _conninfra_step_create_register_action(p_reg_info, param_num, params);

	if (ret != 0) {
		kfree(p_reg_act);
		return NULL;
	}
	p_reg_info->drv_type = drv_type;

	return &(p_reg_act->base);
}


/*
 * Support:
 * GPIO | R(0) | Pin number
 */
struct step_action *conninfra_step_create_gpio_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_gpio_action *p_gpio_act = NULL;
	long write, symbol;
	char buf[128] = "";

	if (param_num != 2) {
		pr_err("STEP failed: init gpio param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	if (osal_strtol(params[0], 0, &write) ||
		osal_strtol(params[1], 0, &symbol)) {
		pr_err("STEP failed: str to value %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	p_gpio_act = kzalloc(sizeof(struct step_gpio_action), GFP_KERNEL);
	if (p_gpio_act == NULL) {
		pr_err("STEP failed: kzalloc gpio fail\n");
		return NULL;
	}
	p_gpio_act->is_write = write;
	p_gpio_act->pin_symbol = symbol;

	return &(p_gpio_act->base);
}

#if 0
/*
 * Support:
 * _RST
 */
struct step_action *conninfra_step_create_chip_reset_action(int param_num, char *params[])
{
	struct step_chip_reset_action *p_crst_act = NULL;

	p_crst_act = kzalloc(sizeof(struct step_chip_reset_action), GFP_KERNEL);
	if (p_crst_act == NULL) {
		pr_err("STEP failed: kzalloc chip reset fail\n");
		return NULL;
	}

	return &(p_crst_act->base);
}
#endif

/*
 * Support:
 * [PD+] | ms
 */
struct step_action *conninfra_step_create_periodic_dump_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_periodic_dump_action *p_pd_act = NULL;

	if (params[0] == NULL) {
		pr_err("STEP failed: param null\n");
		return NULL;
	}

	p_pd_act = kzalloc(sizeof(struct step_periodic_dump_action), GFP_KERNEL);
	if (p_pd_act == NULL) {
		pr_err("STEP failed: kzalloc fail\n");
		return NULL;
	}

	p_pd_act->pd_entry = (struct step_pd_entry *)params[0];
	return &(p_pd_act->base);
}

/*
 * Support:
 * SHOW | Message (no space)
 */
struct step_action *conninfra_step_create_show_string_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_show_string_action *p_show_act = NULL;
	char buf[128] = "";

	if (param_num != 1) {
		pr_err("STEP failed: init show param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	p_show_act = kzalloc(sizeof(struct step_show_string_action), GFP_KERNEL);
	if (p_show_act == NULL) {
		pr_err("STEP failed: kzalloc show string fail\n");
		return NULL;
	}

	p_show_act->content = kzalloc((osal_strlen(params[0]) + 1), GFP_KERNEL);
	if (p_show_act->content == NULL) {
		pr_err("STEP failed: kzalloc show string content fail\n");
		kfree(p_show_act);
		return NULL;
	}
	osal_memcpy(p_show_act->content, params[0], osal_strlen(params[0]));
	return &(p_show_act->base);
}

/*
 * Support:
 * _SLP | time (ms)
 */
struct step_action *conninfra_step_create_sleep_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_sleep_action *p_sleep_act = NULL;
	long ms;
	char buf[128] = "";

	if (param_num != 1) {
		pr_err("STEP failed: init sleep param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	if (osal_strtol(params[0], 0, &ms)) {
		pr_err("STEP failed: str to value %s\n",
			params[0]);
		return NULL;
	}

	p_sleep_act = kzalloc(sizeof(struct step_sleep_action), GFP_KERNEL);
	if (p_sleep_act == NULL) {
		pr_err("STEP failed: kzalloc sleep fail\n");
		return NULL;
	}
	p_sleep_act->ms = ms;

	return &(p_sleep_act->base);
}

/*
 * Support:
 * COND | Check temp register ID ($) | Left temp register ID ($) | Operator | Right temp register ID ($)
 * COND | Check temp register ID ($) | Left temp register ID ($) | Operator | value
 */
struct step_action *conninfra_step_create_condition_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_condition_action *p_cond_act = NULL;
	unsigned int res_reg_id, l_reg_id, r_reg_id = 0;
	long value = 0;
	int mode;
	enum step_condition_operator_id op_id;
	char buf[128] = "";

	if (param_num != 4) {
		pr_err("STEP failed: init sleep param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	res_reg_id = conninfra_step_parse_temp_register_id(params[0]);
	l_reg_id = conninfra_step_parse_temp_register_id(params[1]);
	if (res_reg_id >= STEP_PARAMETER_SIZE || l_reg_id >= STEP_PARAMETER_SIZE) {
		pr_err("STEP failed: register id failed: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	op_id = conninfra_step_parse_operator_id(params[2]);
	if (op_id >= STEP_OPERATOR_MAX) {
		pr_err("STEP failed: operator id failed: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	if (*params[(param_num - 1)] == STEP_TEMP_REGISTER_SYMBOL) {
		r_reg_id = conninfra_step_parse_temp_register_id(params[3]);
		mode = STEP_CONDITION_RIGHT_REGISTER;
		if (r_reg_id >= STEP_PARAMETER_SIZE) {
			pr_err("STEP failed: register id failed: %s\n",
				conninfra_step_save_params_msg(param_num, params, buf, 128));
			return NULL;
		}
	} else {
		if (osal_strtol(params[3], 0, &value)) {
			pr_err("STEP failed: str to value %s\n",
				conninfra_step_save_params_msg(param_num, params, buf, 128));
			return NULL;
		}
		mode = STEP_CONDITION_RIGHT_VALUE;
	}

	p_cond_act = kzalloc(sizeof(struct step_condition_action), GFP_KERNEL);
	if (p_cond_act == NULL) {
		pr_err("STEP failed: kzalloc condition fail\n");
		return NULL;
	}

	p_cond_act->result_temp_reg_id = res_reg_id;
	p_cond_act->l_temp_reg_id = l_reg_id;
	p_cond_act->operator_id = op_id;
	p_cond_act->r_temp_reg_id = r_reg_id;
	p_cond_act->value = value;
	p_cond_act->mode = mode;

	return &(p_cond_act->base);
}

/*
 * Support:
 * _VAL | Save temp register ID ($) | Value
 */
struct step_action *conninfra_step_create_value_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_value_action *p_val_act = NULL;
	unsigned int reg_id;
	long value;
	char buf[128] = "";

	if (param_num != 2) {
		pr_err("STEP failed: init sleep param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	reg_id = conninfra_step_parse_temp_register_id(params[0]);
	if (reg_id >= STEP_PARAMETER_SIZE) {
		pr_err("STEP failed: register id failed: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	if (osal_strtol(params[1], 0, &value)) {
		pr_err("STEP failed: str to value %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	p_val_act = kzalloc(sizeof(struct step_value_action), GFP_KERNEL);
	if (p_val_act == NULL) {
		pr_err("STEP failed: kzalloc value fail\n");
		return NULL;
	}

	p_val_act->temp_reg_id = reg_id;
	p_val_act->value = value;

	return &(p_val_act->base);
}

/*
 * Support:
 * CEMI | Check temp register ID (#) | R(0) | Begin offset | End offset
 * CEMI | Check temp register ID (#) | R(0) | Begin offset | mask | Output temp register ID ($)
 * CEMI | Check temp register ID (#) | R(0) | Begin offset | Output temp register ID ($)
 */
struct step_action *conninfra_step_create_condition_emi_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_condition_emi_action *p_cond_emi_act = NULL;
	struct step_emi_info *p_emi_info = NULL;
	unsigned int reg_id;
	char buf[128] = "";
	int ret;

	if (param_num < 1) {
		pr_err("STEP failed: EMI no params\n");
		return NULL;
	}

	reg_id = conninfra_step_parse_temp_register_id(params[0]);
	if (reg_id >= STEP_PARAMETER_SIZE) {
		pr_err("STEP failed: condition register id failed: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	p_cond_emi_act = kzalloc(sizeof(struct step_condition_emi_action), GFP_KERNEL);
	if (p_cond_emi_act == NULL) {
		pr_err("STEP failed: kzalloc condition emi fail\n");
		return NULL;
	}

	p_emi_info = &p_cond_emi_act->info;
	p_cond_emi_act->cond_reg_id = reg_id;
	ret = _conninfra_step_create_emi_action(p_emi_info, param_num - 1, &params[1]);

	if (ret != 0) {
		kfree(p_cond_emi_act);
		return NULL;
	}

	return &(p_cond_emi_act->base);
}

/*
 * Support:
 * CREG | Check temp register ID (#) | R(0) | Pre-define base address ID | offset | times | delay time(ms)
 * CREG | Check temp register ID (#) | R(0) | AP Physical address        | offset | times | delay time(ms)
 * CREG | Check temp register ID (#) | R(0) | Pre-define base address ID | offset | mask  | Output temp register ID ($)
 * CREG | Check temp register ID (#) | R(0) | AP Physical address        | offset | mask  | Output temp register ID ($)
 * CREG | Check temp register ID (#) | R(0) | Pre-define base address ID | offset | Output temp register ID ($)
 * CREG | Check temp register ID (#) | R(0) | AP Physical address        | offset | Output temp register ID ($)
 * CREG | Check temp register ID (#) | W(1) | AP Physical address        | offset | value
 * CREG | Check temp register ID (#) | W(1) | AP Physical address        | offset | value
 * CREG | Check temp register ID (#) | W(1) | AP Physical address        | offset | value | mask
 * CREG | Check temp register ID (#) | W(1) | AP Physical address        | offset | value | mask
 */
struct step_action *conninfra_step_create_condition_register_action(enum step_drv_type drv_type, int param_num, char *params[])
{
	struct step_condition_register_action *p_cond_reg_act = NULL;
	struct step_register_info *p_reg_info;
	unsigned int reg_id;
	char buf[128] = "";
	int ret;

	if (param_num < 0) {
		pr_err("STEP failed: Init EMI param(%d): %s\n", param_num,
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	reg_id = conninfra_step_parse_temp_register_id(params[0]);
	if (reg_id >= STEP_PARAMETER_SIZE) {
		pr_err("STEP failed: condition register id failed: %s\n",
			conninfra_step_save_params_msg(param_num, params, buf, 128));
		return NULL;
	}

	p_cond_reg_act = kzalloc(sizeof(struct step_condition_register_action), GFP_KERNEL);
	if (p_cond_reg_act == NULL) {
		pr_err("STEP failed: kzalloc condition register fail\n");
		return NULL;
	}

	p_reg_info = &p_cond_reg_act->info;
	p_cond_reg_act->cond_reg_id = reg_id;
	ret = _conninfra_step_create_register_action(p_reg_info, param_num - 1, &params[1]);

	if (ret != 0) {
		kfree(p_cond_reg_act);
		return NULL;
	}

	p_reg_info->drv_type = drv_type;

	return &(p_cond_reg_act->base);
}



/*******************************************************************************
 *              I N T E R N A L   F U N C T I O N S   W I T H   U T
********************************************************************************/

