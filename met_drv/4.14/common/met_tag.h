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

#ifndef __MET_TAG_EX_H__
#define __MET_TAG_EX_H__

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

extern void met_sched_switch(struct task_struct *prev, struct task_struct *next);

extern int tracing_mark_write(int type, unsigned int class_id,
		const char *name, unsigned int value,
		unsigned int value2, unsigned int value3);

#endif				/* __MET_TAG_EX_H__ */
