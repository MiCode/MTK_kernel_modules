// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/sched/clock.h>
#include <linux/kernel.h>
#include <linux/cpuhotplug.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/irq.h>
#if 0				/* fix me later, no such file on current tree */
#include <mach/mt_cpuxgpt.h>
#endif
#include <asm/arch_timer.h>

#define	MET_USER_EVENT_SUPPORT
#include "interface.h"
#include "sampler.h"
#include "met_struct.h"
#include "util.h"
#include "switch.h"
#include "trace.h"
#include "met_drv.h"
#include "met_tag.h" /* for tracing_mark_write */

#include "cpu_pmu.h"	/* for using kernel perf PMU driver */

#include "met_kernel_symbol.h"

#undef	DEBUG_CPU_NOTIFY
/* #define DEBUG_CPU_NOTIFY */
#if	defined(DEBUG_CPU_NOTIFY)
#ifdef CONFIG_MET_MODULE
#define	dbg_met_tag_oneshot	met_tag_oneshot_real
#else
#define	dbg_met_tag_oneshot	met_tag_oneshot
#endif /* CONFIG_MET_MODULE */
#else
#define	dbg_met_tag_oneshot(class_id, name, value)	({ 0; })
#endif

static int start;
static unsigned int online_cpu_map;
static int curr_polling_cpu;
static int cpu_related_cnt;
static int cpu_related_polling_hdlr_cnt;

static int preferred_cpu_list[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static int calc_preferred_polling_cpu(unsigned int cpu_map)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(preferred_cpu_list); i++) {
		if (cpu_map & (1 << preferred_cpu_list[i]))
			return preferred_cpu_list[i];
	}

	return -1;
}

static void wq_sync_buffer(struct work_struct *work)
{
	int cpu;
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct met_cpu_struct *met_cpu_ptr = container_of(dw, struct met_cpu_struct, dwork);

	cpu = smp_processor_id();
	if (met_cpu_ptr->cpu != cpu) {
		/* panic("ERROR"); */
		return;
	}

	/* sync_samples(cpu); */
	/* don't re-add the work if we're shutting down */
	if (met_cpu_ptr->work_enabled)
		schedule_delayed_work(dw, DEFAULT_TIMER_EXPIRE);
}

static enum hrtimer_restart met_hrtimer_notify(struct hrtimer *hrtimer)
{
	int cpu;
	int *count;
	unsigned long long stamp;
	struct met_cpu_struct *met_cpu_ptr = container_of(hrtimer, struct met_cpu_struct, hrtimer);
	struct metdevice *c;
#if	defined(DEBUG_CPU_NOTIFY)
	char msg[32];
#endif

	cpu = smp_processor_id();
#if	defined(DEBUG_CPU_NOTIFY)
	{
		char msg[32];

		snprintf(msg, sizeof(msg), "met_hrtimer notify_%d", cpu);
		dbg_met_tag_oneshot(0, msg, 1);
	}
#endif

	if (met_cpu_ptr->cpu != cpu) {
		/* panic("ERROR2"); */
		dbg_met_tag_oneshot(0, msg, -3);
		return HRTIMER_NORESTART;
	}

	list_for_each_entry(c, &met_list, list) {
		if (c->ondiemet_mode == 0) {
			if ((c->mode == 0) || (c->timed_polling == NULL))
				continue;
		} else if (c->ondiemet_mode == 1) {
			if ((c->mode == 0) || (c->ondiemet_timed_polling == NULL))
				continue;
		} else if (c->ondiemet_mode == 2) {
			if ((c->mode == 0) || ((c->timed_polling == NULL)
					       && (c->ondiemet_timed_polling == NULL)))
				continue;
		}

		count = per_cpu_ptr(c->polling_count, cpu);
		if ((*count) > 0) {
			(*count)--;
			continue;
		}

		*(count) = c->polling_count_reload;

		stamp = cpu_clock(cpu);

		if (c->cpu_related == 0) {
			if (cpu == curr_polling_cpu) {
				if (c->ondiemet_mode == 0) {
					c->timed_polling(stamp, 0);
				} else if (c->ondiemet_mode == 1) {
					c->ondiemet_timed_polling(stamp, 0);
				} else if (c->ondiemet_mode == 2) {
					if (c->timed_polling)
						c->timed_polling(stamp, 0);
					if (c->ondiemet_timed_polling)
						c->ondiemet_timed_polling(stamp, 0);
				}
			}
		} else {
			if (c->ondiemet_mode == 0) {
				c->timed_polling(stamp, cpu);
			} else if (c->ondiemet_mode == 1) {
				c->ondiemet_timed_polling(stamp, cpu);
			} else if (c->ondiemet_mode == 2) {
				if (c->timed_polling)
					c->timed_polling(stamp, 0);
				if (c->ondiemet_timed_polling)
					c->ondiemet_timed_polling(stamp, 0);
			}
		}
	}

