/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __CONNSYSLOG_HW_CONFIG_H__
#define __CONNSYSLOG_HW_CONFIG_H__

enum CONN_EMI_BLOCK_TYPE {
	CONN_EMI_BLOCK_PRIMARY = 0,
	CONN_EMI_BLOCK_MCU = 1,
	CONN_EMI_BLOCK_TYPE_END,
};

struct connlog_emi_block {
	unsigned int type;
	unsigned int size;
};

struct connlog_emi_config {
	phys_addr_t log_offset;
	unsigned int log_size;
	struct connlog_emi_block block[CONN_EMI_BLOCK_TYPE_END];
};

#endif /* __CONNSYSLOG_HW_CONFIG_H__ */
