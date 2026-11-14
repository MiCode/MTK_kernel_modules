/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _CONNSYS_COREDUMP_HW_CONFIG_H_
#define _CONNSYS_COREDUMP_HW_CONFIG_H_

enum cr_category {
	CONN_HOST_CSR = 0,
	CONN_INFRA_CR = 1,
	SUBSYS_CR = 2,
};

struct coredump_hw_config {
	char* name;
	phys_addr_t start_offset;
	unsigned int size;
	unsigned int seg1_cr;
	unsigned int seg1_value_end;
	unsigned int seg1_start_addr;
	unsigned int seg1_phy_addr;
	unsigned int task_table_size;
	char** task_map_table;
	char* exception_tag_name;
};

#endif
