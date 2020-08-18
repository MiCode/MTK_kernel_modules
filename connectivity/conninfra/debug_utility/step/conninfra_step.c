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
#include "conninfra_core.h"
#include "consys_hw.h"
#include "emi_mng.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************/

static void conninfra_step_remove_emi_action(struct step_action *p_act);
static void conninfra_step_remove_register_action(struct step_action *p_act);
static void conninfra_step_remove_gpio_action(struct step_action *p_act);
static void conninfra_step_remove_periodic_dump_action(struct step_action *p_act);
static void conninfra_step_remove_show_string_action(struct step_action *p_act);
static void conninfra_step_remove_sleep_action(struct step_action *p_act);
static void conninfra_step_remove_condition_action(struct step_action *p_act);
static void conninfra_step_remove_value_action(struct step_action *p_act);
static void conninfra_step_remove_condition_emi_action(struct step_action *p_act);
static void conninfra_step_remove_condition_register_action(struct step_action *p_act);


static int conninfra_step_operator_result_greater(int l_val, int r_val);
static int conninfra_step_operator_result_greater_equal(int l_val, int r_val);
static int conninfra_step_operator_result_less(int l_val, int r_val);
static int conninfra_step_operator_result_less_equal(int l_val, int r_val);
static int conninfra_step_operator_result_equal(int l_val, int r_val);
static int conninfra_step_operator_result_not_equal(int l_val, int r_val);
static int conninfra_step_operator_result_and(int l_val, int r_val);
static int conninfra_step_operator_result_or(int l_val, int r_val);


/*******************************************************************************
 *                           D E F I N E
********************************************************************************/

/*******************************************************************************
 *                           P R I V A T E   D A T A
********************************************************************************/
struct step_env_struct g_infra_step_env;

struct step_drv_type_def g_step_drv_type_def[STEP_DRV_TYPE_MAX] = {
	[STEP_DRV_TYPE_NO_DEFINE] = {"NoDefine", -1, NULL, NULL},
	[STEP_DRV_TYPE_CONNINFRA] = {"CONNINFRA", STEP_CONNINFRA_TP_MAX, NULL, &g_infra_step_env.conninfra_actions[0]},
	[STEP_DRV_TYPE_BT] = {"BT", STEP_BT_TP_MAX, NULL, &g_infra_step_env.bt_actions[0]},
	[STEP_DRV_TYPE_FM] = {"FM", STEP_FM_TP_MAX, NULL, &g_infra_step_env.fm_actions[0]},
	[STEP_DRV_TYPE_GPS] = {"GPS", STEP_GPS_TP_MAX, NULL, &g_infra_step_env.gps_actions[0]},
	[STEP_DRV_TYPE_WIFI] = {"WF", STEP_WF_TP_MAX, NULL, &g_infra_step_env.wf_actions[0]},
};



static const struct step_action_contrl conninfra_step_action_map[] = {
	[STEP_ACTION_INDEX_EMI] = {
		conninfra_step_create_emi_action,
		conninfra_step_do_emi_action,
		conninfra_step_remove_emi_action
	},
	[STEP_ACTION_INDEX_REGISTER] = {
		conninfra_step_create_register_action,
		conninfra_step_do_register_action,
		conninfra_step_remove_register_action
	},
	[STEP_ACTION_INDEX_GPIO] = {
		conninfra_step_create_gpio_action,
		conninfra_step_do_gpio_action,
		conninfra_step_remove_gpio_action
	},
	[STEP_ACTION_INDEX_PERIODIC_DUMP] = {
		conninfra_step_create_periodic_dump_action,
		conninfra_step_do_periodic_dump_action,
		conninfra_step_remove_periodic_dump_action,
	},
	[STEP_ACTION_INDEX_SHOW_STRING] = {
		conninfra_step_create_show_string_action,
		conninfra_step_do_show_string_action,
		conninfra_step_remove_show_string_action
	},
	[STEP_ACTION_INDEX_SLEEP] = {
		conninfra_step_create_sleep_action,
		conninfra_step_do_sleep_action,
		conninfra_step_remove_sleep_action
	},
	[STEP_ACTION_INDEX_CONDITION] = {
		conninfra_step_create_condition_action,
		conninfra_step_do_condition_action,
		conninfra_step_remove_condition_action
	},
	[STEP_ACTION_INDEX_VALUE] = {
		conninfra_step_create_value_action,
		conninfra_step_do_value_action,
		conninfra_step_remove_value_action
	},
	[STEP_ACTION_INDEX_CONDITION_EMI] = {
		conninfra_step_create_condition_emi_action,
		conninfra_step_do_condition_emi_action,
		conninfra_step_remove_condition_emi_action
	},
	[STEP_ACTION_INDEX_CONDITION_REGISTER] = {
		conninfra_step_create_condition_register_action,
		conninfra_step_do_condition_register_action,
		conninfra_step_remove_condition_register_action
	},
};