	if (met_cpu_ptr->hrtimer_online_check) {
		online_cpu_map |= (1 << cpu);
		met_cpu_ptr->hrtimer_online_check = 0;
		dbg_met_tag_oneshot(0, "met_online check done", cpu);
		if (calc_preferred_polling_cpu(online_cpu_map) == cpu) {
			curr_polling_cpu = cpu;
			dbg_met_tag_oneshot(0, "met_curr polling cpu", cpu);
		}
	}

	if (met_cpu_ptr->work_enabled) {
		hrtimer_forward_now(hrtimer, ns_to_ktime(DEFAULT_HRTIMER_EXPIRE));
		dbg_met_tag_oneshot(0, msg, 0);
		return HRTIMER_RESTART;
	}
	dbg_met_tag_oneshot(0, msg, 0);
	return HRTIMER_NORESTART;
}

static void __met_init_cpu_related_device(void *unused)
{
	struct metdevice *c;

	list_for_each_entry(c, &met_list, list) {
		*(this_cpu_ptr(c->polling_count)) = 0;
		if (c->ondiemet_mode == 0) {
			if ((c->cpu_related) && (c->mode) && (c->start))
				c->start();
		} else if (c->ondiemet_mode == 1) {
			if (((c->cpu_related)) && (c->mode) && (c->ondiemet_start))
				c->ondiemet_start();
		} else if (c->ondiemet_mode == 2) {
			if ((c->cpu_related) && (c->mode) && (c->start))
				c->start();
			if (((c->cpu_related)) && (c->mode) && (c->ondiemet_start))
				c->ondiemet_start();
		}
	}
}

static void __met_hrtimer_register(void *unused)
{
	struct met_cpu_struct *met_cpu_ptr = NULL;
	struct hrtimer *hrtimer = NULL;
	/* struct delayed_work *dw; */
	/*struct metdevice *c;*/

	met_cpu_ptr = this_cpu_ptr(&met_cpu);
#if	defined(DEBUG_CPU_NOTIFY)
	{
		char msg[32];

		snprintf(msg, sizeof(msg), "met_hrtimer status_%d", met_cpu_ptr->cpu);
		dbg_met_tag_oneshot(0, msg, 1);
	}
#endif
	/*
	 * do not open HRtimer when EVENT timer enable
	 */
	if (!(met_switch.mode & MT_SWITCH_EVENT_TIMER)) {

		hrtimer = &met_cpu_ptr->hrtimer;
		hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		hrtimer->function = met_hrtimer_notify;

		if (DEFAULT_HRTIMER_EXPIRE) {
			met_cpu_ptr->work_enabled = 1;
			/* schedule_delayed_work_on(smp_processor_id(), dw, DEFAULT_TIMER_EXPIRE); */
			hrtimer_start(hrtimer, ns_to_ktime(DEFAULT_HRTIMER_EXPIRE),
				      HRTIMER_MODE_REL_PINNED);
		}
	}
}

