/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "conninfra_step.h"
#include "conninfra_step_test.h"
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <asm/io.h>
#include "emi_mng.h"
#include "consys_hw.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"


void conninfra_step_test_update_result(int result, struct step_test_report *p_report, char *err_result)
{
	if (result != TEST_FAIL) {
		p_report->pass++;
	} else {
		pr_err("%s", err_result);
		p_report->fail++;
	}
}

void conninfra_step_test_update_result_report(struct step_test_report *p_dest_report,
	struct step_test_report *p_src_report)
{
	p_dest_report->pass += p_src_report->pass;
	p_dest_report->fail += p_src_report->fail;
	p_dest_report->check += p_src_report->check;
}

void conninfra_step_test_show_result_report(char *test_name, struct step_test_report *p_report, int sec_begin, int usec_begin,
	int sec_end, int usec_end)
{
	unsigned int total = 0;
	unsigned int pass = 0;
	unsigned int fail = 0;
	unsigned int check = 0;
	int sec = 0;
	int usec = 0;

	pass = p_report->pass;
	fail = p_report->fail;
	check = p_report->check;

	if (usec_end >= usec_begin) {
		sec = sec_end - sec_begin;
		usec = usec_end - usec_begin;
	} else {
		sec = sec_end - sec_begin - 1;
		usec = usec_end - usec_begin + 1000000;
	}

	total = pass + fail + check;
	pr_info("%s Total: %d, PASS: %d, FAIL: %d, CHECK: %d, Spend Time: %d.%.6d\n",
		test_name, total, pass, fail, check, sec, usec);
}

