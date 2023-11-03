// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>

#include "met_drv.h"
#include "trace.h"
#include "mtk_typedefs.h"
#include "core_plf_init.h"
#include "core_plf_trace.h"
#include "interface.h"


struct regulator_hdlr {
	char name[32];
	struct regulator *hdlr;
};

static struct regulator_hdlr *met_use_cpu_regulator;
static struct regulator_hdlr *met_use_gpu_regulator;

static int avail_cpu_hdlr_cnt;
static int avail_gpu_hdlr_cnt;
static int met_use_cpu_rg_cnt;
static int met_use_gpu_rg_cnt;
static char *cpu_rg_node;
static char *gpu_rg_node;
static unsigned int *g_u4Volt;

#define CPU_RG_NODE "mediatek,mt-cpufreq"
#define GPU_RG_NODE "mediatek,gpufreq"
#define CPU_REGULATOR_1 "proc1"
#define CPU_REGULATOR_2 "proc2"
#define GPU_REGULATOR_1 "_vgpu"

static int ptpod_started;
static struct kobject *kobj_ptpod;
static struct delayed_work get_volt_dwork;


static int parsing_use_regulator(const char *buf,
								size_t n,
								struct regulator_hdlr **rg,
								int *rg_cnt)
{
	char *reg_buf, *reg_name;
	char *ch;
	int input_cnt = 0;
	int ret = 0;

	reg_buf = kmalloc(n, GFP_KERNEL);
	if (reg_buf == NULL) {
		PR_BOOTMSG("alloc reg_buf fail !!!\n");
		ret = -EINVAL;
		goto out;
	}

	strncpy(reg_buf, buf, n);
	reg_buf[n-1] = '\0';

	ch = reg_buf;
	while (*ch)
		if (*ch++ == ',')
			input_cnt++;
	input_cnt++;

	if (*rg != NULL) {
		*rg_cnt = 0;
		kfree(*rg);
	}

	*rg = kzalloc(sizeof(struct regulator_hdlr) * input_cnt, GFP_KERNEL);
	if (*rg == NULL) {
		PR_BOOTMSG("alloc rg fail !!!\n");
		ret = -EINVAL;
		goto out;
	}

	while ((reg_name = strsep(&reg_buf, ","))) {
		strlcpy((*rg)[*rg_cnt].name, reg_name, sizeof((*rg)[*rg_cnt].name));
		(*rg_cnt)++;
	}

out:
	if (reg_buf)
		kfree(reg_buf);

	return ret;
}

static ssize_t cpu_regulator_show(struct kobject *kobj,
								struct kobj_attribute *attr,
								char *buf)
{
	int i;
	int size = 0;

	if (met_use_cpu_rg_cnt) {
		for (i = 0; i < met_use_cpu_rg_cnt; i++)
			size += snprintf(buf+size, PAGE_SIZE-size, "%s,", met_use_cpu_regulator[i].name);

		buf[size - 1] = '\n';
	} else
		size += snprintf(buf+size, PAGE_SIZE-size, "No use cpu regulator !!!\n");

	return size;
}

static ssize_t cpu_regulator_store(struct kobject *kobj,
								struct kobj_attribute *attr,
								const char *buf,
								size_t n)
{
	int ret;

	if ((n == 0) || (n > 128) || (buf == NULL))
		return -EINVAL;

	ret = parsing_use_regulator(buf, n, &met_use_cpu_regulator, &met_use_cpu_rg_cnt);
	if (ret < 0)
		return ret;

	return n;
}

static ssize_t gpu_regulator_show(struct kobject *kobj,
								struct kobj_attribute *attr,
								char *buf)
{
	int i;
	int size = 0;

	if (met_use_gpu_rg_cnt) {
		for (i = 0; i < met_use_gpu_rg_cnt; i++)
			size += snprintf(buf+size, PAGE_SIZE-size, "%s,", met_use_gpu_regulator[i].name);

		buf[size - 1] = '\n';
	} else
		size += snprintf(buf+size, PAGE_SIZE-size, "No use gpu regulator !!!\n");

	return size;
}

