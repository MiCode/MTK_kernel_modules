/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/slab.h>
#include "osal.h"
#include "conninfra_step_test.h"
#include "conninfra_step.h"
#include "emi_mng.h"
#include "consys_hw.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"


#define TEST_WRITE_CR_TEST_CASE 0

struct step_test_check g_conninfra_step_test_check;


static void conninfra_step_test_create_emi_action(struct step_test_report *p_report);
static void conninfra_step_test_create_cond_emi_action(struct step_test_report *p_report);
static void conninfra_step_test_create_register_action(struct step_test_report *p_report);
static void conninfra_step_test_create_cond_register_action(struct step_test_report *p_report);
static void conninfra_step_test_check_register_symbol(struct step_test_report *p_report);
static void conninfra_step_test_create_other_action(struct step_test_report *p_report);
static void conninfra_step_test_do_emi_action(struct step_test_report *p_report);
static void conninfra_step_test_do_cond_emi_action(struct step_test_report *p_report);
static void conninfra_step_test_do_register_action(struct step_test_report *p_report);
static void conninfra_step_test_do_cond_register_action(struct step_test_report *p_report);
static void conninfra_step_test_do_gpio_action(struct step_test_report *p_report);
static void conninfra_step_test_create_periodic_dump(struct step_test_report *p_report);
static void conninfra_step_test_do_show_action(struct step_test_report *p_report);
static void conninfra_step_test_do_sleep_action(struct step_test_report *p_report);
static void conninfra_step_test_do_condition_action(struct step_test_report *p_report);
static void conninfra_step_test_do_value_action(struct step_test_report *p_report);

//static void conninfra_step_test_parse_data3(struct step_test_report *p_report);
static void conninfra_step_test_parse_data1(struct step_test_report *p_report);
static void conninfra_step_test_parse_data2(struct step_test_report *p_report);

void conninfra_step_test_clear_parameter(char *params[])
{
	int i = 0;

	for (i = 0; i < STEP_PARAMETER_SIZE; i++)
		params[i] = NULL;
}

#define index_of_array(current_addr, base_addr, type) \
	(((unsigned long)current_addr - (unsigned long)base_addr) / sizeof(type))


int conninfra_step_test_check_write_tp2(struct step_action_list *p_act_list, enum step_drv_type drv_type,
	enum step_action_id act_id, int param_num, char *params[])
{
	int index;
	int i;
	int tp_id;
	struct step_test_scenario *cur_scn = g_conninfra_step_test_check.cur_test_scn;

	if (cur_scn == NULL)
		return 0;

	if (g_conninfra_step_test_check.step_check_result == TEST_FAIL)
		return 0;

	index = cur_scn->test_idx;
	cur_scn->test_idx++;

	if (cur_scn->pt_acts[index].tp_id != -1) {

		tp_id = index_of_array(p_act_list, g_step_drv_type_def[drv_type].action_list, struct step_action_list);

		if (tp_id != cur_scn->pt_acts[index].tp_id) {
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
			pr_err("STEP test failed: [%d] drvType=[%d] tp_id %d: expect %d\n", index,
				drv_type, tp_id, cur_scn->pt_acts[index].tp_id);
			return 0;
		}
	}

	if (act_id != cur_scn->pt_acts[index].act_id) {
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		pr_err("STEP test failed: [%d] act_id %d: expect %d\n", index,
			act_id,	cur_scn->pt_acts[index].act_id);
		return 0;
	}

	if (param_num != cur_scn->pt_acts[index].param_num) {
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		pr_err("STEP test failed: [%d] param num %d: expect %d\n", index,
			param_num, cur_scn->pt_acts[index].param_num);
		return 0;
	}

	for (i = 0; i < STEP_PARAMETER_SIZE; i++) {
		if (cur_scn->pt_acts[index].params[i] == NULL || strcmp(cur_scn->pt_acts[index].params[i], "") == 0)
			break;

		if (params[i] == NULL || strcmp(params[i], cur_scn->pt_acts[index].params[i]) != 0) {
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
			pr_err("STEP test failed: [%d] params[%d] %s: expect %s(%d)\n", index, i, params[i],
				cur_scn->pt_acts[index].params[i]);
			return 0;
		}
	}

	g_conninfra_step_test_check.step_check_result = TEST_PASS;
	return 0;
}

void __conninfra_step_test_parse_data2(const char *buf, struct step_test_report *p_report, char *err_result)
{
	struct step_test_scenario *cur_scn = g_conninfra_step_test_check.cur_test_scn;

	conninfra_step_parse_data(buf, osal_strlen((char *)buf), conninfra_step_test_check_write_tp2);
	if (cur_scn->total_test_sz != cur_scn->test_idx) {
		pr_err("STEP test failed: index %d: expect total %d\n",
			cur_scn->total_test_sz, cur_scn->test_idx);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	conninfra_step_test_update_result(g_conninfra_step_test_check.step_check_result, p_report, err_result);
}

void conninfra_step_test_check_emi_act(unsigned int len, ...)
{
	unsigned int offset;
	unsigned int check_result;
	unsigned int value;
	unsigned char *p_virtual_addr = NULL;
	va_list args;

	if (g_conninfra_step_test_check.step_check_result == TEST_FAIL)
		return;

	offset = g_conninfra_step_test_check.step_check_emi_offset[g_conninfra_step_test_check.step_check_index];

	p_virtual_addr = conninfra_step_test_get_emi_virt_offset(offset);
	if (!p_virtual_addr) {
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		pr_err("STEP test failed: p_virtual_addr offset(%d) is null", offset);
		return;
	}
	check_result = CONSYS_REG_READ(p_virtual_addr);

	va_start(args, len);
	value = va_arg(args, unsigned int);
	va_end(args);

	conninfra_step_test_put_emi_virt_offset();

	if (check_result == value) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else {
		pr_err("STEP test failed: check index [%d] Value is %d, expect %d offset=[%x]",
				g_conninfra_step_test_check.step_check_index,
				value, check_result, offset);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		return;
	}

	if (g_conninfra_step_test_check.step_check_temp_register_id != -1) {
		if (g_infra_step_env.temp_register[g_conninfra_step_test_check.step_check_temp_register_id] !=
			(check_result & g_conninfra_step_test_check.step_test_mask)) {
			pr_err("STEP test failed: Register id(%d) value is %d, expect %d mask 0x%08x",
				g_conninfra_step_test_check.step_check_temp_register_id,
				g_infra_step_env.temp_register[g_conninfra_step_test_check.step_check_temp_register_id],
				check_result, g_conninfra_step_test_check.step_test_mask);
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		}
	}

	g_conninfra_step_test_check.step_check_index++;
}

void conninfra_step_test_check_reg_read_act(unsigned int len, ...)
{
	unsigned int check_result;
	unsigned int value;
	va_list args;

	if (g_conninfra_step_test_check.step_check_result == TEST_FAIL)
		return;

	va_start(args, len);
	value = va_arg(args, unsigned int);
	check_result = CONSYS_REG_READ(g_conninfra_step_test_check.step_check_register_addr);

	if (check_result == value) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else {
		pr_err("STEP test failed: Value is %d, expect %d(0x%08x)", value, check_result,
			g_conninfra_step_test_check.step_check_register_addr);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}

	if (g_conninfra_step_test_check.step_check_temp_register_id != -1) {
		if (g_infra_step_env.temp_register[g_conninfra_step_test_check.step_check_temp_register_id] !=
			(check_result & g_conninfra_step_test_check.step_test_mask)) {
			pr_err("STEP test failed: Register id(%d) value is %d, expect %d",
				g_conninfra_step_test_check.step_check_temp_register_id,
				g_infra_step_env.temp_register[g_conninfra_step_test_check.step_check_temp_register_id],
				check_result);
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		}
	}

	va_end(args);
}

void conninfra_step_test_check_reg_write_act(unsigned int len, ...)
{
	unsigned int value;
	va_list args;
	unsigned int mask = g_conninfra_step_test_check.step_test_mask;

	va_start(args, len);
	value = va_arg(args, unsigned int);

	if (value == 0xdeadfeed) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else if (mask == 0xFFFFFFFF) {
		if (g_conninfra_step_test_check.step_check_write_value == value) {
			g_conninfra_step_test_check.step_check_result = TEST_PASS;
		} else {
			pr_err("STEP test failed: Value is %d, expect %d", value,
				g_conninfra_step_test_check.step_check_write_value);
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		}
	} else {
		if ((mask & value) != (mask & g_conninfra_step_test_check.step_check_write_value)) {
			pr_err("STEP test failed: Overrite value: %d, expect value %d origin %d mask %d",
				value,
				g_conninfra_step_test_check.step_check_write_value,
				g_conninfra_step_test_check.step_recovery_value,
				mask);
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		} else if ((~mask & value) != (~mask & g_conninfra_step_test_check.step_recovery_value)) {
			pr_err("STEP test failed: No change value: %d, expect value %d origin %d mask %d",
				value,
				g_conninfra_step_test_check.step_check_write_value,
				g_conninfra_step_test_check.step_recovery_value,
				mask);
			g_conninfra_step_test_check.step_check_result = TEST_FAIL;
		} else {
			g_conninfra_step_test_check.step_check_result = TEST_PASS;
		}
	}

	va_end(args);
}

void conninfra_step_test_check_show_act(unsigned int len, ...)
{
	char *content;
	va_list args;

	va_start(args, len);
	content = va_arg(args, char*);
	if (content == NULL || g_conninfra_step_test_check.step_check_result_string == NULL) {
		pr_err("STEP test failed: content is NULL");
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	if (content != NULL && g_conninfra_step_test_check.step_check_result_string != NULL &&
		osal_strcmp(content, g_conninfra_step_test_check.step_check_result_string) == 0) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else {
		pr_err("STEP test failed: content(%s), expect(%s)",
			content, g_conninfra_step_test_check.step_check_result_string);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	va_end(args);
}

void conninfra_step_test_check_condition(unsigned int len, ...)
{
	int value;
	va_list args;

	va_start(args, len);
	value = va_arg(args, int);
	if (value == g_conninfra_step_test_check.step_check_result_value) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else {
		pr_err("STEP test failed: value(%d), expect(%d)",
			value, g_conninfra_step_test_check.step_check_result_value);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	va_end(args);
}

void conninfra_step_test_check_value_act(unsigned int len, ...)
{
	int value;
	va_list args;

	va_start(args, len);
	value = va_arg(args, int);
	if (value == g_conninfra_step_test_check.step_check_result_value) {
		g_conninfra_step_test_check.step_check_result = TEST_PASS;
	} else {
		pr_err("STEP test failed: value(%d), expect(%d)",
			value, g_conninfra_step_test_check.step_check_result_value);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	va_end(args);
}

void conninfra_step_test_clear_check_data(void)
{
	unsigned int i = 0, j = 0;

	for (i = 0; i < STEP_TEST_ACTION_NUMBER; i++) {
		g_conninfra_step_test_check.step_check_test_tp_id[i] = 0;
		g_conninfra_step_test_check.step_check_test_act_id[i] = 0;
		g_conninfra_step_test_check.step_check_params_num[i] = 0;
		g_conninfra_step_test_check.step_check_emi_offset[i] = 0;
		for (j = 0; j < STEP_PARAMETER_SIZE; j++)
			g_conninfra_step_test_check.step_check_params[i][j] = "";
	}

	g_conninfra_step_test_check.step_check_total = 0;
	g_conninfra_step_test_check.step_check_index = 0;
	g_conninfra_step_test_check.step_check_result = TEST_PASS;
	g_conninfra_step_test_check.step_check_register_addr = 0;
	g_conninfra_step_test_check.step_test_mask = 0xFFFFFFFF;
	g_conninfra_step_test_check.step_recovery_value = 0;
	g_conninfra_step_test_check.step_check_result_value = 0;
	g_conninfra_step_test_check.step_check_temp_register_id = -1;

	g_conninfra_step_test_check.cur_test_scn = NULL;
}

void conninfra_step_test_clear_temp_register(void)
{
	int i;

	for (i = 0; i < STEP_TEMP_REGISTER_SIZE; i++)
		g_infra_step_env.temp_register[i] = 0;
}

#if 0
void __conninfra_step_test_parse_data(const char *buf, struct step_test_report *p_report, char *err_result)
{
	conninfra_step_parse_data(buf, osal_strlen((char *)buf), conninfra_step_test_check_write_tp);
	if (g_conninfra_step_test_check.step_check_total != g_conninfra_step_test_check.step_check_index) {
		pr_err("STEP test failed: index %d: expect total %d\n", g_conninfra_step_test_check.step_check_index,
				g_conninfra_step_test_check.step_check_total);
		g_conninfra_step_test_check.step_check_result = TEST_FAIL;
	}
	conninfra_step_test_update_result(g_conninfra_step_test_check.step_check_result, p_report, err_result);
}
#endif

void __conninfra_step_test_do_emi_action(enum step_action_id act_id, int param_num, char *params[], int result_of_action,
	struct step_test_report *p_report, char *err_result)
{
	struct step_action *p_act = NULL;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		if (conninfra_step_do_emi_action(p_act, conninfra_step_test_check_emi_act) ==
			result_of_action) {
			if (g_conninfra_step_test_check.step_check_total != g_conninfra_step_test_check.step_check_index) {
				p_report->fail++;
				pr_err("%s, index %d: expect total %d\n", err_result,
					g_conninfra_step_test_check.step_check_index, g_conninfra_step_test_check.step_check_total);
			} else if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
				p_report->pass++;
			} else {
				p_report->fail++;
				pr_err("%s\n", err_result);
			}
		} else {
			pr_err("%s, expect result is %d\n", err_result, result_of_action);
			p_report->fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		if (result_of_action == -1) {
			p_report->pass++;
		} else {
			p_report->fail++;
			pr_err("%s, Create failed\n", err_result);
		}
	}
}

void __conninfra_step_test_do_cond_emi_action(enum step_action_id act_id, int param_num, char *params[], int result_of_action,
	struct step_test_report *p_report, char *err_result)
{
	struct step_action *p_act = NULL;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		if (conninfra_step_do_condition_emi_action(p_act, conninfra_step_test_check_emi_act) ==
			result_of_action) {
			if (g_conninfra_step_test_check.step_check_total != g_conninfra_step_test_check.step_check_index) {
				p_report->fail++;
				pr_err("%s, index %d: expect total %d\n", err_result,
					g_conninfra_step_test_check.step_check_index,	g_conninfra_step_test_check.step_check_total);
			} else if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
				p_report->pass++;
			} else {
				p_report->fail++;
				pr_err("%s\n", err_result);
			}
		} else {
			pr_err("%s, expect result is %d\n", err_result, result_of_action);
			p_report->fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		if (result_of_action == -1) {
			p_report->pass++;
		} else {
			p_report->fail++;
			pr_err("%s, Create failed\n", err_result);
		}
	}
}

void __conninfra_step_test_do_register_action(enum step_action_id act_id, int param_num, char *params[], int result_of_action,
	struct step_test_report *p_report, char *err_result)
{
	struct step_action *p_act = NULL;
	int result;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		if (osal_strcmp(params[0], "1") == 0) {
			/* Write register test */
			if (g_conninfra_step_test_check.step_check_register_addr != 0) {
				g_conninfra_step_test_check.step_recovery_value =
					CONSYS_REG_READ(g_conninfra_step_test_check.step_check_register_addr);
			}
			result = conninfra_step_do_register_action(p_act, conninfra_step_test_check_reg_write_act);
			if (result == result_of_action) {
				if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
					p_report->pass++;
				} else {
					p_report->fail++;
					pr_err("%s\n", err_result);
				}
			} else if (result == -2) {
				p_report->check++;
				pr_info("STEP check: %s, no clock is ok?\n", err_result);
			} else {
				pr_err("%s, expect result is %d\n", err_result,
					result_of_action);
				p_report->fail++;
			}
			if (g_conninfra_step_test_check.step_check_register_addr != 0) {
				CONSYS_REG_WRITE(g_conninfra_step_test_check.step_check_register_addr,
					g_conninfra_step_test_check.step_recovery_value);
			}
		} else {
			/* Read register test */
			g_conninfra_step_test_check.step_check_result = TEST_PASS;
			result = conninfra_step_do_register_action(p_act, conninfra_step_test_check_reg_read_act);
			if (result == result_of_action) {
				if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
					p_report->pass++;
				} else {
					p_report->fail++;
					pr_err("%s\n", err_result);
				}
			} else if (result == -2) {
				p_report->check++;
				pr_info("STEP check: %s, no clock is ok?\n", err_result);
			} else {
				pr_err("%s, expect result is %d\n", err_result,
					result_of_action);
				p_report->fail++;
			}
		}
		conninfra_step_remove_action(p_act);
	} else {
		if (result_of_action == -1) {
			p_report->pass++;
		} else {
			p_report->fail++;
			pr_err("%s, Create failed\n", err_result);
		}
	}
}

