/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

struct bt_dbg_command {
	bool write;
	unsigned int w_addr;
	unsigned int mask;
	unsigned int value;
	bool read;
	unsigned int r_addr;
};

struct bt_dump_list {
	char *tag;
	char *description;
	unsigned int read_cmd_size;
	unsigned int dump_size;
	const struct bt_dbg_command *cmd_list;
};