static ssize_t gpu_regulator_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	int ret;

	if ((n == 0) || (n > 128) || (buf == NULL))
		return -EINVAL;

	ret = parsing_use_regulator(buf, n, &met_use_gpu_regulator, &met_use_gpu_rg_cnt);
	if (ret < 0)
		return ret;

	return n;
}

static ssize_t cpu_rg_dts_show(struct kobject *kobj,
							struct kobj_attribute *attr,
							char *buf)
{
	int size = 0;

	if (cpu_rg_node)
		size += snprintf(buf+size, PAGE_SIZE-size, "%s\n", cpu_rg_node);
	 else
		size += snprintf(buf+size, PAGE_SIZE-size, "No use cpu dts node !!!\n");

	return size;
}

static ssize_t cpu_rg_dts_store(struct kobject *kobj,
							struct kobj_attribute *attr,
							const char *buf,
							size_t n)
{
	if ((n == 0) || (n > 128) || (buf == NULL))
		return -EINVAL;

	if (cpu_rg_node != NULL)
		kfree(cpu_rg_node);

	cpu_rg_node = kmalloc(n, GFP_KERNEL);
	if (cpu_rg_node == NULL) {
		return -EINVAL;
	}

	strncpy(cpu_rg_node, buf, n);
	cpu_rg_node[n-1] = '\0';

	return n;
}

static ssize_t gpu_rg_dts_show(struct kobject *kobj,
							struct kobj_attribute *attr,
							char *buf)
{
	int size = 0;

	if (gpu_rg_node)
		size += snprintf(buf + size, PAGE_SIZE - size, "%s\n", gpu_rg_node);
	 else
		size += snprintf(buf + size, PAGE_SIZE - size, "No use gpu dts node !!!\n");

	return size;
}

static ssize_t gpu_rg_dts_store(struct kobject *kobj,
							struct kobj_attribute *attr,
							const char *buf,
							size_t n)
{
	if ((n == 0) || (n > 128) || (buf == NULL))
		return -EINVAL;

	if (gpu_rg_node != NULL)
		kfree(gpu_rg_node);

	gpu_rg_node = kmalloc(n, GFP_KERNEL);
	if (gpu_rg_node == NULL) {
		return -EINVAL;
	}

	strncpy(gpu_rg_node, buf, n);
	gpu_rg_node[n-1] = '\0';

	return n;
}

DECLARE_KOBJ_ATTR(cpu_regulator);
DECLARE_KOBJ_ATTR(gpu_regulator);
DECLARE_KOBJ_ATTR(cpu_rg_dts);
DECLARE_KOBJ_ATTR(gpu_rg_dts);

noinline void ms_ptpod(void)
{
	char *SOB, *EOB;

	MET_TRACE_GETBUF(&SOB, &EOB);

	EOB = ms_formatD_EOL(EOB, avail_cpu_hdlr_cnt + avail_gpu_hdlr_cnt, g_u4Volt);

	MET_TRACE_PUTBUF(SOB, EOB);
}

static void free_alloc_memory(void)
{
	if (cpu_rg_node)
		kfree(cpu_rg_node);

	if (met_use_cpu_regulator)
		kfree(met_use_cpu_regulator);

	if (gpu_rg_node)
		kfree(gpu_rg_node);

	if (met_use_gpu_regulator)
		kfree(met_use_gpu_regulator);

	if (g_u4Volt)
		kfree(g_u4Volt);
}

