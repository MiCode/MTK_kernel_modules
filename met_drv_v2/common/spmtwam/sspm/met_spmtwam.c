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


struct metdevice met_spmtwam;
static struct kobject *kobj_spmtwam;

static struct twam_cfg twam_config;
static struct met_spmtwam_para spmtwam_para[MAX_EVENT_COUNT];

static unsigned int twam_dbg_enable = TWAM_DEBUG_SIG_DISABLE;
static bool twam_clock_mode = TWAM_SPEED_MODE;		/* true:speed mode, false:normal mode */
static unsigned int window_len = 1300000;	/* 50 ms in 26 MHz */

static int used_count;
static int start;


#define MONTYPE_SHOW_IMPLEMENT(cfg) \
	do { \
		int i; \
		i = snprintf(buf, PAGE_SIZE, "%d\n", cfg.monitor_type); \
		return i; \
	} while (0)

#define MONTYPE_STORE_IMPLEMENT(cfg) \
	do { \
		int value; \
		if ((n == 0) || (buf == NULL)) \
			return -EINVAL; \
		if (kstrtoint(buf, 10, &value) != 0) \
			return -EINVAL; \
		if (value < 0 || value > 3) \
			return -EINVAL; \
		cfg.monitor_type = value; \
		return n; \
	} while (0)

static ssize_t montype0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(twam_config.byte[0]);
}

static ssize_t montype0_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(twam_config.byte[0]);
}

static ssize_t montype1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(twam_config.byte[1]);
}

static ssize_t montype1_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(twam_config.byte[1]);
}

static ssize_t montype2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(twam_config.byte[2]);
}

static ssize_t montype2_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(twam_config.byte[2]);
}

static ssize_t montype3_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	MONTYPE_SHOW_IMPLEMENT(twam_config.byte[3]);
}

static ssize_t montype3_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	MONTYPE_STORE_IMPLEMENT(twam_config.byte[3]);
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

static struct kobj_attribute montype0_attr = __ATTR(montype0, 0664, montype0_show, montype0_store);
static struct kobj_attribute montype1_attr = __ATTR(montype1, 0664, montype1_show, montype1_store);
static struct kobj_attribute montype2_attr = __ATTR(montype2, 0664, montype2_show, montype2_store);
static struct kobj_attribute montype3_attr = __ATTR(montype3, 0664, montype3_show, montype3_store);
static struct kobj_attribute window_len_attr = __ATTR(window_len, 0664, window_len_show, window_len_store);


/* create spmtwam related kobj node */
#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(montype0); \
		KOBJ_ATTR_ITEM(montype1); \
		KOBJ_ATTR_ITEM(montype2); \
		KOBJ_ATTR_ITEM(montype3); \
		KOBJ_ATTR_ITEM(window_len); \
	} while (0)

static int met_spmtwam_create(struct kobject *parent)
{
	int i;
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

	/* init. */
	for (i = 0; i < MAX_EVENT_COUNT; i++)
		twam_config.byte[i].monitor_type = 0x2;

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
}

void ms_spmtwam(struct twam_cfg *cfg, struct twam_select *twam_sel)
{
	switch (used_count) {
	case 1:
		MET_TRACE(MP_FMT1,
			(cfg->byte[0].id));
		break;
	case 2:
		MET_TRACE(MP_FMT2,
			(cfg->byte[0].id),
			(cfg->byte[1].id));
		break;
	case 3:
		MET_TRACE(MP_FMT3,
			(cfg->byte[0].id),
			(cfg->byte[1].id),
			(cfg->byte[2].id));
		break;
	case 4:
		MET_TRACE(MP_FMT4,
			(cfg->byte[0].id),
			(cfg->byte[1].id),
			(cfg->byte[2].id),
			(cfg->byte[3].id));
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

#if 0
void sspm_twam_debug(void)
{
/*
	PR_BOOTMSG("[MET_TWAM] byte0 idle=%d, event=%d, type=%d \n",twam_config.byte[0].signal,twam_config.byte[0].id,twam_config.byte[0].monitor_type);
	PR_BOOTMSG("[MET_TWAM] byte1 idle=%d, event=%d, type=%d \n",twam_config.byte[1].signal,twam_config.byte[1].id,twam_config.byte[1].monitor_type);
	PR_BOOTMSG("[MET_TWAM] byte2 idle=%d, event=%d, type=%d \n",twam_config.byte[2].signal,twam_config.byte[2].id,twam_config.byte[2].monitor_type);
	PR_BOOTMSG("[MET_TWAM] byte3 idle=%d, event=%d, type=%d \n",twam_config.byte[3].signal,twam_config.byte[3].id,twam_config.byte[3].monitor_type);
	PR_BOOTMSG("[MET_TWAM] twam_clock_mode=%d, window_len=%d \n",twam_clock_mode, window_len);
*/
}
#endif

/*
 * Called from "met-cmd --start"
 */
static void spmtwam_start(void)
{
	if (spm_twam_met_enable_symbol) {
		if (true == spm_twam_met_enable_symbol()) {
			if (spm_twam_enable_monitor_symbol)
				spm_twam_enable_monitor_symbol(TWAM_DISABLE, TWAM_DEBUG_SIG_DISABLE, NULL);
			else {
				MET_SPMTWAM_ERR("spm_twam_enable_monitor_symbol is NULL\n");
				return;
			}
		}
	} else {
		MET_SPMTWAM_ERR("spm_twam_met_enable_symbol is NULL\n");
		return;
	}

	if (spm_twam_config_channel_symbol)
		spm_twam_config_channel_symbol(&twam_config, twam_clock_mode, window_len);
	else {
		MET_SPMTWAM_ERR("spm_twam_config_channel_symbol is NULL\n");
		return;
	}

	if (spm_twam_enable_monitor_symbol) {
		spm_twam_enable_monitor_symbol(TWAM_ENABLE, twam_dbg_enable, ms_spmtwam);
		/* sspm_twam_debug(); */
	} else {
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
	if (spm_twam_enable_monitor_symbol)
		spm_twam_enable_monitor_symbol(TWAM_DISABLE, TWAM_DEBUG_SIG_DISABLE, NULL);
	else {
		MET_SPMTWAM_ERR("spm_twam_enable_monitor_symbol is NULL\n");
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

static int assign_slot_sspm_twam(char idle_sig, unsigned int event)
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

	twam_config.byte[used_count].id = event;
	twam_config.byte[used_count].signal = sig2int;

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

		if (assign_slot_sspm_twam(signal, event) < 0) {
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
