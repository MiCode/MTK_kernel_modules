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

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/fs.h>

#include "met_drv.h"
#include "trace.h"
#include "core_plf_init.h"
#include "core_plf_trace.h"

static unsigned int MT_GPU_DVFS_IDX = NR_MT_CPU_DVFS;
static unsigned int g_u4GPUVolt;
static unsigned int g_u4Volt[NR_MT_CPU_DVFS + 1];

/* g_ap_ptpod: cpu volt from ap or sspm setting
 * if 1: cpu volt from ap
 * if 0: cpu volt from sspm */
static unsigned int g_ap_ptpod;

/* gpu_volt_enable:
 * if 1: enable gpu volt to output
 * if 0: disable gpu volt to output */
static unsigned int gpu_volt_enable = 1;

/* get_volt_by_wq:
 * if 1: get cpu/gpu volt by workqueue
 * if 0: get cpu/gpu volt in irq */
static unsigned int get_volt_by_wq;

static int ptpod_started;
static struct kobject *kobj_ptpod;
static struct delayed_work get_volt_dwork;

noinline void ms_ptpod(void)
{
	char *SOB, *EOB;

	if (g_ap_ptpod) {
		MET_TRACE_GETBUF(&SOB, &EOB);

		if (gpu_volt_enable) {
			g_u4Volt[MT_GPU_DVFS_IDX] = g_u4GPUVolt;
			EOB = ms_formatD_EOL(EOB, ARRAY_SIZE(g_u4Volt), g_u4Volt);
		} else
			EOB = ms_formatD_EOL(EOB, ARRAY_SIZE(g_u4Volt) - 1, g_u4Volt);

		MET_TRACE_PUTBUF(SOB, EOB);
	} else {
		if (gpu_volt_enable) {
			MET_TRACE_GETBUF(&SOB, &EOB);
			EOB = ms_formatD_EOL(EOB, 1, &g_u4GPUVolt);
			MET_TRACE_PUTBUF(SOB, EOB);
		}
	}
}

#if 0
static void ptpod_cpu_voltSampler(enum mt_cpu_dvfs_id id, unsigned int volt)
{
	switch (id) {
	case MT_CPU_DVFS_LL:
		g_u4CPUVolt_LL = volt;
		break;
	case MT_CPU_DVFS_L:
		g_u4CPUVolt_L = volt;
		break;
	case MT_CPU_DVFS_CCI:
		g_u4CPUVolt_CCI = volt;
		break;
	default:
		return;
	}
	ptpod();
}
#endif

#if 0
static void ptpod_gpu_voltSampler(unsigned int a_u4Volt)
{
	g_u4GPUVolt = (a_u4Volt+50)/100;

	if (ptpod_started)
		ptpod();
}
#endif

#define PTPOD_CONF_SHOW_IMPLEMENT(var) \
	do { \
		int i; \
		i = snprintf(buf, PAGE_SIZE, "%d\n", var); \
		return i; \
	} while (0)

#define PTPOD_CONF_STORE_IMPLEMENT(var) \
	do { \
		int value; \
		if ((n == 0) || (buf == NULL)) \
			return -EINVAL; \
		if (kstrtoint(buf, 0, &value) != 0) \
			return -EINVAL; \
		if (value == 1) \
			var = 1; \
		else \
			var = 0; \
		return n; \
	} while (0)


static ssize_t ap_ptpod_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	PTPOD_CONF_SHOW_IMPLEMENT(g_ap_ptpod);
}

static ssize_t ap_ptpod_enable_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	PTPOD_CONF_STORE_IMPLEMENT(g_ap_ptpod);
}

static ssize_t gpu_volt_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	PTPOD_CONF_SHOW_IMPLEMENT(gpu_volt_enable);
}

static ssize_t gpu_volt_enable_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	PTPOD_CONF_STORE_IMPLEMENT(gpu_volt_enable);
}

static ssize_t get_volt_by_wq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	PTPOD_CONF_SHOW_IMPLEMENT(get_volt_by_wq);
}

static ssize_t get_volt_by_wq_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	PTPOD_CONF_STORE_IMPLEMENT(get_volt_by_wq);
}

static struct kobj_attribute ap_ptpod_enable_attr = __ATTR(ap_ptpod_enable, 0664, ap_ptpod_enable_show, ap_ptpod_enable_store);
static struct kobj_attribute gpu_volt_enable_attr = __ATTR(gpu_volt_enable, 0664, gpu_volt_enable_show, gpu_volt_enable_store);
static struct kobj_attribute get_volt_by_wq_attr = __ATTR(get_volt_by_wq, 0664, get_volt_by_wq_show, get_volt_by_wq_store);

/* create ptpod related kobj node */
#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(ap_ptpod_enable); \
		KOBJ_ATTR_ITEM(gpu_volt_enable); \
		KOBJ_ATTR_ITEM(get_volt_by_wq); \
	} while (0)

