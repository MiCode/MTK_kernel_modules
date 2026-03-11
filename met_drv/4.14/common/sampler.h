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

#ifndef _SAMPLER_H_
#define _SAMPLER_H_

/*
 * sampling rate: 1ms
 * log generating rate: 10ms
 */
#if 0
#define DEFAULT_TIMER_EXPIRE (HZ / 100)
#define DEFAULT_HRTIMER_EXPIRE (TICK_NSEC / 10)
#else
extern int met_timer_expire;	/* in jiffies */
extern int met_hrtimer_expire;	/* in us */
#define DEFAULT_TIMER_EXPIRE (met_timer_expire)
#define DEFAULT_HRTIMER_EXPIRE (met_hrtimer_expire)
#endif
/*
 * sampling rate: 10ms
 * log generating rate: 100ms
 */
/* #define DEFAULT_TIMER_EXPIRE (HZ / 10) */
/* #define DEFAULT_HRTIMER_EXPIRE (TICK_NSEC / 1) */

int met_hrtimer_start(void);
void met_hrtimer_stop(void);
int sampler_start(void);
void sampler_stop(void);

extern struct list_head met_list;
extern void add_cookie(struct pt_regs *regs, int cpu);
extern int met_hrtimer_suspend(void);
extern void met_hrtimer_resume(void);
extern void met_event_timer_notify(void);

#ifdef CONFIG_CPU_FREQ
#include "power.h"
#endif

#endif				/* _SAMPLER_H_ */
