// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_MCUPM

extern long met_mcupm_api_ready;

#include "mcupm_driver.h"
#include "mcupm_ipi_id.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_mcupm_api/met_mcupm_api.h"

extern int met_mcupm_log_init(void);
extern int met_mcupm_log_uninit(void);
extern int met_ondiemet_attr_init_mcupm(void);
extern int met_ondiemet_attr_uninit_mcupm(void);

#endif /* MET_MCUPM */

static int __init met_api_init(void)
{
#ifdef MET_MCUPM

    met_mcupm_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_mcupm_api/met_mcupm_api.h"

    met_mcupm_log_init();
    met_ondiemet_attr_init_mcupm();

#endif /* MET_MCUPM */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_MCUPM

    met_mcupm_log_uninit();
    met_ondiemet_attr_uninit_mcupm();

    met_mcupm_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_mcupm_api/met_mcupm_api.h"

#endif /* MET_MCUPM */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");