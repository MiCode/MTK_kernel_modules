// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <asm/page.h>
#include "interface.h"
#include "met_drv.h"

#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
#include <trace/events/gpu.h>

#define show_secs_from_ns(ns) \
	({ \
		u64 t = ns + (NSEC_PER_USEC / 2); \
		do_div(t, NSEC_PER_SEC); \
		t; \
	})

#define show_usecs_from_ns(ns) \
	({ \
		u64 t = ns + (NSEC_PER_USEC / 2) ; \
		u32 rem; \
		do_div(t, NSEC_PER_USEC); \
		rem = do_div(t, USEC_PER_SEC); \
	})

static int event_gpu_registered;
static int event_gpu_enabled;

noinline void gpu_sched_switch(const char *gpu_name, u64 timestamp,
				      u32 next_ctx_id, s32 next_prio, u32 next_job_id)
{
	MET_TRACE("gpu_name=%s ts=%llu.%06lu next_ctx_id=%lu next_prio=%ld next_job_id=%lu\n",
		   gpu_name,
		   (unsigned long long)show_secs_from_ns(timestamp),
		   (unsigned long)show_usecs_from_ns(timestamp),
		   (unsigned long)next_ctx_id, (long)next_prio, (unsigned long)next_job_id);
}

MET_DEFINE_PROBE(gpu_sched_switch, TP_PROTO(const char *gpu_name, u64 timestamp,
		u32 next_ctx_id, s32 next_prio, u32 next_job_id))
{
	gpu_sched_switch(gpu_name, timestamp, next_ctx_id, next_prio, next_job_id);
}

noinline void gpu_job_enqueue(u32 ctx_id, u32 job_id, const char *type)
{
	MET_TRACE("ctx_id=%lu job_id=%lu type=%s",
		   (unsigned long)ctx_id, (unsigned long)job_id, type);
}

MET_DEFINE_PROBE(gpu_job_enqueue, TP_PROTO(u32 ctx_id, u32 job_id, const char *type))
{
	gpu_job_enqueue(ctx_id, job_id, type);
}
#endif


#ifdef  MET_EVENT_POWER_SUPPORT
#include "met_power.h"
#include "met_kernel_symbol.h"

static int event_power_registered;
static int event_power_enabled;

const char *
met_trace_print_symbols_seq(char* pclass_name, unsigned long val,
			const struct trace_print_flags *symbol_array)
{
	int i;
    size_t new_fsize=0;
    char _buf[32];
	const char *ret = pclass_name;

	for (i = 0;  symbol_array[i].name; i++) {

		if (val != symbol_array[i].mask)
			continue;

		new_fsize = sprintf(pclass_name, symbol_array[i].name, strlen(symbol_array[i].name));
		break;
	}

	if (new_fsize == 0) {
		snprintf(_buf, 32, "0x%lx", val);
		new_fsize = sprintf(pclass_name, _buf, strlen(_buf));
	}

	return ret;
}

#define __print_symbolic(pclass_name, value, symbol_array...)			\
	({								\
		static const struct trace_print_flags symbols[] =	\
			{ symbol_array, { -1, NULL }};			\
		met_trace_print_symbols_seq(pclass_name, value, symbols);		\
	})

#ifdef pm_qos_update_request
#undef pm_qos_update_request
#endif
void pm_qos_update_request(int pm_qos_class, s32 value)
{
	char class_name[64];
	MET_TRACE("pm_qos_class=%s value=%d owner=%s\n",
	  __print_symbolic(class_name, pm_qos_class,
		{ _PM_QOS_CPU_DMA_LATENCY,	"CPU_DMA_LATENCY" },
		{ _PM_QOS_NETWORK_LATENCY,	"NETWORK_LATENCY" },
		{ _PM_QOS_NETWORK_THROUGHPUT,	"NETWORK_THROUGHPUT" }),
	  value);
}
//#endif

