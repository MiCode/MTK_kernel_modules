/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <linux/fs.h>

struct tag_chipid {
	u32 size;
	u32 hw_code;
	u32 hw_subcode;
	u32 hw_ver;
	u32 sw_ver;
};
extern unsigned int met_get_chipid_from_atag(void);

#ifdef MET_USER_EVENT_SUPPORT
extern int tag_reg(struct file_operations *const fops, struct kobject *kobj);
extern int tag_unreg(void);
#include "met_drv.h"
#include "met_tag.h"
extern struct bltable_t bltab;
#endif

extern struct metdevice met_stat;
extern struct metdevice met_cpupmu;
extern struct metdevice met_cookie;
extern struct metdevice met_memstat;
extern struct metdevice met_switch;
extern struct metdevice met_trace_event;
extern struct metdevice met_dummy_header;
extern struct metdevice met_backlight;
extern struct metdevice met_mcupm;

/* This variable will decide which method to access the CPU PMU counter */
/*     0: access registers directly */
/*     others: via Linux perf driver */
extern unsigned int met_cpu_pmu_method;

/*
 * controls whether re-configuring pmu events after leaving cpu off state
 */
extern unsigned int met_cpu_pm_pmu_reconfig;

extern int met_parse_num(const char *str, unsigned int *value, int len);
extern void met_set_suspend_notify(int flag);

#define	PR_CPU_NOTIFY
#if	defined(PR_CPU_NOTIFY)
extern int met_cpu_notify;
#endif

//#undef	MET_BOOT_MSG
#define	MET_BOOT_MSG
#if	defined(MET_BOOT_MSG)
extern char met_boot_msg_tmp[256];
extern int pr_bootmsg(int str_len, char *str);
#define	PR_BOOTMSG(fmt, args...) { \
	int str_len = snprintf(met_boot_msg_tmp, sizeof(met_boot_msg_tmp), \
			       fmt, ##args); \
	pr_bootmsg(str_len, met_boot_msg_tmp); }
#define	PR_BOOTMSG_ONCE(fmt, args...) { \
	static int once; \
	if (!once) { \
		int str_len = snprintf(met_boot_msg_tmp, \
				       sizeof(met_boot_msg_tmp), \
				       fmt, ##args); \
		pr_bootmsg(str_len, met_boot_msg_tmp); \
		once = 1; \
	} }
#else
#define	pr_bootmsg(str_len, str)
#define PR_BOOTMSG(fmt, args...)
#define	PR_BOOTMSG_ONCE(fmt, args...)
#endif

#endif	/* __INTERFACE_H__ */
