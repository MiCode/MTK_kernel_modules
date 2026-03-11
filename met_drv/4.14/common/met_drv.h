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


#ifndef MET_DRV
#define MET_DRV

#include <linux/version.h>
#include <linux/preempt.h>
#include <linux/device.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>
#include <linux/clk.h>

extern int met_mode;
extern int core_plf_init(void);
extern void core_plf_exit(void);

#define MET_MODE_TRACE_CMD_OFFSET	(1)
#define MET_MODE_TRACE_CMD			(1<<MET_MODE_TRACE_CMD_OFFSET)

#ifdef CONFIG_MET_MODULE
#define my_preempt_enable() preempt_enable()
#else
#define my_preempt_enable() preempt_enable_no_resched()
#endif

#define MET_STRBUF_SIZE		1024
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_nmi);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_irq);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf_sirq);
DECLARE_PER_CPU(char[MET_STRBUF_SIZE], met_strbuf);

#ifdef CONFIG_TRACING
#define TRACE_PUTS(p) \
	do { \
		trace_puts(p);; \
	} while (0)
#else
#define TRACE_PUTS(p) do {} while (0)
#endif

#define GET_MET_TRACE_BUFFER_ENTER_CRITICAL() \
	({ \
		char *pmet_strbuf; \
		preempt_disable(); \
		if (in_nmi()) \
			pmet_strbuf = per_cpu(met_strbuf_nmi, smp_processor_id()); \
		else if (in_irq()) \
			pmet_strbuf = per_cpu(met_strbuf_irq, smp_processor_id()); \
		else if (in_softirq()) \
			pmet_strbuf = per_cpu(met_strbuf_sirq, smp_processor_id()); \
		else \
			pmet_strbuf = per_cpu(met_strbuf, smp_processor_id()); \
		pmet_strbuf;\
	})

#define PUT_MET_TRACE_BUFFER_EXIT_CRITICAL(pmet_strbuf) \
	do {\
		if (pmet_strbuf)\
			TRACE_PUTS(pmet_strbuf); \
		my_preempt_enable(); \
	} while (0)

