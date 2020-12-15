/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

struct met_api_tbl {
	int (*met_tag_start)(unsigned int class_id, const char *name);
	int (*met_tag_end)(unsigned int class_id, const char *name);
	int (*met_tag_async_start)(unsigned int class_id, const char *name, unsigned int cookie);
	int (*met_tag_async_end)(unsigned int class_id, const char *name, unsigned int cookie);
	int (*met_tag_oneshot)(unsigned int class_id, const char *name, unsigned int value);
	int (*met_tag_userdata)(char *pData);
	int (*met_tag_dump)(unsigned int class_id, const char *name, void *data, unsigned int length);
	int (*met_tag_disable)(unsigned int class_id);
	int (*met_tag_enable)(unsigned int class_id);
	int (*met_set_dump_buffer)(int size);
	int (*met_save_dump_buffer)(const char *pathname);
	int (*met_save_log)(const char *pathname);
	int (*met_show_bw_limiter)(void);
	int (*met_reg_bw_limiter)(void *fp);
	int (*met_show_clk_tree)(const char *name, unsigned int addr, unsigned int status);
	int (*met_reg_clk_tree)(void *fp);
	void (*met_sched_switch)(struct task_struct *prev, struct task_struct *next);
	int (*enable_met_backlight_tag)(void);
	int (*output_met_backlight_tag)(int level);
};

extern struct met_api_tbl met_ext_api;
