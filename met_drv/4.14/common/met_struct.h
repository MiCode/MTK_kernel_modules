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

DECLARE_PER_CPU(struct met_cpu_struct, met_cpu);

#endif				/* _MET_STRUCT_H_ */
