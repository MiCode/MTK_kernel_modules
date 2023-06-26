// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <dvfsrc-exp.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"

#include "core_plf_init.h"
#include "core_plf_trace.h"

/*======================================================================*/
/*	Global variable definitions					*/
/*======================================================================*/

/* Global variables */
struct metdevice met_vcoredvfs;
int polling_mode = 1;

/* external symbols */
#define DEFAULT_INFO_NUM 3
#define DEFAULT_SRC_NUM 1

char *default_info_name[DEFAULT_INFO_NUM] = {
	"OPP",
	"VOLT",
	"FREQ"
};

char *default_src_name[DEFAULT_SRC_NUM] = {
	"MD2SPM"
};

unsigned int opp_info[DEFAULT_INFO_NUM];
unsigned int src_req[DEFAULT_SRC_NUM];

static int met_vcorefs_get_num_opp(void)
{
	if (vcorefs_get_num_opp_symbol)
		return vcorefs_get_num_opp_symbol();
	else
		return 0;
}

static int met_vcorefs_get_opp_info_num(void)
{
	if (vcorefs_get_opp_info_num_symbol)
		return vcorefs_get_opp_info_num_symbol();
	else
		return DEFAULT_INFO_NUM;
}


static int met_vcorefs_get_src_req_num(void)
{
	if (vcorefs_get_src_req_num_symbol)
		return vcorefs_get_src_req_num_symbol();
	else
		return DEFAULT_SRC_NUM;
}


static char **met_vcorefs_get_opp_info_name(void)
{
	if (vcorefs_get_opp_info_name_symbol)
		return vcorefs_get_opp_info_name_symbol();
	else
		return default_info_name;
}

static char **met_vcorefs_get_src_req_name(void)
{
	if (vcorefs_get_src_req_name_symbol)
		return vcorefs_get_src_req_name_symbol();
	else
		return default_src_name;
}

static unsigned int *met_vcorefs_get_opp_info(void)
{
	if (vcorefs_get_opp_info_symbol)
		return vcorefs_get_opp_info_symbol();

	return opp_info;
}

static unsigned int *met_vcorefs_get_src_req(void)
{
	if (vcorefs_get_src_req_symbol)
		return vcorefs_get_src_req_symbol();

	return src_req;
}


/*======================================================================*/
/*	File Node definitions					*/
/*======================================================================*/

static struct kobject *kobj_met_vcoredvfs;

static ssize_t vcorefs_polling_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t vcorefs_polling_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n);
static struct kobj_attribute vcorefs_polling_attr =
__ATTR(vcorefs_polling, 0664, vcorefs_polling_show, vcorefs_polling_store);

/*======================================================================*/
/*	Utilities					*/
/*======================================================================*/

static inline int do_vcoredvfs(void)
{
	return met_vcoredvfs.mode;
}

/*======================================================================*/
/*	Data Output					*/
/*======================================================================*/

noinline void vcorefs(unsigned char cnt, unsigned int *value)
{
	char *SOB, *EOB;

	MET_TRACE_GETBUF(&SOB, &EOB);
	EOB = ms_formatH_EOL(EOB, cnt, value);
	MET_TRACE_PUTBUF(SOB, EOB);
}

noinline void vcorefs_kicker(unsigned char cnt, int *value)
{
	char *SOB, *EOB;

	MET_TRACE_GETBUF(&SOB, &EOB);
	EOB = ms_formatH_EOL(EOB, cnt, value);
	MET_TRACE_PUTBUF(SOB, EOB);
}

noinline void ms_vcorefs(unsigned char cnt, unsigned int *value)
{
	char *SOB, *EOB;

	MET_TRACE_GETBUF(&SOB, &EOB);
	EOB = ms_formatH_EOL(EOB, cnt, value);
	MET_TRACE_PUTBUF(SOB, EOB);
}

/*======================================================================*/
/*	Callback functions						*/
/*======================================================================*/
void vcoredvfs_irq(int opp)
{
	int num;
	unsigned int *out;

	num = met_vcorefs_get_opp_info_num();
	out = met_vcorefs_get_opp_info();

	vcorefs(num, out);
}

/*======================================================================*/
/*	File Node Operations						*/
/*======================================================================*/
static ssize_t vcorefs_polling_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", polling_mode);
}

static ssize_t vcorefs_polling_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	polling_mode = value;

	return n;
}

/*======================================================================*/
/*	MET Device Operations						*/
/*======================================================================*/
static int met_vcoredvfs_create(struct kobject *parent)
{
	int ret = 0;

	kobj_met_vcoredvfs = parent;

	ret = sysfs_create_file(kobj_met_vcoredvfs, &vcorefs_polling_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create vcoredvfs in sysfs\n");
		return ret;
	}

	return ret;
}

static void met_vcoredvfs_delete(void)
{
	sysfs_remove_file(kobj_met_vcoredvfs, &vcorefs_polling_attr.attr);
}

static void met_vcoredvfs_start(void)
{
	vcoredvfs_irq(-1);
}

static void met_vcoredvfs_stop(void)
{
	vcoredvfs_irq(-1);
}

static void met_vcoredvfs_polling(unsigned long long stamp, int cpu)
{
	int num;
	unsigned int *out;

	if (!do_vcoredvfs())
		return;

	/* vcorefs opp information */
	if (polling_mode)
		vcoredvfs_irq(-1);

	/* vcorefs source request */
	num = met_vcorefs_get_src_req_num();
	out = met_vcorefs_get_src_req();

	ms_vcorefs(num, out);
}

static const char help[] =
	"  --vcoredvfs				monitor VCORE DVFS\n";
static int vcoredvfs_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static const char header_dvfs[] = "met-info [000] 0.0: met_vcorefs_header:";

static const char header_ms_dvfs[] = "met-info [000] 0.0: ms_vcorefs_header:";

static int vcoredvfs_print_header(char *buf, int len)
{
	int ret = 0;
	int idx = 0;
	int num = 0;
	char **header;

	ret = snprintf(buf, PAGE_SIZE, "met-info [000] 0.0: met_vcorefs_cfg: NUM_OPP:%d\n", met_vcorefs_get_num_opp());

	/* opp information */
	num = met_vcorefs_get_opp_info_num();
	header = met_vcorefs_get_opp_info_name();

	ret += snprintf(buf + ret, PAGE_SIZE, header_dvfs);
	for (idx = 0; idx < num; idx++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", header[idx]);
	buf[strlen(buf)-1] = '\n';

	/* source requests */
	num = met_vcorefs_get_src_req_num();
	header = met_vcorefs_get_src_req_name();

	ret += snprintf(buf + ret, PAGE_SIZE - ret, header_ms_dvfs);
	for (idx = 0; idx < num; idx++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", header[idx]);
	buf[strlen(buf)-1] = '\n';

	met_vcoredvfs.mode = 0;

	return ret;
}

struct metdevice met_vcoredvfs = {
	.name = "vcoredvfs",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_vcoredvfs_create,
	.delete_subfs = met_vcoredvfs_delete,
	.cpu_related = 0,
	.start = met_vcoredvfs_start,
	.stop = met_vcoredvfs_stop,
	.polling_interval = 1,	/* ms */
	.timed_polling = met_vcoredvfs_polling,
	.print_help = vcoredvfs_print_help,
	.print_header = vcoredvfs_print_header,
};
