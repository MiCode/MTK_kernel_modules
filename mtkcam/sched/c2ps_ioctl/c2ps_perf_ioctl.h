/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#ifndef _C2PS_IOCTL_C2PS_PERF_IOCTL_H_
#define _C2PS_IOCTL_C2PS_PERF_IOCTL_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ioctl.h>

#define MAX_TASK_NAME_SIZE 10
#define MAX_CPU_NUM CONFIG_MAX_NR_CPUS

extern int debug_log_on;

typedef struct C2PS_INIT_PARAM {
	u32 camfps;
	u32 max_uclamp_cluster0;
	u32 max_uclamp_cluster1;
	u32 max_uclamp_cluster2;
} C2PS_INIT_PARAM;

typedef struct C2PS_UNINIT_PARAM {
	u32 dummy;
} C2PS_UNINIT_PARAM;

typedef struct C2PS_TASK_INIT_PARAMS {
	u32 task_id;
	u32 task_group_head;
	u32 default_uclamp;
	u32 task_target_time;
	u32 task_group_target_time;
	u32 perf_prefer_ratio;
	bool is_vip_task;
	bool is_dynamic_tid;
	char task_name[MAX_TASK_NAME_SIZE];
} C2PS_TASK_INIT_PARAMS;

typedef struct C2PS_PACKAGE {
	u32 tid;
	u32 task_id;
	u32 serial_no;
	u32 mode_change_hint;
} C2PS_PACKAGE;

typedef struct C2PS_INFO_NOTIFY {
	u32 serial_no;
	u32 cur_camfps;
} C2PS_INFO_NOTIFY;

typedef struct C2PS_SINGLE_SHOT_PARAM {
	int uclamp_max[MAX_CPU_NUM];
	int idle_rate_alert;
	int timeout;
	int uclamp_max_placeholder1[MAX_CPU_NUM];
	int uclamp_max_placeholder2[MAX_CPU_NUM];
	int uclamp_max_placeholder3[MAX_CPU_NUM];
	bool reset_param;
} C2PS_SINGLE_SHOT_PARAM;

#define C2PS_IOCTL_MAGIC 'g'
#define C2PS_ACTIVATE       _IOW(C2PS_IOCTL_MAGIC, 27, C2PS_INIT_PARAM)
#define C2PS_ADD_TASK       _IOW(C2PS_IOCTL_MAGIC, 28, C2PS_TASK_INIT_PARAMS)
#define C2PS_TASK_START     _IOW(C2PS_IOCTL_MAGIC, 29, C2PS_PACKAGE)
#define C2PS_TASK_END       _IOW(C2PS_IOCTL_MAGIC, 30, C2PS_PACKAGE)
#define C2PS_TASK_CHANGE    _IOW(C2PS_IOCTL_MAGIC, 31, C2PS_PACKAGE)
#define C2PS_DESTROY        _IOW(C2PS_IOCTL_MAGIC, 32, C2PS_UNINIT_PARAM)
#define C2PS_NOTIFY_VSYNC   _IOW(C2PS_IOCTL_MAGIC, 33, C2PS_INFO_NOTIFY)
#define C2PS_NOTIFY_CAMFPS  _IOW(C2PS_IOCTL_MAGIC, 34, C2PS_INFO_NOTIFY)
#define C2PS_TASK_SINGLE_SHOT    _IOW(C2PS_IOCTL_MAGIC, 35, C2PS_SINGLE_SHOT_PARAM)

#define C2PS_LOGD(fmt, ...)                                             \
    do {                                                                \
		if (debug_log_on)                                               \
			pr_debug("[C2PS_IOCTL]: %s " fmt, __func__, ##__VA_ARGS__); \
	} while (0)

#endif  // C2PS_IOCTL_C2PS_PERF_IOCTL_H_

