/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __CONNV3_COREDUMP_H__
#define __CONNV3_COREDUMP_H__

#include <linux/types.h>

/* Error code */
#define CONNV3_COREDUMP_ERR_TIME_OUT		-1
#define CONNV3_COREDUMP_ERR_CHIP_RESET_ONLY	-2
#define CONNV3_COREDUMP_ERR_WRONG_STATUS	-3
#define CONNV3_COREDUMP_ERR_INVALID_INPUT	-4
#define CONNV3_COREDUMP_ERR_GET_LOCK_FAIL	-5

#define CONNV3_EMI_MAP_DEV_NODE_SIZE	128
#define CONNV3_DUMP_INFO_SIZE		180
#define CONNV3_ASSERT_INFO_SIZE		164
#define CONNV3_ASSERT_TYPE_SIZE		64
#define CONNV3_TASK_NAME_SIZE		64
#define CONNV3_ASSERT_KEYWORD_SIZE	64
#define CONNV3_ASSERT_REASON_SIZE	128
#define CONNV3_AEE_INFO_SIZE		240
#define CONNV3_SUBSYS_TAG_SIZE		16

#define CONNV3_COREDUMP_FORCE_DUMP	"FORCE_DUMP"
#define CONNV3_COREDUMP_SAVE_TO_EMI 3

enum connv3_coredump_mode {
	CONNV3_DUMP_MODE_RESET_ONLY = 0,
	CONNV3_DUMP_MODE_AEE = 1,
	CONNV3_DUMP_MODE_DAEMON = 2,
	CONNV3_DUMP_MODE_MAX,
};

enum connv3_issue_type {
	CONNV3_ISSUE_FW_ASSERT = 0,
	CONNV3_ISSUE_FW_EXCEPTION,
	CONNV3_ISSUE_DRIVER_ASSERT,
	CONNV3_ISSUE_FORCE_DUMP,
	CONNV3_ISSUE_MAX,
};

struct connv3_coredump_event_cb {
	char dev_node[CONNV3_EMI_MAP_DEV_NODE_SIZE];
	unsigned int emi_size;
	unsigned int mcif_emi_size;
	unsigned int emi2_size;
	void (*get_save_emi)(phys_addr_t *base, size_t *size);
};

struct connv3_issue_info {
	enum connv3_issue_type issue_type;
	unsigned int drv_type;
	char assert_info[CONNV3_ASSERT_INFO_SIZE];
	char reason[CONNV3_ASSERT_REASON_SIZE];
	char task_name[CONNV3_TASK_NAME_SIZE];
	char subsys_tag[CONNV3_SUBSYS_TAG_SIZE];
	char exception_log[CONNV3_AEE_INFO_SIZE];
	unsigned int fw_isr;
	unsigned int fw_irq;
};

extern void* connv3_coredump_init(int conn_type, const struct connv3_coredump_event_cb *cb);
extern void connv3_coredump_deinit(void *handler);

/* config relative */
extern enum connv3_coredump_mode connv3_coredump_get_mode(void);
extern void connv3_coredump_set_dump_mode(enum connv3_coredump_mode mode);

extern int connv3_coredump_start(void *handler, const int drv, const char *reason, const char *dump_msg , const char *fw_version);
extern int connv3_coredump_send(void *handler, char *tag, char *content, unsigned int length);
extern int connv3_coredump_get_issue_info(void *handler, struct connv3_issue_info *issue_info, char *xml_str, unsigned int xml_str_size);
extern int connv3_coredump_emi(void *handler);
extern int connv3_coredump_end(void *handler, char *customized_string);
extern void connv3_coredump_set_memdump_mode(unsigned int mode);


#endif /* __CONNV3_COREDUMP_H__ */
