// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_EMI

#include "met_drv.h"

extern long met_emi_api_ready;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_emi_api/met_emi_api.h"

int met_register(struct metdevice *met);
int met_deregister(struct metdevice *met);
extern struct metdevice met_sspm_emi;
#ifdef MET_PLF_EXIST
extern struct metdevice met_emi;
#endif

#endif /* MET_EMI */

static int __init met_api_init(void)
{
#ifdef MET_EMI
    int met_sspm_emi_ret;
#ifdef MET_PLF_EXIST
    int met_emi_ret;
#endif

    met_sspm_emi_ret = met_deregister(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    met_emi_ret = met_deregister(&met_emi);
#endif

    met_emi_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_emi_api/met_emi_api.h"

    if (!met_sspm_emi_ret) met_register(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    if (!met_emi_ret) met_register(&met_emi);
#endif

#endif /* MET_EMI */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_EMI
    int met_sspm_emi_ret;
#ifdef MET_PLF_EXIST
    int met_emi_ret;
#endif

    met_sspm_emi_ret = met_deregister(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    met_emi_ret = met_deregister(&met_emi);
#endif

    met_emi_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_emi_api/met_emi_api.h"

    if (!met_sspm_emi_ret) met_register(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    if (!met_emi_ret) met_register(&met_emi);
#endif

#endif /* MET_EMI */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");