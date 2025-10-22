// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_GPU

extern long met_gpu_adv_api_ready;

#include <mtk_gpu_utility.h>

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_gpu_adv_api/met_gpu_adv_api.h"

#endif /* MET_GPU */

static int __init met_api_init(void)
{
#ifdef MET_GPU

    met_gpu_adv_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_gpu_adv_api/met_gpu_adv_api.h"

#endif /* MET_GPU */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_GPU

    met_gpu_adv_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_gpu_adv_api/met_gpu_adv_api.h"

#endif /* MET_GPU */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");