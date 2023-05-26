// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/dcache.h>
#include <linux/types.h>
#include <linux/dcookies.h>
#include <linux/sched.h>
#include <linux/fs.h>

static int __init met_drv_init(void)
{
	pr_info("Hello MET default module\n");
	return 0;
}

static void __exit met_drv_exit(void)
{
}
module_init(met_drv_init);
module_exit(met_drv_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_DEFAULT");
MODULE_LICENSE("GPL");
