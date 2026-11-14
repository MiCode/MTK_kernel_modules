/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MET_TAG_EX_H__
#define __MET_TAG_EX_H__

#include <linux/version.h>

#ifdef BUILD_WITH_MET
void force_sample(void *unused);
#else
#include <linux/string.h>
#endif

/* Black List Table */
struct bltable_t {
	struct mutex mlock;
	/* flag - Bit31: Global ON/OFF; Bit0~30: ON/OF slot map of class_id */
	unsigned int flag;
	int class_id[MAX_EVENT_CLASS];
};

#if KERNEL_VERSION(6, 0, 0) <= LINUX_VERSION_CODE
void met_sched_switch(void *data, bool preempt, struct task_struct *prev, struct task_struct *next, unsigned int prev_state);
#else
void met_sched_switch(void *data, bool preempt, struct task_struct *prev, struct task_struct *next);
#endif
extern int tracing_mark_write(int type, unsigned int class_id,
		const char *name, unsigned int value,
		unsigned int value2, unsigned int value3);

#endif				/* __MET_TAG_EX_H__ */
