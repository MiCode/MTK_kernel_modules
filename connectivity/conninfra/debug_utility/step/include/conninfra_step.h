/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNINFRA_STEP_H_
#define _CONNINFRA_STEP_H_

#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/rwsem.h>
#include "connsys_step.h"
#include "conninfra_step_base.h"
#include "conninfra.h"
#include "consys_hw.h"

#define STEP_CONFIG_NAME "CONNINFRA_STEP.cfg"
#define STEP_VERSION 2

#define STEP_PERIODIC_DUMP_WORK_QUEUE "conninfra_step_pd_wq"

#define STEP_ACTION_NAME_EMI "_EMI"
#define STEP_ACTION_NAME_REGISTER "_REG"
#define STEP_ACTION_NAME_GPIO "GPIO"
#define STEP_ACTION_NAME_DISABLE_RESET "DRST"
#define STEP_ACTION_NAME_CHIP_RESET "_RST"
#define STEP_ACTION_NAME_KEEP_WAKEUP "WAK+"
#define STEP_ACTION_NAME_CANCEL_WAKEUP "WAK-"
#define STEP_ACTION_NAME_SHOW_STRING "SHOW"
#define STEP_ACTION_NAME_SLEEP "_SLP"
#define STEP_ACTION_NAME_CONDITION "COND"
#define STEP_ACTION_NAME_VALUE "_VAL"
#define STEP_ACTION_NAME_CONDITION_EMI "CEMI"
#define STEP_ACTION_NAME_CONDITION_REGISTER "CREG"

enum consys_conninfra_step_trigger_point {
	STEP_CONNINFRA_TP_NO_DEFINE = STEP_TP_NO_DEFINE,
	/* CONNINFRA TRIGGER POINT */
	STEP_CONNINFRA_TP_BEFORE_CHIP_RESET,
	STEP_CONNINFRA_TP_AFTER_CHIP_RESET,
	/* thermal */
	STEP_CONNINFRA_TP_BEFORE_READ_THERMAL,
	/* power on sequence */
	STEP_CONNINFRA_TP_POWER_ON_START,
	STEP_CONNINFRA_TP_POWER_ON_BEFORE_GET_CONNSYS_ID, /* 5 */
	STEP_CONNINFRA_TP_POWER_ON_END,
	/* power off */
	STEP_CONNINFRA_TP_BEFORE_POWER_OFF,
	/* Suspend/Resume */
	STEP_CONNINFRA_TP_WHEN_AP_SUSPEND,
	STEP_CONNINFRA_TP_WHEN_AP_RESUME, /* 9 */

	STEP_CONNINFRA_TP_MAX,
};


/********* action ***********/

struct step_action_list {
	struct list_head list;
};

struct step_action {
	struct list_head list;
	enum step_action_id action_id;
};


/**********  pd *****************/
struct step_pd_entry {
	bool is_enable;
	unsigned int expires_ms;
	struct step_action_list action_list;
	struct delayed_work pd_work;
	struct list_head list;
};

struct step_pd_struct {
	bool is_init;
	struct workqueue_struct *step_pd_wq;
	struct list_head pd_list;
};

typedef int (*STEP_WRITE_ACT_TO_LIST) (struct step_action_list *, enum step_drv_type, enum step_action_id, int, char **);
typedef void (*STEP_DO_EXTRA) (unsigned int, ...);

#define STEP_OUTPUT_LOG 0
#define STEP_OUTPUT_REGISTER 1


/****************** ACTION *****************/
struct step_emi_info {
	bool is_write;
	unsigned int begin_offset;
	unsigned int end_offset;
	int value;
	unsigned int temp_reg_id;
	int output_mode;
	int mask;
};

struct step_emi_action {
	struct step_emi_info info;
	struct step_action base;
};

struct step_register_info {
	enum step_drv_type drv_type;
	bool is_write;
	unsigned int address_type;
	unsigned long address;
	unsigned int offset;
	unsigned int times;
	unsigned int delay_time;
	int value;
	int mask;
	unsigned int temp_reg_id;
	int output_mode;
};

struct step_register_action {
	struct step_register_info info;
	struct step_action base;
};

struct step_gpio_action {
	bool is_write;
	unsigned int pin_symbol;
	struct step_action base;
};

struct step_periodic_dump_action {
	struct step_pd_entry *pd_entry;
	struct step_action base;
};

struct step_show_string_action {
	char *content;
	struct step_action base;
};

struct step_sleep_action {
	unsigned int ms;
	struct step_action base;
};

#define STEP_CONDITION_RIGHT_REGISTER 0
#define STEP_CONDITION_RIGHT_VALUE 1
struct step_condition_action {
	unsigned int result_temp_reg_id;
	unsigned int l_temp_reg_id;
	unsigned int r_temp_reg_id;
	int value;
	int mode;
	enum step_condition_operator_id operator_id;
	struct step_action base;
};

struct step_value_action {
	unsigned int temp_reg_id;
	int value;
	struct step_action base;
};

struct step_condition_emi_action {
	unsigned int cond_reg_id;
	struct step_emi_info info;
	struct step_action base;
};

struct step_condition_register_action {
	unsigned int cond_reg_id;
	struct step_register_info info;
	struct step_action base;
};

