// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/fs.h>
#include <linux/ctype.h>

#include <mtk_spm.h>
#include "met_drv.h"
#include "trace.h"
#include "core_plf_init.h"
#include "interface.h"
#include "met_spmtwam.h"

/* #define SPM_TWAM_DEBUG */

#define INFRA_FMEM_DCM_BASE 0x1020E000
#define INFRA_FMEM_DCM_SIZE 0x1000
#define FMEM_MUX_ADDR_OFFSET 0x200
#define FMEM_MUX_VALUE 0x780
#define TWAM_DBG_SIG_BASE 0x0D0A0000
#define TWAM_DBG_SIG_SIZE 0x100
#define TWAM_DBG_SIG_OFFSET 0x94


struct metdevice met_spmtwam;
static struct kobject *kobj_spmtwam;
/* static void __iomem *fmem_dcm_base; */
static void __iomem *twam_dbg_signal_base;
static struct met_spmtwam_para spmtwam_para[MAX_EVENT_COUNT];

#ifdef SPM_TWAM_DEBUG
static unsigned int debug_signal_val;
#endif

static struct twam_sig twamsig;
static struct twam_sig montype;			/* b'00: rising, b'01: falling, b'10: high, b'11: low */
static struct twam_sig dbgout;
static unsigned int used_count;
static int start;
static bool twam_clock_mode = TWAM_SPEED_MODE;		/* true:speed mode, false:normal mode */
static unsigned int window_len = 1300000;	/* 50 ms in 26 MHz */
static unsigned int idle_sel;

#define MONTYPE_SHOW_IMPLEMENT(num) \
	do { \
		int i; \
		i = snprintf(buf, PAGE_SIZE, "%d\n", montype.sig ## num); \
		return i; \
	} while (0)

#define MONTYPE_STORE_IMPLEMENT(num) \
	do { \
		int value; \
		if ((n == 0) || (buf == NULL)) \
			return -EINVAL; \
		if (kstrtoint(buf, 10, &value) != 0) \
			return -EINVAL; \
		if (value < 0 || value > 3) \
			return -EINVAL; \
		montype.sig ## num= value; \
		return n; \
	} while (0)

#define DBGOUT_SHOW_IMPLEMENT(num) \
	do { \
		int i; \
		i = snprintf(buf, PAGE_SIZE, "%d\n", dbgout.sig ## num); \
		if (i < 0) \
			return -1; \
		return i; \
	} while (0)

#define DBGOUT_STORE_IMPLEMENT(num) \
	do { \
		int value; \
		if ((n == 0) || (buf == NULL)) \
			return -EINVAL; \
		if (kstrtoint(buf, 10, &value) != 0) \
			return -EINVAL; \
		if (value < 0 || value > 127) \
			return -EINVAL; \
		dbgout.sig ## num = value; \
		return n; \
	} while (0)


static ssize_t montype0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(0);
}

static ssize_t montype0_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(0);
}

static ssize_t montype1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(1);
}

static ssize_t montype1_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(1);
}

static ssize_t montype2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(2);
}

static ssize_t montype2_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(2);
}

static ssize_t montype3_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(3);
}

static ssize_t montype3_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(3);
}

static ssize_t window_len_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;

	i = snprintf(buf, PAGE_SIZE, "%d\n", window_len);

	return i;
}

static ssize_t window_len_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 10, &value) != 0)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	window_len = value;

	return n;
}

static ssize_t dbgout0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	DBGOUT_SHOW_IMPLEMENT(0);
}

static ssize_t dbgout0_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	DBGOUT_STORE_IMPLEMENT(0);
}

static ssize_t dbgout1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	DBGOUT_SHOW_IMPLEMENT(1);
}

static ssize_t dbgout1_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	DBGOUT_STORE_IMPLEMENT(1);
}

static ssize_t dbgout2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	DBGOUT_SHOW_IMPLEMENT(2);
}

static ssize_t dbgout2_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	DBGOUT_STORE_IMPLEMENT(2);
}

static ssize_t dbgout3_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	DBGOUT_SHOW_IMPLEMENT(3);
}

static ssize_t dbgout3_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	DBGOUT_STORE_IMPLEMENT(3);
}

#ifdef SPM_TWAM_DEBUG
/* extern void *mt_spm_base_get(void); */
static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = snprintf(buf, PAGE_SIZE, "0x%x\n", debug_signal_val);
	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d, %d, %d, %d\n",
			twam_sig_size[0], twam_sig_size[1], twam_sig_size[2], twam_sig_size[3]);
/*	ret += snprintf(buf+ret, PAGE_SIZE-ret, "spm_base_addr: %p\n", mt_spm_base_get()); */

	return ret;
}
static struct kobj_attribute debug_attr = __ATTR_RO(debug);
#endif