#define MET_TRACE(FORMAT, args...) \
	do { \
		int _met_trace_str; \
		char *pmet_strbuf; \
		preempt_disable(); \
		if (in_nmi()) \
			pmet_strbuf = per_cpu(met_strbuf_nmi, smp_processor_id()); \
		else if (in_irq()) \
			pmet_strbuf = per_cpu(met_strbuf_irq, smp_processor_id()); \
		else if (in_softirq()) \
			pmet_strbuf = per_cpu(met_strbuf_sirq, smp_processor_id()); \
		else \
			pmet_strbuf = per_cpu(met_strbuf, smp_processor_id()); \
		if (met_mode & MET_MODE_TRACE_CMD) \
			_met_trace_str = snprintf(pmet_strbuf, MET_STRBUF_SIZE, "%s: " FORMAT, __func__, ##args); \
		else \
			_met_trace_str = snprintf(pmet_strbuf, MET_STRBUF_SIZE, FORMAT, ##args); \
		if (_met_trace_str > 0) \
			TRACE_PUTS(pmet_strbuf); \
		my_preempt_enable(); \
	} while (0)

/*
 * SOB: start of buf
 * EOB: end of buf
 */
#define MET_TRACE_GETBUF(pSOB, pEOB) \
	({ \
		preempt_disable(); \
		if (in_nmi()) \
			*pSOB = per_cpu(met_strbuf_nmi, smp_processor_id()); \
		else if (in_irq()) \
			*pSOB = per_cpu(met_strbuf_irq, smp_processor_id()); \
		else if (in_softirq()) \
			*pSOB = per_cpu(met_strbuf_sirq, smp_processor_id()); \
		else \
			*pSOB = per_cpu(met_strbuf, smp_processor_id()); \
		*pEOB = *pSOB; \
		if (met_mode & MET_MODE_TRACE_CMD) \
			*pEOB += snprintf(*pEOB, MET_STRBUF_SIZE, "%s: ", __func__); \
	})

#define MET_TRACE_PUTBUF(SOB, EOB) \
	({ \
		__trace_puts(_THIS_IP_, (SOB), (uintptr_t)((EOB)-(SOB))); \
		my_preempt_enable(); \
	})

#define MET_FTRACE_DUMP(TRACE_NAME, args...)			\
	do {							\
		trace_##TRACE_NAME(args);;			\
	} while (0)


#define MET_TYPE_PMU	1
#define MET_TYPE_BUS	2
#define MET_TYPE_MISC	3

enum met_action {
	MET_CPU_ONLINE,
	MET_CPU_OFFLINE,

	NR_MET_ACTION,
};

struct metdevice {
	struct list_head list;
	int type;
	const char *name;
	struct module *owner;
	struct kobject *kobj;

	int (*create_subfs)(struct kobject *parent);
	void (*delete_subfs)(void);
	int mode;
	int ondiemet_mode;	/* new for ondiemet; 1: call ondiemet functions */
	int cpu_related;
	int polling_interval;
	int polling_count_reload;
	int __percpu *polling_count;
	int header_read_again;	/*for header size > 1 page */
	void (*start)(void);
	void (*uniq_start)(void);
	void (*stop)(void);
	void (*uniq_stop)(void);
	int (*reset)(void);
	void (*timed_polling)(unsigned long long stamp, int cpu);
	void (*tagged_polling)(unsigned long long stamp, int cpu);
	int (*print_help)(char *buf, int len);
	int (*print_header)(char *buf, int len);
	int (*process_argument)(const char *arg, int len);
	void (*cpu_state_notify)(long cpu, unsigned long action);

	void (*ondiemet_start)(void);
	void (*uniq_ondiemet_start)(void);
	void (*ondiemet_stop)(void);
	void (*uniq_ondiemet_stop)(void);
	int (*ondiemet_reset)(void);
	int (*ondiemet_print_help)(char *buf, int len);
	int (*ondiemet_print_header)(char *buf, int len);
	int (*ondiemet_process_argument)(const char *arg, int len);
	void (*ondiemet_timed_polling)(unsigned long long stamp, int cpu);
	void (*ondiemet_tagged_polling)(unsigned long long stamp, int cpu);

	struct list_head exlist;	/* for linked list before register */
	void (*suspend)(void);
	void (*resume)(void);

	unsigned long long prev_stamp;
	spinlock_t my_lock;
	void *reversed1;
};

int met_register(struct metdevice *met);
int met_deregister(struct metdevice *met);
int met_set_platform(const char *plf_name, int flag);
int met_set_chip_id(const unsigned int chip_id);
int met_set_topology(const char *topology_name, int flag);
int met_devlink_add(struct metdevice *met);
int met_devlink_del(struct metdevice *met);
int met_devlink_register_all(void);
int met_devlink_deregister_all(void);

int fs_reg(int met_minor);
void fs_unreg(void);

/******************************************************************************
 * Tracepoints
 ******************************************************************************/
#define MET_DEFINE_PROBE(probe_name, proto) \
		static void probe_##probe_name(void *data, PARAMS(proto))
#define MET_REGISTER_TRACE(probe_name) \
		register_trace_##probe_name(probe_##probe_name, NULL)
#define MET_UNREGISTER_TRACE(probe_name) \
		unregister_trace_##probe_name(probe_##probe_name, NULL)


/* ====================== Tagging API ================================ */

#define MAX_EVENT_CLASS	31
#define MAX_TAGNAME_LEN	128
#define MET_CLASS_ALL	0x80000000

/* IOCTL commands of MET tagging */
struct mtag_cmd_t {
	unsigned int class_id;
	unsigned int value;
	unsigned int slen;
	char tname[MAX_TAGNAME_LEN];
	void *data;
	unsigned int size;
};

#define TYPE_START		1
#define TYPE_END		2
#define TYPE_ONESHOT		3
#define TYPE_ENABLE		4
#define TYPE_DISABLE		5
#define TYPE_REC_SET		6
#define TYPE_DUMP		7
#define TYPE_DUMP_SIZE		8
#define TYPE_DUMP_SAVE		9
#define TYPE_USRDATA		10
#define TYPE_DUMP_AGAIN		11
#define TYPE_ASYNC_START	12
#define TYPE_ASYNC_END		13
#define TYPE_MET_SUSPEND	15
#define TYPE_MET_RESUME		16

/* Use 'm' as magic number */
#define MTAG_IOC_MAGIC  'm'
/* Please use a different 8-bit number in your code */
#define MTAG_CMD_START		_IOW(MTAG_IOC_MAGIC, TYPE_START, struct mtag_cmd_t)
#define MTAG_CMD_END		_IOW(MTAG_IOC_MAGIC, TYPE_END, struct mtag_cmd_t)
#define MTAG_CMD_ONESHOT	_IOW(MTAG_IOC_MAGIC, TYPE_ONESHOT, struct mtag_cmd_t)
#define MTAG_CMD_ENABLE		_IOW(MTAG_IOC_MAGIC, TYPE_ENABLE, int)
#define MTAG_CMD_DISABLE	_IOW(MTAG_IOC_MAGIC, TYPE_DISABLE, int)
#define MTAG_CMD_REC_SET	_IOW(MTAG_IOC_MAGIC, TYPE_REC_SET, int)
#define MTAG_CMD_DUMP		_IOW(MTAG_IOC_MAGIC, TYPE_DUMP, struct mtag_cmd_t)
#define MTAG_CMD_DUMP_SIZE	_IOWR(MTAG_IOC_MAGIC, TYPE_DUMP_SIZE, int)
#define MTAG_CMD_DUMP_SAVE	_IOW(MTAG_IOC_MAGIC, TYPE_DUMP_SAVE, struct mtag_cmd_t)
#define MTAG_CMD_USRDATA	_IOW(MTAG_IOC_MAGIC, TYPE_USRDATA, struct mtag_cmd_t)
#define MTAG_CMD_DUMP_AGAIN	_IOW(MTAG_IOC_MAGIC, TYPE_DUMP_AGAIN, void *)
#define MTAG_CMD_ASYNC_START	_IOW(MTAG_IOC_MAGIC, TYPE_ASYNC_START, struct mtag_cmd_t)
#define MTAG_CMD_ASYNC_END	_IOW(MTAG_IOC_MAGIC, TYPE_ASYNC_END, struct mtag_cmd_t)

/* include file */
extern int met_tag_start_real(unsigned int class_id, const char *name);
extern int met_tag_end_real(unsigned int class_id, const char *name);
extern int met_tag_async_start_real(unsigned int class_id, const char *name, unsigned int cookie);
extern int met_tag_async_end_real(unsigned int class_id, const char *name, unsigned int cookie);
extern int met_tag_oneshot_real(unsigned int class_id, const char *name, unsigned int value);
extern int met_tag_userdata_real(char *pData);
extern int met_tag_dump_real(unsigned int class_id, const char *name, void *data, unsigned int length);
extern int met_tag_disable_real(unsigned int class_id);
extern int met_tag_enable_real(unsigned int class_id);
extern int met_set_dump_buffer_real(int size);
extern int met_save_dump_buffer_real(const char *pathname);
extern int met_save_log_real(const char *pathname);
extern int met_show_bw_limiter_real(void);
extern int met_reg_bw_limiter_real(void *fp);
extern int met_show_clk_tree_real(const char *name, unsigned int addr, unsigned int status);
extern int met_reg_clk_tree_real(void *fp);
extern int enable_met_backlight_tag_real(void);
extern int output_met_backlight_tag_real(int level);

#endif	/* MET_DRV */
