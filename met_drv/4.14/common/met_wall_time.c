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

#include <linux/sched/clock.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/div64.h>
#include <linux/types.h>
#include <linux/kallsyms.h>
#include <linux/ktime.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"
#include "interface.h"

#define MODE_CUSOM_CLKSRC       2
struct metdevice met_wall_time;

/**                                                                        */
/* How to add a new clocksource:                                           */
/*                                                                         */
/* 1. add constant for new clocksource in #define-macro                    */
/* 2. declare new weakref function                                         */
/* 3. implement handler functions:                                         */
/*     (1) clksrc_attr_t::*ready:                                          */
/*         check if ...                                                    */
/*         (i) clocksource correctly working                               */
/*         (ii) weakref function is not null                               */
/*     (2) clksrc_attr_t::*get_cnt: read clocksource from weakref function */
/* 4. place attrs of new clocksource into clksrc_attr_tb                   */
/* 5. update DEFAULT_CLKSRC_STR                                            */
/* 6. update help message                                                  */
/**                                                                        */

#define __SYS_TIMER             0x0
#define __GPT1                  0x1
#define __GPT2                  0x2
#define __GPT3                  0x3
#define __GPT4                  0x4
#define __GPT5                  0x5
#define __GPT6                  0x6

#define DEFAULT_CLKSRC_STR      "SYS_TIMER"

extern u64 met_arch_counter_get_cntvct(void);
extern ktime_t ktime_get(void);
u64 (*met_arch_counter_get_cntvct_symbol)(void);

int __sys_timer_get_cnt(u8 clksrc, u64 *cycles)
{
	if (met_arch_counter_get_cntvct_symbol)
		*cycles = met_arch_counter_get_cntvct_symbol();
	return 0;
}


struct clksrc_attr_t {
	u8 clksrc;
	const char *clksrc_str;
	/* checks if clksrc/get_cnt function is working/available */
	int (*clksrc_ready)(u8 clksrc);
	int (*clksrc_get_cnt)(u8 clksrc, u64 *cycles);
};

struct clksrc_attr_t clksrc_attr_tb[] = {
	{__SYS_TIMER, "SYS_TIMER", NULL, __sys_timer_get_cnt},
	/* {__GPT1, "GPT1", __gpt_timer_ready, __gpt_timer_get_cnt}, */
	/* {__GPT2, "GPT2", __gpt_timer_ready, __gpt_timer_get_cnt}, */
	/* {__GPT3, "GPT3", __gpt_timer_ready, __gpt_timer_get_cnt}, */
	/* {__GPT4, "GPT4", __gpt_timer_ready, __gpt_timer_get_cnt}, */
	/* {__GPT5, "GPT5", __gpt_timer_ready, __gpt_timer_get_cnt}, */
	/* {__GPT6, "GPT6", __gpt_timer_ready, __gpt_timer_get_cnt}, */
};

static const struct clksrc_attr_t *lookup_clksrc_attr_tb(const char *clksrc_str, int len);
static const struct clksrc_attr_t *wall_time_attr;

/* definitions for auto-sampling of working freq. of clocksource */
/* maximum tolerable error percentage(%) of sampled clock freq */
#define FREQ_ERR_PERCENT        3

/* expected working freq. of clocksources */
static const u32 freq_level[] = { 32768, 13000000, 26000000 };

static u32 lookup_freq_level(u32 freq);

/* flag indicating whether sampling is on-going */
static u32 do_sample;
static u32 freq;
static u64 start_us_ts;
static u64 start_wall_time;
/* end definitions for sampling of freq. */

static void wall_time_start(void)
{
	met_arch_counter_get_cntvct_symbol = (void *)symbol_get(met_arch_counter_get_cntvct);

	if (met_wall_time.mode != MODE_CUSOM_CLKSRC) {
		wall_time_attr = lookup_clksrc_attr_tb(DEFAULT_CLKSRC_STR,
						       strlen(DEFAULT_CLKSRC_STR));
	}

	freq = 0;
	do_sample = 1;

	if (wall_time_attr) {

		/* XXX: always use CPU 0 */
		start_us_ts = cpu_clock(0);
		wall_time_attr->clksrc_get_cnt(wall_time_attr->clksrc, &start_wall_time);

		/* us_ts = ap_ts/1000; */
		do_div(start_us_ts, 1000);
	}
}

