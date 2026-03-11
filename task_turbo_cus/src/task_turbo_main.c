// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/module.h>

#include "task_turbo_ko.h"

#line __LINE__ "vendor/mediatek/kernel_modules/task_turbo_int/src/task_turbo_main.c"

static void __exit task_turbo_v_exit(void) {}

static int __init task_turbo_v_init(void)
{
	init_task_turbo_hook();
	pr_info("%s %d: task turbo hook done", __func__, __LINE__);
	return 0;
}

module_init(task_turbo_v_init);
module_exit(task_turbo_v_exit);

MODULE_LICENSE("Proprietary");
MODULE_DESCRIPTION("MediaTek task_turbo_v");
MODULE_AUTHOR("MediaTek Inc.");