static int init_cg_regulator_setting(void)
{
	int ret = 0;

	avail_cpu_hdlr_cnt = 0;
	avail_gpu_hdlr_cnt = 0;

	if (!cpu_rg_node) {
		cpu_rg_node = kzalloc(sizeof(CPU_RG_NODE), GFP_KERNEL);
		if (cpu_rg_node == NULL) {
			ret = -1;
			goto error;
		}

		strlcpy(cpu_rg_node, CPU_RG_NODE, sizeof(CPU_RG_NODE));
	}

	if (!met_use_cpu_regulator) {
		met_use_cpu_regulator = kzalloc(sizeof(struct regulator_hdlr) * 2, GFP_KERNEL);
		if (met_use_cpu_regulator == NULL) {
			ret = -1;
			goto error;
		}

		strlcpy(met_use_cpu_regulator[0].name, CPU_REGULATOR_1, sizeof(met_use_cpu_regulator[0].name));
		strlcpy(met_use_cpu_regulator[1].name, CPU_REGULATOR_2, sizeof(met_use_cpu_regulator[1].name));

		met_use_cpu_rg_cnt = 2;
	}

	if (!gpu_rg_node) {
		gpu_rg_node = kzalloc(sizeof(GPU_RG_NODE), GFP_KERNEL);
		if (gpu_rg_node == NULL) {
			ret = -1;
			goto error;
		}

		strlcpy(gpu_rg_node, GPU_RG_NODE, sizeof(GPU_RG_NODE));
	}

	if (!met_use_gpu_regulator) {
		met_use_gpu_regulator = kzalloc(sizeof(struct regulator_hdlr) * 1, GFP_KERNEL);
		if (met_use_gpu_regulator == NULL) {
			ret = -1;
			goto error;
		}

		strlcpy(met_use_gpu_regulator[0].name, GPU_REGULATOR_1, sizeof(met_use_gpu_regulator[0].name));

		met_use_gpu_rg_cnt = 1;
	}

	return 0;

error:
	free_alloc_memory();

	return ret;
}

static int get_cg_regulator_hdlr(void)
{
	int i, ret = 0, avail_hdlr = 0;
	struct device_node *node = NULL, *met_node;
	struct regulator *hdlr;

	met_node = met_device.this_device->of_node;

	ret = init_cg_regulator_setting();
	if (ret < 0) {
		PR_BOOTMSG("init cg regulator setting fail ...\n");
		goto error;
	}

	/* get cpu regulator hdlr */
	node = of_find_compatible_node(NULL, NULL, cpu_rg_node);
	met_device.this_device->of_node = node;

	for (i = 0; i < met_use_cpu_rg_cnt; i++) {
		hdlr = devm_regulator_get_optional(met_device.this_device,
				met_use_cpu_regulator[i].name);
		if (IS_ERR(hdlr)) {
			met_use_cpu_regulator[i].hdlr = NULL;
			PR_BOOTMSG("Failed to get %s regulator hdlr (%ld)\n",
					met_use_cpu_regulator[i].name, PTR_ERR(hdlr));
		} else {
			met_use_cpu_regulator[i].hdlr = hdlr;
			avail_hdlr++;
			avail_cpu_hdlr_cnt++;
			PR_BOOTMSG("get %s regulator hdlr (%p)\n",
					met_use_cpu_regulator[i].name, hdlr);
		}
	}

	/* get gpu regulator hdlr */
	node = of_find_compatible_node(NULL, NULL, gpu_rg_node);
	met_device.this_device->of_node = node;

	for (i = 0; i < met_use_gpu_rg_cnt; i++) {
		hdlr = devm_regulator_get_optional(met_device.this_device,
				met_use_gpu_regulator[i].name);
		if (IS_ERR(hdlr)) {
			met_use_gpu_regulator[i].hdlr = NULL;
			PR_BOOTMSG("Failed to get %s regulator hdlr (%ld)\n",
					met_use_gpu_regulator[i].name, PTR_ERR(hdlr));
		} else {
			met_use_gpu_regulator[i].hdlr = hdlr;
			avail_hdlr++;
			avail_gpu_hdlr_cnt++;
			PR_BOOTMSG("get %s regulator hdlr (%p)\n",
					met_use_gpu_regulator[i].name, hdlr);
		}
	}

	met_device.this_device->of_node = met_node;

	if (!avail_hdlr) {
		ret = -1;
		PR_BOOTMSG("NO get any available regulator hdlr !!!\n");
	}

	if (g_u4Volt)
		kfree(g_u4Volt);

	g_u4Volt = kzalloc(sizeof(int) * (avail_cpu_hdlr_cnt + avail_gpu_hdlr_cnt), GFP_KERNEL);
	if (g_u4Volt == NULL) {
		ret = -1;
		PR_BOOTMSG("alloc g_u4Volt fail !!!\n");
	}

error:
	return ret;
}

