/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MET_STRUCT_H_
#define _MET_STRUCT_H_

#include <linux/hrtimer.h>

struct met_cpu_struct {
	struct hrtimer hrtimer;
	struct delayed_work dwork;
/* struct kmem_cache *cachep; */
/* struct list_head sample_head; */
/* spinlock_t list_lock; */
/* struct mutex list_sync_lock; */
	int work_enabled;
	int cpu;
	int hrtimer_online_check;
/* char name[16]; */
};

extern struct met_cpu_struct __percpu *met_cpu;

#endif				/* _MET_STRUCT_H_ */