void __conninfra_step_test_do_cond_register_action(enum step_action_id act_id, int param_num,
	char *params[], int result_of_action,
	struct step_test_report *p_report, char *err_result)
{
	struct step_action *p_act = NULL;
	int result;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	pr_info("[%s] == ", __func__);
	if (p_act != NULL) {
		if (osal_strcmp(params[1], "1") == 0) {
			/* Write register test */
			if (g_conninfra_step_test_check.step_check_register_addr != 0) {
				g_conninfra_step_test_check.step_recovery_value =
					CONSYS_REG_READ(g_conninfra_step_test_check.step_check_register_addr);
			}

			result = conninfra_step_do_condition_register_action(p_act, conninfra_step_test_check_reg_write_act);
			if (result == result_of_action) {
				if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
					p_report->pass++;
				} else {
					p_report->fail++;
					pr_err("%s\n", err_result);
				}
			} else if (result == -2) {
				p_report->check++;
				pr_info("STEP check: %s, no clock is ok?\n", err_result);
			} else {
				pr_err("%s, expect result is %d\n", err_result, result_of_action);
				p_report->fail++;
			}
			if (g_conninfra_step_test_check.step_check_register_addr != 0) {
				CONSYS_REG_WRITE(g_conninfra_step_test_check.step_check_register_addr,
					g_conninfra_step_test_check.step_recovery_value);
			}
		} else {
			pr_info("[%s] == read register test ", __func__);
			/* Read register test */
			g_conninfra_step_test_check.step_check_result = TEST_PASS;
			result = conninfra_step_do_condition_register_action(p_act, conninfra_step_test_check_reg_read_act);
			if (result == result_of_action) {
				if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
					p_report->pass++;
				} else {
					p_report->fail++;
					pr_err("%s\n", err_result);
				}
			} else if (result == -2) {
				p_report->check++;
				pr_info("STEP check: %s, no clock is ok?\n", err_result);
			} else {
				pr_err("%s, expect result is %d\n", err_result, result_of_action);
				p_report->fail++;
			}
		}
		conninfra_step_remove_action(p_act);
	} else {
		if (result_of_action == -1) {
			p_report->pass++;
		} else {
			p_report->fail++;
			pr_err("%s, Create failed\n", err_result);
		}
	}
}

void conninfra_step_test_all(void)
{
	struct step_test_report report = {0, 0, 0};
	//bool is_enable_step = g_infra_step_env.is_enable;
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;

	conninfra_step_init();

	report.pass = 0;
	report.fail = 0;
	report.check = 0;

	pr_info("STEP test: All start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	g_infra_step_env.is_enable = 0;

	conninfra_step_test_read_file(&report);

	//conninfra_step_test_parse_data3(&report);

	conninfra_step_test_parse_data1(&report);
	conninfra_step_test_parse_data2(&report);

	g_infra_step_env.is_enable = 1;
	conninfra_step_test_create_emi_action(&report);
	conninfra_step_test_create_cond_emi_action(&report);
	conninfra_step_test_create_register_action(&report);
	conninfra_step_test_create_cond_register_action(&report);
	conninfra_step_test_check_register_symbol(&report);
	conninfra_step_test_create_other_action(&report);
	//conninfra_step_test_parse_data(&report);

	if (true) {
	conninfra_step_test_do_emi_action(&report);
	conninfra_step_test_do_cond_emi_action(&report);
	}

	/* NOT test yet */
	conninfra_step_test_do_register_action(&report);
	/* NOT test yet */
	conninfra_step_test_do_cond_register_action(&report);

	conninfra_step_test_do_gpio_action(&report);
	//wmt_step_test_do_wakeup_action(&report);
	conninfra_step_test_create_periodic_dump(&report);
	conninfra_step_test_do_show_action(&report);
	conninfra_step_test_do_sleep_action(&report);
	conninfra_step_test_do_condition_action(&report);
	conninfra_step_test_do_value_action(&report);

	//conninfra_step_test_do_chip_reset_action(&report);
	//g_infra_step_env.is_enable = is_enable_step;

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: All result",
		&report, sec_begin, usec_begin, sec_end, usec_end);

	conninfra_step_deinit();
}