static struct kobj_attribute montype0_attr = __ATTR(montype0, 0664, montype0_show, montype0_store);
static struct kobj_attribute montype1_attr = __ATTR(montype1, 0664, montype1_show, montype1_store);
static struct kobj_attribute montype2_attr = __ATTR(montype2, 0664, montype2_show, montype2_store);
static struct kobj_attribute montype3_attr = __ATTR(montype3, 0664, montype3_show, montype3_store);
static struct kobj_attribute window_len_attr = __ATTR(window_len, 0664, window_len_show, window_len_store);
static struct kobj_attribute dbgout0_attr = __ATTR(dbgout0, 0664, dbgout0_show, dbgout0_store);
static struct kobj_attribute dbgout1_attr = __ATTR(dbgout1, 0664, dbgout1_show, dbgout1_store);
static struct kobj_attribute dbgout2_attr = __ATTR(dbgout2, 0664, dbgout2_show, dbgout2_store);
static struct kobj_attribute dbgout3_attr = __ATTR(dbgout3, 0664, dbgout3_show, dbgout3_store);


/* create spmtwam related kobj node */
#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(montype0); \
		KOBJ_ATTR_ITEM(montype1); \
		KOBJ_ATTR_ITEM(montype2); \
		KOBJ_ATTR_ITEM(montype3); \
		KOBJ_ATTR_ITEM(window_len); \
		KOBJ_ATTR_ITEM(dbgout0); \
		KOBJ_ATTR_ITEM(dbgout1); \
		KOBJ_ATTR_ITEM(dbgout2); \
		KOBJ_ATTR_ITEM(dbgout3); \
	} while (0)