#ifdef pm_qos_update_target
#undef pm_qos_update_target
#endif
void pm_qos_update_target(unsigned int action, int prev_value, int curr_value)
{
	char class_name[64];

	MET_TRACE("action=%s prev_value=%d curr_value=%d\n",
	  __print_symbolic(class_name, action,
		{ _PM_QOS_ADD_REQ,	"ADD_REQ" },
		{ _PM_QOS_UPDATE_REQ,	"UPDATE_REQ" },
		{ _PM_QOS_REMOVE_REQ,	"REMOVE_REQ" }),
	  prev_value, curr_value);
}
#endif
//#endif

static int reset_driver_stat(void)
{
#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	event_gpu_enabled = 0;
#endif
#ifdef MET_EVENT_POWER_SUPPORT
	event_power_enabled = 0;
#endif

	met_trace_event.mode = 0;
	return 0;
}



static void met_event_start(void)
{
#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	/* register trace event for gpu */
	do {
		if (!event_gpu_enabled)
			break;
		if (MET_REGISTER_TRACE(gpu_sched_switch)) {
			pr_debug("can not register callback of gpu_sched_switch\n");
			break;
		}
		if (MET_REGISTER_TRACE(gpu_job_enqueue)) {
			pr_debug("can not register callback of gpu_job_enqueue\n");
			MET_UNREGISTER_TRACE(gpu_sched_switch);
			break;
		}
		event_gpu_registered = 1;
	} while (0);
#endif

#ifdef  MET_EVENT_POWER_SUPPORT
	/* register trace event for power */
	do {
		if (!event_power_enabled)
			break;
		if (met_export_api_symbol->met_reg_event_power)
			if (met_export_api_symbol->met_reg_event_power()) {
				pr_debug("can not register callback of met_reg_event_power\n");
				break;
			}
		event_power_registered = 1;
	} while (0);
#endif

}

static void met_event_stop(void)
{
#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	/* unregister trace event for gpu */
	if (event_gpu_registered) {
		MET_UNREGISTER_TRACE(gpu_job_enqueue);
		MET_UNREGISTER_TRACE(gpu_sched_switch);
		event_gpu_registered = 0;
	}
#endif

#ifdef  MET_EVENT_POWER_SUPPORT
	/* unregister trace event for power */
	if (event_power_registered) {
		if (met_export_api_symbol->met_unreg_event_power)
			met_export_api_symbol->met_unreg_event_power();
		event_power_registered = 0;
	}
#endif
}

static int met_event_process_argument(const char *arg, int len)
{
	int	ret = -1;

#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	if (strcasecmp(arg, "gpu") == 0) {
		event_gpu_enabled = 1;
		met_trace_event.mode = 1;
		ret = 0;
	}
#endif
#ifdef  MET_EVENT_POWER_SUPPORT
	if (strcasecmp(arg, "power") == 0) {
		event_power_enabled = 1;
		met_trace_event.mode = 1;
		ret = 0;
	}
#endif
	return ret;
}

static const char help[] = "\b"
#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	"  --event=gpu                           output gpu trace events\n"
#endif
#ifdef  MET_EVENT_POWER_SUPPORT
	"  --event=power						 output pmqos trace events\n"
#endif
	;

static int met_event_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static const char header[] =
	"met-info [000] 0.0: met_ftrace_event:"
#if	IS_ENABLED(CONFIG_GPU_TRACEPOINTS)
	" gpu:gpu_sched_switch gpu:gpu_job_enqueue"
#endif
#ifdef  MET_EVENT_POWER_SUPPORT
	" power:pm_qos_update_request power:pm_qos_update_target"
#endif
	"\n";

static int met_event_print_header(char *buf, int len)
{
	int	ret;

	ret = snprintf(buf, PAGE_SIZE, header);
	return ret;
}

struct metdevice met_trace_event = {
	.name			= "event",
	.type			= MET_TYPE_PMU,
	.start			= met_event_start,
	.stop			= met_event_stop,
	.reset			= reset_driver_stat,
	.process_argument	= met_event_process_argument,
	.print_help		= met_event_print_help,
	.print_header		= met_event_print_header,
};