noinline void met_mono_time(void)
{
	/* mono time vs local time */
	u64 cur_local_ns = sched_clock();
	ktime_t cur_mono_ts = ktime_get();

	MET_TRACE("TS.APTS=%llu TS.MONO=%llu\n", (unsigned long long)cur_local_ns,
		(unsigned long long)ktime_to_ns(cur_mono_ts));
}

noinline void met_ap_wall_time(unsigned long long ts, int cpu)
{
	u64 ap_ts;
	u64 us_ts;
	u64 sec;
	u64 usec;
	u64 cycles;
	u64 f;

	if (wall_time_attr) {

		wall_time_attr->clksrc_get_cnt(wall_time_attr->clksrc, &cycles);
		ap_ts = cpu_clock(cpu);

		us_ts = ap_ts;
		do_div(us_ts, 1000);	/* us_ts = ap_ts/1000; */

		sec = us_ts;
		usec = do_div(sec, 1000000);	/* sec = us_ts/1000000; usec = us_ts%1000000; */
		MET_TRACE("TS.APTS=%llu.%06llu WCLK=%llu\n", (unsigned long long)sec,
			   (unsigned long long)usec, (unsigned long long)cycles);

		// print local time vs mono time for gpu time shift
		met_mono_time();

		if (do_sample) {

			do_sample = 0;

			f = (cycles - start_wall_time) * 1000000;
			do_div(f, us_ts - start_us_ts);

			/* don't worry about the u64 -> u32 assignment,           */
			/* sampled wall-clock freq is expected to be below 2^32-1 */
			freq = lookup_freq_level(f);

			/* debug message */
			/* MET_TRACE("wall_time debug: result: %u," */
			/*         "start cycle: %llu, end cycle: %llu, cycle diff: %llu," */
			/*         "start us: %llu, end us: %llu, us diff: %llu", */
			/*         f, */
			/*         start_wall_time, cycles, cycles - start_wall_time, */
			/*         start_us_ts, us_ts, us_ts - start_us_ts); */

			if (freq != 0)
				met_tag_oneshot_real(33880, "_WCLK_FREQ_", freq);
		}
	}
}

static const char help[] =
"  --wall_time                             output wall-clock syncing info in system timer\n";
/* "  --wall_time=SYS_TIMER|GPT[1-6]  output wall-clock syncing info in custom clocksource\n"; */

static int wall_time_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static const char *header =
"met-info [000] 0.0: WCLK: %d\n"
"met-info [000] 0.0: clocksource: %s\n";

static int wall_time_print_header(char *buf, int len)
{
	return snprintf(buf, len, header,
			freq == 0 ? -1 : freq,
			wall_time_attr ? wall_time_attr->clksrc_str : "NONE");
}

static int wall_time_process_argument(const char *arg, int len)
{
	/* reset wall-time clocksource */
	wall_time_attr = lookup_clksrc_attr_tb(arg, len);

	if (!wall_time_attr) {
		met_wall_time.mode = 0;
		return -1;
	}

	met_wall_time.mode = MODE_CUSOM_CLKSRC;
	return 0;
}

static const struct clksrc_attr_t *lookup_clksrc_attr_tb(const char *clksrc_str, int len)
{
	int i;
	const struct clksrc_attr_t *attr;
	int tb_nmemb = sizeof(clksrc_attr_tb) / sizeof(*clksrc_attr_tb);

	for (i = 0; i < tb_nmemb; i++) {

		attr = clksrc_attr_tb + i;

		if (strlen(attr->clksrc_str) == len &&
		    strncmp(clksrc_str, attr->clksrc_str, len) == 0) {
			return attr;
		}
	}

	return NULL;
}

static u32 lookup_freq_level(u32 freq)
{

	int ii;
	int freq_nmemb = sizeof(freq_level) / sizeof(*freq_level);
	u32 fdiff;

	for (ii = 0; ii < freq_nmemb; ii++) {
		fdiff = freq_level[ii] > freq ? freq_level[ii] - freq : freq - freq_level[ii];
		if (fdiff < freq_level[ii] * FREQ_ERR_PERCENT / 100)
			return freq_level[ii];
	}

	return 0;
}

struct metdevice met_wall_time = {
	.name = "wall_time",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.start = wall_time_start,
	.polling_interval = 1000,
	.timed_polling = met_ap_wall_time,
	.print_help = wall_time_print_help,
	.print_header = wall_time_print_header,
	.process_argument = wall_time_process_argument,
};
EXPORT_SYMBOL(met_wall_time);
