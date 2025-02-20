/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

struct wlan_dbg_command {
	u_int8_t write;
	uint32_t w_addr;
	uint32_t mask;
	uint32_t value;
	u_int8_t read;
	uint32_t r_addr;
};

struct wlan_dump_list {
	uint8_t *tag;
	uint8_t *description;
	uint32_t read_cmd_size;
	uint32_t dump_size;
	const struct wlan_dbg_command *cmd_list;
};