void conninfra_step_test_read_file(struct step_test_report *p_report)
{
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;

	pr_info("STEP test: Read file start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	/********************************
	 ******** Test case 1 ***********
	 ******* Wrong file name ********
	 ********************************/
	if (-1 == conninfra_step_read_file("conninfra_wrong_file.cfg")) {
		temp_report.pass++;
	} else {
		pr_err("STEP test failed: (Read file TC-1) expect no file\n");
		temp_report.fail++;
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Read file result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}


char *parse_data_buf_tc1 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0x9c\r\n"
	"[AT] _REG 0 0x08 5 2\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT] _REG 1 0x08 30\r\n"
	"[AT] GPIO 0 8\r\n"
	"[AT] GPIO 1 6 3\r\n"
	"[AT] _REG 1 0x08 30 0xFF00FF00\r\n"
	"[TP CONNINFRA 3] Before read consys thermal\r\n"
	"[AT] SHOW Hello_World\r\n"
	"[AT] _SLP 1000\r\n"
	"[AT] COND $1 $2 == $3\r\n"
	"[AT] COND $1 $2 == 16\r\n"
	"[AT] _VAL $0 0x66\r\n"
	"[TP CONNINFRA 4] Power on sequence(0): Start power on\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[AT] _REG 0 0x08 0xFFFFFFFF $1\r\n"
	"[AT] CEMI $2 0 0x50 0xFFFFFFFF $1\r\n"
	"[AT] CREG $2 0 0x08 0xFFFFFFFF $1\r\n"
	"[TP WF 1] Command Timeout\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP WF 2] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP BT 1] Command Timeout\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP BT 2] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP GPS 1] Command timeout\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP GPS 2] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP FM 1] Command timeout\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[TP FM 2] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"\r\n";


struct step_test_scenario parse_data_buf_scn1 = {
	"// TC1\r\n", 0, 23,
	{ /* array */
		/* ----------------------------------------------------------------------- */
		/* [TP CONNINFRA 1] */
		/* tp_id,										act_id */
		/* 		params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 				STEP_ACTION_INDEX_EMI,
			3, {"0", "0x50", "0x9c"} },
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 				STEP_ACTION_INDEX_REGISTER,
			4, {"0", "0x08", "5", "2"} },
		/* [TP CONNINFRA 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,				STEP_ACTION_INDEX_REGISTER,
			3, {"1", "0x08", "30"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,				STEP_ACTION_INDEX_GPIO,
			2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,				STEP_ACTION_INDEX_GPIO,
			3, {"1", "6", "3"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,				STEP_ACTION_INDEX_REGISTER,
			4, {"1", "0x08", "30", "0xFF00FF00"}},
		/* [TP CONNINFRA 3] */
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,			STEP_ACTION_INDEX_SHOW_STRING,
			1, {"Hello_World"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,			STEP_ACTION_INDEX_SLEEP,
			1, {"1000"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,			STEP_ACTION_INDEX_CONDITION,
			4, {"$1", "$2", "==", "$3"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,			STEP_ACTION_INDEX_CONDITION,
			4, {"$1", "$2", "==", "16"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,			STEP_ACTION_INDEX_VALUE,
			2, {"$0", "0x66"}},
		/* [TP CONNINFRA 4] */
		{ STEP_CONNINFRA_TP_POWER_ON_START,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		{ STEP_CONNINFRA_TP_POWER_ON_START,			STEP_ACTION_INDEX_REGISTER,
			4, {"0", "0x08", "0xFFFFFFFF", "$1"}},
		{ STEP_CONNINFRA_TP_POWER_ON_START,			STEP_ACTION_INDEX_CONDITION_EMI,
			5, {"$2", "0", "0x50", "0xFFFFFFFF", "$1"}},
		{ STEP_CONNINFRA_TP_POWER_ON_START,			STEP_ACTION_INDEX_CONDITION_REGISTER,
			5, {"$2", "0", "0x08", "0xFFFFFFFF", "$1"}},
		/* ----------------------------------------------------------------------- */
		/* [TP WF 1] */
		{ STEP_WF_TP_COMMAND_TIMEOUT,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* [TP WF 2] */
		{ STEP_WF_TP_BEFORE_CHIP_RESET,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* ----------------------------------------------------------------------- */
		/* [TP BT 1] */
		{ STEP_BT_TP_COMMAND_TIMEOUT,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* [TP BT 2] */
		{ STEP_BT_TP_BEFORE_CHIP_RESET,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* ----------------------------------------------------------------------- */
		/* [TP GPS 1] */
		{ STEP_GPS_TP_COMMAND_TIMEOUT,		STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* [TP GPS 2] */
		{ STEP_GPS_TP_BEFORE_CHIP_RESET,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* ----------------------------------------------------------------------- */
		/* [TP FM 1] */
		{ STEP_FM_TP_COMMAND_TIMEOUT,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* [TP FM 2] */
		{ STEP_FM_TP_BEFORE_CHIP_RESET,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
	}
};

static void conninfra_step_test_parse_data_tc1(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_buf_scn1.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_buf_scn1.pt_acts[i].tp_id = -1;
	parse_data_buf_scn1.test_idx = 0;

	g_conninfra_step_test_check.cur_test_scn = &parse_data_buf_scn1;
	__conninfra_step_test_parse_data2(parse_data_buf_tc1, temp_report,
		"STEP test case failed: (Parse data TC-1) Normal case\n");

}

const char *parse_data_buf_tc2 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI   0  0x50   0x9c \r\n"
	"[AT]    _REG 0   0x08 5   2\r\n"
	"  [PD+] 2\r\n"
	"    [AT] _EMI 0 0x50 0x9c\r\n"
	"     [PD-] \r\n"
	" [TP CONNINFRA 2] After Chip reset\r\n"
	"[AT]    _REG   1   0x08  30\r\n"
	"[AT]    GPIO  0  8\r\n"
	" [AT]  GPIO   1  6   3\r\n"
	"  [PD+]     5\r\n"
	"       [AT]    _EMI 0     0x50 0x9c\r\n"
	"  [PD-]   \r\n"
	"[TP CONNINFRA  3] Before read consys thermal\r\n"
	" [AT]    SHOW    Hello\r\n"
	"[AT]    _SLP     1000\r\n"
	"       [AT]   COND   $1    $2    ==     $3\r\n"
	"       [AT] _VAL   $0    0x66\r\n"
	"[TP CONNINFRA 4] Power on sequence(0): Start power on\r\n"
	"[AT]   CEMI $2    0 0x50     0xFFFFFFFF    $1\r\n"
	"   [AT]   CREG   $2 0    0x08    0xFFFFFFFF    $1\r\n"
	"\r\n";

struct step_test_scenario parse_data_buf_scn_tc2 = {
	"// TC1\r\n", 0, 15,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id,										act_id */
		/* [TP 1] */
		/* 		params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_REGISTER,
				4, {"0", "0x08", "5", "2"}},
		{ -1, 											STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}},
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_REGISTER,
				3, {"1", "0x08", "30"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,		STEP_ACTION_INDEX_GPIO,
				3, {"1", "6", "3"}},
		{ -1, 										STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,		STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}
		},
		/* [TP 3] */
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SHOW_STRING,
				1, {"Hello"}
		},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SLEEP,
				1, {"1000"}
		},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_CONDITION,
				4, {"$1", "$2", "==", "$3"}
		},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_VALUE,
				2, {"$0", "0x66"}
		},
		/* [TP 4] */
		{ STEP_CONNINFRA_TP_POWER_ON_START, 		STEP_ACTION_INDEX_CONDITION_EMI,
				5, {"$2", "0", "0x50", "0xFFFFFFFF", "$1"}
		},
		{ STEP_CONNINFRA_TP_POWER_ON_START, 		STEP_ACTION_INDEX_CONDITION_REGISTER,
				5, {"$2", "0", "0x08", "0xFFFFFFFF", "$1"}
		},
	}
};

static void conninfra_step_test_parse_data_tc2(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_buf_scn_tc2.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_buf_scn_tc2.pt_acts[i].tp_id = -1;
	parse_data_buf_scn_tc2.test_idx = 0;

	g_conninfra_step_test_check.cur_test_scn = &parse_data_buf_scn_tc2;
	__conninfra_step_test_parse_data2(parse_data_buf_tc2, temp_report,
		"STEP test case failed: (Parse data TC-2) Normal case with some space\n");
}

char* parse_data_buf_tc3 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI 0x50 0x9c\r\n"
	"[AT] _REG 0 5 2\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT] _REG 1 0x08\r\n"
	"[AT] GPIO 0\r\n"
	"[AT] GPIO 6 3\r\n"
	"[TP CONNINFRA 3] Before read consys thermal\r\n"
	"[AT] SHOW\r\n"
	"[AT] _SLP\r\n"
	"[AT] COND $1 $2 >\r\n"
	"[AT] _VAL 0x66\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc3 = {
	"// TC3 \r\n", 0, 9,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id */
		/* [TP 1] */
		/* 		act_id,								params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_EMI,
				2, {"0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_REGISTER,
				3, {"0", "5", "2"}},
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_REGISTER,
				2, {"1", "0x08"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_GPIO,
				1, {"0"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_GPIO,
				2, {"6", "3"}},
		/* [TP 3] */
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SHOW_STRING,
				0, {}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SLEEP,
				0, {}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_CONDITION,
				3, {"$1", "$2", ">"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_VALUE,
				1, {"0x66"}},
	}
};

static void conninfra_step_test_parse_data_tc3(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc3.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc3.pt_acts[i].tp_id = -1;
	parse_data_scn_tc3.test_idx = 0;

	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc3;

	__conninfra_step_test_parse_data2(parse_data_buf_tc3, temp_report,
		"STEP test case failed: (Parse data TC-3) Not enough parameter\n");


}

static char *parse_data_buf_tc4 =
	"// TEST NOW\r\n"
	"\r\n"
	"[Tp COnnINFRA 1] Before Chip reset\r\n"
	"[aT] _emi 0 0x50 0x9c\r\n"
	"[At] _reg 0 0x08 5 2\r\n"
	"[tP CONNinfra 2] After Chip reset\r\n"
	"[at] _reg 1 0x08 30\r\n"
	"[at] gpio 0 8\r\n"
	"[AT] gpio 1 6 3\r\n"
	"[AT] _reg 1 0x08 30 0xFF00FF00\r\n"
	"[pd+] 5\r\n"
	"[At] gpio 0 8\r\n"
	"[pd-]\r\n"
	"[tp conninfra 3] Before read consys thermal\r\n"
	"[AT] show Hello_World\r\n"
	"[AT] _slp 1000\r\n"
	"[AT] cond $1 $2 == $3\r\n"
	"[AT] _val $0 0x66\r\n"
	"[TP CONNINFRA 4] Power on sequence(0): Start power on\r\n"
	"[AT] cemi $2 0 0x50 0xFFFFFFFF $1\r\n"
	"[at] creg $2 0 0x08 0xffffffff $1\r\n"
	"[TP wf 1] Command timeout\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"[tp WF 2] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0xFFFFFFFF $1\r\n"
	"\r\n";

static struct step_test_scenario parse_data_scn_tc4 = {
	"// TC4 \r\n", 0, 16,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id */
		/* 		act_id,								params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 			STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 			STEP_ACTION_INDEX_REGISTER,
				4, {"0", "0x08", "5", "2"}},
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				3, {"1", "0x08", "30"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				3, {"1", "6", "3"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				4, {"1", "0x08", "30", "0xFF00FF00"}},
		{ -1, 											STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}},
		/* [TP 3] */
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SHOW_STRING,
				1, {"Hello_World"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_SLEEP,
				1, {"1000"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_CONDITION,
				4, {"$1", "$2", "==", "$3"}},
		{ STEP_CONNINFRA_TP_BEFORE_READ_THERMAL, 		STEP_ACTION_INDEX_VALUE,
				2, {"$0", "0x66"}},
		/* [TP 4] */
		{ STEP_CONNINFRA_TP_POWER_ON_START, 		STEP_ACTION_INDEX_CONDITION_EMI,
				5, {"$2", "0", "0x50", "0xFFFFFFFF", "$1"}},
		{ STEP_CONNINFRA_TP_POWER_ON_START, 		STEP_ACTION_INDEX_CONDITION_REGISTER,
				5, {"$2", "0", "0x08", "0xffffffff", "$1"}},
		/* ----------------------------------------------------------------------- */
		/* [TP WF 1] */
		{ STEP_WF_TP_COMMAND_TIMEOUT,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},
		/* [TP WF 2] */
		{ STEP_WF_TP_BEFORE_CHIP_RESET,			STEP_ACTION_INDEX_EMI,
			4, {"0", "0x50", "0xFFFFFFFF", "$1"}},

	}
};

static void conninfra_step_test_parse_data_tc4(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc4.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc4.pt_acts[i].tp_id = -1;
	parse_data_scn_tc4.test_idx = 0;

	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc4;
	__conninfra_step_test_parse_data2(parse_data_buf_tc4, temp_report,
		"STEP test case failed: (Parse data TC-4) Upcase and lowercase\n");
}

char *parse_data_buf_tc5 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT] _REG 1 0x08 30\r\n"
	"[AT] GPIO 0 8\r\n"
	"[AT] GPIO 1 6 3\r\n"
	"[tp conninfra 3] Before read consys thermal\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0x9c\r\n"
	"[AT] _REG 0 0x08 5 2\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc5 = {
	"// TC4 \r\n", 0, 5,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id */
		/* 		act_id,								params */
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				3, {"1", "0x08", "30"}
		},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}
		},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				3, {"1", "6", "3"}
		},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 	STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}
		},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				4, {"0", "0x08", "5", "2"}
		},
	}
};

static void conninfra_step_test_parse_data_tc5(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc5.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc5.pt_acts[i].tp_id = -1;
	parse_data_scn_tc5.test_idx = 0;

	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc5;
	__conninfra_step_test_parse_data2(parse_data_buf_tc5, temp_report,
		"STEP test case failed: (Parse data TC-5) TP sequence switch\n");

}

char *parse_data_buf_tc6 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI 0 0x50 0x9c // show emi 0x50~0x9c\r\n"
	"// show cregister\r\n"
	"[AT] _REG 0 0x08 5 2\r\n"
	"// Do some action\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT] _REG 1 0x08 30\r\n"
	"[AT] GPIO 0 8\r\n"
	"[AT] GPIO 1 6 3\r\n"
	"[PD+] 5 // pd start\r\n"
	"[AT] GPIO 0 8 // just do it\r\n"
	"// Do some action\r\n"
	"[PD-] // pd ned\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc6 = {
	"// TC4 \r\n", 0, 7,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id */
		/* 		act_id,								params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 	STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				4, {"0", "0x08", "5", "2"}},
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_REGISTER,
				3, {"1", "0x08", "30"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_GPIO,
				3, {"1", "6", "3"}},
		{ -1, 										STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 	STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}},
	}
};

static void conninfra_step_test_parse_data_tc6(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc6.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc6.pt_acts[i].tp_id = -1;

	parse_data_scn_tc6.test_idx = 0;
	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc6;

	__conninfra_step_test_parse_data2(parse_data_buf_tc6, temp_report,
		"STEP test case failed: (Parse data TC-6) More comment\n");

}

char *parse_data_buf_tc7 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP adfacdadf 1]When Command timeout\r\n"
	"[AT] _EMI 0 0x50 0x9c\r\n"
	"[TPCONNINFRA1] Before Chip reset\r\n"
	"[TP-CONNINFRA-1] Before Chip reset\r\n"
	"[T P 2] After Chip reset\r\n"
	"[AT] _slp\r\n"
	"[TP CONNINFRA 2 After Chip reset\r\n"
	"[AT] _slp\r\n"
	"[PD+]\r\n"
	"[PD-]\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT]_REG 1 0x08 30\r\n"
	"[A  T] GPIO 0 8\r\n"
	"[ AT ] GPIO 1 6 3\r\n"
	"AT GPIO 0 8\r\n"
	"[AT WAK+\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc7 = {
	"// TC7 \r\n", 0, 0,
	{}
};


static void conninfra_step_test_parse_data_tc7(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc7.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc7.pt_acts[i].tp_id = -1;

	parse_data_scn_tc7.test_idx = 0;
	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc7;

	__conninfra_step_test_parse_data2(parse_data_buf_tc7, temp_report,
		"STEP test case failed: (Parse data TC-7) Wrong format\n");

}

char *parse_data_buf_tc8 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[PD+] 5\r\n"
	"[AT] _EMI 0 0x50 0x9c\r\n"
	"[AT] _REG 0 0x08 5 2\r\n"
	"[PD-]\r\n"
	"[TP CONNINFRA 2] After Chip reset\r\n"
	"[AT] _REG 1 0x08 30\r\n"
	"[PD+] 3\r\n"
	"[AT] GPIO 0 8\r\n"
	"[PD-]\r\n"
	"[AT] GPIO 1 6 3\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc8 = {
	"// TC4 \r\n", 0, 7,
	{ /* array */
		/* tp_id */
		/* 		act_id,								params */
		{ -1, 										STEP_ACTION_INDEX_EMI,
				3, {"0", "0x50", "0x9c"}},
		{ -1, 										STEP_ACTION_INDEX_REGISTER,
				4, {"0", "0x08", "5", "2"}},
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET, 		STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}},
		/* [TP 2] */
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET,		STEP_ACTION_INDEX_REGISTER,
				3, {"1", "0x08", "30"}},
		{ -1, 										STEP_ACTION_INDEX_GPIO,
				2, {"0", "8"}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_PERIODIC_DUMP,
				0, {}},
		{ STEP_CONNINFRA_TP_AFTER_CHIP_RESET, 		STEP_ACTION_INDEX_GPIO,
				3, {"1", "6", "3"}},
	}
};