static void __met_hrtimer_stop(void *unused)
{
	struct met_cpu_struct *met_cpu_ptr;
	struct hrtimer *hrtimer;
	/* struct delayed_work *dw; */
	struct metdevice *c;

	met_cpu_ptr = this_cpu_ptr(&met_cpu);
#if	defined(DEBUG_CPU_NOTIFY)
	{
		char msg[32];

		snprintf(msg, sizeof(msg), "met_hrtimer status_%d", met_cpu_ptr->cpu);
		dbg_met_tag_oneshot(0, msg, 0);
	}
#endif
	/*
	 * do not open HRtimer when EVENT timer enable
	 */
	if (!(met_switch.mode & MT_SWITCH_EVENT_TIMER)) {
		hrtimer = &met_cpu_ptr->hrtimer;
		/* dw = &met_cpu_ptr->dwork; */

		met_cpu_ptr->work_enabled = 0;
		hrtimer_cancel(hrtimer);

		/* cancel_delayed_work_sync(dw); */
	}
	list_for_each_entry(c, &met_list, list) {
		if (c->ondiemet_mode == 0) {
			if ((c->cpu_related) && (c->mode) && (c->stop))
				c->stop();
		} else if (c->ondiemet_mode == 1) {
			if ((c->cpu_related) && (c->mode) && (c->ondiemet_stop))
				c->ondiemet_stop();
		} else if (c->ondiemet_mode == 2) {
			if ((c->cpu_related) && (c->mode) && (c->stop))
				c->stop();
			if ((c->cpu_related) && (c->mode) && (c->ondiemet_stop))
				c->ondiemet_stop();
		}
		*(this_cpu_ptr(c->polling_count)) = 0;
	}
}