typedef int (*STEP_OPERATOR_RESULT) (int, int);
static const STEP_OPERATOR_RESULT conninfra_step_operator_result_map[] = {
	[STEP_OPERATOR_GREATER] = conninfra_step_operator_result_greater,
	[STEP_OPERATOR_GREATER_EQUAL] = conninfra_step_operator_result_greater_equal,
	[STEP_OPERATOR_LESS] = conninfra_step_operator_result_less,
	[STEP_OPERATOR_LESS_EQUAL] = conninfra_step_operator_result_less_equal,
	[STEP_OPERATOR_EQUAL] = conninfra_step_operator_result_equal,
	[STEP_OPERATOR_NOT_EQUAL] = conninfra_step_operator_result_not_equal,
	[STEP_OPERATOR_AND] = conninfra_step_operator_result_and,
	[STEP_OPERATOR_OR] = conninfra_step_operator_result_or,
};

/*******************************************************************************
 *                      I N T E R N A L   F U N C T I O N S
********************************************************************************/

static void conninfra_step_init_list(void)
{
	unsigned int i = 0;

	for (i = 0; i < STEP_CONNINFRA_TP_MAX; i++)
		INIT_LIST_HEAD(&(g_infra_step_env.conninfra_actions[i].list));

	for (i = 0; i < STEP_WF_TP_MAX; i++)
		INIT_LIST_HEAD(&(g_infra_step_env.wf_actions[i].list));

	for (i = 0; i < STEP_BT_TP_MAX; i++)
		INIT_LIST_HEAD(&(g_infra_step_env.bt_actions[i].list));

	for (i = 0; i < STEP_GPS_TP_MAX; i++)
		INIT_LIST_HEAD(&(g_infra_step_env.gps_actions[i].list));

	for (i = 0; i < STEP_FM_TP_MAX; i++)
		INIT_LIST_HEAD(&(g_infra_step_env.fm_actions[i].list));

}

static unsigned char __iomem *conninfra_step_get_emi_base_address(void)
{
	struct consys_emi_addr_info* emi_addr_info = NULL;

	if (g_infra_step_env.emi_base_addr == NULL) {
		emi_addr_info = emi_mng_get_phy_addr();
		if (emi_addr_info != NULL) {
			g_infra_step_env.emi_base_addr = ioremap_nocache(emi_addr_info->emi_ap_phy_addr,
											emi_addr_info->emi_size);
			g_infra_step_env.emi_size = emi_addr_info->emi_size;
		}
	}

	return g_infra_step_env.emi_base_addr;
}

static void conninfra_step_clear_action_list(struct step_action_list *action_list)
{
	struct step_action *p_act, *p_act_next;

	list_for_each_entry_safe(p_act, p_act_next, &(action_list->list), list) {
		list_del_init(&p_act->list);
		conninfra_step_remove_action(p_act);
	}
}

static void conninfra_step_clear_list(void)
{
	unsigned int i = 0;

	for (i = 0; i < STEP_CONNINFRA_TP_MAX; i++)
		conninfra_step_clear_action_list(&g_infra_step_env.conninfra_actions[i]);

	for (i = 0; i < STEP_WF_TP_MAX; i++)
		conninfra_step_clear_action_list(&g_infra_step_env.wf_actions[i]);

	for (i = 0; i < STEP_BT_TP_MAX; i++)
		conninfra_step_clear_action_list(&g_infra_step_env.bt_actions[i]);

	for (i = 0; i < STEP_GPS_TP_MAX; i++)
		conninfra_step_clear_action_list(&g_infra_step_env.gps_actions[i]);

	for (i = 0; i < STEP_FM_TP_MAX; i++)
		conninfra_step_clear_action_list(&g_infra_step_env.fm_actions[i]);

#if 0
	for (i = 0; i < STEP_DRV_TYPE_MAX; i++)
		for (j = 0; j < STEP_TRIGGER_POINT_MAX; j++)
			conninfra_step_clear_action_list(&g_infra_step_env.actions[i][j]);
#endif
}

static void conninfra_step_unioremap_emi(void)
{
	if (g_infra_step_env.emi_base_addr != NULL) {
		iounmap(g_infra_step_env.emi_base_addr);
		g_infra_step_env.emi_base_addr = NULL;
	}
}

static unsigned char *conninfra_step_get_emi_virt_addr(unsigned char *emi_base_addr, unsigned int offset)
{
	unsigned char *p_virtual_addr = NULL;

	if (offset > g_infra_step_env.emi_size) {
		pr_err("STEP failed: offset size %d over MAX size(%d)\n", offset,
			g_infra_step_env.emi_size);
		return NULL;
	}
	p_virtual_addr = emi_base_addr + offset;

	return p_virtual_addr;
}

static int conninfra_step_get_cfg(const char *p_patch_name, osal_firmware **pp_patch)
{
	osal_firmware *fw = NULL;

	*pp_patch = NULL;
	if (request_firmware((const struct firmware **)&fw, p_patch_name, NULL) != 0)
		return -1;

	pr_debug("Load step cfg %s ok!!\n", p_patch_name);
	*pp_patch = fw;

	return 0;
}

static int conninfra_step_release_cfg(osal_firmware ** ppPatch)
{
	if (ppPatch != NULL) {
		release_firmware((const struct firmware *)*ppPatch);
		*ppPatch = NULL;
	}
	return 0;
}