static void conninfra_step_test_parse_data_tc8(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc8.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc8.pt_acts[i].tp_id = -1;

	parse_data_scn_tc8.test_idx = 0;
	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc8;

	__conninfra_step_test_parse_data2(parse_data_buf_tc8, temp_report,
		"STEP test case failed: (Parse data TC-8) Periodic dump\n");

}

char *parse_data_buf_tc9 =
	"// TEST NOW\r\n"
	"\r\n"
	"[TP CONNINFRA 1] Before Chip reset\r\n"
	"[AT] _EMI 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0x10 0x11 0x12 0x13\r\n"
	"\r\n";

struct step_test_scenario parse_data_scn_tc9 = {
	"// TC4 \r\n", 0, 1,
	{ /* array */
		/* struct step_test_scn_tp_act */
		/* tp_id */
		/* 		act_id,								params */
		{ STEP_CONNINFRA_TP_BEFORE_CHIP_RESET,		STEP_ACTION_INDEX_EMI,
				10, {"0x1", "0x2", "0x3", "0x4", "0x5", "0x6", "0x7", "0x8", "0x9", "0x10"}
		},
	}
};

static void conninfra_step_test_parse_data_tc9(struct step_test_report *temp_report)
{
	int i;

	for (i = parse_data_scn_tc9.total_test_sz; i < STEP_TEST_ACTION_NUMBER; i++)
		parse_data_scn_tc9.pt_acts[i].tp_id = -1;

	parse_data_scn_tc9.test_idx = 0;
	g_conninfra_step_test_check.cur_test_scn = &parse_data_scn_tc9;

	__conninfra_step_test_parse_data2(parse_data_buf_tc9, temp_report,
		"STEP test case failed: (Parse data TC-8) Periodic dump\n");

}

