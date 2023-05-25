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
/* #include "plf_init.h" */
#include "sspm/ondiemet_sspm.h"

static void sspm_walltime_start(void)
{
	ondiemet_module[ONDIEMET_SSPM] |= ID_WALL_TIME;
}

static void sspm_walltime_stop(void)
{
}

static const char sspm_walltime_header[] = "met-info [000] 0.0: sspm WCLK: freq\n";

static int sspm_walltime_print_header(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, sspm_walltime_header);
}

struct metdevice met_sspm_walltime = {
	.name = "sspm_wall_time",
	.owner = THIS_MODULE,
	.type = MET_TYPE_BUS,
	.cpu_related = 0,
	.ondiemet_mode = 0,
	.print_header = sspm_walltime_print_header,
	.ondiemet_start = sspm_walltime_start,
	.ondiemet_stop = sspm_walltime_stop,
	.ondiemet_print_header = sspm_walltime_print_header,
};
EXPORT_SYMBOL(met_sspm_walltime);