static void conninfra_step_sleep_or_delay(int ms)
{
	/* msleep < 20ms can sleep for up to 20ms */
	if (ms < 20)
		udelay(ms * 1000);
	else
		osal_sleep_ms(ms);
}

struct step_action_list *conninfra_step_get_tp_list(enum step_drv_type drv_type, int tp_id)
{
	if (drv_type <= STEP_DRV_TYPE_NO_DEFINE || drv_type >= STEP_DRV_TYPE_MAX) {
		pr_err("STEP failed: incorrect drv type: %d\n", drv_type);
		return NULL;
	}

	if (tp_id <= STEP_TP_NO_DEFINE || tp_id >= g_step_drv_type_def[drv_type].tp_max) {
		pr_err("STEP failed: Write action to drv_type [%d] tp_id: [%d]\n",
					drv_type, tp_id);
		return NULL;
	}

	return &g_step_drv_type_def[drv_type].action_list[tp_id];
}

static void _conninfra_step_do_actions(struct step_action_list *action_list)
{
	struct step_action *p_act, *p_act_next;

	list_for_each_entry_safe(p_act, p_act_next, &action_list->list, list) {
		if (p_act->action_id <= STEP_ACTION_INDEX_NO_DEFINE || p_act->action_id >= STEP_ACTION_INDEX_MAX) {
			pr_err("STEP failed: Wrong action id %d\n", (int)p_act->action_id);
			continue;
		}

		if (conninfra_step_action_map[p_act->action_id].func_do_action != NULL)
			conninfra_step_action_map[p_act->action_id].func_do_action(p_act, NULL);
		else
			pr_err("STEP failed: Action is NULL\n");
	}
}

static void conninfra_step_start_work(struct step_pd_entry *p_entry)
{
	unsigned int timeout;
	int result = 0;

	if (!g_infra_step_env.pd_struct.step_pd_wq) {
		pr_err("STEP failed: step wq doesn't run\n");
		result = -1;
	}

	if (p_entry == NULL) {
		pr_err("STEP failed: entry is null\n");
		result = -1;
	}

	if (result == 0) {
		timeout = p_entry->expires_ms;
		queue_delayed_work(g_infra_step_env.pd_struct.step_pd_wq, &p_entry->pd_work, timeout);
	}
}

static void conninfra_step_pd_work(struct work_struct *work)
{
	struct step_pd_entry *p_entry;
	struct delayed_work *delayed_work;
	int result = 0;

	if (down_read_trylock(&g_infra_step_env.init_rwsem)) {
		if (!g_infra_step_env.is_enable) {
			pr_err("STEP failed: step doesn`t enable\n");
			result = -1;
		}

		delayed_work = to_delayed_work(work);
		if (delayed_work == NULL) {
			pr_err("STEP failed: work is NULL\n");
			result = -1;
		}

		if (result == 0) {
			p_entry = container_of(delayed_work, struct step_pd_entry, pd_work);

			pr_info("STEP show: Periodic dump: %d ms\n", p_entry->expires_ms);
			_conninfra_step_do_actions(&p_entry->action_list);
			conninfra_step_start_work(p_entry);
		}

		up_read(&g_infra_step_env.init_rwsem);
	}
}

static struct step_pd_entry *conninfra_step_create_periodic_dump_entry(unsigned int expires)
{
	struct step_pd_entry *p_pd_entry = NULL;

	p_pd_entry = kzalloc(sizeof(struct step_pd_entry), GFP_KERNEL);
	if (p_pd_entry == NULL) {
		pr_err("STEP failed: kzalloc fail\n");
		return NULL;
	}
	p_pd_entry->expires_ms = expires;

	INIT_DELAYED_WORK(&p_pd_entry->pd_work, conninfra_step_pd_work);
	INIT_LIST_HEAD(&(p_pd_entry->action_list.list));

	return p_pd_entry;
}

static void conninfra_step_print_trigger_time(enum consys_conninfra_step_trigger_point tp_id, char *reason)
{
	const char *p_trigger_name = NULL;

	/* TODO: print trigger point name */
	//p_trigger_name = STEP_TRIGGER_TIME_NAME[tp_id];
	p_trigger_name = "";
	if (reason != NULL)
		pr_info("STEP show: Trigger point: %s reason: %s\n", p_trigger_name, reason);
	else
		pr_info("STEP show: Trigger point: %s\n", p_trigger_name);
}

void conninfra_step_do_actions_from_tp(enum step_drv_type drv_type,
		unsigned int tp_id, char *reason)
{
	int result = 0;

	if (down_read_trylock(&g_infra_step_env.init_rwsem)) {
		if (g_infra_step_env.is_enable == 0)
			result = -1;


		if (tp_id <= STEP_TP_NO_DEFINE || tp_id >= g_step_drv_type_def[drv_type].tp_max) {
			pr_err("STEP failed: Do actions from tp_id: %d\n", tp_id);
			result = -1;
		//} else if (list_empty(&g_infra_step_env.actions[tp_id].list)) {
		} else if (list_empty(&g_step_drv_type_def[drv_type].action_list[tp_id].list)) {
			result = -1;
		}

		if (result == 0) {
			conninfra_step_print_trigger_time(tp_id, reason);
			_conninfra_step_do_actions(&g_step_drv_type_def[drv_type].action_list[tp_id]);
		}

		up_read(&g_infra_step_env.init_rwsem);
	}
}

