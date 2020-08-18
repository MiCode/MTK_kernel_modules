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
#include "conninfra_step_parse_act.h"

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************/

static int conninfra_step_access_line_state_init(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);
static int conninfra_step_access_line_state_tp(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);

static int conninfra_step_access_line_state_tp_id(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);
static int conninfra_step_access_line_state_at(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);
static int conninfra_step_access_line_state_at_op(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);
static int conninfra_step_access_line_state_pd(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info);


/*******************************************************************************
 *                           D E F I N E
********************************************************************************/
#define STEP_EMI_ACT_INT (int)(*(int *)STEP_ACTION_NAME_EMI)
#define STEP_REG_ACT_INT (int)(*(int *)STEP_ACTION_NAME_REGISTER)
#define STEP_GPIO_ACT_INT (int)(*(int *)STEP_ACTION_NAME_GPIO)
#define STEP_DISABLE_RESET_ACT_INT (int)(*(int *)STEP_ACTION_NAME_DISABLE_RESET)
#define STEP_CHIP_RESET_ACT_INT (int)(*(int *)STEP_ACTION_NAME_CHIP_RESET)
#define STEP_KEEP_WAKEUP_ACT_INT (int)(*(int *)STEP_ACTION_NAME_KEEP_WAKEUP)
#define STEP_CANCEL_KEEP_WAKEUP_ACT_INT (int)(*(int *)STEP_ACTION_NAME_CANCEL_WAKEUP)
#define STEP_SHOW_STRING_ACT_INT (int)(*(int *)STEP_ACTION_NAME_SHOW_STRING)
#define STEP_SLEEP_ACT_INT (int)(*(int *)STEP_ACTION_NAME_SLEEP)
#define STEP_CONDITION_ACT_INT (int)(*(int *)STEP_ACTION_NAME_CONDITION)
#define STEP_VALUE_ACT_INT (int)(*(int *)STEP_ACTION_NAME_VALUE)
#define STEP_CONDITION_EMI_ACT_INT (int)(*(int *)STEP_ACTION_NAME_CONDITION_EMI)
#define STEP_CONDITION_REG_ACT_INT (int)(*(int *)STEP_ACTION_NAME_CONDITION_REGISTER)

#define STEP_PARSE_LINE_STATE_INIT 		0
#define STEP_PARSE_LINE_STATE_TP		1
#define STEP_PARSE_LINE_STATE_TP_ID		2
#define STEP_PARSE_LINE_STATE_AT		3
#define STEP_PARSE_LINE_STATE_AT_OP		4
#define STEP_PARSE_LINE_STATE_PD_START	5
#define STEP_PARSE_LINE_STATE_PD_END	6

/*******************************************************************************
 *                           P R I V A T E   D A T A
********************************************************************************/


typedef int (*STEP_LINE_STATE) (char *,
	struct step_target_act_list_info *, struct step_parse_line_data_param_info *);

static const STEP_LINE_STATE conninfra_step_line_state_action_map[] = {
	[STEP_PARSE_LINE_STATE_INIT] = conninfra_step_access_line_state_init,
	[STEP_PARSE_LINE_STATE_TP] = conninfra_step_access_line_state_tp,
	[STEP_PARSE_LINE_STATE_TP_ID] = conninfra_step_access_line_state_tp_id,
	[STEP_PARSE_LINE_STATE_AT] = conninfra_step_access_line_state_at,
	[STEP_PARSE_LINE_STATE_AT_OP] = conninfra_step_access_line_state_at_op,
	[STEP_PARSE_LINE_STATE_PD_START] = conninfra_step_access_line_state_pd,
};

/*******************************************************************************
 *                      I N T E R N A L   F U N C T I O N S
********************************************************************************/

#define STEP_PARSE_LINE_RET_CONTINUE 0
#define STEP_PARSE_LINE_RET_BREAK 1

static void conninfra_step_set_line_state(int *p_state, int value)
{
	*p_state = value;
}

static unsigned char conninfra_step_to_upper(char str)
{
	if ((str >= 'a') && (str <= 'z'))
		return str + ('A' - 'a');
	else
		return str;
}

static void conninfra_step_string_to_upper(char *tok)
{
	for (; *tok != '\0'; tok++)
		*tok = conninfra_step_to_upper(*tok);
}

static int conninfra_step_get_int_from_four_char(char *str)
{
	unsigned char char_array[4];
	int i;

	for (i = 0; i < 4; i++) {
		if (*(str + i) == '\0')
			return -1;

		char_array[i] = conninfra_step_to_upper(*(str + i));
	}

	return *(int *)char_array;
}



static unsigned int conninfra_step_parse_tp_id(enum step_drv_type drv_type, char *str)
{
	long tp_id = STEP_TP_NO_DEFINE;

	if (osal_strtol(str, 10, &tp_id)) {
		pr_err("STEP failed: str to value %s\n", str);
		return STEP_TP_NO_DEFINE;
	}
	if (tp_id <= STEP_TP_NO_DEFINE || tp_id >= g_step_drv_type_def[drv_type].tp_max)
		return STEP_TP_NO_DEFINE;

	return tp_id;
}


