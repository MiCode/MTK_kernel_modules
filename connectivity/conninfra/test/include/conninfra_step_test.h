/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNINFRA_STEP_TEST_H_
#define _CONNINFRA_STEP_TEST_H_

#include "osal.h"
#include "conninfra_step.h"

#define STEP_TEST_CONSYS_EMI_WMT_OFFSET 0x68000

#define TEST_FAIL -1
#define TEST_PASS 0
#define TEST_CHECK 1
#define STEP_TEST_ACTION_NUMBER 30

extern struct step_env_struct g_infra_step_env;
extern struct platform_device *g_pdev;

struct step_test_report {
	unsigned int pass;
	unsigned int fail;
	unsigned int check;
};

struct step_test_scn_tp_act {
	int tp_id;
	int act_id;
	int param_num;
	const char* params[STEP_PARAMETER_SIZE];
};
struct step_test_scenario {
	const char* title;
	int test_idx;
	int total_test_sz;
	struct step_test_scn_tp_act pt_acts[STEP_TEST_ACTION_NUMBER];
};

struct step_test_check {
	unsigned int step_check_total;
	int step_check_test_tp_id[STEP_TEST_ACTION_NUMBER];
	int step_check_test_act_id[STEP_TEST_ACTION_NUMBER];
	char *step_check_params[STEP_TEST_ACTION_NUMBER][STEP_PARAMETER_SIZE];
	int step_check_params_num[STEP_TEST_ACTION_NUMBER];
	unsigned int step_check_index;
	int step_check_result;
	char *step_check_result_string;
	int step_check_result_value;
	unsigned int step_check_emi_offset[STEP_TEST_ACTION_NUMBER];
	size_t step_check_register_addr;
	size_t step_check_write_value;
	unsigned int step_test_mask;
	unsigned int step_recovery_value;
	int step_check_temp_register_id;

	struct step_test_scenario* cur_test_scn;
};



/* step test utility ++++ */

void conninfra_step_test_show_result_report(char *test_name, struct step_test_report *p_report, int sec_begin, int usec_begin,
	int sec_end, int usec_end);
void conninfra_step_test_update_result_report(struct step_test_report *p_dest_report,
	struct step_test_report *p_src_report);

void conninfra_step_test_create_action(enum step_action_id act_id, int param_num, char *params[], int result_of_action,
	int check_params[], struct step_test_report *p_report, char *err_result);

void conninfra_step_test_update_result(int result, struct step_test_report *p_report, char *err_result);
/* step test utility ---- */

void conninfra_step_test_all(void);
void conninfra_step_test_read_file(struct step_test_report *p_report);
//void conninfra_step_test_parse_data1(struct step_test_report *p_report);
//void conninfra_step_test_parse_data1(struct step_test_report *p_report);

/* EMI test utility */
int conninfra_step_test_get_emi_offset(unsigned char buf[], int offset);
unsigned char* conninfra_step_test_get_emi_virt_offset(unsigned int offset);
void conninfra_step_test_put_emi_virt_offset(void);

/* reg test utility */
//int conninfra_step_test_find_can_write_register(void);
//int conninfra_step_test_get_reg_base_phy_addr(unsigned char buf[], unsigned int index);

/* unchecked ++++  */
//void wmt_step_test_create_emi_action(struct step_test_report *p_report);
//void wmt_step_test_create_cond_emi_action(struct step_test_report *p_report);
//void wmt_step_test_create_register_action(struct step_test_report *p_report);
//void wmt_step_test_create_cond_register_action(struct step_test_report *p_report);
//void wmt_step_test_check_register_symbol(struct step_test_report *p_report);
//void wmt_step_test_create_other_action(struct step_test_report *p_report);
//void wmt_step_test_do_emi_action(struct step_test_report *p_report);
//void wmt_step_test_do_cond_emi_action(struct step_test_report *p_report);
//void wmt_step_test_do_register_action(struct step_test_report *p_report);
//void wmt_step_test_do_cond_register_action(struct step_test_report *p_report);
//void conninfra_step_test_do_gpio_action(struct step_test_report *p_report);
//void conninfra_step_test_do_chip_reset_action(struct step_test_report *p_report);
//void conninfra_step_test_create_periodic_dump(struct step_test_report *p_report);
//void conninfra_step_test_do_show_action(struct step_test_report *p_report);
//void conninfra_step_test_do_sleep_action(struct step_test_report *p_report);
//void conninfra_step_test_do_condition_action(struct step_test_report *p_report);
//void conninfra_step_test_do_value_action(struct step_test_report *p_report);

/* No need */
//void wmt_step_test_do_disable_reset_action(struct step_test_report *p_report);
//void wmt_step_test_do_wakeup_action(struct step_test_report *p_report);

/* unchecked ----  */


#endif /* end of _CONNINFRA_STEP_TEST_H_ */