static int conninfra_step_write_action(struct step_action_list *p_list, enum step_drv_type drv_type,
	enum step_action_id act_id, int param_num, char *params[])
{
	struct step_action *p_action;

	if (p_list == NULL) {
		pr_err("STEP failed: p_list is null\n");
		return -1;
	}

	p_action = conninfra_step_create_action(drv_type, act_id, param_num, params);
	if (p_action != NULL) {
		list_add_tail(&(p_action->list), &(p_list->list));
		return 0;
	}

	return -1;
}

static int conninfra_step_operator_result_greater(int l_val, int r_val)
{
	return (l_val > r_val);
}

static int conninfra_step_operator_result_greater_equal(int l_val, int r_val)
{
	return (l_val >= r_val);
}

static int conninfra_step_operator_result_less(int l_val, int r_val)
{
	return (l_val < r_val);
}

static int conninfra_step_operator_result_less_equal(int l_val, int r_val)
{
	return (l_val <= r_val);
}

static int conninfra_step_operator_result_equal(int l_val, int r_val)
{
	return (l_val == r_val);
}

static int conninfra_step_operator_result_not_equal(int l_val, int r_val)
{
	return (l_val != r_val);
}

static int conninfra_step_operator_result_and(int l_val, int r_val)
{
	return (l_val && r_val);
}

static int conninfra_step_operator_result_or(int l_val, int r_val)
{
	return (l_val || r_val);
}


void conninfra_step_create_emi_cb(int mode, int write,
	unsigned int begin, unsigned int mask, unsigned int reg_id) {
}

static int conninfra_step_do_write_register_action(struct step_register_info *p_reg_info,
	STEP_DO_EXTRA func_do_extra)
{
	phys_addr_t phy_addr;
	void __iomem *p_addr = NULL;

	phy_addr = p_reg_info->address + p_reg_info->offset;

	if (phy_addr & 0x3) {
		pr_err("STEP failed: phy_addr(0x%08x) page failed\n", phy_addr);
		return -1;
	}

	p_addr = ioremap_nocache(phy_addr, 0x4);
	if (p_addr) {
		CONSYS_REG_WRITE_MASK((unsigned int *)p_addr, p_reg_info->value, p_reg_info->mask);
		pr_info(
			"STEP show: reg write Phy addr(0x%08x): 0x%08x\n",
			(unsigned int)phy_addr, CONSYS_REG_READ(p_addr));
		if (func_do_extra != NULL)
			func_do_extra(1, CONSYS_REG_READ(p_addr));
		iounmap(p_addr);
	} else {
		pr_err("STEP failed: ioremap(0x%08x) is NULL\n", phy_addr);
		return -1;
	}
	return 0;
}

static void _conninfra_step_do_read_register_action(struct step_register_info *p_reg_info,
	STEP_DO_EXTRA func_do_extra, char *info, int value)
{
	int i;

	for (i = 0; i < p_reg_info->times; i++) {
		if (i > 0)
			conninfra_step_sleep_or_delay(p_reg_info->delay_time);

		if (p_reg_info->output_mode == STEP_OUTPUT_REGISTER) {
			g_infra_step_env.temp_register[p_reg_info->temp_reg_id] = value & p_reg_info->mask;

			pr_info("[%s] value=[%d] mask=[%d]", __func__, value, p_reg_info->mask);
		} else
			pr_info("%s", info);

		if (func_do_extra != NULL)
			func_do_extra(1, value);
	}
}

static int conninfra_step_do_read_register_action(struct step_register_info *p_reg_info,
	STEP_DO_EXTRA func_do_extra)
{
	phys_addr_t phy_addr;
	void __iomem *p_addr = NULL;
	char buf[128];

	phy_addr = p_reg_info->address + p_reg_info->offset;

	if (phy_addr & 0x3) {
		pr_err("STEP failed: phy_addr(0x%08x) page failed\n", phy_addr);
		return -1;
	}

	p_addr = ioremap_nocache(phy_addr, 0x4);
	if (p_addr) {
		sprintf(buf, "STEP show: reg read Phy addr(0x%08x): 0x%08x\n",
			(unsigned int)phy_addr, CONSYS_REG_READ(p_addr));
		_conninfra_step_do_read_register_action(p_reg_info, func_do_extra, buf,
			CONSYS_REG_READ(p_addr));
		iounmap(p_addr);
	} else {
		pr_err("STEP failed: ioremap(0x%08x) is NULL\n", phy_addr);
		return -1;
	}

	return 0;
}

static void conninfra_step_remove_emi_action(struct step_action *p_act)
{
	struct step_emi_action *p_emi_act = NULL;

	p_emi_act = clist_entry_action(emi, p_act);
	kfree(p_emi_act);
}

