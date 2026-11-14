// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/module.h>

#include "perf_common_ko.h"

#line __LINE__ "vendor/mediatek/kernel_modules/perf_common_cus/src/perf_common_main.c"

static void __exit perf_common_v_exit(void) {}

static int __init perf_common_v_init(void)
{
	init_perf_common_hook();
	pr_info("%s %d: perf_common hook done", __func__, __LINE__);
	return 0;
}

module_init(perf_common_v_init);
module_exit(perf_common_v_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek perf_common_v");
MODULE_AUTHOR("MediaTek Inc.");