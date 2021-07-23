// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"

#include "mtk_typedefs.h"
#include "core_plf_init.h"
#include "mtk_emi_bm.h"
#include "interface.h"
#include "met_dramc.h"


/*======================================================================*/
/*	MET Device Operations						*/
/*======================================================================*/

static int met_emi_create(struct kobject *parent)
{
	int ret = 0;

	ret = met_emi_create_basic(parent, &met_sspm_emi);
	return ret;
}

static void met_emi_delete(void)
{
	met_emi_delete_basic();
}

static void met_emi_resume(void)
{
	met_emi_resume_basic();
}

static int emi_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, emi_help_msg);
}

static int emi_print_header(char *buf, int len)
{
	len = emi_print_header_basic(buf,len);
	return len;
}

static int ondiemet_emi_print_header(char *buf, int len)
{
	return emi_print_header(buf, len);
}

static void ondiemet_emi_start(void)
{
	ondiemet_emi_start_basic();
}

static void ondiemet_emi_stop(void)
{
	ondiemet_emi_stop_basic();
}


struct metdevice met_sspm_emi = {
	.name			= "emi",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.create_subfs		= met_emi_create,
	.delete_subfs		= met_emi_delete,
	.resume			= met_emi_resume,
	.print_header		= emi_print_header,
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	.ondiemet_start		= ondiemet_emi_start,
	.ondiemet_stop		= ondiemet_emi_stop,
	.ondiemet_print_help	= emi_print_help,
	.ondiemet_print_header	= ondiemet_emi_print_header,
#endif
#endif
	.ondiemet_mode		= 1,
};
EXPORT_SYMBOL(met_sspm_emi);