static void conninfra_step_remove_register_action(struct step_action *p_act)
{
	struct step_register_action *p_reg_act = NULL;

	p_reg_act = clist_entry_action(register, p_act);
	kfree(p_reg_act);
}

static void conninfra_step_remove_gpio_action(struct step_action *p_act)
{
	struct step_gpio_action *p_gpio_act = NULL;

	p_gpio_act = clist_entry_action(gpio, p_act);
	kfree(p_gpio_act);
}

static void conninfra_step_remove_periodic_dump_action(struct step_action *p_act)
{
	struct step_periodic_dump_action *p_pd = NULL;

	p_pd = clist_entry_action(periodic_dump, p_act);
	kfree(p_pd);
}

static void conninfra_step_remove_show_string_action(struct step_action *p_act)
{
	struct step_show_string_action *p_show = NULL;

	p_show = clist_entry_action(show_string, p_act);
	if (p_show->content != NULL)
		kfree(p_show->content);

	kfree(p_show);
}

static void conninfra_step_remove_sleep_action(struct step_action *p_act)
{
	struct step_sleep_action *p_sleep = NULL;

	p_sleep = clist_entry_action(sleep, p_act);
	kfree(p_sleep);
}

static void conninfra_step_remove_condition_action(struct step_action *p_act)
{
	struct step_condition_action *p_cond = NULL;

	p_cond = clist_entry_action(condition, p_act);
	kfree(p_cond);
}

static void conninfra_step_remove_value_action(struct step_action *p_act)
{
	struct step_value_action *p_val = NULL;

	p_val = clist_entry_action(value, p_act);
	kfree(p_val);
}

static void conninfra_step_remove_condition_emi_action(struct step_action *p_act)
{
	struct step_condition_emi_action *p_cond_emi_act = NULL;

	p_cond_emi_act = clist_entry_action(condition_emi, p_act);
	kfree(p_cond_emi_act);
}

static void conninfra_step_remove_condition_register_action(struct step_action *p_act)
{
	struct step_condition_register_action *p_cond_reg_act = NULL;

	p_cond_reg_act = clist_entry_action(condition_register, p_act);
	kfree(p_cond_reg_act);
}

static int _conninfra_step_do_emi_action(struct step_emi_info *p_emi_info, STEP_DO_EXTRA func_do_extra)
{
	unsigned char *p_emi_begin_addr = NULL, *p_emi_end_addr = NULL;
	unsigned char __iomem *emi_base_addr = NULL;
	unsigned int dis = 0, temp = 0, i = 0;
	struct consys_emi_addr_info *emi_addr_info = NULL;

	if (p_emi_info->is_write != 0) {
		pr_err("STEP failed: Only support dump EMI region\n");
		return -1;
	}

	if (p_emi_info->begin_offset > p_emi_info->end_offset) {
		temp = p_emi_info->begin_offset;
		p_emi_info->begin_offset = p_emi_info->end_offset;
		p_emi_info->end_offset = temp;
	}
	dis = p_emi_info->end_offset - p_emi_info->begin_offset;

	emi_base_addr = conninfra_step_get_emi_base_address();
	if (emi_base_addr == NULL) {
		pr_err("STEP failed: EMI base address is NULL\n");
		return -1;
	}

	if (p_emi_info->begin_offset & 0x3) {
		pr_err("STEP failed: begin offset(0x%08x) page failed\n",
			p_emi_info->begin_offset);
		return -1;
	}

	p_emi_begin_addr = conninfra_step_get_emi_virt_addr(emi_base_addr, p_emi_info->begin_offset);
	p_emi_end_addr = conninfra_step_get_emi_virt_addr(emi_base_addr, p_emi_info->end_offset);
	if (!p_emi_begin_addr) {
		pr_err("STEP failed: Get NULL begin virtual address 0x%08x\n",
			p_emi_info->begin_offset);
		return -1;
	}

	if (!p_emi_end_addr) {
		pr_err("STEP failed: Get NULL end virtual address 0x%08x\n",
			p_emi_info->end_offset);
		return -1;
	}

	for (i = 0; i < dis; i += 0x4) {
		if (p_emi_info->output_mode == STEP_OUTPUT_REGISTER) {
			g_infra_step_env.temp_register[p_emi_info->temp_reg_id] =
				(CONSYS_REG_READ(p_emi_begin_addr + i) & p_emi_info->mask);
		} else {
			emi_addr_info = emi_mng_get_phy_addr();
			//if (emi_mng_get_phy_addr(&phy_addr, &phy_size) != 0)
			if (emi_addr_info == NULL)
				pr_warn("STEP show: get phy addr fail");

			pr_info("STEP show: EMI action, Phy address(0x%08x): 0x%08x\n",
				(unsigned int) (emi_addr_info->emi_ap_phy_addr + p_emi_info->begin_offset + i),
				CONSYS_REG_READ(p_emi_begin_addr + i));
		}

		if (func_do_extra != NULL)
			func_do_extra(1, CONSYS_REG_READ(p_emi_begin_addr + i));
	}

	return 0;
}

