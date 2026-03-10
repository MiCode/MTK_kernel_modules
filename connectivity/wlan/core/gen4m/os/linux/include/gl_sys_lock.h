/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define WFSYS_LOCK_MAX_TRACE		10
#define WFSYS_LOCK_MAX_HOLD_TIME	10
#define WFSYS_LOCK_PRINT_PERIOD		1

struct wfsys_lock_dbg_t {
	struct task_struct *task;
	int pid;
	uint64_t start_time;
	uint64_t start_time_sec;
	uint64_t start_time_nsec;
	unsigned long addrs[WFSYS_LOCK_MAX_TRACE];
	unsigned int  nr_entries;
};

void wfsys_lock(void);
int wfsys_trylock(void);
void wfsys_unlock(void);
int wfsys_is_locked(void);