static int met_pmu_cpu_notify(enum met_action action, unsigned int cpu)
{
	struct met_cpu_struct *met_cpu_ptr;
	struct delayed_work *dw;
	int preferred_polling_cpu;
	struct metdevice *c;

	if (start == 0)
		return NOTIFY_OK;

#if	defined(DEBUG_CPU_NOTIFY)
	{
		char msg[32];

		snprintf(msg, sizeof(msg), "met_cpu notify_%d", cpu);
		dbg_met_tag_oneshot(0, msg, action);
	}
#elif	defined(PR_CPU_NOTIFY)
	{
		char msg[32];

		if (met_cpu_notify) {
			snprintf(msg, sizeof(msg), "met_cpu notify_%d", cpu);
			dbg_met_tag_oneshot(0, msg, action);
		}
	}
#endif

	if (cpu < 0 || cpu >= NR_CPUS)
		return NOTIFY_OK;

	switch (action) {
	case MET_CPU_ONLINE:
		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		met_cpu_ptr->hrtimer_online_check = 1;
		dbg_met_tag_oneshot(0, "met_online check", cpu);

		if (cpu_related_cnt == 0) {
			/*pr_info("%s, %d: curr_polling_cpu is alive = %d\n",
			 *		__func__, __LINE__, online_cpu_map & (1 << curr_polling_cpu));
			 */

			online_cpu_map |= (1 << cpu);

			/* check curr_polling_cpu is alive, if it is down,
			 * start current cpu hrtimer, and change it to be currr_pollling_cpu
			 */
			if ((online_cpu_map & (1 << curr_polling_cpu)) == 0) {
				if (met_export_api_symbol->met_smp_call_function_single)
					met_export_api_symbol->met_smp_call_function_single(cpu,
											__met_hrtimer_register, NULL, 1);
				curr_polling_cpu = cpu;
			}
		} else {
			if (cpu_related_polling_hdlr_cnt) {
				if (met_export_api_symbol->met_smp_call_function_single) {
					met_export_api_symbol->met_smp_call_function_single(cpu,
											__met_init_cpu_related_device, NULL, 1);
					met_export_api_symbol->met_smp_call_function_single(cpu,
											__met_hrtimer_register, NULL, 1);
				}
			} else {
				/*pr_info("%s, %d: curr_polling_cpu is alive = %d\n",
				 *		__func__, __LINE__, online_cpu_map & (1 << curr_polling_cpu));
				 */

				online_cpu_map |= (1 << cpu);

				/* check curr_polling_cpu is alive, if it is down,
				 * start current cpu hrtimer, and change it to be currr_pollling_cpu
				 */
				if ((online_cpu_map & (1 << curr_polling_cpu)) == 0) {
					if (met_export_api_symbol->met_smp_call_function_single) {
						met_export_api_symbol->met_smp_call_function_single(cpu,
												__met_init_cpu_related_device, NULL, 1);
						met_export_api_symbol->met_smp_call_function_single(cpu,
												__met_hrtimer_register, NULL, 1);
					}
					curr_polling_cpu = cpu;
				}
			}
		}

#if IS_ENABLED(CONFIG_CPU_FREQ)
		force_power_log(cpu);
#endif
		list_for_each_entry(c, &met_list, list) {
			if (c->cpu_state_notify)
				c->cpu_state_notify(cpu, action);
		}
		break;

	case MET_CPU_OFFLINE:
		list_for_each_entry(c, &met_list, list) {
			if (c->cpu_state_notify)
				c->cpu_state_notify(cpu, action);
		}

		online_cpu_map &= ~(1 << cpu);
		dbg_met_tag_oneshot(0, "met_offline cpu", cpu);
		if (cpu == curr_polling_cpu) {
			/* pr_info("%s, %d: curr_polling_cpu %d is down\n",
			 *		__func__, __LINE__, curr_polling_cpu);
			 */
			preferred_polling_cpu = calc_preferred_polling_cpu(online_cpu_map);
			/* pr_info("%s, %d: preferred_polling_cpu = %d\n",
			 *		__func__, __LINE__, preferred_polling_cpu);
			 */
			if (preferred_polling_cpu != -1) {
				curr_polling_cpu = preferred_polling_cpu;
				dbg_met_tag_oneshot(0, "met_curr polling cpu", curr_polling_cpu);

				if (cpu_related_cnt == 0) {
					/* pr_info("%s, %d: start cpu %d hrtimer start\n",
					 *		__func__, __LINE__, curr_polling_cpu);
					 */
					if (met_export_api_symbol->met_smp_call_function_single)
						met_export_api_symbol->met_smp_call_function_single(curr_polling_cpu,
												__met_hrtimer_register, NULL, 1);
				} else if (cpu_related_polling_hdlr_cnt == 0) {
					if (met_export_api_symbol->met_smp_call_function_single)
						met_export_api_symbol->met_smp_call_function_single(curr_polling_cpu,
												__met_hrtimer_register, NULL, 1);
				}
			}
		}
		if (met_export_api_symbol->met_smp_call_function_single)
			met_export_api_symbol->met_smp_call_function_single(cpu,
									__met_hrtimer_stop, NULL, 1);

		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		dw = &met_cpu_ptr->dwork;
		cancel_delayed_work_sync(dw);

		/* sync_samples(cpu); */
		break;
	default:
		list_for_each_entry(c, &met_list, list) {
			if (c->cpu_state_notify)
				c->cpu_state_notify(cpu, action);
		}
	}

	return NOTIFY_OK;
}

static int _met_pmu_cpu_notify_online(unsigned int cpu)
{
	met_pmu_cpu_notify(MET_CPU_ONLINE, cpu);

	return 0;
}

static int _met_pmu_cpu_notify_offline(unsigned int cpu)
{
	met_pmu_cpu_notify(MET_CPU_OFFLINE, cpu);

	return 0;
}