static bool conninfra_step_reg_readable(struct step_register_info *p_reg_info)
{
	phys_addr_t phy_addr;
	enum step_drv_type drv_type = p_reg_info->drv_type;

	if (p_reg_info->address_type == 0) {
		phy_addr = p_reg_info->address + p_reg_info->offset;
		switch (drv_type) {
		case STEP_DRV_TYPE_CONNINFRA:
			return consys_reg_mng_reg_readable();
		case STEP_DRV_TYPE_BT:
		case STEP_DRV_TYPE_FM:
		case STEP_DRV_TYPE_GPS:
		case STEP_DRV_TYPE_WIFI:
			if (g_step_drv_type_def[drv_type].drv_cb == NULL)
				return 0;
			return g_step_drv_type_def[drv_type].drv_cb->readable_cb(phy_addr);
		default:
			pr_err("STEP: reg readable drv type(%d) incorrect", drv_type);
		};
	} else {
		return consys_reg_mng_reg_readable();
	}
	return 0;
}

int _conninfra_step_do_register_action(struct step_register_info *p_reg_info, STEP_DO_EXTRA func_do_extra)
{
	int ret = 0, r;

	ret = conninfra_core_force_conninfra_wakeup();
	if (ret) {
		pr_err("STEP failed: wakeup conninfra fail\n");
		return -3;
	}

	if (!conninfra_step_reg_readable(p_reg_info)) {
		pr_err("STEP failed: register cannot read\n");
		ret = -1;
		goto err;
	}

	if (p_reg_info->is_write == 1)
		ret = conninfra_step_do_write_register_action(p_reg_info, func_do_extra);
	else
		ret = conninfra_step_do_read_register_action(p_reg_info, func_do_extra);

err:
	r = conninfra_core_force_conninfra_sleep();
	if (r)
		pr_err("STEP failed: sleep conninfra fail\n");

	return ret;
}

void conninfra_step_setup(void)
{
	if (!g_infra_step_env.is_setup) {
		g_infra_step_env.is_setup = true;
		init_rwsem(&g_infra_step_env.init_rwsem);
	}
}

/*******************************************************************************
 *              I N T E R N A L   F U N C T I O N S   W I T H   U T
********************************************************************************/
int conninfra_step_do_emi_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_emi_action *p_emi_act = NULL;
	struct step_emi_info *p_emi_info = NULL;

	p_emi_act = clist_entry_action(emi, p_act);
	p_emi_info = &p_emi_act->info;
	return _conninfra_step_do_emi_action(p_emi_info, func_do_extra);
}

int conninfra_step_do_condition_emi_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_condition_emi_action *p_cond_emi_act = NULL;
	struct step_emi_info *p_emi_info = NULL;

	p_cond_emi_act = clist_entry_action(condition_emi, p_act);
	p_emi_info = &p_cond_emi_act->info;

	if (g_infra_step_env.temp_register[p_cond_emi_act->cond_reg_id] == 0) {
		pr_info("STEP show: Dont do emi, condition %c%d is %d\n",
			STEP_TEMP_REGISTER_SYMBOL, p_emi_info->temp_reg_id,
			g_infra_step_env.temp_register[p_cond_emi_act->cond_reg_id]);
		return -1;
	}

	return _conninfra_step_do_emi_action(p_emi_info, func_do_extra);
}

int conninfra_step_do_register_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_register_action *p_reg_act = NULL;
	struct step_register_info *p_reg_info = NULL;

	p_reg_act = clist_entry_action(register, p_act);
	p_reg_info = &p_reg_act->info;

	return _conninfra_step_do_register_action(p_reg_info, func_do_extra);
}

int conninfra_step_do_condition_register_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_condition_register_action *p_cond_reg_act = NULL;
	struct step_register_info *p_reg_info = NULL;

	p_cond_reg_act = clist_entry_action(condition_register, p_act);
	p_reg_info = &p_cond_reg_act->info;

	if (g_infra_step_env.temp_register[p_cond_reg_act->cond_reg_id] == 0) {
		pr_info("STEP show: Dont do register, condition %c%d is %d\n",
			STEP_TEMP_REGISTER_SYMBOL, p_reg_info->temp_reg_id,
			g_infra_step_env.temp_register[p_cond_reg_act->cond_reg_id]);
		return -1;
	}

	return _conninfra_step_do_register_action(p_reg_info, func_do_extra);
}

int conninfra_step_do_gpio_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_gpio_action *p_gpio_act = NULL;

	p_gpio_act = clist_entry_action(gpio, p_act);
	if (p_gpio_act->is_write == 1) {
		pr_err("STEP failed: Only support dump GPIO\n");
		return -1;
	}

#ifdef KERNEL_gpio_dump_regs_range
	KERNEL_gpio_dump_regs_range(p_gpio_act->pin_symbol, p_gpio_act->pin_symbol);
#else
	pr_info("STEP show: No support gpio dump\n");
#endif
	if (func_do_extra != NULL)
		func_do_extra(0);

	return 0;
}

