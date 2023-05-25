// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/export.h>
#include "zlib.h"
#include "udc.h"

extern int register_udc_functions(unsigned int id, void *f);

static int __init udc_lib_mod_init(void)
{
	int ret = 0;

	pr_notice("udc lib init\n");
	register_udc_functions(ID_deflateInit2,deflateInit2_);
	register_udc_functions(ID_deflateSetDict,deflateSetDictionary);
	register_udc_functions(ID_deflateEnd,deflateEnd);
	register_udc_functions(ID_deflateReset,deflateReset);
	register_udc_functions(ID_deflate,deflate);
	register_udc_functions(ID_deflateBound,deflateBound);
	register_udc_functions(ID_udc_chksum,udcChecksum);
	register_udc_functions(ID_udc_QueryPara,udcQueryParam);
	register_udc_functions(ID_udc_GetCmpLen,udcGetCmpLen);

	return ret;
}

static void __exit udc_lib_mod_exit(void)
{
}

#if 0
void udc_pr_notice(int h0, int h1, int h2, int h3, int t0, int t1, int t2, int t3)
{
	pr_notice("start %02X %02X %02X %02X -- %02X %02X %02X %02X end\n",
		h0,h1,h2,h3,t0,t1,t2,t3);
}
#endif // 0

module_init(udc_lib_mod_init);
module_exit(udc_lib_mod_exit);

MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("UDC lib");
MODULE_LICENSE("GPL v2");
