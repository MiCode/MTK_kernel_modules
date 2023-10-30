// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
/* #include <linux/fs.h> */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
/* #include <linux/proc_fs.h> */
#include <linux/sched.h>
/* #include <linux/seq_file.h> */
/* #include <linux/slab.h> */
#include <linux/time.h>
#include <linux/irqnr.h>
#include <linux/vmalloc.h>
/* #include <asm/cputime.h> */
/* #include <asm-generic/cputime.h> */
#include <linux/tick.h>
#include <linux/jiffies.h>

#include <asm/page.h>
#include <linux/slab.h>

#include "stat.h"
#include "interface.h"
#include "met_drv.h"
#include "trace.h"
#include "mtk_typedefs.h"

#define MS_STAT_FMT	"%5lu.%06lu"
#define FMTLX7		",%llx,%llx,%llx,%llx,%llx,%llx,%llx\n"
#define FMTLX10		",%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx\n"


static int __percpu *cpu_status;


/* void ms_st(unsigned long long timestamp, unsigned char cnt, unsigned int *value) */
noinline void ms_st(unsigned long long timestamp, unsigned char cnt, u64 *value)
{
	unsigned long nano_rem = do_div(timestamp, 1000000000);

	switch (cnt) {
	case 10:
		MET_TRACE(MS_STAT_FMT FMTLX10, (unsigned long)(timestamp), (nano_rem/1000),
			value[0], value[1], value[2], value[3], value[4],
			value[5], value[6], value[7], value[8], value[9]);
		break;
	case 7:
		MET_TRACE(MS_STAT_FMT FMTLX7, (unsigned long)(timestamp), (nano_rem/1000),
			value[0], value[1], value[2], value[3], value[4],
			value[5], value[6]);
		break;
	}
}

static int met_stat_create_subfs(struct kobject *parent)
{
	int ret = 0;

	cpu_status = alloc_percpu(typeof(*cpu_status));
	if (!cpu_status) {
		PR_BOOTMSG("percpu cpu_status allocate fail\n");
		pr_debug("percpu cpu_status allocate fail\n");
		return 0;
	}

	return ret;
}

static void met_stat_delete_subfs(void)
{
	if (cpu_status) {
		free_percpu(cpu_status);
	}
}

static void met_stat_start(void)
{
	int	cpu = raw_smp_processor_id();

	if (!cpu_status) {
		MET_TRACE("percpu cpu_status allocate fail\n");
		met_stat.mode = 0;
		return;
	}

	if (get_ctrl_flags() & 1)
		met_stat.mode = 0;

	*per_cpu_ptr(cpu_status, cpu) = MET_CPU_ONLINE;
}

static void met_stat_stop(void)
{
}

static int do_stat(void)
{
	return met_stat.mode;
}

u64 met_usecs_to_cputime64(u64 n)
{
#if (NSEC_PER_SEC % HZ) == 0
	/* Common case, HZ = 100, 128, 200, 250, 256, 500, 512, 1000 etc. */
	return div_u64(n, NSEC_PER_SEC / HZ);
#elif (HZ % 512) == 0
	/* overflow after 292 years if HZ = 1024 */
	return div_u64(n * HZ / 512, NSEC_PER_SEC / 512);
#else
	/*
	 * Generic case - optimized for cases where HZ is a multiple of 3.
	 * overflow after 64.99 years, exact for HZ = 60, 72, 90, 120 etc.
	 */
	return div_u64(n * 9, (9ull * NSEC_PER_SEC + HZ / 2) / HZ);
#endif
}

static u64 met_get_idle_time(int cpu)
{
	u64 idle, idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL) {
		/* !NO_HZ so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	} else
		idle = met_usecs_to_cputime64(idle_time);

	return idle;
}

static u64 get_iowait_time(int cpu)
{
	u64 iowait, iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL) {
		/* !NO_HZ so we can rely on cpustat.iowait */
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	} else
		iowait = met_usecs_to_cputime64(iowait_time);

	return iowait;
}


static unsigned int stat_os_polling(u64 *value, int i)
{
	int j = -1;

	/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_USER]);	/* user */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_NICE]);	/* nice */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM]);	/* system */
	value[++j] = jiffies_64_to_clock_t(met_get_idle_time(i));	/* idle */
	value[++j] = jiffies_64_to_clock_t(get_iowait_time(i));	/* iowait */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_IRQ]);	/* irq */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ]);	/* softirq */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_STEAL]);	/* steal */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_GUEST]);	/* guest */
	value[++j] = jiffies_64_to_clock_t(kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE]);	/* guest_nice */

	return j + 1;
}

static void met_stat_polling(unsigned long long stamp, int cpu)
{
	unsigned char count;
	u64 value[10] = {0};

	if (*per_cpu_ptr(cpu_status, cpu) != MET_CPU_ONLINE)
		return;

	/* return; */
	if (do_stat()) {
		count = stat_os_polling(value, cpu);
		if (count)
			ms_st(stamp, count, value);
	}
}

static const char header[] =
	"met-info [000] 0.0: met_st_header: user,nice,system,idle,iowait,irq,softirq,steal,guest,guest_nice\n";

static const char help[] = "  --stat                                monitor stat\n";


static int met_stat_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int met_stat_print_header(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, header);
}

static void met_stat_cpu_state_notify(long cpu, unsigned long action)
{
	if (!cpu_status)
		return;

	*per_cpu_ptr(cpu_status, cpu) = action;
}


struct metdevice met_stat = {
	.name = "stat",
	.type = MET_TYPE_PMU,
	.cpu_related = 1,
	.create_subfs = met_stat_create_subfs,
	.delete_subfs = met_stat_delete_subfs,
	.start = met_stat_start,
	.stop = met_stat_stop,
	.polling_interval = 30,
	.timed_polling = met_stat_polling,
	.print_help = met_stat_print_help,
	.print_header = met_stat_print_header,
	.cpu_state_notify = met_stat_cpu_state_notify,
};