int conninfra_step_do_periodic_dump_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_periodic_dump_action *p_pd_act = NULL;

	p_pd_act = clist_entry_action(periodic_dump, p_act);
	if (p_pd_act->pd_entry->is_enable == 0) {
		pr_info("STEP show: Start periodic dump(%d ms)\n",
			p_pd_act->pd_entry->expires_ms);
		conninfra_step_start_work(p_pd_act->pd_entry);
		p_pd_act->pd_entry->is_enable = 1;
	}

	if (func_do_extra != NULL)
		func_do_extra(0);

	return 0;
}

int conninfra_step_do_show_string_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_show_string_action *p_show_act = NULL;

	p_show_act = clist_entry_action(show_string, p_act);

	pr_info("STEP show: %s\n", p_show_act->content);

	if (func_do_extra != NULL)
		func_do_extra(1, p_show_act->content);

	return 0;
}

int conninfra_step_do_sleep_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_sleep_action *p_sleep_act = NULL;

	p_sleep_act = clist_entry_action(sleep, p_act);

	conninfra_step_sleep_or_delay(p_sleep_act->ms);

	if (func_do_extra != NULL)
		func_do_extra(0);

	return 0;
}

int conninfra_step_do_condition_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_condition_action *p_cond_act = NULL;
	int result, l_val, r_val;

	p_cond_act = clist_entry_action(condition, p_act);

	l_val = g_infra_step_env.temp_register[p_cond_act->l_temp_reg_id];

	if (p_cond_act->mode == STEP_CONDITION_RIGHT_REGISTER)
		r_val = g_infra_step_env.temp_register[p_cond_act->r_temp_reg_id];
	else
		r_val = p_cond_act->value;

	if (conninfra_step_operator_result_map[p_cond_act->operator_id]) {
		result = conninfra_step_operator_result_map[p_cond_act->operator_id] (l_val, r_val);
		g_infra_step_env.temp_register[p_cond_act->result_temp_reg_id] = result;

		pr_info("STEP show: Condition %d(%c%d) op %d(%c%d) => %d(%c%d)\n",
			l_val, STEP_TEMP_REGISTER_SYMBOL, p_cond_act->l_temp_reg_id,
			r_val, STEP_TEMP_REGISTER_SYMBOL, p_cond_act->r_temp_reg_id,
			result, STEP_TEMP_REGISTER_SYMBOL, p_cond_act->result_temp_reg_id);
	} else {
		pr_err("STEP failed: operator no define id: %d\n", p_cond_act->operator_id);
	}

	if (func_do_extra != NULL)
		func_do_extra(1, g_infra_step_env.temp_register[p_cond_act->result_temp_reg_id]);

	return 0;
}

int conninfra_step_do_value_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra)
{
	struct step_value_action *p_val_act = NULL;

	p_val_act = clist_entry_action(value, p_act);

	g_infra_step_env.temp_register[p_val_act->temp_reg_id] = p_val_act->value;

	if (func_do_extra != NULL)
		func_do_extra(1, g_infra_step_env.temp_register[p_val_act->temp_reg_id]);

	return 0;
}

struct step_action *conninfra_step_create_action(enum step_drv_type drv_type, enum step_action_id act_id,
		int param_num, char *params[])
{
	struct step_action *p_act = NULL;

	if (act_id <= STEP_ACTION_INDEX_NO_DEFINE || act_id >= STEP_ACTION_INDEX_MAX) {
		pr_err("STEP failed: Create action id: %d\n", act_id);
		return NULL;
	}

	if (conninfra_step_action_map[act_id].func_create_action != NULL)
		p_act = conninfra_step_action_map[act_id].func_create_action(drv_type, param_num, params);
	else
		pr_err("STEP failed: Create no define id: %d\n", act_id);

	if (p_act != NULL)
		p_act->action_id = act_id;

	return p_act;
}

int conninfra_step_init_pd_env(void)
{
	g_infra_step_env.pd_struct.step_pd_wq = create_workqueue(STEP_PERIODIC_DUMP_WORK_QUEUE);
	if (!g_infra_step_env.pd_struct.step_pd_wq) {
		pr_err("create_workqueue fail\n");
		return -1;
	}
	pr_info("[%s] step_pd_wq [%p]", __func__, g_infra_step_env.pd_struct.step_pd_wq);
	INIT_LIST_HEAD(&g_infra_step_env.pd_struct.pd_list);

	return 0;
}

int conninfra_step_deinit_pd_env(void)
{
	struct step_pd_entry *p_current;
	struct step_pd_entry *p_next;

	if (!g_infra_step_env.pd_struct.step_pd_wq)
		return -1;

	list_for_each_entry_safe(p_current, p_next, &g_infra_step_env.pd_struct.pd_list, list) {
		cancel_delayed_work(&p_current->pd_work);
		conninfra_step_clear_action_list(&p_current->action_list);
	}
	pr_info("[%s] step_pd_wq [%p]", __func__, g_infra_step_env.pd_struct.step_pd_wq);

	destroy_workqueue(g_infra_step_env.pd_struct.step_pd_wq);
	g_infra_step_env.pd_struct.step_pd_wq = NULL;

	return 0;
}

struct step_pd_entry *conninfra_step_get_periodic_dump_entry(unsigned int expires)
{
	struct step_pd_entry *p_current;