void conninfra_step_test_parse_data1(struct step_test_report *p_report)
{
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;

	pr_info("STEP test: Parse data start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	/********************************
	 ******** Test case 1 ***********
	 ******** Normal case ***********
	 ********************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc1(&temp_report);

	/*********************************
	 ******** Test case 2 ************
	 ** Normal case with some space **
	 *********************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc2(&temp_report);

	/***************************************************
	 ****************** Test case 3 ********************
	 ** Failed case not enough parameter (Can parser) **
	 ***************************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc3(&temp_report);

	/***************************************************
	 ****************** Test case 4 ********************
	 ************** Upcase and lowercase ***************
	 ***************************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc4(&temp_report);

	/*************************************************
	 ****************** Test case 5 ******************
	 ************** TP sequence switch ***************
	 *************************************************/

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Parse data result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);

}
void conninfra_step_test_parse_data2(struct step_test_report *p_report)
{
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;

	pr_info("STEP test: Parse data start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);

	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc5(&temp_report);

	/*********************************
	 ********* Test case 6 ***********
	 ********* More comment **********
	 *********************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc6(&temp_report);

	/*********************************
	 ********* Test case 7 ***********
	 ********* Wrong format **********
	 *********************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc7(&temp_report);

	/********************************
	 ******** Test case 8 ***********
	 ******* Periodic dump **********
	 ********************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc8(&temp_report);

	/*********************************
	 ******** Test case 9 ************
	 *** Boundary: Much parameter ****
	 *********************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_parse_data_tc9(&temp_report);

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Parse data result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_create_emi_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num = 0;

	pr_info("STEP test: Create EMI action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_EMI;

	/*****************************
	 ******** Test case 1 ********
	 **** EMI create for read ****
	 *****************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x50";
	params[2] = "0x9c";
	param_num = 3;
	check_params[0] = 0;
	check_params[1] = 0x50;
	check_params[2] = 0x9c;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-1) EMI create");

	/************************************
	 ********** Test case 2 ************
	 **** EMI create fail less param ****
	 ************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x50";
	param_num = 2;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-2) EMI create fail");

	/*************************************************
	 **************** Test case 3 *******************
	 ********** Save emi to temp register ************
	 *************************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x50";
	params[2] = "0x00000030";
	params[3] = "$3";
	param_num = 4;
	check_params[0] = 0;
	check_params[1] = 0x50;
	check_params[2] = 0x00000030;
	check_params[3] = 3;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-3) Save emi to temp register");

	/*************************************************
	 **************** Test case 4 *******************
	 ** Boundary: Save emi to wrong temp register ****
	 *************************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x50";
	params[2] = "0x00000030";
	params[3] = "$30";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-4) Boundary: Save emi to wrong temp register");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create EMI action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_create_cond_emi_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num = 0;

	pr_info("STEP test: Create condition EMI action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CONDITION_EMI;

	/*************************************************
	 **************** Test case 1 *******************
	 ************ Condition emi create ***************
	 *************************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "0";
	params[2] = "0x50";
	params[3] = "0x70";
	param_num = 4;
	check_params[0] = 1;
	check_params[1] = 0;
	check_params[2] = 0x50;
	check_params[3] = 0x70;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-1) Condition emi create");

	/*************************************************
	 **************** Test case 2 *******************
	 ****** Save condition emi to temp register ******
	 *************************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$2";
	params[1] = "0";
	params[2] = "0x50";
	params[3] = "0x00000030";
	params[4] = "$3";
	param_num = 5;
	check_params[0] = 2;
	check_params[1] = 0;
	check_params[2] = 0x50;
	check_params[3] = 0x00000030;
	check_params[4] = 3;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-2) Save condition emi to temp register");

	/***********************************************************
	 ******************** Test case 3 *************************
	 ** Boundary: Save condition emi to wrong temp register ****
	 ***********************************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "0";
	params[2] = "0x50";
	params[3] = "0x00000030";
	params[4] = "$30";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-3) Boundary: Boundary: Save condition emi to wrong temp register");

	/***********************************************************
	 ******************** Test case 4 *************************
	 ** Boundary: Save condition emi is wrong temp register ****
	 ***********************************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$30";
	params[1] = "0";
	params[2] = "0x50";
	params[3] = "0x00000030";
	params[4] = "$1";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-4) Boundary: Boundary: Save condition emi is wrong temp register");

	/*************************************************
	 **************** Test case 5 *******************
	 ******* Condition emi create less params ********
	 *************************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "0";
	params[2] = "0x50";
	param_num = 3;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-5) Condition emi create less params");

	/*************************************************
	 **************** Test case 6 *******************
	 ******* Condition emi create wrong symbol *******
	 *************************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	params[1] = "0";
	params[2] = "0x50";
	params[3] = "0x00000030";
	params[4] = "$1";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-6) Condition emi create wrong symbol");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create condition EMI action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_create_register_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num = 0;

	pr_info("STEP test: Create Register action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_REGISTER;

	/****************************************
	 ************ Test case 1 ***************
	 **** REGISTER(Addr) create for read ****
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "2";
	params[4] = "10";
	param_num = 5;
	check_params[0] = 0;
	check_params[1] = 0x124dfad;
	check_params[2] = 0x9c;
	check_params[3] = 2;
	check_params[4] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-1) REG create read");

	/*****************************************
	 ************ Test case 2 ****************
	 **** REGISTER(Addr) create for write ****
	 *****************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "15";
	param_num = 4;
	check_params[0] = 1;
	check_params[1] = 0x124dfad;
	check_params[2] = 0x9c;
	check_params[3] = 15;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-2) REG create write");

	/******************************************
	 ************** Test case 3 ***************
	 ******* Boundary: read wrong symbol ******
	 ******************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#10000";
	params[2] = "0x204";
	params[3] = "1";
	params[4] = "0";
	param_num = 5;
	check_params[0] = 0;
	check_params[1] = 0x124dfad;
	check_params[2] = 0x9c;
	check_params[3] = 2;
	check_params[4] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-3) Boundary: read wrong symbol");

	/****************************************************
	 **************** Test case 4 **********************
	 **** REGISTER(Addr) create read fail less param ****
	 ****************************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	param_num = 3;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-4) REG create read fail");

	/*****************************************************
	 ************ Test case 5 ***************************
	 **** REGISTER(Addr) create write fail less param ****
	 *****************************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	param_num = 3;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-5) REG create write fail");

	/*****************************************
	 ************ Test case 6 ****************
	 ** REGISTER(Addr) create for write bit ***
	 *****************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "15";
	params[4] = "0xFF00FF00";
	param_num = 5;
	check_params[0] = 1;
	check_params[1] = 0x124dfad;
	check_params[2] = 0x9c;
	check_params[3] = 15;
	check_params[4] = 0xFF00FF00;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-6) REG create write");

	/*********************************************************
	 ******************** Test case 7 ***********************
	 **** REGISTER(Addr) create for read to temp register ****
	 *********************************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "0x00000030";
	params[4] = "$5";
	param_num = 5;
	check_params[0] = 0;
	check_params[1] = 0x124dfad;
	check_params[2] = 0x9c;
	check_params[3] = 0x00000030;
	check_params[4] = 5;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-7) REGISTER(Addr) create for read to temp register");

	/*********************************************************
	 ******************** Test case 8 ***********************
	 *** REGISTER(Symbol) create for read to temp register ***
	 *********************************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x9c";
	params[3] = "0x00000030";
	params[4] = "$7";
	param_num = 5;
	check_params[0] = 0;
	check_params[1] = 1;
	check_params[2] = 0x9c;
	check_params[3] = 0x00000030;
	check_params[4] = 7;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-8) REGISTER(Symbol) create for read to temp register");

	/*********************************************************
	 ******************** Test case 9 ***********************
	 ********** REGISTER(Symbol) create for read *************
	 *********************************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x9c";
	params[3] = "1";
	params[4] = "10";
	param_num = 5;
	check_params[0] = 0;
	check_params[1] = 1;
	check_params[2] = 0x9c;
	check_params[3] = 1;
	check_params[4] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-9) REGISTER(Symbol) create for read");

	/*********************************************************
	 ******************** Test case 10 ***********************
	 ************ REGISTER(Addr) less parameter **************
	 *********************************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "0x555";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-10) REGISTER(Addr) less parameter");

	/*********************************************************
	 ******************** Test case 11 ***********************
	 ************ REGISTER(Symbol) less parameter **************
	 *********************************************************/
	pr_info("STEP test: TC 11\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x9c";
	params[3] = "0x555";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-11) REGISTER(Symbol) less parameter");

	/**********************************************************
	 *********************** Test case 12 *********************
	 ** Boundary: REGISTER(Addr) read to worng temp register **
	 **********************************************************/
	pr_info("STEP test: TC 12\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "0x124dfad";
	params[2] = "0x9c";
	params[3] = "0x00000030";
	params[4] = "$35";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-12) Boundary: REGISTER(Addr) read to worng temp registe");

	/************************************************************
	 *********************** Test case 13 ***********************
	 ** Boundary: REGISTER(Symbol) read to worng temp register **
	 ************************************************************/
	pr_info("STEP test: TC 13\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x9c";
	params[3] = "0x00000030";
	params[4] = "$35";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-13) Boundary: REGISTER(Symbol) read to worng temp registe");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create register action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_create_cond_register_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num = 0;

	pr_info("STEP test: Create condition Register action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CONDITION_REGISTER;

	/****************************************
	 ************ Test case 1 ***************
	 **** COND_REG(Addr) create for read ****
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$5";
	params[1] = "0";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "2";
	params[5] = "10";
	param_num = 6;
	check_params[0] = 5;
	check_params[1] = 0;
	check_params[2] = 0x124dfad;
	check_params[3] = 0x9c;
	check_params[4] = 2;
	check_params[5] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-1) COND_REG(Addr) create for read");

	/*****************************************
	 ************ Test case 2 ****************
	 **** COND_REG(Addr) create for write ****
	 *****************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$7";
	params[1] = "1";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "15";
	param_num = 5;
	check_params[0] = 7;
	check_params[1] = 1;
	check_params[2] = 0x124dfad;
	check_params[3] = 0x9c;
	check_params[4] = 15;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-2) COND_REG(Addr) create write");

	/******************************************
	 ************** Test case 3 ***************
	 ******* Boundary: read wrong symbol ******
	 ******************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$2";
	params[1] = "0";
	params[2] = "#10000";
	params[3] = "0x204";
	params[4] = "1";
	params[5] = "0";
	param_num = 6;
	check_params[0] = 2;
	check_params[1] = 0;
	check_params[2] = 0x124dfad;
	check_params[3] = 0x9c;
	check_params[4] = 2;
	check_params[5] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-3) Boundary: read wrong symbol");

	/****************************************************
	 **************** Test case 4 **********************
	 **** COND_REG(Addr) create read fail less param ****
	 ****************************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$3";
	params[1] = "0";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-4) COND_REG create read fail");

	/*****************************************************
	 ************ Test case 5 ***************************
	 **** COND_REG(Addr) create write fail less param ****
	 *****************************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$4";
	params[1] = "1";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-5) COND_REG create write fail");

	/*****************************************
	 ************ Test case 6 ****************
	 ** COND_REG(Addr) create for write bit ***
	 *****************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$9";
	params[1] = "1";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "15";
	params[5] = "0xFF00FF00";
	param_num = 6;
	check_params[0] = 9;
	check_params[1] = 1;
	check_params[2] = 0x124dfad;
	check_params[3] = 0x9c;
	check_params[4] = 15;
	check_params[5] = 0xFF00FF00;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-6) COND_REG create write");

	/*********************************************************
	 ******************** Test case 7 ***********************
	 **** COND_REG(Addr) create for read to temp register ****
	 *********************************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$9";
	params[1] = "0";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "0x00000030";
	params[5] = "$5";
	param_num = 6;
	check_params[0] = 9;
	check_params[1] = 0;
	check_params[2] = 0x124dfad;
	check_params[3] = 0x9c;
	check_params[4] = 0x00000030;
	check_params[5] = 5;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-7) COND_REG(Addr) create for read to temp register");

	/*********************************************************
	 ******************** Test case 8 ***********************
	 *** COND_REG(Symbol) create for read to temp register ***
	 *********************************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "0x00000030";
	params[5] = "$7";
	param_num = 6;
	check_params[0] = 1;
	check_params[1] = 0;
	check_params[2] = 1;
	check_params[3] = 0x9c;
	check_params[4] = 0x00000030;
	check_params[5] = 7;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-8) COND_REG(Symbol) create for read to temp register");

	/*********************************************************
	 ******************** Test case 9 ***********************
	 ********** COND_REG(Symbol) create for read *************
	 *********************************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$2";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "1";
	params[5] = "10";
	param_num = 6;
	check_params[0] = 2;
	check_params[1] = 0;
	check_params[2] = 1;
	check_params[3] = 0x9c;
	check_params[4] = 1;
	check_params[5] = 10;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-9) COND_REG(Symbol) create for read");

	/*********************************************************
	 ******************** Test case 10 ***********************
	 ************ COND_REG(Addr) less parameter **************
	 *********************************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$3";
	params[1] = "0";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "0x555";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-10) COND_REG(Addr) less parameter");

	/*********************************************************
	 ******************** Test case 11 ***********************
	 ************ COND_REG(Symbol) less parameter **************
	 *********************************************************/
	pr_info("STEP test: TC 11\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$4";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "0x555";
	param_num = 5;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-11) COND_REG(Symbol) less parameter");

	/**********************************************************
	 *********************** Test case 12 *********************
	 ** Boundary: COND_REG(Addr) read to worng temp register **
	 **********************************************************/
	pr_info("STEP test: TC 12\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$5";
	params[1] = "0";
	params[2] = "0x124dfad";
	params[3] = "0x9c";
	params[4] = "0x00000030";
	params[5] = "$35";
	param_num = 6;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-12) Boundary: COND_REG(Addr) read to worng temp registe");

	/************************************************************
	 *********************** Test case 13 ***********************
	 ** Boundary: COND_REG(Symbol) read to worng temp register **
	 ************************************************************/
	pr_info("STEP test: TC 13\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$6";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "0x00000030";
	params[5] = "$35";
	param_num = 6;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-13) Boundary: COND_REG(Symbol) read to worng temp registe");

	/*********************************************************
	 ******************** Test case 14 ***********************
	 ************* COND_REG(Symbol) worng symbol *************
	 *********************************************************/
	pr_info("STEP test: TC 14\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "8";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "1";
	params[5] = "10";
	param_num = 6;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-14) Boundary: COND_REG(Symbol) worng symbol");

	/*********************************************************
	 ******************** Test case 15 ***********************
	 ********* COND_REG(Symbol) worng temp register id *******
	 *********************************************************/
	pr_info("STEP test: TC 15\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "$88";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x9c";
	params[4] = "1";
	params[5] = "10";
	param_num = 6;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-15) Boundary: COND_REG(Symbol) read to worng temp registe");


	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create condition register action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}


#if 0
int conninfra_step_test_get_symbol_num(void)
{
	int len;
	struct device_node *node = NULL;

	if (g_pdev != NULL) {
		node = g_pdev->dev.of_node;
		if (node) {
			of_get_property(node, "reg", &len);
			len /= (of_n_addr_cells(node) + of_n_size_cells(node));
			len /= (sizeof(int));
		} else {
			pr_err("STEP test failed: node null");
			return -1;
		}
	} else {
		pr_err("STEP test failed: gdev null");
		return -1;
	}

	return len;
}
#endif

void conninfra_step_test_check_register_symbol(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num = 0;
	int i = 0;
	/* use CONSYS_BASE_ADDR_MAX directly */
	//int symbol_num = conninfra_step_test_get_symbol_num();
	int symbol_num = consys_reg_mng_get_reg_symbol_num();
	unsigned char buf[4];

	pr_info("STEP test: Check Register symbol start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_REGISTER;
	/*********************************************************
	 ******************** Test case 1 ***********************
	 ********** REGISTER(Symbol) create for read *************
	 *********************************************************/
	pr_info("STEP test: TC 1\n");
	if (symbol_num < 0) {
		temp_report.fail++;
	} else {
		pr_info("STEP test: symbol_num = [%d]\n", symbol_num);
		for (i = 1; i <= symbol_num; i++) {
			conninfra_step_test_clear_parameter(params);
			params[0] = "0";
			snprintf(buf, 4, "#%d", i);
			params[1] = buf;
			params[2] = "0x9c";
			params[3] = "1";
			params[4] = "10";
			param_num = 5;
			check_params[0] = 0;
			check_params[1] = i;
			check_params[2] = 0x9c;
			check_params[3] = 1;
			check_params[4] = 10;
			conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
				"STEP test case failed: (Check Register symbol TC-1) REGISTER(Symbol) create for read");
		}
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Check Register symbol result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_create_other_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	int check_params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	struct step_pd_entry fack_pd_entry;
	int param_num = 0;

	pr_info("STEP test: Create other action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	/******************************************
	 ************ Test case 1 *****************
	 ********** GPIO create for read **********
	 ******************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_GPIO;
	params[0] = "0";
	params[1] = "8";
	param_num = 2;
	check_params[0] = 0;
	check_params[1] = 8;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-1) GPIO create read");

	/*************************************************
	 **************** Test case 6 *******************
	 ********** GPIO create fail less param **********
	 *************************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_GPIO;
	params[0] = "0";
	param_num = 1;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-6) GPIO create fail");

	/*************************************************
	 **************** Test case 7 *******************
	 ************** Periodic dump create *************
	 *************************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_PERIODIC_DUMP;
	params[0] = (char*)&fack_pd_entry;
	param_num = 1;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-7) Periodic dump create fail");

	/*************************************************
	 **************** Test case 8 *******************
	 ****** Periodic dump create fail no param *******
	 *************************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_PERIODIC_DUMP;
	param_num = 0;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-8) Periodic dump create fail");

	/*************************************************
	 **************** Test case 9 *******************
	 **************** Show create ********************
	 *************************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_SHOW_STRING;
	params[0] = "Hello";
	param_num = 1;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-9) Show create");

	/*************************************************
	 **************** Test case 10 *******************
	 ******** Show create failed no param ************
	 *************************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_SHOW_STRING;
	param_num = 0;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-10) Show create failed no param");

	/*************************************************
	 **************** Test case 11 *******************
	 **************** Sleep create *******************
	 *************************************************/
	pr_info("STEP test: TC 11\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_SLEEP;
	params[0] = "1000";
	param_num = 1;
	check_params[0] = 1000;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-11) Sleep create");

	/*************************************************
	 **************** Test case 12 *******************
	 ********* Sleep create failed no param **********
	 *************************************************/
	pr_info("STEP test: TC 12\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_SLEEP;
	param_num = 0;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-12) Sleep create failed no param");

	/*************************************************
	 **************** Test case 13 *******************
	 ************** Condition create *****************
	 *************************************************/
	pr_info("STEP test: TC 13\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "==";
	params[3] = "$2";
	param_num = 4;
	check_params[0] = 0;
	check_params[1] = 1;
	check_params[2] = STEP_OPERATOR_EQUAL;
	check_params[3] = 2;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-13) Condition create");

	/*************************************************
	 **************** Test case 14 *******************
	 *********** Condition create value **************
	 *************************************************/
	pr_info("STEP test: TC 14\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "==";
	params[3] = "16";
	param_num = 4;
	check_params[0] = 0;
	check_params[1] = 1;
	check_params[2] = STEP_OPERATOR_EQUAL;
	check_params[3] = 16;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-14) Condition create");

	/*************************************************
	 **************** Test case 15 *******************
	 ****** Condition create failed less param *******
	 *************************************************/
	pr_info("STEP test: TC 15\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "$2";
	param_num = 3;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-15) Condition create failed less param");

	/*************************************************
	 **************** Test case 16 *******************
	 ******** Condition create failed no value********
	 *************************************************/
	pr_info("STEP test: TC 16\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "==";
	param_num = 1;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-16) Condition create failed no value");

	/*************************************************
	 **************** Test case 17 *******************
	 * Boundary: Condition create failed over reg id *
	 *************************************************/
	pr_info("STEP test: TC 17\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$25";
	params[1] = "$1";
	params[2] = "==";
	params[3] = "$2";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-17) Boundary: Condition create failed over reg id");

	/*************************************************
	 **************** Test case 18 *******************
	 * Boundary: Condition create failed over reg id *
	 *************************************************/
	pr_info("STEP test: TC 18\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "==";
	params[3] = "$20";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-18) Boundary: Condition create failed over reg id");

	/*************************************************
	 **************** Test case 19 *******************
	 ******** Condition create failed operator********
	 *************************************************/
	pr_info("STEP test: TC 19\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_CONDITION;
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "&";
	params[3] = "$2";
	param_num = 4;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-19) Condition create failed operator");

	/*************************************************
	 **************** Test case 20 *******************
	 **************** Value create *******************
	 *************************************************/
	pr_info("STEP test: TC 20\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_VALUE;
	params[0] = "$0";
	params[1] = "0x123";
	param_num = 2;
	check_params[0] = 0;
	check_params[1] = 0x123;
	conninfra_step_test_create_action(act_id, param_num, params, 0, check_params, &temp_report,
		"STEP test case failed: (Create action TC-20) Condition create");

	/*************************************************
	 **************** Test case 21 *******************
	 ******* Value create failed wrong order *********
	 *************************************************/
	pr_info("STEP test: TC 21\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_VALUE;
	params[0] = "0x123";
	params[1] = "$1";
	param_num = 2;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-21) Value create failed wrong order");

	/*************************************************
	 **************** Test case 22 *******************
	 ********* Value create failed no value **********
	 *************************************************/
	pr_info("STEP test: TC 22\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_VALUE;
	params[0] = "$1";
	param_num = 1;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-22) Value create failed no value");

	/*************************************************
	 **************** Test case 23 *******************
	 *** Boundary: Value create failed over reg id ***
	 *************************************************/
	pr_info("STEP test: TC 23\n");
	conninfra_step_test_clear_parameter(params);
	act_id = STEP_ACTION_INDEX_VALUE;
	params[0] = "$25";
	params[1] = "0x123";
	param_num = 2;
	conninfra_step_test_create_action(act_id, param_num, params, -1, check_params, &temp_report,
		"STEP test case failed: (Create action TC-23) Boundary: Value create failed over reg id");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create other action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

#if 0
/* this offset only for FPGA */
#define CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET 0x4f000
/* formal chip use this offset*/
//#define CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET 0x27f000

int conninfra_step_test_get_emi_offset(unsigned char buf[], int offset)
{
	P_CONSYS_EMI_ADDR_INFO emi_phy_addr = NULL;

	emi_phy_addr = emi_mng_get_phy_addr();
	if (emi_phy_addr != NULL) {
		/* TODO: fix this, change the region to coredump */

		snprintf(buf, 11, "0x%08x", (CONNINFRA_STEP_TEST_EMI_COREDUMP_OFFSET /*->emi_core_dump_offset*/ + offset));
		//snprintf(buf, 11, "0x%08x", ((unsigned int)emi_phy_addr->emi_ap_phy_addr /*->emi_core_dump_offset*/ + offset));
	} else {
		pr_err("STEP test failed: emi_phy_addr is NULL\n");
		return -1;
	}

	return 0;
}
#endif

void conninfra_step_test_do_emi_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	unsigned char buf_begin[11];
	unsigned char buf_end[11];
	int param_num;
	struct consys_emi_addr_info* emi_phy_addr = NULL;

	pr_info("STEP test: Do EMI action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_EMI;

	emi_phy_addr = emi_mng_get_phy_addr();
	if (emi_phy_addr == NULL) {
		temp_report.fail++;
		osal_gettimeofday(&sec_end, &usec_end);
		conninfra_step_test_show_result_report("STEP result: Do EMI action result",
			&temp_report, sec_begin, usec_begin, sec_end, usec_end);
		conninfra_step_test_update_result_report(p_report, &temp_report);
		return;
	}

	if (conninfra_step_test_get_emi_offset(buf_begin, 0x0) != 0) {
		temp_report.fail++;
		osal_gettimeofday(&sec_end, &usec_end);
		conninfra_step_test_show_result_report("STEP result: Do EMI action result",
			&temp_report, sec_begin, usec_begin, sec_end, usec_end);
		conninfra_step_test_update_result_report(p_report, &temp_report);
		return;
	}

	/*****************************************
	 ************ Test case 1 ****************
	 ********** EMI dump 32 bit **************
	 *****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x44);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x48);
	params[2] = buf_end;
	param_num = 3;
	g_conninfra_step_test_check.step_check_total = 1;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x44;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-1) dump 32bit");


	/*****************************************
	 ************ Test case 2 ****************
	 ****** EMI dump check for address *******
	 *****************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x24);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x44);
	params[2] = buf_end;
	param_num = 3;
	g_conninfra_step_test_check.step_check_total = 8;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x24;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x28;
	g_conninfra_step_test_check.step_check_emi_offset[2] = 0x2c;
	g_conninfra_step_test_check.step_check_emi_offset[3] = 0x30;
	g_conninfra_step_test_check.step_check_emi_offset[4] = 0x34;
	g_conninfra_step_test_check.step_check_emi_offset[5] = 0x38;
	g_conninfra_step_test_check.step_check_emi_offset[6] = 0x3c;
	g_conninfra_step_test_check.step_check_emi_offset[7] = 0x40;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-2) more address");

	/*****************************************
	 ************ Test case 3 ****************
	 **** EMI dump begin larger than end *****
	 *****************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x20);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x08);
	params[2] = buf_end;
	param_num = 3;
	g_conninfra_step_test_check.step_check_total = 6;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x0c;
	g_conninfra_step_test_check.step_check_emi_offset[2] = 0x10;
	g_conninfra_step_test_check.step_check_emi_offset[3] = 0x14;
	g_conninfra_step_test_check.step_check_emi_offset[4] = 0x18;
	g_conninfra_step_test_check.step_check_emi_offset[5] = 0x1c;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-3) begin larger than end");

	/****************************************
	 ************ Test case 4 ***************
	 ******** EMI only support read *********
	 ****************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x20);
	params[2] = buf_end;
	param_num = 3;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do EMI action TC-4) only support read");

	/****************************************
	 ************ Test case 5 ***************
	 ********* EMI dump not 32bit ***********
	 ****************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x0e);
	params[2] = buf_end;
	param_num = 3;
	g_conninfra_step_test_check.step_check_total = 2;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x0c;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-5) not 32bit");

	/*****************************************
	 ************ Test case 6 ****************
	 ***** EMI dump over emi max size ********
	 *****************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, (emi_phy_addr->emi_size + 0x08));
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, (emi_phy_addr->emi_size+ 0x0e));
	params[2] = buf_end;
	param_num = 3;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do EMI action TC-6) over emi max size");

	/*****************************************
	 ************ Test case 7 ****************
	 ************* page fault ****************
	 *****************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x02);
	params[1] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x08);
	params[2] = buf_end;
	param_num = 3;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do EMI action TC-7) page fault");

	/*****************************************
	 ************ Test case 8 ****************
	 ********** save to temp reg *************
	 *****************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[1] = buf_begin;
	params[2] = "0x0F0F0F0F";
	params[3] = "$1";
	param_num = 4;
	g_conninfra_step_test_check.step_check_total = 1;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 1;
	__conninfra_step_test_do_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-8) save to temp reg");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do EMI action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_do_cond_emi_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	unsigned char buf_begin[11];
	unsigned char buf_end[11];
	int param_num;
	struct consys_emi_addr_info* emi_phy_addr = NULL;

	pr_info("STEP test: Do condition EMI action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CONDITION_EMI;

	emi_phy_addr = emi_mng_get_phy_addr();
	if (emi_phy_addr == NULL) {
		temp_report.fail++;
		osal_gettimeofday(&sec_end, &usec_end);
		conninfra_step_test_show_result_report("STEP result: Do Condition EMI action result",
			&temp_report, sec_begin, usec_begin, sec_end, usec_end);
		conninfra_step_test_update_result_report(p_report, &temp_report);
		return;
	}

	/*****************************************
	 ************ Test case 1 ****************
	 ********** EMI dump 32 bit **************
	 *****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x44);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x48);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[0] = 1;

	g_conninfra_step_test_check.step_check_total = 1;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x44;
	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-1) dump 32bit");

	/*****************************************
	 ************ Test case 2 ****************
	 ****** EMI dump check for address *******
	 *****************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$1";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x24);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x44);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[1] = 1;

	g_conninfra_step_test_check.step_check_total = 8;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x24;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x28;
	g_conninfra_step_test_check.step_check_emi_offset[2] = 0x2c;
	g_conninfra_step_test_check.step_check_emi_offset[3] = 0x30;
	g_conninfra_step_test_check.step_check_emi_offset[4] = 0x34;
	g_conninfra_step_test_check.step_check_emi_offset[5] = 0x38;
	g_conninfra_step_test_check.step_check_emi_offset[6] = 0x3c;
	g_conninfra_step_test_check.step_check_emi_offset[7] = 0x40;
	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-2) more address");

	/*****************************************
	 ************ Test case 3 ****************
	 **** EMI dump begin larger than end *****
	 *****************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x20);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x08);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[0] = 15;

	g_conninfra_step_test_check.step_check_total = 6;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x0c;
	g_conninfra_step_test_check.step_check_emi_offset[2] = 0x10;
	g_conninfra_step_test_check.step_check_emi_offset[3] = 0x14;
	g_conninfra_step_test_check.step_check_emi_offset[4] = 0x18;
	g_conninfra_step_test_check.step_check_emi_offset[5] = 0x1c;
	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-3) begin larger than end");

	/****************************************
	 ************ Test case 4 ***************
	 ******** EMI only support read *********
	 ****************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$1";
	params[1] = "1";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x20);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[1] = 1;

	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-4) only support read");

	/****************************************
	 ************ Test case 5 ***************
	 ********* EMI dump not 32bit ***********
	 ****************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x0e);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[0] = 1;

	g_conninfra_step_test_check.step_check_total = 2;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_check_emi_offset[1] = 0x0c;
	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-5) not 32bit");

	/*****************************************
	 ************ Test case 6 ****************
	 ***** EMI dump over emi max size ********
	 *****************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$9";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, (emi_phy_addr->emi_size + 0x08));
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, (emi_phy_addr->emi_size + 0x0e));
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[9] = 1;

	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-6) over emi max size");

	/*****************************************
	 ************ Test case 7 ****************
	 ************* page fault ****************
	 *****************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x02);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x08);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[0] = 1;

	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-7) page fault");

	/*****************************************
	 ************ Test case 8 ****************
	 ********** save to temp reg *************
	 *****************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[2] = buf_begin;
	params[3] = "0x0F0F0F0F";
	params[4] = "$1";
	param_num = 5;
	g_conninfra_step_test_check.step_check_total = 1;
	g_conninfra_step_test_check.step_check_emi_offset[0] = 0x08;
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 1;
	g_infra_step_env.temp_register[0] = 1;
	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do EMI action TC-8) save to temp reg");


	/*****************************************
	 ************ Test case 9 ****************
	 ******** condition invalid **************
	 *****************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	conninfra_step_test_get_emi_offset(buf_begin, 0x08);
	params[2] = buf_begin;
	conninfra_step_test_get_emi_offset(buf_end, 0x0e);
	params[3] = buf_end;
	param_num = 4;
	g_infra_step_env.temp_register[0] = 0;

	__conninfra_step_test_do_cond_emi_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do COND EMI action TC-9) condition invalid");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do condition EMI action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_do_register_action(struct step_test_report *p_report)
{
#define TEST_BUF_LEN 12
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	unsigned char buf[TEST_BUF_LEN], buf2[TEST_BUF_LEN];
	int param_num;
	unsigned int can_write_idx = 0, chip_id_idx = 0;
	unsigned long can_write_offset = 0, chip_id_offset = 0;
	unsigned long can_write_vir_addr = 0, can_write_phy_addr = 0;
	unsigned long chip_id_vir_addr = 0, chip_id_phy_addr = 0;
	unsigned long first_idx_vir_addr = 0, first_idx_phy_addr = 0;
	unsigned char can_write_offset_char[11];

	pr_info("STEP test: Do register action start\n");

	/* TODO: need to redefine the test case */
#if TEST_WRITE_CR_TEST_CASE
	if (consys_reg_mng_find_can_write_reg(&can_write_idx, &can_write_offset)) {
		p_report->fail++;
		pr_err("STEP test: Do register action init can_write_offset failed\n");
		return;
	}
#endif
	snprintf(can_write_offset_char, 11, "0x%08lx", can_write_offset);

	if (consys_reg_mng_get_chip_id_idx_offset(&chip_id_idx, &chip_id_offset)) {
		p_report->fail++;
		pr_err("STEP test: Do register action init chip id idx and offset fail \n");
		return;
	}
	can_write_vir_addr = consys_reg_mng_get_virt_addr_by_idx(can_write_idx);
	can_write_phy_addr = consys_reg_mng_get_phy_addr_by_idx(can_write_idx);

	chip_id_vir_addr = consys_reg_mng_get_virt_addr_by_idx(chip_id_idx);
	chip_id_phy_addr = consys_reg_mng_get_phy_addr_by_idx(chip_id_idx);

	pr_info("[%s] chipId idx=[%d] vir=[%p] phy=[%p] offset=[%x]", __func__,
				chip_id_idx, chip_id_vir_addr, chip_id_phy_addr, chip_id_offset);

	if (can_write_vir_addr == 0 || can_write_phy_addr == 0 ||
		chip_id_vir_addr == 0 || chip_id_phy_addr == 0) {
		p_report->fail++;
		pr_err("STEP test: Do register action init vir/phy addr fail [%x][%x] [%x][%x] chipidIdx=[%d]\n",
					can_write_vir_addr, can_write_phy_addr,
					chip_id_vir_addr, chip_id_phy_addr, chip_id_idx);
		return;
	}

	pr_info("[%s] offset=[%s]", __func__, can_write_offset_char);

	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_REGISTER;
	/****************************************
	 ************ Test case 1 ***************
	 ******** REG read HW chip id **********
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);

	snprintf(buf, TEST_BUF_LEN, "#%d", chip_id_idx+1);
	snprintf(buf2, TEST_BUF_LEN, "0x%lx", chip_id_offset);

	params[0] = "0";
	//params[1] = "#1";
	params[1] = buf;
	//params[2] = "0x04";
	params[2] = buf2;
	params[3] = "1";
	params[4] = "0";
	param_num = 5;
	/* Chip id may different by projects, TODO: the address should be define on consys_hw */
	//g_conninfra_step_test_check.step_check_register_addr = (CON_REG_INFRA_CFG_ADDR + 0x04);
	g_conninfra_step_test_check.step_check_register_addr = (chip_id_vir_addr + chip_id_offset);
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-1) MCU chip id");

	/**********************************************
	 *************** Test case 2 ******************
	 ** REG read MCU chip id by physical address **
	 **********************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	snprintf(buf, TEST_BUF_LEN, "0x%08lx", chip_id_phy_addr);
	params[1] = buf;
	//params[2] = "0x04";
	params[2] = buf2;
	params[3] = "1";
	params[4] = "0";
	param_num = 5;
	g_conninfra_step_test_check.step_check_register_addr = (chip_id_vir_addr + chip_id_offset);
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-3) MCU chip id by phy");

	/*****************************************
	 ************* Test case 3 ***************
	 ******** REG read over base size ********
	 *****************************************/
	pr_info("STEP test: TC 3 >>>> \n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x11204";
	params[3] = "1";
	params[4] = "0";
	param_num = 5;
	__conninfra_step_test_do_register_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do register action TC-5) Over size");

	/******************************************
	 ************** Test case 4 ***************
	 ***** REG read over base size by phy *****
	 ******************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";

	first_idx_phy_addr = consys_reg_mng_get_phy_addr_by_idx(0);
	first_idx_vir_addr = consys_reg_mng_get_virt_addr_by_idx(0);
	if (first_idx_phy_addr != 0 && first_idx_vir_addr != 0) {
		snprintf(buf, TEST_BUF_LEN, "0x%08lx", first_idx_phy_addr);
		params[1] = buf;
		params[2] = "0x204";
		params[3] = "1";
		params[4] = "0";
		param_num = 5;
		g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x204);
		__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
			"STEP test case failed: (Do register action TC-6) Over size by phy");
	} else {
		p_report->fail++;
		pr_err("STEP test case failed: get physical address failed\n");
	}

	/******************************************
	 ************** Test case 5 ***************
	 *************** REG write ****************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);

	snprintf(buf, TEST_BUF_LEN, "#%d", can_write_idx+1);
	params[0] = "1";
	//params[1] = "#0";
	params[1] = buf;
	params[2] = can_write_offset_char;
	params[3] = "0x2";
	param_num = 4;
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x2;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-7) REG write");
#endif

	/******************************************
	 ************** Test case 6 ***************
	 *********** REG write by phy *************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";

	snprintf(buf, TEST_BUF_LEN, "0x%08x", can_write_phy_addr);
	params[1] = buf;
	params[2] = can_write_offset_char;
	params[3] = "0x7";
	param_num = 4;
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x7;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-8) REG write by phy");
#endif

	/******************************************
	 ************** Test case 7 ***************
	 ************* REG write bit **************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);

	snprintf(buf, TEST_BUF_LEN, "#%d", can_write_idx+1);

	params[0] = "1";
	//params[1] = "#0";
	params[1] = buf;
	params[2] = can_write_offset_char;
	params[3] = "0x321";
	params[4] = "0x00F";
	param_num = 5;
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x001;
	g_conninfra_step_test_check.step_test_mask = 0x00F;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-9) REG write bit");
#endif

	/******************************************
	 ************** Test case 8 **************
	 ********* REG write bit by phy ***********
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "1";
	//if (conninfra_step_test_get_reg_base_phy_addr(buf, 0) == 0) {
	snprintf(buf, TEST_BUF_LEN, "0x%08x", can_write_phy_addr);
	params[1] = buf;
	params[2] = can_write_offset_char;
	params[3] = "0x32f";
	params[4] = "0x002";
	param_num = 5;
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x002;
	g_conninfra_step_test_check.step_test_mask = 0x002;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-10) REG write bit by phy");
#endif

	/******************************************
	 ************** Test case 9 **************
	 ********* REG read to temp reg ***********
	 ******************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "0";
	params[1] = "#1";
	params[2] = "0x08";
	params[3] = "0x0F0F0F0F";
	params[4] = "$2";
	param_num = 5;
	g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x08);
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 2;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-11) REG read to temp reg");

	/******************************************
	 ************** Test case 10 **************
	 ******* REG read phy to temp reg *********
	 ******************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "0";

	snprintf(buf, TEST_BUF_LEN, "0x%08lx", first_idx_phy_addr);
	params[1] = buf;
	params[2] = "0x08";
	params[3] = "0x0F0F0F0F";
	params[4] = "$3";
	param_num = 5;
	g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x08);
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 3;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-12) REG read phy to temp reg");

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do register action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_do_cond_register_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	unsigned char buf[TEST_BUF_LEN], buf2[TEST_BUF_LEN];
	int param_num;
	unsigned int can_write_idx = 0, chip_id_idx = 0;
	unsigned long can_write_offset = 0, chip_id_offset = 0;
	unsigned long can_write_vir_addr = 0, can_write_phy_addr = 0;
	unsigned long chip_id_vir_addr, chip_id_phy_addr;
	unsigned long first_idx_vir_addr, first_idx_phy_addr;
	unsigned char can_write_offset_char[11];

	pr_info("STEP test: Do condition register action start\n");

#if TEST_WRITE_CR_TEST_CASE
	if (consys_reg_mng_find_can_write_reg(&can_write_idx, &can_write_offset)) {
		p_report->fail++;
		pr_err("STEP test: Do register action init can_write_offset failed\n");
		return;
	}
#endif
	snprintf(can_write_offset_char, 11, "0x%08lx", can_write_offset);

	if (consys_reg_mng_get_chip_id_idx_offset(&chip_id_idx, &chip_id_offset)) {
		p_report->fail++;
		pr_err("STEP test: Do register action init chip id idx and offset fail \n");
		return;
	}
	can_write_vir_addr = consys_reg_mng_get_virt_addr_by_idx(can_write_idx);
	can_write_phy_addr = consys_reg_mng_get_phy_addr_by_idx(can_write_idx);

	chip_id_vir_addr = consys_reg_mng_get_virt_addr_by_idx(chip_id_idx);
	chip_id_phy_addr = consys_reg_mng_get_phy_addr_by_idx(chip_id_idx);

	if (can_write_vir_addr == 0 || can_write_phy_addr == 0 ||
		chip_id_vir_addr == 0 || chip_id_phy_addr == 0) {
		p_report->fail++;
		pr_err("STEP test: Do register action init vir/phy addr fail [%x][%x][%x] \n"
					, can_write_vir_addr, chip_id_vir_addr, chip_id_phy_addr);
		return;
	}

	pr_info("STEP test: chipId=[%d] [%x][%x] offset=[%x] canWrite=[%d] [%x][%x]",
			chip_id_idx, chip_id_vir_addr, chip_id_phy_addr, chip_id_offset,
			can_write_idx, can_write_vir_addr, can_write_phy_addr);

	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CONDITION_REGISTER;
	/****************************************
	 ************ Test case 1 ***************
	 ******** REG read MCU chip id **********
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();

	snprintf(buf, TEST_BUF_LEN, "#%d", chip_id_idx+1);
	snprintf(buf2, TEST_BUF_LEN, "0x%08lx", chip_id_offset);

	params[0] = "$0";
	params[1] = "0";
	//params[2] = "#1";
	params[2] = buf;
	//params[3] = "0x04";
	params[3] = buf2;
	params[4] = "1";
	params[5] = "0";
	param_num = 6;
	g_infra_step_env.temp_register[0] = 1;

	g_conninfra_step_test_check.step_check_register_addr = (chip_id_vir_addr + chip_id_offset);
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-1) MCU chip id");

	/**********************************************
	 *************** Test case 2 ******************
	 ** REG read chip id by physical address **
	 **********************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$2";
	params[1] = "0";

	snprintf(buf, TEST_BUF_LEN, "0x%08lx", chip_id_phy_addr);
	params[2] = buf;
	//params[3] = "0x04";
	params[3] = buf2;
	params[4] = "1";
	params[5] = "0";
	param_num = 6;
	g_infra_step_env.temp_register[2] = 1;

	g_conninfra_step_test_check.step_check_register_addr = (chip_id_vir_addr + chip_id_offset);
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-3) MCU chip id by phy");

	/*****************************************
	 ************* Test case 3 ***************
	 ******** REG read over base size ********
	 *****************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$4";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x11204";
	params[4] = "1";
	params[5] = "0";
	param_num = 6;
	g_infra_step_env.temp_register[4] = 10;

	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do cond register action TC-5) Over size");

	/******************************************
	 ************** Test case 4 ***************
	 ***** REG read over base size by phy *****
	 ******************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$5";
	params[1] = "0";

	first_idx_phy_addr = consys_reg_mng_get_phy_addr_by_idx(0);
	first_idx_vir_addr = consys_reg_mng_get_virt_addr_by_idx(0);

	if (first_idx_phy_addr != 0 && first_idx_vir_addr != 0) {
		snprintf(buf, TEST_BUF_LEN, "0x%08lx", first_idx_phy_addr);
		params[2] = buf;
		params[3] = "0x204";
		params[4] = "1";
		params[5] = "0";
		param_num = 6;
		g_infra_step_env.temp_register[5] = 1;

		g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x204);
		__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
			"STEP test case failed: (Do cond register action TC-6) Over size by phy");
	} else {
		p_report->fail++;
		pr_err("STEP test case failed: get physical address failed\n");
	}

	/******************************************
	 ************** Test case 5 ***************
	 *************** REG write ****************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();

	snprintf(buf, TEST_BUF_LEN, "#%d", can_write_idx+1);
	params[0] = "$6";
	params[1] = "1";
	params[2] = buf;
	params[3] = can_write_offset_char;
	params[4] = "0x2";
	param_num = 5;
	g_infra_step_env.temp_register[6] = 1;

	//g_conninfra_step_test_check.step_check_register_addr = (CON_REG_INFRA_RGU_ADDR + can_write_offset);
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x2;
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-7) REG write");
#endif

	/******************************************
	 ************** Test case 6 ***************
	 *********** REG write by phy *************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$7";
	params[1] = "1";
	//if (conninfra_step_test_get_reg_base_phy_addr(buf, 0) == 0) {

	snprintf(buf, TEST_BUF_LEN, "0x%08x", can_write_phy_addr);
	params[2] = buf;
	params[3] = can_write_offset_char;
	params[4] = "0x7";
	param_num = 5;
	g_infra_step_env.temp_register[7] = 1;

	//g_conninfra_step_test_check.step_check_register_addr = (CON_REG_INFRA_RGU_ADDR + can_write_offset);
	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x7;
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-8) REG write by phy");
#endif
	//} else {
	//	p_report->fail++;
	//	pr_err("STEP test case failed: get physical address failed\n");
	//}

	/******************************************
	 ************** Test case 7 ***************
	 ************* REG write bit **************
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();

	snprintf(buf, TEST_BUF_LEN, "#%d", can_write_idx+1);
	params[0] = "$8";
	params[1] = "1";
	params[2] = buf;
	params[3] = can_write_offset_char;
	params[4] = "0x321";
	params[5] = "0x00F";
	param_num = 6;
	g_infra_step_env.temp_register[8] = 1;

	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	//g_conninfra_step_test_check.step_check_register_addr = (CON_REG_INFRA_RGU_ADDR + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x001;
	g_conninfra_step_test_check.step_test_mask = 0x00F;
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-9) REG write bit");
#endif

	/******************************************
	 ************** Test case 8 **************
	 ********* REG write bit by phy ***********
	 ******************************************/
#if TEST_WRITE_CR_TEST_CASE
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$9";
	params[1] = "1";
	snprintf(buf, TEST_BUF_LEN, "0x%08x", can_write_phy_addr);
	params[2] = buf;
	params[3] = can_write_offset_char;
	params[4] = "0x32f";
	params[5] = "0x002";
	param_num = 6;
	g_infra_step_env.temp_register[9] = 1;

	g_conninfra_step_test_check.step_check_register_addr = (can_write_vir_addr + can_write_offset);
	g_conninfra_step_test_check.step_check_write_value = 0x002;
	g_conninfra_step_test_check.step_test_mask = 0x002;
	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do cond register action TC-10) REG write bit by phy");
#endif

	/******************************************
	 ************** Test case 9 **************
	 ********* REG read to temp reg ***********
	 ******************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$8";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x08";
	params[4] = "0x0F0F0F0F";
	params[5] = "$2";
	param_num = 6;
	g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x08);
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 2;
	g_infra_step_env.temp_register[8] = 1;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-11) REG read to temp reg");

	/******************************************
	 ************** Test case 10 **************
	 ******* REG read phy to temp reg *********
	 ******************************************/
	pr_info("STEP test: TC 12\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$8";
	params[1] = "0";
	snprintf(buf, TEST_BUF_LEN, "0x%08lx", first_idx_phy_addr);
	params[2] = buf;
	params[3] = "0x08";
	params[4] = "0x0F0F0F0F";
	params[5] = "$3";
	param_num = 6;
	g_conninfra_step_test_check.step_check_register_addr = (first_idx_vir_addr + 0x08);
	g_conninfra_step_test_check.step_test_mask = 0x0F0F0F0F;
	g_conninfra_step_test_check.step_check_temp_register_id = 3;
	g_infra_step_env.temp_register[8] = 1;
	__conninfra_step_test_do_register_action(act_id, param_num, params, 0, &temp_report,
		"STEP test case failed: (Do register action TC-12) REG read phy to temp reg");

	/******************************************
	 ************** Test case 11 **************
	 ************* condition invalid **********
	 ******************************************/
	pr_info("STEP test: TC 13\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$8";
	params[1] = "1";
	params[2] = "#1";
	params[3] = "0x160";
	params[4] = "0x123";
	params[5] = "0xF00";
	param_num = 6;
	g_infra_step_env.temp_register[8] = 0;

	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do cond register action TC-13) condition invalid");

	/******************************************
	 ************** Test case 12 **************
	 ********** condition invalid write *******
	 ******************************************/
	pr_info("STEP test: TC 12\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$6";
	params[1] = "1";
	params[2] = "#1";
	params[3] = "0x110";
	params[4] = "0x200";
	param_num = 5;
	g_infra_step_env.temp_register[6] = 0;

	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do cond register action TC-14) condition invalid write");

	/******************************************
	 ************** Test case 13 **************
	 ********** condition invalid read *******
	 ******************************************/
	pr_info("STEP test: TC 13\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	conninfra_step_test_clear_temp_register();
	params[0] = "$0";
	params[1] = "0";
	params[2] = "#1";
	params[3] = "0x08";
	params[4] = "1";
	params[5] = "0";
	param_num = 6;
	g_infra_step_env.temp_register[0] = 0;

	__conninfra_step_test_do_cond_register_action(act_id, param_num, params, -1, &temp_report,
		"STEP test case failed: (Do cond register action TC-15) REG write");


	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do condition register action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

void conninfra_step_test_do_gpio_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do GPIO action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_GPIO;
	/****************************************
	 ************* Test case 1 **************
	 ************* GPIO read #8 *************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "0";
	params[1] = "8";
	param_num = 2;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		if (conninfra_step_do_gpio_action(p_act, NULL) == 0) {
			pr_info("STEP check: Do gpio action TC-1(Read #8): search(8: )");
			temp_report.check++;
		} else {
			pr_err("STEP test case failed: (Do gpio action TC-1) Read #8\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: (Do gpio action TC-1) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do GPIO action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

#if 0
void conninfra_step_test_do_chip_reset_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do chip reset action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CHIP_RESET;
	/****************************************
	 ************* Test case 1 **************
	 ************* chip reset ***************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	param_num = 0;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		if (wmt_step_do_chip_reset_action(p_act, NULL) == 0) {
			pr_info("STEP check: Do chip reset TC-1(chip reset): Trigger AEE");
			temp_report.check++;
		} else {
			pr_err("STEP test case failed: (Do chip reset action TC-1) chip reset\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: (Do chip reset action TC-1) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do chip reset action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

#if 0
void wmt_step_test_do_wakeup_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do wakeup action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	/****************************************
	 ************* Test case 1 **************
	 ***** Wakeup then read/write reg *******
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	act_id = STEP_ACTION_INDEX_KEEP_WAKEUP;
	param_num = 0;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		wmt_step_do_keep_wakeup_action(p_act, NULL);
		conninfra_step_test_do_register_action(&temp_report);
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: (Do wakeup) Create failed\n");
	}

	act_id = STEP_ACTION_INDEX_CANCEL_WAKEUP;
	param_num = 0;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		wmt_step_do_cancel_wakeup_action(p_act, NULL);
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: (Do cancel wakeup) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do wakeup action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

void conninfra_step_test_do_show_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do show action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_SHOW_STRING;
	/****************************************
	 ************* Test case 1 **************
	 ********** Show Hello world ************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_parameter(params);
	params[0] = "Hello_World";
	param_num = 1;

	g_conninfra_step_test_check.step_check_result_string = "Hello_World";
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_show_string_action(p_act, conninfra_step_test_check_show_act);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do show TC-1(Show Hello world)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do show TC-1(Show Hello world) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do show action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}

#if 1
void conninfra_step_test_do_sleep_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int check_sec_b, check_sec_e;
	int check_usec_b, check_usec_e;
	int param_num;

	pr_info("STEP test: Do sleep action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_SLEEP;
	/****************************************
	 ************* Test case 1 **************
	 *************** Sleep 1s ***************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_parameter(params);
	params[0] = "1000";
	param_num = 1;

	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		osal_gettimeofday(&check_sec_b, &check_usec_b);
		conninfra_step_do_sleep_action(p_act, NULL);
		osal_gettimeofday(&check_sec_e, &check_usec_e);
		if (check_sec_e > check_sec_b) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do show TC-1(Sleep 1s), begin(%d.%d) end(%d.%d)\n",
				check_sec_b, check_usec_b, check_sec_e, check_usec_e);
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do show TC-1(Sleep 1s) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do sleep action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

#if 1
void conninfra_step_test_do_condition_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do condition action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_CONDITION;
	/****************************************
	 ************* Test case 1 **************
	 *********** Condition equal ************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "==";
	params[3] = "$2";
	param_num = 4;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-1(equal)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-1(equal) Create failed\n");
	}

	/****************************************
	 ************* Test case 2 **************
	 ********** Condition greater ***********
	 ****************************************/
	pr_info("STEP test: TC 2\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$0";
	params[1] = "$1";
	params[2] = ">";
	params[3] = "$2";
	param_num = 4;
	g_infra_step_env.temp_register[1] = 0;
	g_infra_step_env.temp_register[2] = 1;

	g_conninfra_step_test_check.step_check_result_value = 0;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-2(greater)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-2(greater) Create failed\n");
	}

	/****************************************
	 ************* Test case 3 **************
	 ******* Condition greater equal ********
	 ****************************************/
	pr_info("STEP test: TC 3\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$0";
	params[1] = "$1";
	params[2] = ">=";
	params[3] = "$2";
	param_num = 4;
	g_infra_step_env.temp_register[1] = 2;
	g_infra_step_env.temp_register[2] = 2;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-3(greater equal)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-3(greater equal) Create failed\n");
	}

	/****************************************
	 ************* Test case 4 **************
	 ************ Condition less ************
	 ****************************************/
	pr_info("STEP test: TC 4\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "<";
	params[3] = "$2";
	param_num = 4;
	g_infra_step_env.temp_register[1] = 10;
	g_infra_step_env.temp_register[2] = 0;

	g_conninfra_step_test_check.step_check_result_value = 0;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-4(less)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-4(less) Create failed\n");
	}

	/****************************************
	 ************* Test case 5 **************
	 ********* Condition less equal *********
	 ****************************************/
	pr_info("STEP test: TC 5\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$0";
	params[1] = "$1";
	params[2] = "<=";
	params[3] = "$2";
	param_num = 4;
	g_infra_step_env.temp_register[1] = 0;
	g_infra_step_env.temp_register[2] = 10;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-5(less equal)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-5(less equal) Create failed\n");
	}

	/****************************************
	 ************* Test case 6 **************
	 ********* Condition not equal **********
	 ****************************************/
	pr_info("STEP test: TC 6\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "!=";
	params[3] = "$3";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 3;
	g_infra_step_env.temp_register[3] = 3;

	g_conninfra_step_test_check.step_check_result_value = 0;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-6(not equal)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-6(not equal) Create failed\n");
	}

	/****************************************
	 ************* Test case 7 **************
	 ************ Condition and *************
	 ****************************************/
	pr_info("STEP test: TC 7\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "&&";
	params[3] = "$3";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 0x10;
	g_infra_step_env.temp_register[3] = 0x00;

	g_conninfra_step_test_check.step_check_result_value = 0;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-7(and)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-7(and) Create failed\n");
	}

	/****************************************
	 ************* Test case 8 **************
	 ************* Condition or *************
	 ****************************************/
	pr_info("STEP test: TC 8\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "||";
	params[3] = "$3";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 0x10;
	g_infra_step_env.temp_register[3] = 0x00;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-8(or)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-8(or) Create failed\n");
	}

	/****************************************
	 ************* Test case 9 **************
	 ****** Condition not equal value *******
	 ****************************************/
	pr_info("STEP test: TC 9\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "!=";
	params[3] = "99";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 99;

	g_conninfra_step_test_check.step_check_result_value = 0;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-9(not equal value)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-9(not equal value) Create failed\n");
	}

	/****************************************
	 ************* Test case 10 *************
	 ********* Condition equal value ********
	 ****************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "==";
	params[3] = "18";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 18;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-10(equal value)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-10(equal value) Create failed\n");
	}

	/****************************************
	 ************* Test case 11 *************
	 ****** Condition equal value (HEX) *****
	 ****************************************/
	pr_info("STEP test: TC 10\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$1";
	params[1] = "$2";
	params[2] = "==";
	params[3] = "0x18";
	param_num = 4;
	g_infra_step_env.temp_register[2] = 24;

	g_conninfra_step_test_check.step_check_result_value = 1;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_condition_action(p_act, conninfra_step_test_check_condition);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do condition TC-11(equal value HEX)\n");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do condition TC-11(equal value HEX) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do condition action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

#if 1
void conninfra_step_test_do_value_action(struct step_test_report *p_report)
{
	enum step_action_id act_id;
	char *params[STEP_PARAMETER_SIZE];
	struct step_action *p_act = NULL;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	int param_num;

	pr_info("STEP test: Do value action start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);
	act_id = STEP_ACTION_INDEX_VALUE;
	/****************************************
	 ************* Test case 1 **************
	 ******* Save value to register *********
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	conninfra_step_test_clear_check_data();
	conninfra_step_test_clear_temp_register();
	conninfra_step_test_clear_parameter(params);
	params[0] = "$2";
	params[1] = "0x66";
	param_num = 2;

	g_conninfra_step_test_check.step_check_result_value = 0x66;
	p_act = conninfra_step_create_action(STEP_DRV_TYPE_CONNINFRA, act_id, param_num, params);
	if (p_act != NULL) {
		conninfra_step_do_value_action(p_act, conninfra_step_test_check_value_act);
		if (g_conninfra_step_test_check.step_check_result == TEST_PASS) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: Do show TC-1(Save value to register)");
			temp_report.fail++;
		}
		conninfra_step_remove_action(p_act);
	} else {
		temp_report.fail++;
		pr_err("STEP test case failed: Do show TC-1(Save value to register) Create failed\n");
	}

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Do value action result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

#if 1
void conninfra_step_test_create_periodic_dump(struct step_test_report *p_report)
{
	int expires_ms;
	struct step_test_report temp_report = {0, 0, 0};
	int sec_begin = 0;
	int usec_begin = 0;
	int sec_end = 0;
	int usec_end = 0;
	struct step_pd_entry *p_current;
	bool is_thread_run_for_test = 0;

	pr_info("STEP test: Create periodic dump start\n");
	osal_gettimeofday(&sec_begin, &usec_begin);

	if (g_infra_step_env.pd_struct.step_pd_wq == NULL) {
		if (conninfra_step_init_pd_env() != 0) {
			pr_err("STEP test case failed: Start thread failed\n");
			return;
		}
		is_thread_run_for_test = 1;
	}

	/****************************************
	 ************* Test case 1 **************
	 *************** Normal *****************
	 ****************************************/
	pr_info("STEP test: TC 1\n");
	expires_ms = 5;
	p_current = conninfra_step_get_periodic_dump_entry(expires_ms);
	if (p_current == NULL) {
		pr_err("STEP test case failed: (Create periodic dump TC-1) No entry\n");
		temp_report.fail++;
	} else {
		if (p_current->expires_ms == expires_ms) {
			temp_report.pass++;
		} else {
			pr_err("STEP test case failed: (Create periodic dump TC-1) Currect %d not %d\n",
				p_current->expires_ms, expires_ms);
			temp_report.fail++;
		}
		list_del_init(&p_current->list);
		kfree(p_current);
	}

	if (is_thread_run_for_test == 1)
		conninfra_step_deinit_pd_env();

	osal_gettimeofday(&sec_end, &usec_end);
	conninfra_step_test_show_result_report("STEP result: Create periodic dump result",
		&temp_report, sec_begin, usec_begin, sec_end, usec_end);
	conninfra_step_test_update_result_report(p_report, &temp_report);
}
#endif