static int conninfra_step_parse_pd_expires(char *ptr)
{
	long expires_ms;

	if (osal_strtol(ptr, 0, &expires_ms))
		return -1;

	return (int)expires_ms;
}

static int conninfra_step_parse_act_id(char *str)
{
	int str_to_int = STEP_ACTION_INDEX_NO_DEFINE;

	if (str == NULL || str == '\0')
		return STEP_ACTION_INDEX_NO_DEFINE;

	str_to_int = conninfra_step_get_int_from_four_char(str);
	if (str_to_int == STEP_EMI_ACT_INT)
		return STEP_ACTION_INDEX_EMI;
	else if (str_to_int == STEP_REG_ACT_INT)
		return STEP_ACTION_INDEX_REGISTER;
	else if (str_to_int == STEP_GPIO_ACT_INT)
		return STEP_ACTION_INDEX_GPIO;
	else if (str_to_int == STEP_SHOW_STRING_ACT_INT)
		return STEP_ACTION_INDEX_SHOW_STRING;
	else if (str_to_int == STEP_SLEEP_ACT_INT)
		return STEP_ACTION_INDEX_SLEEP;
	else if (str_to_int == STEP_CONDITION_ACT_INT)
		return STEP_ACTION_INDEX_CONDITION;
	else if (str_to_int == STEP_VALUE_ACT_INT)
		return STEP_ACTION_INDEX_VALUE;
	else if (str_to_int == STEP_CONDITION_EMI_ACT_INT)
		return STEP_ACTION_INDEX_CONDITION_EMI;
	else if (str_to_int == STEP_CONDITION_REG_ACT_INT)
		return STEP_ACTION_INDEX_CONDITION_REGISTER;
	else
		return STEP_ACTION_INDEX_NO_DEFINE;

}


static int conninfra_step_access_line_state_init(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	conninfra_step_string_to_upper(tok);
	if (osal_strcmp(tok, "[TP") == 0) {
		conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_TP);
		return STEP_PARSE_LINE_RET_CONTINUE;
	}

	if (p_parse_info->tp_id == STEP_TP_NO_DEFINE) {
		pr_err("STEP failed: Set trigger point first: %s\n", tok);
		return STEP_PARSE_LINE_RET_BREAK;
	}

	if (osal_strcmp(tok, "[PD+]") == 0) {
		conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_PD_START);
		return STEP_PARSE_LINE_RET_CONTINUE;
	}

	if (osal_strcmp(tok, "[PD-]") == 0) {
		conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_PD_END);
		return STEP_PARSE_LINE_RET_BREAK;
	}

	if (osal_strcmp(tok, "[AT]") == 0) {
		conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_AT);
		return STEP_PARSE_LINE_RET_CONTINUE;
	}

	return STEP_PARSE_LINE_RET_BREAK;
}

static int conninfra_step_drv_type(char *tok, enum step_drv_type *drv_type)
{
	int i;
	const char *p;

	for (i = 0; i < STEP_DRV_TYPE_MAX; i++) {
		p = g_step_drv_type_def[i].drv_type_str;
		if (!strncmp(tok, p, strlen(p))) {
			*drv_type = (enum step_drv_type)i;
			return strlen(p);
		}
	}
	return -1;
}

static int conninfra_step_access_line_state_tp(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	int ret = 0;
	enum step_drv_type drv_type = STEP_DRV_TYPE_NO_DEFINE;

	conninfra_step_string_to_upper(tok);
	if (p_parse_info->p_pd_entry != NULL) {
		pr_err("STEP failed: Please add [PD-] after [PD+], tok = %s\n", tok);
		p_parse_info->p_pd_entry = NULL;
	}

	ret = conninfra_step_drv_type(tok, &drv_type);
	if (ret <= 0) {
		pr_err("STEP failed: Trigger point format is wrong: %s\n", tok);
		return STEP_PARSE_LINE_RET_BREAK;
	}

	p_parse_info->drv_type = drv_type;
	conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_TP_ID);

	return STEP_PARSE_LINE_RET_CONTINUE;
}

static int conninfra_step_access_line_state_tp_id(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	char *pch;
	enum step_drv_type drv_type = p_parse_info->drv_type;

	if (p_parse_info->p_pd_entry != NULL) {
		pr_err("STEP failed: Please add [PD-] after [PD+], tok = %s\n", tok);
		p_parse_info->p_pd_entry = NULL;
	}

	pch = osal_strchr(tok, ']');
	if (pch == NULL) {
		pr_err("STEP failed: Trigger point format is wrong: %s\n", tok);
	} else {
		*pch = '\0';
		p_parse_info->tp_id = conninfra_step_parse_tp_id(drv_type, tok);
		p_parse_info->p_target_list = conninfra_step_get_tp_list(drv_type, p_parse_info->tp_id);

		if (p_parse_info->tp_id == STEP_TP_NO_DEFINE)
			pr_err("STEP failed: Trigger point no define: %s\n", tok);
	}

	return STEP_PARSE_LINE_RET_BREAK;
}