	if (expires <= 0)
		return NULL;

	if (!g_infra_step_env.pd_struct.step_pd_wq) {
		if (conninfra_step_init_pd_env() != 0)
			return NULL;
	}

	p_current = conninfra_step_create_periodic_dump_entry(expires);
	if (p_current == NULL)
		return NULL;
	list_add_tail(&(p_current->list), &(g_infra_step_env.pd_struct.pd_list));

	return p_current;
}

int conninfra_step_read_file(const char *file_name)
{
	int ret = -1;
	const osal_firmware *p_step_cfg = NULL;

	if (g_infra_step_env.is_enable == 1)
		return 0;

	if (0 == conninfra_step_get_cfg(file_name, (osal_firmware **) &p_step_cfg)) {
		if (0 == conninfra_step_parse_data((const char *)p_step_cfg->data, p_step_cfg->size,
			conninfra_step_write_action)) {
			ret = 0;
		} else {
			ret = -1;
		}

		conninfra_step_release_cfg((osal_firmware **) &p_step_cfg);

		return ret;
	}

	pr_info("STEP read file, %s is not exist\n", file_name);

	return ret;
}

void conninfra_step_remove_action(struct step_action *p_act)
{
	if (p_act != NULL) {
		if (p_act->action_id <= STEP_ACTION_INDEX_NO_DEFINE || p_act->action_id >= STEP_ACTION_INDEX_MAX) {
			pr_err("STEP failed: Wrong action id %d\n", (int)p_act->action_id);
			return;
		}

		if (conninfra_step_action_map[p_act->action_id].func_remove_action != NULL)
			conninfra_step_action_map[p_act->action_id].func_remove_action(p_act);
	} else {
		pr_err("STEP failed: Action is NULL\n");
	}
}

void conninfra_step_print_version(void)
{
	pr_info("STEP version: %d\n", STEP_VERSION);
}

/*******************************************************************************
 *                      E X T E R N A L   F U N C T I O N S
********************************************************************************/

void conninfra_step_init(void)
{
	if (g_infra_step_env.is_enable)
		return;

	conninfra_step_setup();
	conninfra_step_init_list();
	if (conninfra_step_read_file(STEP_CONFIG_NAME) == 0) {
		conninfra_step_print_version();
		down_write(&g_infra_step_env.init_rwsem);
		g_infra_step_env.is_enable = 1;
		up_write(&g_infra_step_env.init_rwsem);
	}
}

void conninfra_step_deinit(void)
{
	down_write(&g_infra_step_env.init_rwsem);
	g_infra_step_env.is_enable = 0;
	up_write(&g_infra_step_env.init_rwsem);
	conninfra_step_clear_list();
	conninfra_step_unioremap_emi();
	conninfra_step_deinit_pd_env();
}

void conninfra_step_do_actions(enum consys_conninfra_step_trigger_point tp_id)
{
#ifdef CFG_CONNINFRA_STEP
	conninfra_step_do_actions_from_tp(STEP_DRV_TYPE_CONNINFRA, tp_id, NULL);
#endif
}

void consys_step_bt_register(struct consys_step_register_cb *cb)
{
#ifdef CFG_CONNINFRA_STEP
	g_step_drv_type_def[STEP_DRV_TYPE_BT].drv_cb = cb;
#endif
}

void consys_step_bt_do_action(enum consys_bt_step_trigger_point tp_id)
{
#ifdef CFG_CONNINFRA_STEP
	conninfra_step_do_actions_from_tp(STEP_DRV_TYPE_BT, tp_id, NULL);
#endif
}

void consys_step_wf_register(struct consys_step_register_cb *cb)
{
#ifdef CFG_CONNINFRA_STEP
	g_step_drv_type_def[STEP_DRV_TYPE_WIFI].drv_cb = cb;
#endif
}

void consys_step_wf_do_action(enum consys_wf_step_trigger_point tp_id)
{
#ifdef CFG_CONNINFRA_STEP
	conninfra_step_do_actions_from_tp(STEP_DRV_TYPE_WIFI, tp_id, NULL);
#endif
}

void consys_step_gps_register(struct consys_step_register_cb *cb)
{
#ifdef CFG_CONNINFRA_STEP
	g_step_drv_type_def[STEP_DRV_TYPE_GPS].drv_cb = cb;
#endif
}

void consys_step_gps_do_action(enum consys_gps_step_trigger_point tp_id)
{
#ifdef CFG_CONNINFRA_STEP
	conninfra_step_do_actions_from_tp(STEP_DRV_TYPE_GPS, tp_id, NULL);
#endif
}

void consys_step_fm_register(struct consys_step_register_cb *cb)
{
#ifdef CFG_CONNINFRA_STEP
	g_step_drv_type_def[STEP_DRV_TYPE_FM].drv_cb = cb;
#endif
}

void consys_step_fm_do_action(enum consys_fm_step_trigger_point tp_id)
{
#ifdef CFG_CONNINFRA_STEP
	conninfra_step_do_actions_from_tp(STEP_DRV_TYPE_FM, tp_id, NULL);
#endif
}


