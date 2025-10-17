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
#define MAX_CRITICAL_TASKS 20

extern int debug_log_on;

struct C2PS_INIT_PARAM {
	u32 camfps;
	u32 max_uclamp_cluster0;
	u32 max_uclamp_cluster1;
	u32 max_uclamp_cluster2;
	u32 ineff_cpu_ceiling_freq0;
	u32 ineff_cpu_ceiling_freq1;
	u32 ineff_cpu_ceiling_freq2;
	u32 lcore_mcore_um_ratio;
	u32 um_floor;
};

struct C2PS_UNINIT_PARAM {
	u32 dummy;
};

struct C2PS_TASK_INIT_PARAMS {
	u32 task_id;
	u32 task_group_head;
	u32 default_uclamp;
	u32 task_target_time;
	u32 task_group_target_time;
	u32 perf_prefer_ratio;
	bool is_vip_task;
	bool is_dynamic_tid;
	bool is_enable_dep_thread;
	char task_name[MAX_TASK_NAME_SIZE];
};

struct C2PS_PACKAGE {
	u32 tid;
	u32 task_id;
	u32 serial_no;
	u32 mode_change_hint;
};

struct C2PS_INFO_NOTIFY {
	u32 serial_no;
	u32 cur_camfps;
};

struct C2PS_SINGLE_SHOT_PARAM {
	u32 tid;
	int uclamp_max[MAX_CPU_NUM];
	int idle_rate_alert;
	int vip_prior;
	u32 vip_throttle_time;
	int uclamp_max_placeholder1[MAX_CPU_NUM];
	int uclamp_max_placeholder2[MAX_CPU_NUM];
	int uclamp_max_placeholder3[MAX_CPU_NUM];
	bool reset_param;
	bool set_task_idle_prefer;
	int critical_task_ids[MAX_CRITICAL_TASKS];
	int critical_task_uclamp[MAX_CRITICAL_TASKS];
	u32 util_margin;
	u32 um_placeholder1;
	u32 um_placeholder2;
	u32 um_placeholder3;
	bool enable_ineff_cpufreq;
	bool switch_um_idle_rate_mode;
	int reserved_1;
	int reserved_2;
	int reserved_3;
};

struct C2PS_SINGLE_SHOT_TASK_PARAM {
	u32 tid;
	u32 uclamp;
};

struct C2PS_ANCHOR_POINT_PARAM {
	int anchor_id;
	bool register_fixed_start;
	u32 anchor_type;
	u32 anchor_order;
	u32 notify_order;
	u32 latency_spec;
	u32 jitter_spec;
} C2PS_ANCHOR_POINT_PARAM;

#define C2PS_IOCTL_MAGIC 'g'
#define C2PS_ACTIVATE       _IOW(C2PS_IOCTL_MAGIC, 27, struct C2PS_INIT_PARAM)
#define C2PS_ADD_TASK       _IOW(C2PS_IOCTL_MAGIC, 28, struct C2PS_TASK_INIT_PARAMS)
#define C2PS_TASK_START     _IOW(C2PS_IOCTL_MAGIC, 29, struct C2PS_PACKAGE)
#define C2PS_TASK_END       _IOW(C2PS_IOCTL_MAGIC, 30, struct C2PS_PACKAGE)
#define C2PS_TASK_CHANGE    _IOW(C2PS_IOCTL_MAGIC, 31, struct C2PS_PACKAGE)
#define C2PS_DESTROY        _IOW(C2PS_IOCTL_MAGIC, 32, struct C2PS_UNINIT_PARAM)
#define C2PS_NOTIFY_VSYNC   _IOW(C2PS_IOCTL_MAGIC, 33, struct C2PS_INFO_NOTIFY)
#define C2PS_NOTIFY_CAMFPS  _IOW(C2PS_IOCTL_MAGIC, 34, struct C2PS_INFO_NOTIFY)
#define C2PS_TASK_SINGLE_SHOT    _IOW(C2PS_IOCTL_MAGIC, 35, struct C2PS_SINGLE_SHOT_PARAM)
#define C2PS_SINGLE_SHOT_TASK_START \
	_IOW(C2PS_IOCTL_MAGIC, 36, struct C2PS_SINGLE_SHOT_TASK_PARAM)
#define C2PS_SINGLE_SHOT_TASK_END \
	_IOW(C2PS_IOCTL_MAGIC, 37, struct C2PS_SINGLE_SHOT_TASK_PARAM)
#define C2PS_ANCHOR_POINT   _IOW(C2PS_IOCTL_MAGIC, 38, struct C2PS_ANCHOR_POINT_PARAM)


#define C2PS_LOGD(fmt, ...)                                                 \
	do {                                                                    \
		if (unlikely(debug_log_on)) {                                       \
			switch (debug_log_on) {                                         \
			case 1:                                                         \
				pr_debug("[C2PS_IOCTL]: %s " fmt, __func__, ##__VA_ARGS__); \
				break;                                                      \
			case 2:                                                         \
				pr_warn("[C2PS_IOCTL]: %s " fmt, __func__, ##__VA_ARGS__);  \
				break;                                                      \
			default:                                                        \
				break;                                                      \
			}                                                               \
		}                                                                   \
	} while (0)

#endif  // C2PS_IOCTL_C2PS_PERF_IOCTL_H_