static int ptpod_create(struct kobject *parent)
{
	int ret = 0;

	kobj_ptpod = parent;

#define KOBJ_ATTR_ITEM(attr_name) \
	do { \
		ret = sysfs_create_file(kobj_ptpod, &attr_name ## _attr.attr); \
		if (ret != 0) { \
			pr_notice("Failed to create " #attr_name " in sysfs\n"); \
			return ret; \
		} \
	} while (0)
	KOBJ_ATTR_LIST;
#undef  KOBJ_ATTR_ITEM

	return 0;
}

static void ptpod_delete(void)
{
#define KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_ptpod, &attr_name ## _attr.attr)

	if (kobj_ptpod != NULL) {
		KOBJ_ATTR_LIST;
		kobj_ptpod = NULL;
	}
#undef  KOBJ_ATTR_ITEM
}

static void update_volt_value(void)
{
	int i;

	if (g_ap_ptpod && mt_cpufreq_get_cur_volt_symbol) {
		/*
		g_u4CPUVolt_LL = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_LL)/100;
		g_u4CPUVolt_L = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_L)/100;
		g_u4CPUVolt_CCI = mt_cpufreq_get_cur_volt_symbol(MT_CPU_DVFS_CCI)/100;
		*/
		for (i = 0; i < NR_MT_CPU_DVFS; i++)
			g_u4Volt[i] = mt_cpufreq_get_cur_volt_symbol(i) / 100;
	}

	if (gpu_volt_enable) {
		if (mt_gpufreq_get_cur_volt_symbol)
			g_u4GPUVolt = ((mt_gpufreq_get_cur_volt_symbol() + 50) / 100);
	}
}

static void get_volt_notify(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&get_volt_dwork, 0);
}

static void met_ptpod_polling_by_wq(struct work_struct *work)
{
	update_volt_value();

	ms_ptpod();
}

static void met_ptpod_polling(unsigned long long stamp, int cpu)
{
	update_volt_value();

	ms_ptpod();
}

/*
 * Called from "met-cmd --start"
 */
static void ptpod_start(void)
{
#if 0
	met_gpufreq_setvolt_registerCB(ptpod_gpu_voltSampler, kFOR_MET_PTPOD_USE);
#endif

	if (get_volt_by_wq) {
		met_ptpod.timed_polling = get_volt_notify;

		INIT_DELAYED_WORK(&get_volt_dwork, met_ptpod_polling_by_wq);
	} else
		met_ptpod.timed_polling = met_ptpod_polling;

	update_volt_value();

	ms_ptpod();

#if 0
	/* register callback */
	if (mt_cpufreq_setvolt_registerCB_symbol)
		mt_cpufreq_setvolt_registerCB_symbol(ptpod_cpu_voltSampler);
#endif

	ptpod_started = 1;
}

/*
 * Called from "met-cmd --stop"
 */
static void ptpod_stop(void)
{
	ptpod_started = 0;

#if 0
	/* unregister callback */
	if (mt_cpufreq_setvolt_registerCB_symbol)
		mt_cpufreq_setvolt_registerCB_symbol(NULL);
#endif

	if (get_volt_by_wq)
		cancel_delayed_work_sync(&get_volt_dwork);

	update_volt_value();

	ms_ptpod();

#if 0
	met_gpufreq_setvolt_registerCB(NULL, kFOR_MET_PTPOD_USE);
#endif
}

static char help[] =
	"  --ptpod                               Measure CPU/GPU voltage\n";
static int ptpod_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

/*
 * It will be called back when run "met-cmd --extract" and mode is 1
 */
static int ptpod_print_header(char *buf, int len)
{
	int str_len = 0;

	if (g_ap_ptpod) {
		int i;

		str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
						"met-info [000] 0.0: met_ptpod_header: ");

		for (i = 0; i < NR_MT_CPU_DVFS; i++)
			str_len += snprintf(buf + str_len, PAGE_SIZE - str_len, "CPUVolt_%d,", i);

		if (gpu_volt_enable)
			str_len += snprintf(buf + str_len, PAGE_SIZE - str_len, "GPUVolt,");

		buf[str_len-1] = '\n';

		str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
							"met-info [000] 0.0: met_ptpod_version: ap\n");
	} else {
		if (gpu_volt_enable) {
			str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
							"met-info [000] 0.0: met_ptpod_header: ");

			str_len += snprintf(buf + str_len, PAGE_SIZE - str_len, "GPUVolt\n");
		}

		str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
							"met-info [000] 0.0: met_ptpod_version: sspm\n");
	}

	if (gpu_volt_enable)
		str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
						"met-info [000] 0.0: met_ptpod_gpu_volt_enable: YES\n");
	else
		str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
						"met-info [000] 0.0: met_ptpod_gpu_volt_enable: NO\n");

	return str_len;
}

struct metdevice met_ptpod = {
	.name = "ptpod",
	.owner = THIS_MODULE,
	.type = MET_TYPE_PMU,
	.cpu_related = 0,
	.create_subfs = ptpod_create,
	.delete_subfs = ptpod_delete,
	.start = ptpod_start,
	.stop = ptpod_stop,
	.timed_polling = met_ptpod_polling,
	.print_help = ptpod_print_help,
	.print_header = ptpod_print_header,
};
EXPORT_SYMBOL(met_ptpod);