int sampler_start(void)
{
	int ret, cpu;
	struct met_cpu_struct *met_cpu_ptr;
	struct metdevice *c;
	int preferred_polling_cpu;

	met_set_suspend_notify(0);

#if	IS_ENABLED(CONFIG_CPU_FREQ)
	force_power_log(POWER_LOG_ALL);
#endif

	for_each_possible_cpu(cpu) {
		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		met_cpu_ptr->work_enabled = 0;
		met_cpu_ptr->hrtimer_online_check = 0;
		hrtimer_init(&met_cpu_ptr->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		met_cpu_ptr->hrtimer.function = met_hrtimer_notify;
		INIT_DELAYED_WORK(&met_cpu_ptr->dwork, wq_sync_buffer);
	}

	start = 0;
	ret = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN,
						   "met:online",
						   _met_pmu_cpu_notify_online,
						   _met_pmu_cpu_notify_offline);

	list_for_each_entry(c, &met_list, list) {

		if (try_module_get(c->owner) == 0)
			continue;

		if ((c->mode) && (c->cpu_related == 1)) {
			cpu_related_cnt = 1;

			if (c->ondiemet_mode == 0) {
				if (c->timed_polling)
					cpu_related_polling_hdlr_cnt = 1;
			} else if (c->ondiemet_mode == 1) {
				if (c->ondiemet_timed_polling)
					cpu_related_polling_hdlr_cnt = 1;
			} else if (c->ondiemet_mode == 2) {
				if (c->timed_polling || c->ondiemet_timed_polling)
					cpu_related_polling_hdlr_cnt = 1;
			}
		}

		if (c->ondiemet_mode == 0) {
			if ((!(c->cpu_related)) && (c->mode) && (c->start))
				c->start();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_start))
				c->uniq_start();
		} else if (c->ondiemet_mode == 1) {
			if ((!(c->cpu_related)) && (c->mode) && (c->ondiemet_start))
				c->ondiemet_start();
			if ((c->cpu_related) && (c->mode) && (c->uniq_ondiemet_start))
				c->uniq_ondiemet_start();
		} else if (c->ondiemet_mode == 2) {
			if ((!(c->cpu_related)) && (c->mode) && (c->start))
				c->start();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_start))
				c->uniq_start();

			if ((!(c->cpu_related)) && (c->mode) && (c->ondiemet_start))
				c->ondiemet_start();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_ondiemet_start))
				c->uniq_ondiemet_start();
		}
	}

	get_online_cpus();
	online_cpu_map = 0;
	for_each_online_cpu(cpu) {
		online_cpu_map |= (1 << cpu);
	}
	dbg_met_tag_oneshot(0, "met_online cpu map", online_cpu_map);
	preferred_polling_cpu = calc_preferred_polling_cpu(online_cpu_map);
	if (preferred_polling_cpu != -1)
		curr_polling_cpu = preferred_polling_cpu;
	dbg_met_tag_oneshot(0, "met_curr polling cpu", curr_polling_cpu);
	start = 1;

	if (cpu_related_cnt == 0) {
		if (met_export_api_symbol->met_smp_call_function_single)
			met_export_api_symbol->met_smp_call_function_single(curr_polling_cpu,
									__met_hrtimer_register, NULL, 1);
	}
	else {
		//on_each_cpu(__met_hrtimer_start, NULL, 1);
		for_each_online_cpu(cpu) {
			if (met_export_api_symbol->met_smp_call_function_single)
				met_export_api_symbol->met_smp_call_function_single(cpu,
										__met_init_cpu_related_device, NULL, 1);
		}

		if (cpu_related_polling_hdlr_cnt) {
			for_each_online_cpu(cpu) {
				if (met_export_api_symbol->met_smp_call_function_single)
					met_export_api_symbol->met_smp_call_function_single(cpu,
											__met_hrtimer_register, NULL, 1);
			}
		} else {
			if (met_export_api_symbol->met_smp_call_function_single)
				met_export_api_symbol->met_smp_call_function_single(curr_polling_cpu,
										__met_hrtimer_register, NULL, 1);
		}
	}
	put_online_cpus();

	return ret;
}

