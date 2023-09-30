// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_GPUEB

extern long met_gpueb_api_ready;

#include <linux/platform_device.h>  /* for struct platform_device used in gpueb_reserved_mem.h & gpueb_ipi.h */
#include "gpueb_reserved_mem.h"
#include "gpueb_ipi.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_gpueb_api/met_gpueb_api.h"

extern int met_gpueb_log_init(void);
extern int met_gpueb_log_uninit(void);
extern int met_ondiemet_attr_init_gpueb(void);
extern int met_ondiemet_attr_uninit_gpueb(void);

#endif /* MET_GPUEB */

static int __init met_api_init(void)
{
#ifdef MET_GPUEB

    met_gpueb_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_gpueb_api/met_gpueb_api.h"

    met_gpueb_log_init();
    met_ondiemet_attr_init_gpueb();

#endif /* MET_GPUEB */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_GPUEB

    met_gpueb_log_uninit();
    met_ondiemet_attr_uninit_gpueb();

    met_gpueb_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_gpueb_api/met_gpueb_api.h"

#endif /* MET_GPUEB */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");