static int conninfra_step_test_check_create_read_reg(struct step_register_info *p_reg_info,
	int check_params[], char *err_result, int check_index)
{
	int result = TEST_FAIL;

	if (p_reg_info->address_type == 0 /*STEP_REGISTER_PHYSICAL_ADDRESS*/) {
		if (p_reg_info->address != check_params[check_index + 1] ||
			p_reg_info->offset != check_params[check_index + 2]) {
			pr_err(
				"%s, C1 reg params: %d, %p, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address,
				p_reg_info->offset);
			return TEST_FAIL;
		}
	} else {
		if (p_reg_info->address_type != check_params[check_index + 1] ||
			p_reg_info->offset != check_params[check_index + 2]) {
			pr_err(
				"%s, C2 reg params: %d, %p, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address,
				p_reg_info->offset);
			return TEST_FAIL;
		}
	}

	if (p_reg_info->output_mode == STEP_OUTPUT_LOG) {
		if (p_reg_info->times != check_params[check_index + 3] ||
			p_reg_info->delay_time != check_params[check_index + 4]) {
			pr_err(
				"%s, C3 reg params: %d, %p, %d, %d, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address,
				p_reg_info->offset, p_reg_info->times, p_reg_info->delay_time);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else if (p_reg_info->output_mode == STEP_OUTPUT_REGISTER) {
		if (p_reg_info->mask != check_params[check_index + 3] ||
			p_reg_info->temp_reg_id != check_params[check_index + 4]) {
			pr_err(
				"%s, C4 reg params: %d, %p, %d, %d, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address,
				p_reg_info->offset, p_reg_info->mask, p_reg_info->temp_reg_id);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else {
		result = TEST_FAIL;
	}

	return result;
}

static int conninfra_step_test_check_create_write_reg(struct step_register_info *p_reg_info,
	int check_params[], char *err_result, int check_index)
{
	int result = TEST_FAIL;

	if (p_reg_info->address_type == 0 /*STEP_REGISTER_PHYSICAL_ADDRESS*/) {
		if (p_reg_info->address != check_params[check_index + 1] ||
			p_reg_info->offset != check_params[check_index + 2] ||
			p_reg_info->value != check_params[check_index + 3]) {
			pr_err(
				"%s, C1 reg params: %d, %p, %d, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address,
				p_reg_info->offset, p_reg_info->value);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else {
		if (p_reg_info->address_type != check_params[check_index + 1] ||
			p_reg_info->offset != check_params[check_index + 2] ||
			p_reg_info->value != check_params[check_index + 3]) {
			pr_err(
				"%s, C2 reg params: %d, %p, %d, %d\n",
				err_result, p_reg_info->is_write, p_reg_info->address_type,
				p_reg_info->offset, p_reg_info->value);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	}

	return result;
}

int conninfra_step_test_check_create_emi(struct step_emi_action *p_emi_act, int check_params[],
	char *err_result)
{
	struct step_emi_info *p_emi_info = NULL;
	int result = TEST_FAIL;

	p_emi_info = &p_emi_act->info;
	if (p_emi_act->base.action_id != STEP_ACTION_INDEX_EMI) {
		pr_err("%s, id wrong: %d\n", err_result, p_emi_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_emi_info->output_mode == STEP_OUTPUT_LOG) {
		if (p_emi_info->is_write != check_params[0] ||
			p_emi_info->begin_offset != check_params[1] ||
			p_emi_info->end_offset != check_params[2]) {
			pr_err("%s, C1 emi log params: %d, %d, %d\n",
				err_result, p_emi_info->is_write, p_emi_info->begin_offset,
				p_emi_info->end_offset);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else if (p_emi_info->output_mode == STEP_OUTPUT_REGISTER) {
		if (p_emi_info->is_write != check_params[0] ||
			p_emi_info->begin_offset != check_params[1] ||
			p_emi_info->mask != check_params[2] ||
			p_emi_info->temp_reg_id != check_params[3]) {
			pr_err("%s, C2 emi reg params: %d, %d, %d, %d\n",
				err_result, p_emi_info->is_write, p_emi_info->begin_offset,
				p_emi_info->mask, p_emi_info->temp_reg_id);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else {
		result = TEST_FAIL;
	}

	return result;
}

int conninfra_step_test_check_create_reg(struct step_register_action *p_reg_act, int check_params[],
	char *err_result)
{
	struct step_register_info *p_reg_info = NULL;
	int result = TEST_FAIL;

	p_reg_info = &p_reg_act->info;
	if (p_reg_act->base.action_id != STEP_ACTION_INDEX_REGISTER) {
		pr_err("%s, id wrong: %d\n", err_result, p_reg_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_reg_info->is_write != check_params[0]) {
		pr_err("%s, write failed: %d expect(%d)", err_result, p_reg_info->is_write, check_params[0]);
		result = TEST_FAIL;
	} else {
		if (p_reg_info->is_write == 0)
			result = conninfra_step_test_check_create_read_reg(p_reg_info, check_params, err_result, 0);
		else
			result = conninfra_step_test_check_create_write_reg(p_reg_info, check_params, err_result, 0);
	}

	return result;
}



int conninfra_step_test_check_create_condition_emi(struct step_condition_emi_action *p_cond_emi_act, int check_params[],
	char *err_result)
{
	struct step_emi_info *p_emi_info = NULL;
	int result = TEST_FAIL;

	p_emi_info = &p_cond_emi_act->info;
	if (p_cond_emi_act->base.action_id != STEP_ACTION_INDEX_CONDITION_EMI) {
		pr_err("%s, id wrong: %d\n", err_result, p_cond_emi_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_emi_info->output_mode == STEP_OUTPUT_LOG) {
		if (p_cond_emi_act->cond_reg_id != check_params[0] ||
			p_emi_info->is_write != check_params[1] ||
			p_emi_info->begin_offset != check_params[2] ||
			p_emi_info->end_offset != check_params[3]) {
			pr_err("%s, C1 emi log params: %d, %d, %d, %d\n",
				err_result, p_cond_emi_act->cond_reg_id, p_emi_info->is_write,
				p_emi_info->begin_offset, p_emi_info->end_offset);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else if (p_emi_info->output_mode == STEP_OUTPUT_REGISTER) {
		if (p_cond_emi_act->cond_reg_id != check_params[0] ||
			p_emi_info->is_write != check_params[1] ||
			p_emi_info->begin_offset != check_params[2] ||
			p_emi_info->mask != check_params[3] ||
			p_emi_info->temp_reg_id != check_params[4]) {
			pr_err("%s, C2 emi reg params: %d, %d, %d, %d, %d\n",
				err_result, p_cond_emi_act->cond_reg_id, p_emi_info->is_write,
				p_emi_info->begin_offset, p_emi_info->mask, p_emi_info->temp_reg_id);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	} else {
		result = TEST_FAIL;
	}

	return result;
}

int conninfra_step_test_check_create_condition_reg(struct step_condition_register_action *p_cond_reg_act, int check_params[],
	char *err_result)
{
	struct step_register_info *p_reg_info = NULL;
	int result = TEST_FAIL;

	p_reg_info = &p_cond_reg_act->info;
	if (p_cond_reg_act->base.action_id != STEP_ACTION_INDEX_CONDITION_REGISTER) {
		pr_err("%s, id wrong: %d\n", err_result, p_cond_reg_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_cond_reg_act->cond_reg_id != check_params[0]) {
		pr_err("%s, reg id failed: %d expect(%d)", err_result,
			p_cond_reg_act->cond_reg_id, check_params[0]);
		result = TEST_FAIL;
	} else if (p_reg_info->is_write != check_params[1]) {
		pr_err("%s, write failed: %d expect(%d)", err_result,
			p_reg_info->is_write, check_params[1]);
		result = TEST_FAIL;
	} else {
		if (p_reg_info->is_write == 0)
			result = conninfra_step_test_check_create_read_reg(p_reg_info, check_params, err_result, 1);
		else
			result = conninfra_step_test_check_create_write_reg(p_reg_info, check_params, err_result, 1);
	}

	return result;
}

int conninfra_step_test_check_create_gpio(struct step_gpio_action *p_gpio_act, int check_params[],
	char *err_result)
{
	int result = TEST_FAIL;

	if (p_gpio_act->base.action_id != STEP_ACTION_INDEX_GPIO) {
		pr_err("%s, id wrong: %d\n", err_result, p_gpio_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_gpio_act->is_write != check_params[0]) {
		pr_err("%s, write failed: %d", err_result, p_gpio_act->is_write);
		result = TEST_FAIL;
	} else {
		if (p_gpio_act->pin_symbol != check_params[1]) {
			pr_err("%s, gpio params: %d, %d\n",
			err_result, p_gpio_act->is_write, p_gpio_act->pin_symbol);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	}

	return result;
}

int conninfra_step_test_check_create_periodic_dump(struct step_periodic_dump_action *p_pd_act,
	int check_params[], char *err_result)
{
	int result = TEST_FAIL;

	if (p_pd_act->base.action_id != STEP_ACTION_INDEX_PERIODIC_DUMP) {
		pr_err("%s, id wrong: %d\n", err_result, p_pd_act->base.action_id);
		result = TEST_FAIL;
	} else {
		result = TEST_PASS;
	}

	return result;
}

int conninfra_step_test_check_create_show_string(struct step_show_string_action *p_show_act,
	int check_params[], char *err_result)
{
	int result = TEST_FAIL;

	if (p_show_act->base.action_id != STEP_ACTION_INDEX_SHOW_STRING) {
		pr_err("%s, id wrong: %d\n", err_result, p_show_act->base.action_id);
		result = TEST_FAIL;
	} else {
		result = TEST_PASS;
	}

	return result;
}

int conninfra_step_test_check_create_sleep(struct step_sleep_action *p_sleep_act,
	int check_params[], char *err_result)
{
	int result = TEST_FAIL;

	if (p_sleep_act->base.action_id != STEP_ACTION_INDEX_SLEEP) {
		pr_err("%s, id wrong: %d\n", err_result, p_sleep_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_sleep_act->ms != check_params[0]) {
		pr_err("%s, param failed: %d expect(%d)", err_result, p_sleep_act->ms, check_params[0]);
		result = TEST_FAIL;
	} else {
		result = TEST_PASS;
	}

	return result;
}

int conninfra_step_test_check_create_condition(struct step_condition_action *p_cond_act,
	int check_params[], char *err_result)
{
	int result = TEST_FAIL;

	if (p_cond_act->base.action_id != STEP_ACTION_INDEX_CONDITION) {
		pr_err("%s, id wrong: %d\n", err_result, p_cond_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_cond_act->result_temp_reg_id != check_params[0] ||
		p_cond_act->l_temp_reg_id != check_params[1] ||
		p_cond_act->operator_id != check_params[2]) {
		pr_err("%s, C1 param failed: %d %d %d %d expect(%d %d %d %d)",
			err_result, p_cond_act->result_temp_reg_id, p_cond_act->l_temp_reg_id,
			p_cond_act->operator_id, p_cond_act->r_temp_reg_id,
			check_params[0], check_params[1], check_params[2], check_params[3]);
		result = TEST_FAIL;
	} else {
		if (p_cond_act->mode == STEP_CONDITION_RIGHT_REGISTER && p_cond_act->r_temp_reg_id != check_params[3]) {
			pr_err("%s, C2 param failed: %d %d %d %d expect(%d %d %d %d)",
			err_result, p_cond_act->result_temp_reg_id, p_cond_act->l_temp_reg_id,
			p_cond_act->operator_id, p_cond_act->r_temp_reg_id,
			check_params[0], check_params[1], check_params[2], check_params[3]);
			result = TEST_FAIL;
		} else if (p_cond_act->mode == STEP_CONDITION_RIGHT_VALUE && p_cond_act->value != check_params[3]) {
			pr_err("%s, C3 param failed: %d %d %d %d expect(%d %d %d %d)",
			err_result, p_cond_act->result_temp_reg_id, p_cond_act->l_temp_reg_id,
			p_cond_act->operator_id, p_cond_act->value,
			check_params[0], check_params[1], check_params[2], check_params[3]);
			result = TEST_FAIL;
		} else {
			result = TEST_PASS;
		}
	}

	return result;
}

int conninfra_step_test_check_create_value(struct step_value_action *p_val_act,
	int check_params[], char *err_result)
{
	int result = TEST_FAIL;

	if (p_val_act->base.action_id != STEP_ACTION_INDEX_VALUE) {
		pr_err("%s, id wrong: %d\n", err_result, p_val_act->base.action_id);
		result = TEST_FAIL;
	} else if (p_val_act->temp_reg_id != check_params[0] ||
		p_val_act->value != check_params[1]) {
		pr_err("%s, param failed: %d %d expect(%d %d)",
			err_result, p_val_act->temp_reg_id, p_val_act->value,
			check_params[0], check_params[1]);
		result = TEST_FAIL;
	} else {
		result = TEST_PASS;
	}

	return result;
}

void conninfra_step_test_create_action(enum step_action_id act_id, int param_num, char *params[], int result_of_action,
	int check_params[], struct step_test_report *p_report, char *err_result)
{
	struct step_action *p_act = NULL;
	int result = TEST_FAIL;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		switch (p_act->action_id) {
		case STEP_ACTION_INDEX_EMI:
			{
				struct step_emi_action *p_emi_act = NULL;

				p_emi_act = clist_entry_action(emi, p_act);
				result = conninfra_step_test_check_create_emi(p_emi_act, check_params,
					err_result);
			}
			break;
		case STEP_ACTION_INDEX_REGISTER:
			{
				struct step_register_action *p_reg_act = NULL;

				p_reg_act = clist_entry_action(register, p_act);
				result = conninfra_step_test_check_create_reg(p_reg_act, check_params,
					err_result);
			}
			break;
		case STEP_ACTION_INDEX_GPIO:
			{
				struct step_gpio_action *p_gpio_act = NULL;

				p_gpio_act = clist_entry_action(gpio, p_act);
				result = conninfra_step_test_check_create_gpio(p_gpio_act, check_params,
					err_result);
			}
			break;
		case STEP_ACTION_INDEX_PERIODIC_DUMP:
			{
				struct step_periodic_dump_action *p_pd_act = NULL;

				p_pd_act = clist_entry_action(periodic_dump, p_act);
				result = conninfra_step_test_check_create_periodic_dump(p_pd_act,
					check_params, err_result);
			}
			break;
		case STEP_ACTION_INDEX_SHOW_STRING:
			{
				struct step_show_string_action *p_show_act = NULL;

				p_show_act = clist_entry_action(show_string, p_act);
				result = conninfra_step_test_check_create_show_string(p_show_act,
					check_params, err_result);
			}
			break;
		case STEP_ACTION_INDEX_SLEEP:
			{
				struct step_sleep_action *p_sleep_act = NULL;

				p_sleep_act = clist_entry_action(sleep, p_act);
				result = conninfra_step_test_check_create_sleep(p_sleep_act,
					check_params, err_result);
			}
			break;
		case STEP_ACTION_INDEX_CONDITION:
			{
				struct step_condition_action *p_cond_act = NULL;

				p_cond_act = clist_entry_action(condition, p_act);
				result = conninfra_step_test_check_create_condition(p_cond_act,
					check_params, err_result);
			}
			break;
		case STEP_ACTION_INDEX_VALUE:
			{
				struct step_value_action *p_val_act = NULL;

				p_val_act = clist_entry_action(value, p_act);
				result = conninfra_step_test_check_create_value(p_val_act,
					check_params, err_result);
			}
			break;
		case STEP_ACTION_INDEX_CONDITION_EMI:
			{
				struct step_condition_emi_action *p_cond_emi_act = NULL;

				p_cond_emi_act = clist_entry_action(condition_emi, p_act);
				result = conninfra_step_test_check_create_condition_emi(p_cond_emi_act, check_params,
					err_result);
			}
			break;
		case STEP_ACTION_INDEX_CONDITION_REGISTER:
			{
				struct step_condition_register_action *p_cond_reg_act = NULL;

				p_cond_reg_act = clist_entry_action(condition_register, p_act);
				result = conninfra_step_test_check_create_condition_reg(p_cond_reg_act, check_params,
					err_result);
			}
			break;
		default:
			result = TEST_FAIL;
			break;
		}

		if (result_of_action == -1)
			result = TEST_FAIL;

		conninfra_step_remove_action(p_act);
	} else {
		if (result_of_action == -1)
			result = TEST_PASS;
		else
			result = TEST_FAIL;
	}
	conninfra_step_test_update_result(result, p_report, err_result);
}

/* this offset only for FPGA */
#define CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET 0x4f000
/* formal chip use this offset*/
//#define CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET 0x27f000

int conninfra_step_test_get_emi_offset(unsigned char buf[], int offset)
{
	struct consys_emi_addr_info *emi_phy_addr = NULL;

	emi_phy_addr = emi_mng_get_phy_addr();
	if (emi_phy_addr != NULL) {
		/* TODO: fix this, change the region to coredump */
		snprintf(buf, 11, "0x%08x", (CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET + offset));
	} else {
		pr_err("STEP test failed: emi_phy_addr is NULL\n");
		return -1;
	}
	return 0;
}

static unsigned char __iomem *emi_map_addr = NULL;

unsigned char* conninfra_step_test_get_emi_virt_offset(unsigned int offset)
{
	struct consys_emi_addr_info *emi_addr_info = NULL;

	if (emi_map_addr == NULL) {
		emi_addr_info = emi_mng_get_phy_addr();
		if (emi_addr_info != NULL) {
			emi_map_addr = ioremap_nocache(emi_addr_info->emi_ap_phy_addr +
							CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET + offset,
							0x100);
		}
	}
	return emi_map_addr;
}

void conninfra_step_test_put_emi_virt_offset(void)
{
	if (emi_map_addr != NULL) {
		iounmap(emi_map_addr);
		emi_map_addr = NULL;
	}
}