static int conninfra_step_access_line_state_at(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	p_parse_line_info->act_id = conninfra_step_parse_act_id(tok);
	if (p_parse_line_info->act_id == STEP_ACTION_INDEX_NO_DEFINE) {
		pr_err("STEP failed: Action no define: %s\n", tok);
		return STEP_PARSE_LINE_RET_BREAK;
	}
	conninfra_step_set_line_state(&p_parse_line_info->state, STEP_PARSE_LINE_STATE_AT_OP);

	return STEP_PARSE_LINE_RET_CONTINUE;
}

static int conninfra_step_access_line_state_at_op(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	p_parse_line_info->act_params[p_parse_line_info->param_index] = tok;
	(p_parse_line_info->param_index)++;

	if (p_parse_line_info->param_index >= STEP_PARAMETER_SIZE) {
		pr_err("STEP failed: Param too much");
		return STEP_PARSE_LINE_RET_BREAK;
	}

	return STEP_PARSE_LINE_RET_CONTINUE;
}

static int conninfra_step_access_line_state_pd(char *tok,
	struct step_target_act_list_info *p_parse_info,
	struct step_parse_line_data_param_info *p_parse_line_info)
{
	int pd_ms = -1;

	pd_ms = conninfra_step_parse_pd_expires(tok);
	if (pd_ms == -1)
		pr_err("STEP failed: PD ms failed %s\n", tok);

	if (p_parse_info->p_pd_entry != NULL)
		pr_err("STEP failed: Please add [PD-] after [PD+], tok = %s\n", tok);

	p_parse_info->p_pd_entry = conninfra_step_get_periodic_dump_entry(pd_ms);
	if (p_parse_info->p_pd_entry == NULL)
		pr_err("STEP failed: p_pd_entry create fail\n");
	else
		p_parse_info->p_target_list = &(p_parse_info->p_pd_entry->action_list);

	return STEP_PARSE_LINE_RET_BREAK;
}




static void conninfra_step_parse_line_data(char *line, struct step_target_act_list_info *p_parse_info,
	STEP_WRITE_ACT_TO_LIST func_act_to_list)
{
	char *tok;
	int line_ret = STEP_PARSE_LINE_RET_BREAK;
	struct step_parse_line_data_param_info parse_line_info;

	parse_line_info.param_index = 0;
	parse_line_info.act_id = STEP_ACTION_INDEX_NO_DEFINE;
	parse_line_info.state = STEP_PARSE_LINE_STATE_INIT;

	while ((tok = osal_strsep(&line, " \t")) != NULL) {
		if (*tok == '\0')
			continue;
		if (osal_strcmp(tok, "//") == 0)
			break;

		if (conninfra_step_line_state_action_map[parse_line_info.state] != NULL) {
			line_ret = conninfra_step_line_state_action_map[parse_line_info.state] (tok,
				p_parse_info, &parse_line_info);
		}

		if (line_ret == STEP_PARSE_LINE_RET_CONTINUE)
			continue;
		else
			break;
	}

	if (parse_line_info.state == STEP_PARSE_LINE_STATE_AT_OP) {
		func_act_to_list(p_parse_info->p_target_list, p_parse_info->drv_type,
			parse_line_info.act_id, parse_line_info.param_index, parse_line_info.act_params);
	} else if (parse_line_info.state == STEP_PARSE_LINE_STATE_PD_END) {
		p_parse_info->p_target_list = conninfra_step_get_tp_list(p_parse_info->drv_type, p_parse_info->tp_id);
		if (p_parse_info->p_pd_entry != NULL) {
			parse_line_info.act_params[0] = (char*)p_parse_info->p_pd_entry;
			func_act_to_list(p_parse_info->p_target_list, p_parse_info->drv_type,
				STEP_ACTION_INDEX_PERIODIC_DUMP, parse_line_info.param_index,
				parse_line_info.act_params);
			p_parse_info->p_pd_entry = NULL;
		}
	}
}



int conninfra_step_parse_data(const char *in_buf, unsigned int size,
	STEP_WRITE_ACT_TO_LIST func_act_to_list)
{
	struct step_target_act_list_info parse_info;
	char *buf, *tmp_buf;
	char *line;

	buf = osal_malloc(size + 1);
	if (!buf) {
		pr_err("STEP failed: Buf malloc\n");
		return -1;
	}

	osal_memcpy(buf, (char *)in_buf, size);
	buf[size] = '\0';

	parse_info.drv_type = STEP_DRV_TYPE_NO_DEFINE;
	parse_info.tp_id = STEP_TP_NO_DEFINE;
	parse_info.p_target_list = NULL;
	parse_info.p_pd_entry = NULL;

	tmp_buf = buf;
	while ((line = osal_strsep(&tmp_buf, "\r\n")) != NULL)
		conninfra_step_parse_line_data(line, &parse_info, func_act_to_list);

	osal_free(buf);

	return 0;
}



/*******************************************************************************
 *              I N T E R N A L   F U N C T I O N S   W I T H   U T
********************************************************************************/