static int met_spmtwam_create(struct kobject *parent)
{
	int ret = 0;

	kobj_spmtwam = parent;

#define KOBJ_ATTR_ITEM(attr_name) \
	do { \
		ret = sysfs_create_file(kobj_spmtwam, &attr_name ## _attr.attr); \
		if (ret != 0) { \
			pr_notice("Failed to create " #attr_name " in sysfs\n"); \
			return ret; \
		} \
	} while (0)
	KOBJ_ATTR_LIST;
#undef  KOBJ_ATTR_ITEM

#ifdef SPM_TWAM_DEBUG
	ret = sysfs_create_file(kobj_spmtwam, &debug_attr.attr);
	if (ret != 0) {
		pr_debug("Failed to create debug in sysfs\n");
		return ret;
	}
#endif

	/* init. */
	montype.sig0 = 0x2;
	montype.sig1 = 0x2;
	montype.sig2 = 0x2;
	montype.sig3 = 0x2;

#if 0
	dbgout.sig0 = 87;
	dbgout.sig1 = 89;
	dbgout.sig2 = 91;
	dbgout.sig3 = 106;
#endif

	return ret;
}

static void met_spmtwam_delete(void)
{
#define KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_spmtwam, &attr_name ## _attr.attr)

	if (kobj_spmtwam != NULL) {
		KOBJ_ATTR_LIST;
		kobj_spmtwam = NULL;
	}
#undef  KOBJ_ATTR_ITEM

#ifdef SPM_TWAM_DEBUG
	sysfs_remove_file(kobj_spmtwam, &debug_attr.attr);
#endif
}

void ms_spmtwam(struct twam_sig *ts)
{
	switch (used_count) {
	case 1:
		MET_TRACE(MP_FMT1,
			(ts->sig0));
		break;
	case 2:
		MET_TRACE(MP_FMT2,
			(ts->sig0),
			(ts->sig1));
		break;
	case 3:
		MET_TRACE(MP_FMT3,
			(ts->sig0),
			(ts->sig1),
			(ts->sig2));
		break;
	case 4:
		MET_TRACE(MP_FMT4,
			(ts->sig0),
			(ts->sig1),
			(ts->sig2),
			(ts->sig3));
		break;
	default:
		MET_SPMTWAM_ERR("No assign profile event\n");
		break;
	}
}

static int reset_driver_stat(void)
{
	met_spmtwam.mode = 0;
	used_count = 0;
	start = 0;
	return 0;
}

void spm_twam_enable_debug_out(struct twam_sig *sig, void *addr)
{
	int value = 0;

	value |= ((1 << 31) | (sig->sig3 << 24) | (sig->sig2 << 16) | (sig->sig1 << 8) | (sig->sig0 << 0));

#ifdef SPM_TWAM_DEBUG
	debug_signal_val = value;
#endif

	writel(value, addr);
}

void spm_twam_disable_debug_out(void *addr)
{
	unsigned int value = 0;

	value = readl(addr);
	value &= (((unsigned int)(1 << 31)) - 1);

#ifdef SPM_TWAM_DEBUG
	debug_signal_val = value;
#endif

	writel(value, addr);
}

/*
 * Called from "met-cmd --start"
 */
static void spmtwam_start(void)
{
	if (idle_sel == 3) {
#if 0
		/* swithc idle signal D id0 pimux to fmem */
		if (fmem_dcm_base == NULL) {
			fmem_dcm_base = ioremap_nocache(INFRA_FMEM_DCM_BASE, INFRA_FMEM_DCM_SIZE);
			if (!fmem_dcm_base) {
				pr_debug("fmem_dcm_base ioremap fail...");
				return;
			}

			writel(FMEM_MUX_VALUE, (fmem_dcm_base + FMEM_MUX_ADDR_OFFSET));
		}
#endif
	} else if (idle_sel == 2) {
		/* debug signal mapping */
		if (twam_dbg_signal_base == NULL) {
			twam_dbg_signal_base = ioremap_nocache(TWAM_DBG_SIG_BASE, TWAM_DBG_SIG_SIZE);
			if (!twam_dbg_signal_base) {
				pr_debug("twam_dbg_signal_base ioremap fail...");
				return;
			}
		}

		spm_twam_enable_debug_out(&dbgout, (twam_dbg_signal_base + TWAM_DBG_SIG_OFFSET));
	}

	if (spm_twam_set_mon_type_symbol)
		spm_twam_set_mon_type_symbol(&montype);
	else {
		MET_SPMTWAM_ERR("spm_twam_set_mon_type_symbol is NULL\n");
		return;
	}

	if (spm_twam_set_window_length_symbol)
		spm_twam_set_window_length_symbol(window_len);
	else {
		MET_SPMTWAM_ERR("spm_twam_set_window_length_symbol is NULL\n");
		return;
	}

	if (spm_twam_set_idle_select_symbol)
		spm_twam_set_idle_select_symbol(idle_sel);
	else {
		MET_SPMTWAM_ERR("spm_twam_set_idle_select_symbol is NULL\n");
		return;
	}

	if (spm_twam_register_handler_symbol)
		spm_twam_register_handler_symbol(ms_spmtwam);
	else {
		MET_SPMTWAM_ERR("spm_twam_register_handler_symbol is NULL\n");
		return;
	}

	if (spm_twam_enable_monitor_symbol)
		spm_twam_enable_monitor_symbol(&twamsig, twam_clock_mode);
	else {
		MET_SPMTWAM_ERR("spm_twam_enable_monitor_symbol is NULL\n");
		return;
	}

	start = 1;
}

/*
 * Called from "met-cmd --stop"
 */
static void spmtwam_stop(void)
{
	if (idle_sel == 3) {
#if 0
		if (fmem_dcm_base)
			iounmap(fmem_dcm_base);
#endif
	} else if (idle_sel == 2) {
		spm_twam_disable_debug_out(twam_dbg_signal_base + TWAM_DBG_SIG_OFFSET);

		if (twam_dbg_signal_base) {
			iounmap(twam_dbg_signal_base);
			twam_dbg_signal_base = NULL;
		}
	}

	if (spm_twam_register_handler_symbol)
		spm_twam_register_handler_symbol(NULL);
	else {
		MET_SPMTWAM_ERR("spm_twam_register_handler_symbol is NULL\n");
		return;
	}

	if (spm_twam_disable_monitor_symbol)
		spm_twam_disable_monitor_symbol();
	else {
		MET_SPMTWAM_ERR("spm_twam_disable_monitor_symbol is NULL\n");
		return;
	}
}

static const char header[] = "met-info [000] 0.0: ms_spmtwam_header: ";

/*
 * It will be called back when run "met-cmd --extract" and mode is 1
 */
static int spmtwam_print_header(char *buf, int len)
{
    int i, total_size;
	char idle_sig;
    unsigned int event;

    total_size = snprintf(buf, PAGE_SIZE, header);

    for (i = 0; i < used_count; i++) {
		idle_sig = spmtwam_para[i].idle_sig;
		event = spmtwam_para[i].event;

		total_size += snprintf(buf + total_size, PAGE_SIZE - total_size,
                        "signal_%c_%02u,", idle_sig, event);
    }

	/* cut the last comma */
	buf[total_size - 1] = '\n';

    total_size += snprintf(buf + total_size, PAGE_SIZE - total_size, "met-info [000] 0.0: spmtwam_clock_mode: %s\n",
            twam_clock_mode == TWAM_SPEED_MODE ? "speed" : "normal");

#ifdef SPMTWAM_SINGLE_IDLE_SIGNAL
    total_size += snprintf(buf + total_size, PAGE_SIZE - total_size, "met-info [000] 0.0: spmtwam_idle_signal_support: %s\n",
					TWAM_SINGLE_IDLE_SIGNAL);
#endif

#ifdef SPMTWAM_MULTIPLE_IDLE_SIGNAL
    total_size += snprintf(buf + total_size, PAGE_SIZE - total_size, "met-info [000] 0.0: spmtwam_idle_signal_support: %s\n",
					TWAM_MULTIPLE_IDLE_SIGNAL);
#endif

    reset_driver_stat();
    return total_size;
}

static int assign_slot(char idle_sig, unsigned int event)
{
	int i;
	int sig2int;

	if (used_count == MAX_EVENT_COUNT) {
		PR_BOOTMSG("%s exceed max used event count\n", MET_SPMTWAM_TAG);
		return -1;
	}

	/* check duplicated */
	for (i = 0; i < used_count; i++) {
		if ((spmtwam_para[i].idle_sig == idle_sig) &&
			(spmtwam_para[i].event == event)) {
			PR_BOOTMSG("%s input duplicated event %u\n", MET_SPMTWAM_TAG, event);
			return -2;
		}
	}

	/* check idle_sig range in a~d or A~D */
	if (tolower(idle_sig) < 'a' || tolower(idle_sig) > 'd') {
		PR_BOOTMSG("%s input idle signal %c is not in a~d range\n",
					MET_SPMTWAM_TAG, idle_sig);
		return -3;
	}

	/* check event no */
	if (event > MAX_TWAM_EVENT_COUNT) {
		PR_BOOTMSG("%s input event %u exceed max twam event %u\n",
					MET_SPMTWAM_TAG, event, MAX_TWAM_EVENT_COUNT);
		return -4;
	}

#ifdef SPMTWAM_SINGLE_IDLE_SIGNAL
	if (used_count > 0) {
		for (i = 0; i < used_count; i++) {
			if (idle_sig != spmtwam_para[i].idle_sig) {
				PR_BOOTMSG("%s %c idle signal is defferent previous, only support one idle signal\n",
							MET_SPMTWAM_TAG, idle_sig);
				return -5;
			}
		}
	}
#endif

	spmtwam_para[used_count].idle_sig = idle_sig;
	spmtwam_para[used_count].event = event;

	sig2int = (int) (tolower(idle_sig) - 'a');
#ifdef SPMTWAM_SINGLE_IDLE_SIGNAL
	idle_sel = sig2int;
#endif

	switch (used_count) {
	case 0:
		twamsig.sig0 = event;
		break;
	case 1:
		twamsig.sig1 = event;
		break;
	case 2:
		twamsig.sig2 = event;
		break;
	case 3:
		twamsig.sig3 = event;
		break;
	}

	used_count++;

	return 0;
}

static char help[] =
	"  --spmtwam=clock:[speed|normal]	default is normal\n"
	"					normal mode monitors 4 channels\n"
	"					speed mode monitors 4 channels\n"
	"  --spmtwam=signal:selx\n"
	"					signal= a ~ d for idle signal A~D select\n"
	"					selx= 0 ~ 31 for for channel event\n";

/*
 * Called from "met-cmd --help"
 */
static int spmtwam_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int spmtwam_process_argument(const char *arg, int len)
{
	if (start == 1)
		reset_driver_stat();

	if (strncmp(arg, "clock:", 6) == 0) {
		if (strncmp(&(arg[6]), "speed", 5) == 0) {
			twam_clock_mode = TWAM_SPEED_MODE;
		} else if (strncmp(&(arg[6]), "normal", 6) == 0) {
			twam_clock_mode = TWAM_NORMAL_MODE;
		} else {
			PR_BOOTMSG("%s unknown clock mode\n", MET_SPMTWAM_TAG);

			goto error;
		}
	} else {
		char signal;
		int event;
		int ret;

		if (len < 3) {
			PR_BOOTMSG("%s input parameter is too short !!!\n", MET_SPMTWAM_TAG);
			goto error;
		}

		/*
		 * parse arguments
		 */
		ret = sscanf(arg, "%c:%u", &signal, &event);
		if (ret < 2) {
			PR_BOOTMSG("%s input parameter is wrong format !!!\n", MET_SPMTWAM_TAG);
			goto error;
		}

		if (assign_slot(signal, event) < 0) {
			goto error;
		}
	}

	met_spmtwam.mode = 1;
	return 0;

error:
	reset_driver_stat();
	return -1;
}

struct metdevice met_spmtwam = {
	.name = "spmtwam",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.create_subfs = met_spmtwam_create,
	.delete_subfs = met_spmtwam_delete,
	.cpu_related = 0,
	.start = spmtwam_start,
	.stop = spmtwam_stop,
	.reset = reset_driver_stat,
	.print_help = spmtwam_print_help,
	.print_header = spmtwam_print_header,
	.process_argument = spmtwam_process_argument
};
EXPORT_SYMBOL(met_spmtwam);