void sampler_stop(void)
{
	int cpu;
	struct met_cpu_struct *met_cpu_ptr;
	struct metdevice *c;
	struct delayed_work *dw;


	get_online_cpus();
	//on_each_cpu(__met_hrtimer_stop, NULL, 1);
	online_cpu_map = 0;
	for_each_online_cpu(cpu) {
		online_cpu_map |= (1 << cpu);
	}

	for_each_online_cpu(cpu) {
		if (met_export_api_symbol->met_smp_call_function_single)
			met_export_api_symbol->met_smp_call_function_single(cpu,
									__met_hrtimer_stop, NULL, 1);
	}

	/* for_each_online_cpu(cpu) { */
	for_each_possible_cpu(cpu) {	/* Just for case */
		met_cpu_ptr = &per_cpu(met_cpu, cpu);
		dw = &met_cpu_ptr->dwork;
		cancel_delayed_work_sync(dw);
		/* sync_samples(cpu); */
	}
	start = 0;
	put_online_cpus();

	cpuhp_remove_state_nocalls(CPUHP_AP_ONLINE_DYN);

	list_for_each_entry(c, &met_list, list) {
		if (c->ondiemet_mode == 0) {
			if ((!(c->cpu_related)) && (c->mode) && (c->stop))
				c->stop();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_stop))
				c->uniq_stop();
		} else if (c->ondiemet_mode == 1) {
			if ((!(c->cpu_related)) && (c->mode) && (c->ondiemet_stop))
				c->ondiemet_stop();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_ondiemet_stop))
				c->uniq_ondiemet_stop();
		} else if (c->ondiemet_mode == 2) {
			if ((!(c->cpu_related)) && (c->mode) && (c->stop))
				c->stop();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_stop))
				c->uniq_stop();

			if ((!(c->cpu_related)) && (c->mode) && (c->ondiemet_stop))
				c->ondiemet_stop();
			else if ((c->cpu_related) && (c->mode) && (c->uniq_ondiemet_stop))
				c->uniq_ondiemet_stop();
		}
		module_put(c->owner);
	}

	cpu_related_cnt = 0;
	cpu_related_polling_hdlr_cnt = 0;
}

#if 0 /* cann't use static now */
enum {
	MET_SUSPEND = 1,
	MET_RESUME = 2,
};

static noinline void tracing_mark_write(int op)
{
	switch (op) {
	case MET_SUSPEND:
		MET_TRACE("C|0|MET_SUSPEND|1");
		break;
	case MET_RESUME:
		MET_TRACE("C|0|MET_SUSPEND|0");
		break;
	}
}
#endif

int met_hrtimer_suspend(void)
{
	struct metdevice *c;

	met_set_suspend_notify(1);
	/* tracing_mark_write(MET_SUSPEND); */
	tracing_mark_write(TYPE_MET_SUSPEND, 0, 0, 0, 0, 0);
	if (start == 0)
		return 0;

	list_for_each_entry(c, &met_list, list) {
		if (c->suspend)
			c->suspend();
	}

	/* get current COUNT */
	MET_TRACE("TS: %llu GPT: %llX", sched_clock(), arch_counter_get_cntvct());
	return 0;
}

void met_hrtimer_resume(void)
{
	struct metdevice *c;

	/* get current COUNT */
	MET_TRACE("TS: %llu GPT: %llX", sched_clock(), arch_counter_get_cntvct());

	/* tracing_mark_write(MET_RESUME); */
	tracing_mark_write(TYPE_MET_RESUME, 0, 0, 0, 0, 0);
	if (start == 0)
		return;

	list_for_each_entry(c, &met_list, list) {
		if (c->resume)
			c->resume();
	}
}

/*
 * event timer:
 * register IRQ, sched_switch event to monitor Polling count
 * count can be printed at any live cpu.
 */
void met_event_timer_notify(void)
{
	unsigned long long stamp;
	struct metdevice *c;
	int cpu = -1;

	if (start == 0)
		return;

	cpu = smp_processor_id();
	list_for_each_entry(c, &met_list, list) {
		stamp = local_clock();

		if (c->prev_stamp == 0)
			c->prev_stamp = stamp;

		/* Critical Section Start */
		/* try spinlock to prevent a event print twice between config time interval */
		if (!spin_trylock(&(c->my_lock)))
			continue;

		/*
		 * DEFAULT_HRTIMER_EXPIRE (met_hrtimer_expire):
		 * sample_rate == 0 --> always print
		 * sample_rate == 1000 --> print interval larger than 1 ms
		 */
		if (DEFAULT_HRTIMER_EXPIRE == 0 || (stamp - c->prev_stamp) < DEFAULT_HRTIMER_EXPIRE) {
			spin_unlock(&(c->my_lock));
			continue;
		}

		c->prev_stamp = stamp;
		spin_unlock(&(c->my_lock));
		/* Critical Section End */

		if ((c->mode == 0) || (c->timed_polling == NULL))
			continue;

		stamp = local_clock();
		c->timed_polling(stamp, cpu);
	}
}

