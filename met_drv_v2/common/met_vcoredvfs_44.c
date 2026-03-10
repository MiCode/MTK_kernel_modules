// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <mtk_vcorefs_governor.h>
#include <mtk_spm_vcore_dvfs.h>

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

static char *met_governor_get_kicker_name(int id)
{
	if (governor_get_kicker_name_symbol)
		return governor_get_kicker_name_symbol(id);
	else
		return "KIR_UNKNOWN";
}

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

static int met_vcorefs_get_src_req_num(void)
{
	if (vcorefs_get_src_req_num_symbol)
		return vcorefs_get_src_req_num_symbol();
	else
		return DEFAULT_SRC_NUM;
}

static unsigned int *met_vcorefs_get_opp_info(void)
{
	int count = 0;

	if (vcorefs_get_opp_info_symbol)
		return vcorefs_get_opp_info_symbol();

	if (vcorefs_get_hw_opp_symbol)
		opp_info[count++] = vcorefs_get_hw_opp_symbol();
	if (vcorefs_get_curr_vcore_symbol)
		opp_info[count++] = vcorefs_get_curr_vcore_symbol(); /* uV */
	if (vcorefs_get_curr_ddr_symbol)
		opp_info[count++] = vcorefs_get_curr_ddr_symbol(); /* kHz */

	return opp_info;
}

static unsigned int *met_vcorefs_get_src_req(void)
{
	if (vcorefs_get_src_req_symbol)
		return vcorefs_get_src_req_symbol();

	if (spm_vcorefs_get_MD_status_symbol)
		src_req[0] = spm_vcorefs_get_MD_status_symbol();
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
void sw_kicker_req(enum dvfs_kicker kicker, enum dvfs_opp opp)
{
	if (!do_vcoredvfs())
		return;

	if (kicker_table_symbol)
		vcorefs_kicker(NUM_KICKER, kicker_table_symbol);
}

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
	if (kicker_table_symbol)
		vcorefs_kicker(NUM_KICKER, kicker_table_symbol);

	vcoredvfs_irq(-1);

	if (!polling_mode && spm_vcorefs_register_handler_symbol)
		spm_vcorefs_register_handler_symbol(vcoredvfs_irq);

	if (vcorefs_register_req_notify_symbol)
		vcorefs_register_req_notify_symbol(sw_kicker_req);
	else
		MET_TRACE("vcorefs_register_req_notify not exist!\n");

	if (!polling_mode && vcorefs_enable_debug_isr_symbol)
		vcorefs_enable_debug_isr_symbol(true);
}

static void met_vcoredvfs_stop(void)
{
	if (!polling_mode && vcorefs_enable_debug_isr_symbol)
		vcorefs_enable_debug_isr_symbol(false);

	if (vcorefs_register_req_notify_symbol)
		vcorefs_register_req_notify_symbol(NULL);

	if (spm_vcorefs_register_handler_symbol)
		spm_vcorefs_register_handler_symbol(NULL);

	if (kicker_table_symbol)
		vcorefs_kicker(NUM_KICKER, kicker_table_symbol);
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

static const char header_kicker[] = "met-info [000] 0.0: met_vcorefs_kicker_header:";

static const char header_ms_dvfs[] = "met-info [000] 0.0: ms_vcorefs_header:";

static int vcoredvfs_print_header(char *buf, int len)
{
	int ret;
	int idx, num;
	char **header;

	ret = snprintf(buf, PAGE_SIZE, "met-info [000] 0.0: met_vcorefs_cfg: NUM_OPP:%d\n", met_vcorefs_get_num_opp());

	/* opp information */
	num = met_vcorefs_get_opp_info_num();
	header = met_vcorefs_get_opp_info_name();

	ret += snprintf(buf + ret, PAGE_SIZE, header_dvfs);
	for (idx = 0; idx < num; idx++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", header[idx]);
	buf[strlen(buf)-1] = '\n';

	/* sw kickers */
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s", header_kicker);
	for (idx = 0; idx < NUM_KICKER; idx++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", met_governor_get_kicker_name(idx));
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