#define clist_entry_action(act_struct, ptr) \
	container_of(ptr, struct step_##act_struct##_action, base)

struct step_reg_addr_info {
	int address_type;
	unsigned long address;
};

/************************* PARSE *************************/
struct step_target_act_list_info {
	enum step_drv_type drv_type;
	//enum consys_step_trigger_point_id tp_id;
	unsigned int tp_id;
	struct step_action_list *p_target_list;
	struct step_pd_entry *p_pd_entry;
};

#define STEP_PARAMETER_SIZE 10
struct step_parse_line_data_param_info {
	int state;
	enum step_action_id act_id;
	char *act_params[STEP_PARAMETER_SIZE];
	int param_index;
};

typedef struct step_action *(*STEP_CREATE_ACTION) (enum step_drv_type, int, char *[]);
typedef int (*STEP_DO_ACTIONS) (struct step_action *, STEP_DO_EXTRA);
typedef void (*STEP_REMOVE_ACTION) (struct step_action *);
struct step_action_contrl {
	STEP_CREATE_ACTION func_create_action;
	STEP_DO_ACTIONS func_do_action;
	STEP_REMOVE_ACTION func_remove_action;
};

struct step_drv_type_def {
	const char* drv_type_str;
	int tp_max;
	struct consys_step_register_cb *drv_cb;
	struct step_action_list *action_list;
};
extern struct step_drv_type_def g_step_drv_type_def[STEP_DRV_TYPE_MAX];


#define STEP_REGISTER_BASE_SYMBOL '#'
#define STEP_TEMP_REGISTER_SYMBOL '$'

#define STEP_VALUE_INFO_UNKNOWN -1
#define STEP_VALUE_INFO_NUMBER 0
#define STEP_VALUE_INFO_SYMBOL_REG_BASE 1
#define STEP_VALUE_INFO_SYMBOL_TEMP_REG 2

#define STEP_TEMP_REGISTER_SIZE 10
struct step_env_struct {
	bool is_enable;
	bool is_keep_wakeup;
	//struct step_action_list actions[STEP_DRV_TYPE_MAX][STEP_TRIGGER_POINT_MAX];
	struct step_action_list conninfra_actions[STEP_CONNINFRA_TP_MAX];
	struct step_action_list wf_actions[STEP_WF_TP_MAX];
	struct step_action_list bt_actions[STEP_BT_TP_MAX];
	struct step_action_list gps_actions[STEP_GPS_TP_MAX];
	struct step_action_list fm_actions[STEP_FM_TP_MAX];

	unsigned char __iomem *emi_base_addr;
	unsigned int emi_size;

	struct step_pd_struct pd_struct;
	int temp_register[STEP_TEMP_REGISTER_SIZE];
	bool is_setup;
	struct rw_semaphore init_rwsem;
};

/********************************************************************************
 *              F U N C T I O N   D E C L A R A T I O N S
*********************************************************************************/
void conninfra_step_init(void);
void conninfra_step_deinit(void);

void conninfra_step_do_actions(enum consys_conninfra_step_trigger_point tp_id);
void conninfra_step_func_crtl_do_actions(enum consys_drv_type type, enum step_func_event_id opId);

#ifdef CFG_CONNINFRA_STEP
#define CONNINFRA_STEP_INIT_FUNC() conninfra_step_init()
#define CONNINFRA_STEP_DEINIT_FUNC() conninfra_step_deinit()
#define CONNINFRA_STEP_DO_ACTIONS_FUNC(tp) conninfra_step_do_actions(tp)
#define CONNINFRA_STEP_FUNC_CTRL_DO_ACTIONS_FUNC(type, id) conninfra_step_func_crtl_do_actions(type, id)
//#define conninfra_STEP_COMMAND_TIMEOUT_DO_ACTIONS_FUNC(reason) conninfra_step_command_timeout_do_actions(reason)

#else
#define CONNINFRA_STEP_INIT_FUNC()
#define CONNINFRA_STEP_DEINIT_FUNC()
#define CONNINFRA_STEP_DO_ACTIONS_FUNC(tp)
#define CONNINFRA_STEP_FUNC_CTRL_DO_ACTIONS_FUNC(type, id)
#define CONNINFRA_STEP_COMMAND_TIMEOUT_DO_ACTIONS_FUNC(reason)
#endif

/********************************************************************************
 *              D E C L A R E   F O R   T E S T
*********************************************************************************/
int conninfra_step_read_file(const char *file_name);
int conninfra_step_parse_data(const char *in_buf, unsigned int size, STEP_WRITE_ACT_TO_LIST func_act_to_list);

int conninfra_step_init_pd_env(void);
int conninfra_step_deinit_pd_env(void);


struct step_action_list *conninfra_step_get_tp_list(enum step_drv_type, int tp_id);

struct step_pd_entry *conninfra_step_get_periodic_dump_entry(unsigned int expires);
struct step_action *conninfra_step_create_action(enum step_drv_type drv_type, enum step_action_id act_id, int param_num, char *params[]);
int conninfra_step_do_emi_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_register_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_gpio_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);

int conninfra_step_do_periodic_dump_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_show_string_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_sleep_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_condition_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_value_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_condition_emi_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
int conninfra_step_do_condition_register_action(struct step_action *p_act, STEP_DO_EXTRA func_do_extra);
void conninfra_step_remove_action(struct step_action *p_act);
void conninfra_step_print_version(void);

void conninfra_step_do_actions_from_tp(enum step_drv_type drv_type,
		unsigned int tp_id, char *reason);

#endif /* end of _CONNINFRA_STEP_H_ */