static void put_cg_regulator_hdlr(void)
{
	int i;

	if (met_use_cpu_regulator)
		for (i = 0; i < met_use_cpu_rg_cnt; i++)
			met_use_cpu_regulator[i].hdlr = NULL;

	if (met_use_gpu_regulator)
		for (i = 0; i < met_use_gpu_rg_cnt; i++)
			met_use_gpu_regulator[i].hdlr = NULL;
}

/* create ptpod related kobj node */
#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(cpu_regulator); \
		KOBJ_ATTR_ITEM(gpu_regulator); \
		KOBJ_ATTR_ITEM(cpu_rg_dts); \
		KOBJ_ATTR_ITEM(gpu_rg_dts); \
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

	ret = init_cg_regulator_setting();
	if (ret < 0)
		PR_BOOTMSG("init cg regulator setting fail ...\n");

	return ret;
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

	put_cg_regulator_hdlr();
	free_alloc_memory();
}

static void update_volt_value(void)
{
	int i;

	for (i = 0; i < met_use_cpu_rg_cnt; i++) {
		if (met_use_cpu_regulator[i].hdlr)
			g_u4Volt[i] = regulator_get_voltage(met_use_cpu_regulator[i].hdlr) / 1000;
	}

	for (i = 0; i < met_use_gpu_rg_cnt; i++) {
		if (met_use_gpu_regulator[i].hdlr)
			g_u4Volt[avail_cpu_hdlr_cnt + i] = regulator_get_voltage(met_use_gpu_regulator[i].hdlr) / 1000;
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
	int ret;

	ret = get_cg_regulator_hdlr();
	if (ret < 0) {
		PR_BOOTMSG("get_cg_regulator_hdlr fail !!!\n");
		return;
	}

	/* use work queue to get voltage by regulator */
	met_ptpod.timed_polling = get_volt_notify;
	INIT_DELAYED_WORK(&get_volt_dwork, met_ptpod_polling_by_wq);

	update_volt_value();

	ms_ptpod();

	ptpod_started = 1;
}

/*
 * Called from "met-cmd --stop"
 */
static void ptpod_stop(void)
{
	if (ptpod_started == 1) {
		ptpod_started = 0;

		cancel_delayed_work_sync(&get_volt_dwork);

		update_volt_value();

		ms_ptpod();
	}
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
	int i;
	int str_len = 0;

	str_len += snprintf(buf + str_len, PAGE_SIZE - str_len,
				"met-info [000] 0.0: met_ptpod_header_v2: ");

	for (i = 0; i < met_use_cpu_rg_cnt; i++) {
		if (met_use_cpu_regulator[i].hdlr)
			str_len += snprintf(buf + str_len,
						PAGE_SIZE - str_len,
						"%s,",
						met_use_cpu_regulator[i].name);
	}

	for (i = 0; i < met_use_gpu_rg_cnt; i++) {
		if (met_use_gpu_regulator[i].hdlr)
			str_len += snprintf(buf + str_len,
						PAGE_SIZE - str_len,
						"%s,",
						met_use_gpu_regulator[i].name);
	}

	buf[str_len-1] = '\n';

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
